#include "FlexGPUParticleEmitterInstance.h"
#include "FlexParticleSpriteEmitter.h"

#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleEmitter.h"


class FFlexSimulationVertexBuffer : public FVertexBuffer
{
public:
	typedef FVertexBuffer Parent;

	static const int32 MinCapacity = 32;

	/** Shader resource of the vertex buffer. */
	FShaderResourceViewRHIRef VertexBufferSRV;

	/** Default constructor. */
	FFlexSimulationVertexBuffer(int32 VertexSizeIn, EPixelFormat VertexFormatIn, int32 BufferCreateFlagsIn = (BUF_Static | BUF_KeepCPUAccessible))
		: VertexSize(VertexSizeIn)
		, VertexFormat(VertexFormatIn)
		, Count(0)
		, Capacity(MinCapacity)
		, BufferCreateFlags(BufferCreateFlagsIn | BUF_ShaderResource)
	{
	}

	// May delete the buffer, and recreate if needs more capacity
	void RequireCapacity(int32 CapacityIn)
	{
		check(IsInRenderingThread());

		if (Capacity < CapacityIn)
		{
			// Make it expand by at least 1/2 the size of it's current size
			int32 MinNewCapacity = Capacity + (Capacity >> 1);
			MinNewCapacity = (MinNewCapacity < MinCapacity) ? MinCapacity : MinNewCapacity;

			// Must be at least as big as MinNewCapacity
			CapacityIn = (CapacityIn < MinNewCapacity) ? MinNewCapacity : CapacityIn;

			// Release if need more capacity
			ReleaseRHI();

			// Set the required capacity
			Capacity = CapacityIn;
		}

		if (Capacity > 0 && !VertexBufferRHI)
		{
			// Set up with the new Capacity
			InitRHI();
		}
	}

	/// Lock - specifying the amount of vertices wanted
	void* Lock(int32 NumVerticesIn)
	{
		RequireCapacity(NumVerticesIn);
		Count = NumVerticesIn;
		return RHILockVertexBuffer(VertexBufferRHI, 0, NumVerticesIn * VertexSize, RLM_WriteOnly);
	}

	/// Unlock - must match up with previous Lock
	void Unlock()
	{
		RHIUnlockVertexBuffer(VertexBufferRHI);
	}

	/**
	* Set the contents
	*/
	void Set(const void* Data, int32 NumVerticesIn)
	{
		void* Dst = Lock(NumVerticesIn);
		FMemory::Memcpy(Dst, Data, VertexSize * NumVerticesIn);
		Unlock();
	}
	/**
	* Initialize RHI resources.
	*/
	virtual void InitRHI() override
	{
		if (Capacity > 0)
		{
			const int32 BufferSize = Capacity * VertexSize;
			check(BufferSize > 0);
			FRHIResourceCreateInfo CreateInfo;
			VertexBufferRHI = RHICreateVertexBuffer(BufferSize, BufferCreateFlags, CreateInfo);
			VertexBufferSRV = RHICreateShaderResourceView(VertexBufferRHI, /*Stride=*/ VertexSize, VertexFormat);
		}
	}

	/**
	* Release RHI resources.
	*/
	virtual void ReleaseRHI() override
	{
		VertexBufferSRV.SafeRelease();
		Parent::ReleaseRHI();
	}

	virtual FString GetFriendlyName() const override { return TEXT("FFlexSimulationVertexBuffer"); }

	// BUF_Static | BUF_KeepCPUAccessible | BUF_ShaderResource

	int32 BufferCreateFlags;			///< Flags to create the buffer
	EPixelFormat VertexFormat;			///< The format of the vertex
	int32 VertexSize;					///< The size of the vertex in bytes
	int32 Capacity;						///< The total number of vertices the buffer can handle
	int32 Count;					///< The number of vertices currently held
};

struct FFlexParticleVertex
{
	FVector4 Position;
	FVector4 Velocity;
};

