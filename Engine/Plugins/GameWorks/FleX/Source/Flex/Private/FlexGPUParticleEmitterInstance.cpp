#include "FlexGPUParticleEmitterInstance.h"
#include "FlexParticleSpriteEmitter.h"
#include "FlexFluidSurfaceComponent.h"

#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/TypeData/ParticleModuleTypeDataGpu.h"

#include "GameWorks/FlexPluginGPUParticles.h"


class FFlexSimulationResourceBuffer
{
public:
	static const int32 MinCapacity = 32;

	FFlexSimulationResourceBuffer(int32 InElementSize, int32 InBufferUsage = (BUF_Static | BUF_KeepCPUAccessible))
	{
		Capacity = 0;
		ElementSize = InElementSize;
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
			Buffer = RHICreateStructuredBuffer(ElementSize, BufferSize, BufferUsage | BUF_ShaderResource, CreateInfo);
			BufferSRV = RHICreateShaderResourceView(Buffer);
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
		return RHILockStructuredBuffer(Buffer, 0, InNumElements * ElementSize, RLM_WriteOnly);
	}

	void Unlock()
	{
		RHIUnlockStructuredBuffer(Buffer);
	}

	void Set(const void* Src, int32 Count)
	{
		if (Count > 0)
		{
			void* Dest = Lock(Count);
			if (Dest)
			{
				FMemory::Memcpy(Dest, Src, ElementSize * Count);
			}
			Unlock();
		}
	}

	const FShaderResourceViewRHIRef& GetSRV() const
	{
		return BufferSRV;
	}

private:
	FStructuredBufferRHIRef Buffer;
	FShaderResourceViewRHIRef BufferSRV;

	int32 Capacity;
	int32 ElementSize;
	int32 BufferUsage;
};

class FFlexSimulationResource : public FRenderResource
{
public:
	static const int32 ParticleIndexSize = sizeof(int);
	static const int32 PositionSize = sizeof(float) * 4;
	static const int32 VelocitySize = sizeof(float) * 4;

	FFlexSimulationResourceBuffer ParticleIndexBuffer;
	FFlexSimulationResourceBuffer PositionBuffer;
	FFlexSimulationResourceBuffer VelocityBuffer;

	/** Default constructor. */
	FFlexSimulationResource(int32 InBufferUsage = (BUF_Static | BUF_KeepCPUAccessible))
		: ParticleIndexBuffer(ParticleIndexSize, InBufferUsage)
		, PositionBuffer(PositionSize, InBufferUsage)
		, VelocityBuffer(VelocitySize, InBufferUsage)
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


FFlexGPUParticleEmitterInstance::FFlexGPUParticleEmitterInstance(FFlexParticleEmitterInstance* InOwner, int32 InParticlesPerTile)
	: Owner(InOwner)
	, ParticlesPerTile(InParticlesPerTile)
	, NumActiveParticles(0)
{
}

FFlexGPUParticleEmitterInstance::~FFlexGPUParticleEmitterInstance()
{
}

namespace
{
	class QuantizedSampler
	{
	public:
		QuantizedSampler()
			: Samples(&EmptySamples), Scale(FVector4(0, 0, 0, 0)), Bias(FVector4(1, 1, 1, 1))
		{
		}

		QuantizedSampler(const TArray<FColor>& InSamples, const FVector4& InScale, const FVector4& InBias)
			: Samples(&InSamples), Scale(InScale), Bias(InBias)
		{
		}

		FVector4 operator() (float Time) const
		{
			FVector4 Result = Bias;
			if (Samples->Num() > 1)
			{
				Time = FMath::Clamp(Time, 0.0f, 1.0f);
				Time *= Samples->Num() - 1;

				const float Blend = FMath::Fractional(Time);
				const int32 Index0 = FMath::TruncToInt(Time);
				const int32 Index1 = FMath::Min(Index0 + 1, Samples->Num() - 1);

				const FColor& Color0 = (*Samples)[Index0];
				const FColor& Color1 = (*Samples)[Index1];

				Result.X += Scale.X * FMath::Lerp(Color0.R / 255.f, Color1.R / 255.f, Blend);
				Result.Y += Scale.Y * FMath::Lerp(Color0.G / 255.f, Color1.G / 255.f, Blend);
				Result.Z += Scale.Z * FMath::Lerp(Color0.B / 255.f, Color1.B / 255.f, Blend);
				Result.W += Scale.W * FMath::Lerp(Color0.A / 255.f, Color1.A / 255.f, Blend);
			}
			return Result;
		}

	private:
		const TArray<FColor>* Samples;
		FVector4 Scale;
		FVector4 Bias;

