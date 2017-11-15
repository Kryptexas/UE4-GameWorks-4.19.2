#include "FlexGPUParticleEmitterInstance.h"
#include "FlexParticleSpriteEmitter.h"

#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleEmitter.h"

#include "GameWorks/FlexPluginGPUParticles.h"


class FFlexSimulationResourceBuffer
{
public:
	static const int32 MinCapacity = 32;

	FFlexSimulationResourceBuffer(int32 InElementSize, int32 InElementFormat, int32 InBufferUsage = (BUF_Static | BUF_KeepCPUAccessible))
	{
		Capacity = 0;
		ElementSize = InElementSize;
		ElementFormat = InElementFormat;
		BufferUsage = InBufferUsage;
	}

	void RequireCapacity(int32 InCapacity)
	{
		check(IsInRenderingThread());

		if (InCapacity > Capacity)
		{
			// Release if need more capacity
			Release();

			// Make it expand by at least 1/2 the size of it's current size
			int32 MinNewCapacity = Capacity + (Capacity >> 1);
			MinNewCapacity = (MinNewCapacity < MinCapacity) ? MinCapacity : MinNewCapacity;
			// Must be at least as big as MinNewCapacity
			Capacity = (InCapacity < MinNewCapacity) ? MinNewCapacity : InCapacity;

			// Set up with the new Capacity
			FRHIResourceCreateInfo CreateInfo;

			const int32 BufferSize = Capacity * ElementSize;
			check(BufferSize > 0);
			Buffer = RHICreateVertexBuffer(BufferSize, BufferUsage, CreateInfo);
			BufferSRV = RHICreateShaderResourceView(Buffer, /*Stride=*/ ElementSize, ElementFormat);
		}
	}

	void Release()
	{
		BufferSRV.SafeRelease();
		Buffer.SafeRelease();
	}

	void* Lock(int32 InNumElements)
	{
		check(InNumElements <= Capacity);
		return RHILockVertexBuffer(Buffer, 0, InNumElements * ElementSize, RLM_WriteOnly);
	}

	void Unlock()
	{
		RHIUnlockVertexBuffer(Buffer);
	}

	void Set(const void* Src, int32 Count)
	{
		void* Dest = Lock(Count);
		if (Dest)
		{
			FMemory::Memcpy(Dest, Src, ElementSize * Count);
		}
		Unlock();
	}

	const FShaderResourceViewRHIRef& GetSRV() const
	{
		return BufferSRV;
	}

private:
	FVertexBufferRHIRef Buffer;
	FShaderResourceViewRHIRef BufferSRV;

	int32 Capacity;
	int32 ElementSize;
	int32 ElementFormat;
	int32 BufferUsage;
};

class FFlexSimulationResource : public FRenderResource
{
public:
	static const int32 ParticleIndexFormat = PF_R32_SINT;
	static const int32 ParticleIndexSize = sizeof(int);

	static const int32 PositionFormat = PF_A32B32G32R32F;
	static const int32 PositionSize = sizeof(float) * 4;

	static const int32 VelocityFormat = PF_A32B32G32R32F;
	static const int32 VelocitySize = sizeof(float) * 4;

	FFlexSimulationResourceBuffer ParticleIndexBuffer;
	FFlexSimulationResourceBuffer PositionBuffer;
	FFlexSimulationResourceBuffer VelocityBuffer;

	/** Default constructor. */
	FFlexSimulationResource(int32 InBufferUsage = (BUF_Static | BUF_KeepCPUAccessible))
		: ParticleIndexBuffer(ParticleIndexSize, ParticleIndexFormat, InBufferUsage)
		, PositionBuffer(PositionSize, PositionFormat, InBufferUsage)
		, VelocityBuffer(VelocitySize, VelocityFormat, InBufferUsage)
	{
	}

	// May delete the buffer, and recreate if needs more capacity
	void RequireCapacity(int32 InIndicesCapacity, int32 InParticlesCapacity)
	{
		ParticleIndexBuffer.RequireCapacity(InIndicesCapacity);
		PositionBuffer.RequireCapacity(InParticlesCapacity);
		VelocityBuffer.RequireCapacity(InParticlesCapacity);
	}

	/**
	* Release RHI resources.
	*/
	virtual void ReleaseRHI() override
	{
		ParticleIndexBuffer.Release();
		PositionBuffer.Release();
		VelocityBuffer.Release();
	}

	virtual FString GetFriendlyName() const override { return TEXT("FFlexSimulationResource"); }
};

struct FFlexParticleSimulationState
{
	TArray<int32> ParticleIndexArray;
	TArray<FVector4> PositionArray;
	TArray<FVector4> VelocityArray;
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