struct FFlexParticleSimulationState
{
	TArray<FFlexParticleVertex> Vertices;
};


FFlexGPUParticleEmitterInstance::FFlexGPUParticleEmitterInstance(FFlexParticleEmitterInstance* InOwner)
	: Owner(InOwner)
{
}

FFlexGPUParticleEmitterInstance::~FFlexGPUParticleEmitterInstance()
{
}

void FFlexGPUParticleEmitterInstance::Tick(float DeltaSeconds, bool bSuppressSpawning, FRenderResource* FlexSimulationResource)
{
	auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(Owner->Emitter->SpriteTemplate);
	if (FlexEmitter == nullptr)
		return;

	FFlexContainerInstance* Container = Owner->Container;
	if (Container && (!GIsEditor || GIsPlayInEditorWorld))
	{
		// all Flex components should be ticked during the synchronization 
		// phase of the Flex update, which corresponds to the EndPhysics tick group
		verify(Container->IsMapped());

		FTransform ParentTransform;
		FVector Translation;
		FQuat Rotation;
		USceneComponent* Parent = nullptr;

		if (FlexParticleIndices.Num() > 0)
		{
			Parent = Owner->Emitter->Component->GetAttachParent();
			if (Parent && FlexEmitter->bLocalSpace)
			{
				//update frame
				ParentTransform = Parent->GetComponentTransform();
				Translation = ParentTransform.GetTranslation();
				Rotation = ParentTransform.GetRotation();

				NvFlexExtMovingFrameUpdate(&Owner->MeshFrame, (float*)(&Translation.X), (float*)(&Rotation.X), DeltaSeconds);
			}
		}

		FFlexSimulationVertexBuffer* SimulationVertexBuffer = (FFlexSimulationVertexBuffer*)FlexSimulationResource;
		if (SimulationVertexBuffer)
		{
			// sync UE4 particles with FLEX
			const int32 NumFlexParticleIndices = FlexParticleIndices.Num();

			FFlexParticleSimulationState* State = new FFlexParticleSimulationState;
			State->Vertices.SetNumUninitialized(NumFlexParticleIndices);

			for (int32 i = 0; i < NumFlexParticleIndices; i++)
			{
				const int32 FlexParticleIndex = FlexParticleIndices[i];
				if (FlexParticleIndex < 0)
				{
					// Just zero for now. Should only be < 0 (ie not set) on the non used members of 
					// the currently being allocated from tile
					// For now mark w as -1 to show that's whats going on
					State->Vertices[i].Position = FVector4(0, 0, 0, -1);
					State->Vertices[i].Velocity = FVector4(0, 0, 0, 0);
					continue;
				}

				if (Parent && FlexEmitter->bLocalSpace)
				{
					// Localize the position and velocity using the localization API
					// NOTE: Once we have a feature to detect particle inside the mesh container
					//       we can then test for it and apply localization as needed.
					FVector4* Position = (FVector4*)&Container->Particles[FlexParticleIndex];
					FVector* Velocity = (FVector*)&Container->Velocities[FlexParticleIndex];

					NvFlexExtMovingFrameApply(&Owner->MeshFrame, (float*)Position, (float*)Velocity,
						1, Owner->LinearInertialScale, Owner->AngularInertialScale, DeltaSeconds);
				}

				// sync UE4 particle with FLEX
				if (Container->SmoothPositions.size() > 0)
				{
					State->Vertices[i].Position = Container->SmoothPositions[FlexParticleIndex];
				}
				else
				{
					State->Vertices[i].Position = Container->Particles[FlexParticleIndex];
				}
				State->Vertices[i].Velocity = Container->Velocities[FlexParticleIndex];
			}

			// Send to the rendering thread
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				FUpdateSimulationVertexBufferGPUCommand,
				FFlexSimulationVertexBuffer*, SimulationVertexBuffer, SimulationVertexBuffer,
				FFlexParticleSimulationState*, State, State,
				{
					SimulationVertexBuffer->Set(State->Vertices.GetData(), State->Vertices.Num());
					delete State;
				});
		}
	}
}


