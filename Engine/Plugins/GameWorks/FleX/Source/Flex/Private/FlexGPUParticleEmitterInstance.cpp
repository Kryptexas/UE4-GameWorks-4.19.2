#include "FlexGPUParticleEmitterInstance.h"
#include "FlexParticleSpriteEmitter.h"

#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleEmitter.h"

struct FFlexParticleSimulationState
{
	TArray<FVector4> Positions;
	TArray<FVector4> Velocities;
};


FFlexGPUParticleEmitterInstance::FFlexGPUParticleEmitterInstance(FFlexParticleEmitterInstance* InOwner, int32 NumParticles)
	: Owner(InOwner)
{
	AllocParticleIndices(NumParticles);
}

FFlexGPUParticleEmitterInstance::~FFlexGPUParticleEmitterInstance()
{
}

void FFlexGPUParticleEmitterInstance::Tick(float DeltaSeconds, bool bSuppressSpawning)
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

		// sync UE4 particles with FLEX
		const int32 NumFlexParticleIndices = FlexParticleIndices.Num();

		FFlexParticleSimulationState* State = new FFlexParticleSimulationState;
		State->Positions.SetNumUninitialized(NumFlexParticleIndices);

		for (int32 i = 0; i < NumFlexParticleIndices; i++)
		{
			const int32 FlexParticleIndex = FlexParticleIndices[i];
			if (FlexParticleIndex < 0)
			{
				// Just zero for now. Should only be < 0 (ie not set) on the non used members of 
				// the currently being allocated from tile
				// For now mark w as -1 to show that's whats going on
				State->Positions[i] = FVector4(0, 0, 0, -1);
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
				State->Positions[i] = Container->SmoothPositions[FlexParticleIndex];
			}
			else
			{
				State->Positions[i] = Container->Particles[FlexParticleIndex];
			}
		}
#if 0
		// Send to the rendering thread
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FUpdateParticleSimulationGPUCommand,
			FParticleSimulationGPU*, Simulation, Simulation,
			FFlexParticleSimulationState*, State, State,
			{
				Simulation->FlexParticlePositionVertexBuffer.Set(State->Positions.GetData(), State->Positions.Num());
				delete State;
			});
#endif
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

void FFlexGPUParticleEmitterInstance::DestroyAllParticles(int32 ParticlesPerTile)
{
	// Release all of the allocated particles
	const int32 NumFlexIndices = FlexParticleIndices.Num();
	for (int32 StartIndex = 0; StartIndex < NumFlexIndices; StartIndex += ParticlesPerTile)
	{
		DestroyParticles(StartIndex, ParticlesPerTile);
	}
}

void FFlexGPUParticleEmitterInstance::AllocParticleIndices(int32 Count)
{
	const int32 NewParticlesIndex = FlexParticleIndices.AddUninitialized(Count);
	FMemory::Memset(&FlexParticleIndices[NewParticlesIndex], uint8(-1), sizeof(int32) * Count);
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