		static TArray<FColor> EmptySamples;
	};

	TArray<FColor> QuantizedSampler::EmptySamples;

	struct FVector4ToFLinearColor
	{
		inline static FLinearColor Convert(const FVector4& In) { return FLinearColor(In.X, In.Y, In.Z, In.W); }
	};
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

		if (NumActiveParticles > 0)
		{
			for (int32 TileIndex = 0; TileIndex < ParticleTiles.Num(); TileIndex++)
			{
				int32 NumActiveParticlesInTile = ParticleTiles[TileIndex].NumActiveParticles;
				for (int32 SubTileIndex = 0, ParticleIndex = TileIndex * ParticlesPerTile; SubTileIndex < NumActiveParticlesInTile; ++SubTileIndex, ++ParticleIndex)
				{
					const int32 FlexParticleIndex = FlexParticleIndices[ParticleIndex];
					check(FlexParticleIndex != -1);

					FParticleData& ParticleData = ParticleDataArray[ParticleIndex];
					ParticleData.RelativeTime += ParticleData.TimeScale * DeltaSeconds;

					if (ParticleData.RelativeTime > 1.0f)
					{
						Container->DestroyParticle(FlexParticleIndex);

						const int32 LastActiveParticleIndex = (ParticleIndex - SubTileIndex) + NumActiveParticlesInTile - 1;
						// swap with last particle
						if (ParticleIndex != LastActiveParticleIndex)
						{
							FlexParticleIndices[ParticleIndex] = FlexParticleIndices[LastActiveParticleIndex];
							ParticleDataArray[ParticleIndex] = ParticleDataArray[LastActiveParticleIndex];
						}
						FlexParticleIndices[LastActiveParticleIndex] = -1;

						NumActiveParticlesInTile--;
						NumActiveParticles--;
					}
				}
				ParticleTiles[TileIndex].NumActiveParticles = NumActiveParticlesInTile;
			}
		}

		if (NumActiveParticles > 0)
		{
			Parent = Owner->Emitter->Component->GetAttachParent();
			if (Parent && FlexEmitter->bLocalSpace)
			{
				//update frame
				ParentTransform = Parent->GetComponentTransform();
				Translation = ParentTransform.GetTranslation();
				Rotation = ParentTransform.GetRotation();

				NvFlexExtMovingFrameUpdate(&Owner->MeshFrame, (float*)(&Translation.X), (float*)(&Rotation.X), DeltaSeconds);

				for (int32 i = 0; i < FlexParticleIndices.Num(); i++)
				{
					const int32 FlexParticleIndex = FlexParticleIndices[i];
					if (FlexParticleIndex != -1)
					{
						// Localize the position and velocity using the localization API
						// NOTE: Once we have a feature to detect particle inside the mesh container
						//       we can then test for it and apply localization as needed.
						FVector4* Position = (FVector4*)&Container->Particles[FlexParticleIndex];
						FVector* Velocity = (FVector*)&Container->Velocities[FlexParticleIndex];

						NvFlexExtMovingFrameApply(&Owner->MeshFrame, (float*)Position, (float*)Velocity,
							1, Owner->LinearInertialScale, Owner->AngularInertialScale, DeltaSeconds);
					}
				}
			}
		}