void FFlexGPUParticleEmitterInstance::DestroyParticles(int32 Start, int32 Count)
{
	// Get the start of the indices of FleX particles for this tile
	int32* DestroyParticleIndices = &FlexParticleIndices[Start];

	// If they are all -1, then there is nothing to free
	if (DestroyParticleIndices[0] >= 0)
	{
		// Look for the last index that is used. We know this will quit while LastIndex >= because [0] >= 0 
		int32 LastIndex = Count - 1;
		while (DestroyParticleIndices[LastIndex] < 0)
		{
			LastIndex--;
		}
		// Destroy all the particles in the tile
		Owner->Container->DestroyParticles(DestroyParticleIndices, (LastIndex + 1));
		// Mark particles indices in tile as no longer used
		FMemory::Memset(DestroyParticleIndices, uint8(-1), sizeof(int32) * (LastIndex + 1));
	}
}

void FFlexGPUParticleEmitterInstance::DestroyAllParticles(int32 ParticlesPerTile, bool bFreeParticleIndices)
{
	// Release all of the allocated particles
	const int32 NumFlexIndices = FlexParticleIndices.Num();
	for (int32 StartIndex = 0; StartIndex < NumFlexIndices; StartIndex += ParticlesPerTile)
	{
		DestroyParticles(StartIndex, ParticlesPerTile);
	}

	if (bFreeParticleIndices)
	{
		FlexParticleIndices.Reset();
	}
}

void FFlexGPUParticleEmitterInstance::AllocParticleIndices(int32 Count)
{
	if (Count > 0)
	{
		const int32 NewParticlesIndex = FlexParticleIndices.AddUninitialized(Count);
		FMemory::Memset(&FlexParticleIndices[NewParticlesIndex], uint8(-1), sizeof(int32) * Count);
	}
}

void FFlexGPUParticleEmitterInstance::FreeParticleIndices(int32 Start, int32 Count)
{
	FlexParticleIndices.RemoveAt(Start, Count);
}

int32 FFlexGPUParticleEmitterInstance::CreateNewParticles(int32 NewStart, int32 NewCount)
{
	NewFlexParticleIndices.SetNum(NewStart + NewCount);

	return Owner->Container->CreateParticles(NewFlexParticleIndices.GetData() + NewStart, NewCount);
}

void FFlexGPUParticleEmitterInstance::DestroyNewParticles(int32 NewStart, int32 NewCount)
{
	Owner->Container->DestroyParticles(NewFlexParticleIndices.GetData() + NewStart, NewCount);
}

void FFlexGPUParticleEmitterInstance::InitNewParticle(int32 NewIndex, int32 RegularIndex)
{
	verify(FlexParticleIndices[RegularIndex] == -1);
	FlexParticleIndices[RegularIndex] = NewFlexParticleIndices[NewIndex];
}

void FFlexGPUParticleEmitterInstance::SetNewParticle(int32 NewIndex, const FVector& Position, const FVector& Velocity)
{
	const int32 FlexParticleIndex = NewFlexParticleIndices[NewIndex];

	Owner->Container->SetParticle(FlexParticleIndex, FVector4(Position, Owner->FlexInvMass), Velocity, Owner->Phase);
}

FRenderResource* FFlexGPUParticleEmitterInstance::CreateSimulationResource()
{
	return new FFlexSimulationVertexBuffer(sizeof(FVector4) * 2, PF_A32B32G32R32F, BUF_Dynamic);
}

FRHIShaderResourceView* FFlexGPUParticleEmitterInstance::GetSimulationResourceView(FRenderResource* FlexSimulationResource)
{
	FFlexSimulationVertexBuffer* SimulationVertexBuffer = (FFlexSimulationVertexBuffer*)FlexSimulationResource;
	return SimulationVertexBuffer ? SimulationVertexBuffer->VertexBufferSRV : nullptr;
}