		FFlexSimulationResource* SimulationResource = (FFlexSimulationResource*)FlexSimulationResource;
		if (SimulationResource)
		{
			// sync UE4 particles with FLEX
			const int32 NumFlexParticleIndices = FlexParticleIndices.Num();
			const int32 NumActiveParticles = Container->GetActiveParticleCount();

			FFlexParticleSimulationState* State = new FFlexParticleSimulationState;
			State->ParticleIndexArray = FlexParticleIndices;

			State->PositionArray.SetNumUninitialized(NumActiveParticles);
			State->VelocityArray.SetNumUninitialized(NumActiveParticles);

			for (int32 i = 0; i < NumActiveParticles; i++)
			{
				if (Parent && FlexEmitter->bLocalSpace)
				{
					// Localize the position and velocity using the localization API
					// NOTE: Once we have a feature to detect particle inside the mesh container
					//       we can then test for it and apply localization as needed.
					FVector4* Position = (FVector4*)&Container->Particles[i];
					FVector* Velocity = (FVector*)&Container->Velocities[i];

					NvFlexExtMovingFrameApply(&Owner->MeshFrame, (float*)Position, (float*)Velocity,
						1, Owner->LinearInertialScale, Owner->AngularInertialScale, DeltaSeconds);
				}

				// sync UE4 particle with FLEX
				if (Container->SmoothPositions.size() > 0)
				{
					State->PositionArray[i] = Container->SmoothPositions[i];
				}
				else
				{
					State->PositionArray[i] = Container->Particles[i];
				}
				State->VelocityArray[i] = Container->Velocities[i];
			}
#if 1
			// Send to the rendering thread
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				FUpdateSimulationResourceGPUCommand,
				FFlexSimulationResource*, SimulationResource, SimulationResource,
				FFlexParticleSimulationState*, State, State,
				{
					const int32 NumIndices = State->ParticleIndexArray.Num();
					const int32 NumParticles = State->PositionArray.Num();
					SimulationResource->RequireCapacity(NumIndices, NumParticles);

					SimulationResource->ParticleIndexBuffer.Set(State->ParticleIndexArray.GetData(), NumIndices);

					SimulationResource->PositionBuffer.Set(State->PositionArray.GetData(), NumParticles);
					SimulationResource->VelocityBuffer.Set(State->VelocityArray.GetData(), NumParticles);

					delete State;
				});
#else
			struct FUpdateSimulationResourceData
			{
				int32 ParticleCount;
				NvFlexSolver* FlexSolver;
				NvFlexLibrary* FlexLib;
			};
			FUpdateSimulationResourceData UpdateSimulationResourceData =
			{

			};


			// Send to the rendering thread
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				FUpdateSimulationResourceGPUCommand,
				FFlexSimulationResource*, SimulationResource, SimulationResource,
				NvFlexLibrary*, FlexLib, FFlexManager::get().GetFlexLib(),
				{
					const int32 Count;

					const void* PositionBufferPtr = GDynamicRHI->RHIGetVertexBufferPtrForFlex(SimulationResource->PositionBuffer);
					const void* VelocityBufferPtr = GDynamicRHI->RHIGetVertexBufferPtrForFlex(SimulationResource->VelocityBuffer);
					if (PositionBufferPtr && VelocityBufferPtr)
					{
						NvFlexBuffer* PositionFlexBuffer = NvFlexRegisterD3DBuffer(FlexLib, PositionBufferPtr, SimulationResource->Capacity, FFlexSimulationResource::PositionSize);
						NvFlexBuffer* VelocityFlexBuffer = NvFlexRegisterD3DBuffer(FlexLib, VelocityBufferPtr, SimulationResource->Capacity, FFlexSimulationResource::VelocitySize);

						NvFlexCopyDesc PositionCopyDesc = { 0, 0, Count };
						//NvFlexGetParticles(FlexSolver, PositionFlexBuffer, &PositionCopyDesc);
						NvFlexCopyDesc VelocityCopyDesc = { 0, 0, Count };
						//NvFlexGetVelocities(FlexSolver, VelocityFlexBuffer, &VelocityCopyDesc);

						NvFlexUnregisterD3DBuffer(VelocityFlexBuffer);
						NvFlexUnregisterD3DBuffer(PositionFlexBuffer);
					}
				});
#endif
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
	return new FFlexSimulationResource(BUF_Dynamic);
}

void FFlexGPUParticleEmitterInstance::FillSimulationParams(FRenderResource* FlexSimulationResource, FFlexGPUParticleSimulationParameters& SimulationParams)
{
	FFlexSimulationResource* SimulationResource = (FFlexSimulationResource*)FlexSimulationResource;
	if (SimulationResource)
	{
		SimulationParams.ParticleIndexBufferSRV = SimulationResource->ParticleIndexBuffer.GetSRV();
		SimulationParams.PositionBufferSRV = SimulationResource->PositionBuffer.GetSRV();
		SimulationParams.VelocityBufferSRV = SimulationResource->VelocityBuffer.GetSRV();
	}
	else
	{
		SimulationParams.ParticleIndexBufferSRV = nullptr;
		SimulationParams.PositionBufferSRV = nullptr;
		SimulationParams.VelocityBufferSRV = nullptr;
	}
}