		// flex container with UE4 particle data for surface rendering
		if (FlexEmitter->Phase.Fluid && Container->FluidSurfaceComponent)
		{
			QuantizedSampler ColorSampler;
			QuantizedSampler MiscSampler;

			FVector4 DynamicColor = FVector4(1, 1, 1, 1); // InitialColor

			const float LocalToWorldScale = Owner->Emitter->Component->GetComponentTransform().GetScale3D().X;

			UParticleLODLevel* LODLevel = FlexEmitter->LODLevels[0];
			if (LODLevel)
			{
				UParticleModuleTypeDataGpu* TypeDataModule = CastChecked<UParticleModuleTypeDataGpu>(LODLevel->TypeDataModule);
				if (TypeDataModule)
				{
					ColorSampler = QuantizedSampler(TypeDataModule->ResourceData.QuantizedColorSamples,
						TypeDataModule->ResourceData.ColorScale, TypeDataModule->ResourceData.ColorBias);

					MiscSampler = QuantizedSampler(TypeDataModule->ResourceData.QuantizedMiscSamples,
						TypeDataModule->ResourceData.MiscScale, TypeDataModule->ResourceData.MiscBias);

					// Setup dynamic color parameter. Only set when using particle parameter distributions.
					FVector4 ColorOverLife(1.0f, 1.0f, 1.0f, 1.0f);
					FVector4 ColorScaleOverLife(1.0f, 1.0f, 1.0f, 1.0f);
					if (TypeDataModule->EmitterInfo.DynamicColorScale.IsCreated())
					{
						ColorScaleOverLife = TypeDataModule->EmitterInfo.DynamicColorScale.GetValue(0.0f, Owner->Emitter->Component);
					}
					if (TypeDataModule->EmitterInfo.DynamicAlphaScale.IsCreated())
					{
						ColorScaleOverLife.W = TypeDataModule->EmitterInfo.DynamicAlphaScale.GetValue(0.0f, Owner->Emitter->Component);
					}

					if (TypeDataModule->EmitterInfo.DynamicColor.IsCreated())
					{
						ColorOverLife = TypeDataModule->EmitterInfo.DynamicColor.GetValue(0.0f, Owner->Emitter->Component);
					}
					if (TypeDataModule->EmitterInfo.DynamicAlpha.IsCreated())
					{
						ColorOverLife.W = TypeDataModule->EmitterInfo.DynamicAlpha.GetValue(0.0f, Owner->Emitter->Component);
					}
					DynamicColor *= (ColorOverLife * ColorScaleOverLife);
				}
			}

			UFlexFluidSurfaceComponent* SurfaceComponent = Container->FluidSurfaceComponent;
			bool bHasAnisotropy = Container->Anisotropy1.size() > 0;
			bool bHasSmoothedPositions = Container->SmoothPositions.size() > 0;

			for (int32 i = 0; i < FlexParticleIndices.Num(); i++)
			{
				const int32 FlexParticleIndex = FlexParticleIndices[i];
				if (FlexParticleIndex != -1)
				{
					const float RelativeTime = ParticleDataArray[i].RelativeTime;
					const float bIsAlive = (RelativeTime <= 1.0f);
					const float InitialSize = ParticleDataArray[i].InitialSize;

					UFlexFluidSurfaceComponent::Particle SurfaceParticle;
					SurfaceParticle.Position = bHasSmoothedPositions ? Container->SmoothPositions[FlexParticleIndex] : Container->Particles[FlexParticleIndex];
					SurfaceParticle.Size = MiscSampler(RelativeTime).X * InitialSize * LocalToWorldScale * bIsAlive;
					SurfaceParticle.Color = FVector4ToFLinearColor::Convert(ColorSampler(RelativeTime) * DynamicColor);
					SurfaceComponent->Particles.Add(SurfaceParticle);

					if (bHasAnisotropy)
					{
						UFlexFluidSurfaceComponent::ParticleAnisotropy SurfaceParticleAnisotropy;
						SurfaceParticleAnisotropy.Anisotropy1 = Container->Anisotropy1[FlexParticleIndex];
						SurfaceParticleAnisotropy.Anisotropy2 = Container->Anisotropy2[FlexParticleIndex];
						SurfaceParticleAnisotropy.Anisotropy3 = Container->Anisotropy3[FlexParticleIndex];
						SurfaceComponent->ParticleAnisotropies.Add(SurfaceParticleAnisotropy);
					}
				}
			}

			SurfaceComponent->NotifyParticleBatch(Owner->Emitter->GetBoundingBox());
		}

		FFlexSimulationResource* SimulationResource = (FFlexSimulationResource*)FlexSimulationResource;
		if (SimulationResource)
		{
			// sync UE4 particles with FLEX
			FFlexParticleSimulationState* State = new FFlexParticleSimulationState;
			State->ParticleIndexArray.SetNumUninitialized(FlexParticleIndices.Num());

			State->PositionArray.SetNumUninitialized(NumActiveParticles);
			State->VelocityArray.SetNumUninitialized(NumActiveParticles);

			// compact particle data
			int32 OutParticleIndex = 0;
			for (int32 i = 0; i < FlexParticleIndices.Num(); i++)
			{
				State->ParticleIndexArray[i] = -1;

				const int32 FlexParticleIndex = FlexParticleIndices[i];
				if (FlexParticleIndex != -1)
				{
					// sync UE4 particle with FLEX
					if (Container->SmoothPositions.size() > 0)
					{
						State->PositionArray[OutParticleIndex] = Container->SmoothPositions[FlexParticleIndex];
					}
					else
					{
						State->PositionArray[OutParticleIndex] = Container->Particles[FlexParticleIndex];
					}
					State->VelocityArray[OutParticleIndex] = Container->Velocities[FlexParticleIndex];

					State->ParticleIndexArray[i] = OutParticleIndex++;
				}
			}
			check(OutParticleIndex == NumActiveParticles);
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

bool FFlexGPUParticleEmitterInstance::ShouldRenderParticles()
{
	if (Owner->Container)
	{
		auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(Owner->Emitter->SpriteTemplate);
		if (FlexEmitter && FlexEmitter->Phase.Fluid)
		{
			UFlexFluidSurfaceComponent* SurfaceComponent = Owner->Container->FluidSurfaceComponent;
			if (SurfaceComponent && SurfaceComponent->ShouldDisableEmitterRendering())
			{
				return false;
			}
		}
	}
	return true;
}

void FFlexGPUParticleEmitterInstance::DestroyTileParticles(int32 TileIndex)
{
	const int32 DestroyParticleCount = ParticleTiles[TileIndex].NumActiveParticles;
	if (DestroyParticleCount > 0)
	{
		int32* DestroyParticleIndices = FlexParticleIndices.GetData() + TileIndex * ParticlesPerTile;

		// Destroy all the particles in the tile
		Owner->Container->DestroyParticles(DestroyParticleIndices, DestroyParticleCount);

		// Mark particles indices in tile as no longer used
		FMemory::Memset(DestroyParticleIndices, uint8(-1), sizeof(int32) * DestroyParticleCount);

		ParticleTiles[TileIndex] = FParticleTileData(); //resets to FParticleTileData defaults: NumAddedParticles = NumActiveParticles = 0;

		NumActiveParticles -= DestroyParticleCount;
	}
}

void FFlexGPUParticleEmitterInstance::DestroyAllParticles(bool bFreeParticleIndices)
{
	// Release all of the allocated particles
	for (int32 TileIndex = 0; TileIndex < ParticleTiles.Num(); ++TileIndex)
	{
		DestroyTileParticles(TileIndex);
	}

	if (bFreeParticleIndices)
	{
		ParticleTiles.Reset();
		FlexParticleIndices.Reset();
		ParticleDataArray.Reset();
	}
}

void FFlexGPUParticleEmitterInstance::AllocParticleIndices(int32 TileCount)
{
	if (TileCount > 0)
	{
		ParticleTiles.AddDefaulted(TileCount);

		const int32 ParticleCount = TileCount * ParticlesPerTile;
		const int32 NewParticlesIndex = FlexParticleIndices.AddUninitialized(ParticleCount);
		FMemory::Memset(&FlexParticleIndices[NewParticlesIndex], uint8(-1), sizeof(int32) * ParticleCount);
		ParticleDataArray.AddUninitialized(ParticleCount);
	}
}

void FFlexGPUParticleEmitterInstance::FreeParticleIndices(int32 TileStart, int32 TileCount)
{
	ParticleTiles.RemoveAt(TileStart, TileCount);
	FlexParticleIndices.RemoveAt(TileStart * ParticlesPerTile, TileCount * ParticlesPerTile);
	ParticleDataArray.RemoveAt(TileStart * ParticlesPerTile, TileCount * ParticlesPerTile);
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

void FFlexGPUParticleEmitterInstance::AddNewParticle(int32 NewIndex, int32 TileIndex, int32 SubTileIndex)
{
	const int32 NumRemovedParticles = ParticleTiles[TileIndex].NumAddedParticles - ParticleTiles[TileIndex].NumActiveParticles;
	check(NumRemovedParticles >= 0);
	const int32 ParticleIndex = TileIndex * ParticlesPerTile + (SubTileIndex - NumRemovedParticles);
	check(FlexParticleIndices[ParticleIndex] == -1);

	const int32 FlexParticleIndex = NewFlexParticleIndices[NewIndex];
	FlexParticleIndices[ParticleIndex] = FlexParticleIndex;
	NewFlexParticleIndices[NewIndex] = ParticleIndex; //set to ParticleIndex - this is safe because SetNewParticle is called after AddNewParticle

	ParticleTiles[TileIndex].NumAddedParticles++;
	ParticleTiles[TileIndex].NumActiveParticles++;

	NumActiveParticles++;
}

void FFlexGPUParticleEmitterInstance::SetNewParticle(int32 NewIndex, const FVector& Position, const FVector& Velocity, float RelativeTime, float TimeScale, float InitialSize)
{
	const int32 ParticleIndex = NewFlexParticleIndices[NewIndex]; //ParticleIndex
	const int32 FlexParticleIndex = FlexParticleIndices[ParticleIndex];
	check(FlexParticleIndex >= 0);
	NewFlexParticleIndices[NewIndex] = FlexParticleIndex; //reset to FlexParticleIndex

	Owner->Container->SetParticle(FlexParticleIndex, FVector4(Position, Owner->FlexInvMass), Velocity, Owner->Phase);

	ParticleDataArray[ParticleIndex] = { RelativeTime, TimeScale, InitialSize };
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
