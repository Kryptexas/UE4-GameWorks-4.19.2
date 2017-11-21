#include "FlexParticleEmitterInstance.h"
#include "FlexFluidSurfaceComponent.h"
#include "FlexParticleSpriteEmitter.h"
#include "FlexManager.h"


FFlexParticleEmitterInstance::FFlexParticleEmitterInstance(FParticleEmitterInstance* Instance)
	: Emitter(Instance)
	, FlexDataOffset(0)
{
	auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(Emitter->SpriteTemplate);
	if (FlexEmitter && FlexEmitter->FlexContainerTemplate)
	{
		FPhysScene* Scene = Emitter->Component->GetWorld()->GetPhysicsScene();

		Container = FFlexManager::get().FindOrCreateFlexContainerInstance(Scene, FlexEmitter->FlexContainerTemplate);
		if (Container)
		{
			Container->Register(this);
			Phase = Container->GetPhase(FlexEmitter->Phase);
		}
	}
	if (FlexEmitter)
	{
		LinearInertialScale = FlexEmitter->InertialScale.LinearInertialScale;
		AngularInertialScale = FlexEmitter->InertialScale.AngularInertialScale;

		// need to ensure tick happens after GPU update
		Emitter->Component->SetTickGroup(TG_EndPhysics);

		USceneComponent* Parent = Emitter->Component->GetAttachParent();
		if (Parent && FlexEmitter->bLocalSpace)
		{
			//update frame
			const FTransform ParentTransform = Parent->GetComponentTransform();
			const FVector Translation = ParentTransform.GetTranslation();
			const FQuat Rotation = ParentTransform.GetRotation();

			NvFlexExtMovingFrameInit(&MeshFrame, (float*)(&Translation.X), (float*)(&Rotation.X));
		}

		FlexInvMass = (FlexEmitter->Mass > 0.0f) ? (1.0f / FlexEmitter->Mass) : 0.0f;
	}
}

FFlexParticleEmitterInstance::~FFlexParticleEmitterInstance()
{
	// ParticleData has to be defined, which it may not be if using a GPU particles
	if (Container && (!GIsEditor || GIsPlayInEditorWorld) && Emitter->ParticleData)
	{
		for (int32 i = 0; i < Emitter->ActiveParticles; i++)
		{
			DECLARE_PARTICLE(Particle, Emitter->ParticleData + Emitter->ParticleStride * Emitter->ParticleIndices[i]);
			verify(FlexDataOffset > 0);
			int32 CurrentOffset = FlexDataOffset;
			const uint8* ParticleBase = (const uint8*)&Particle;
			PARTICLE_ELEMENT(int32, FlexParticleIndex);
			Container->DestroyParticle(FlexParticleIndex);
		}
	}

	if (Container)
		Container->Unregister(this);
}

void FFlexParticleEmitterInstance::Tick(float DeltaTime, bool bSuppressSpawning)
{
	auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(Emitter->SpriteTemplate);
	if (FlexEmitter == nullptr)
		return;

	if (Container && (!GIsEditor || GIsPlayInEditorWorld))
	{
		ExecutePendingComponentsToAttach();
		SynchronizeAttachments(DeltaTime);

		// all Flex components should be ticked during the synchronization 
		// phase of the Flex update, which corresponds to the EndPhysics tick group
		verify(Container->IsMapped());

		// process report shapes
		if (Container->CollisionReportComponents.Num() > 0)
		{
			for (int32 i = 0; i < Emitter->ActiveParticles; i++)
			{
				DECLARE_PARTICLE(Particle, Emitter->ParticleData + Emitter->ParticleStride * Emitter->ParticleIndices[i]);

				verify(FlexDataOffset > 0);

				int32 CurrentOffset = FlexDataOffset;
				const uint8* ParticleBase = (const uint8*)&Particle;
				PARTICLE_ELEMENT(int32, FlexParticleIndex);

				const int ContactIndex = Container->ContactIndices[FlexParticleIndex];
				if (ContactIndex == -1)
					continue;

				bool bKillParticle = false;

				const uint32 Count = Container->ContactCounts[ContactIndex];
				for (uint32 c = 0; c < Count; c++)
				{
					FVector4 ContactVelocity = Container->ContactVelocities[ContactIndex*FFlexContainerInstance::MaxContactsPerParticle + c];
					int32 FlexShapeIndex = int(ContactVelocity.W);
					int32 ShapeReportIndex = Container->CollisionReportIndices[FlexShapeIndex];
					if (ShapeReportIndex >= 0)
					{
						UFlexCollisionComponent* CollisionComp = Container->CollisionReportComponents[ShapeReportIndex];

						if (CollisionComp)
						{
							CollisionComp->Count++;

							if (CollisionComp->bDrain)
								bKillParticle = true;
						}
					}
				}

				if (bKillParticle)
				{
					Emitter->KillParticle(i);
					continue;
				}
			}
		}

		FTransform ParentTransform;
		FVector Translation;
		FQuat Rotation;
		USceneComponent* Parent = nullptr;

		if (Emitter->ActiveParticles > 0)
		{
			Parent = Emitter->Component->GetAttachParent();
			if (Parent && FlexEmitter->bLocalSpace)
			{
				//update frame
				ParentTransform = Parent->GetComponentTransform();
				Translation = ParentTransform.GetTranslation();
				Rotation = ParentTransform.GetRotation();

				NvFlexExtMovingFrameUpdate(&MeshFrame, (float*)(&Translation.X), (float*)(&Rotation.X), DeltaTime);
			}
		}

		// sync UE4 particles with FLEX
		for (int32 i = 0; i < Emitter->ActiveParticles; i++)
		{
			DECLARE_PARTICLE(Particle, Emitter->ParticleData + Emitter->ParticleStride * Emitter->ParticleIndices[i]);

			verify(FlexDataOffset > 0);

			int32 CurrentOffset = FlexDataOffset;
			const uint8* ParticleBase = (const uint8*)&Particle;
			PARTICLE_ELEMENT(int32, FlexParticleIndex);

			if (Parent && FlexEmitter->bLocalSpace)
			{
				// Localize the position and velocity using the localization API
				// NOTE: Once we have a feature to detect particle inside the mesh container
				//       we can then test for it and apply localization as needed.
				FVector4* Positions = (FVector4*)&Container->Particles[FlexParticleIndex];
				FVector* Velocities = (FVector*)&Container->Velocities[FlexParticleIndex];

				NvFlexExtMovingFrameApply(&MeshFrame, (float*)Positions, (float*)Velocities,
					1, LinearInertialScale, AngularInertialScale, DeltaTime);
			}

			// sync UE4 particle with FLEX
			if (Container->SmoothPositions.size() > 0)
			{
				Particle.Location = Container->SmoothPositions[FlexParticleIndex];
			}
			else
			{
				Particle.Location = Container->Particles[FlexParticleIndex];
			}

			Particle.Velocity = Container->Velocities[FlexParticleIndex];
		}

		// flex container with UE4 particle data for surface rendering
		if (FlexEmitter->Phase.Fluid && Container->FluidSurfaceComponent)
		{
			UFlexFluidSurfaceComponent* SurfaceComponent = Container->FluidSurfaceComponent;
			bool bHasAnisotropy = Container->Anisotropy1.size() > 0;
			bool bHasSmoothedPositions = Container->SmoothPositions.size() > 0;

			for (int32 i = 0; i < Emitter->ActiveParticles; i++)
			{
				DECLARE_PARTICLE(Particle, Emitter->ParticleData + Emitter->ParticleStride * Emitter->ParticleIndices[i]);

				verify(FlexDataOffset > 0);

				int32 CurrentOffset = FlexDataOffset;
				const uint8* ParticleBase = (const uint8*)&Particle;
				PARTICLE_ELEMENT(int32, FlexParticleIndex);

				UFlexFluidSurfaceComponent::Particle SurfaceParticle;
				SurfaceParticle.Position = bHasSmoothedPositions ? Container->SmoothPositions[FlexParticleIndex] : Container->Particles[FlexParticleIndex];
				SurfaceParticle.Size = Particle.Size.X;
				SurfaceParticle.Color = Particle.Color;
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

			SurfaceComponent->NotifyParticleBatch(Emitter->GetBoundingBox());
		}
	}
}

uint32 FFlexParticleEmitterInstance::GetRequiredBytes(uint32 uiBytes)
{
	auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(Emitter->SpriteTemplate);

	if (FlexEmitter && FlexEmitter->FlexContainerTemplate)
	{
		FlexDataOffset = Emitter->PayloadOffset + uiBytes;

		// flex particle index
		uiBytes += sizeof(int32);
	}
	return uiBytes;
}

bool FFlexParticleEmitterInstance::SpawnParticle(struct FBaseParticle* Particle, uint32 CurrentParticleIndex)
{
	auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(Emitter->SpriteTemplate);
	if (FlexEmitter == nullptr)
		return true;

	if (Container && (!GIsEditor || GIsPlayInEditorWorld))
	{
		verify(FlexDataOffset > 0);

		int32 CurrentOffset = FlexDataOffset;
		const uint8* ParticleBase = (const uint8*)Particle;
		PARTICLE_ELEMENT(int32, FlexParticleIndex);

		// allocate a new particle in the flex solver and store a
		// reference to it in this particle's payload
		FlexParticleIndex = Container->CreateParticle(FVector4(Particle->Location, FlexInvMass), Particle->Velocity, Phase);

		if (FlexParticleIndex == -1)
		{
			// could not allocate a flex particle so kill immediately
			Emitter->KillParticle(CurrentParticleIndex);
			return false;
		}

		Particle->Flags |= STATE_Particle_FreezeTranslation;
	}
	return true;
}

void FFlexParticleEmitterInstance::KillParticle(int32 KillIndex)
{
	if (Container && (!GIsEditor || GIsPlayInEditorWorld))
	{
		verify(FlexDataOffset > 0);

		const uint8* ParticleBase = Emitter->ParticleData + KillIndex * Emitter->ParticleStride;
		int32 CurrentOffset = FlexDataOffset;
		PARTICLE_ELEMENT(int32, FlexParticleIndex);

		if (FlexParticleIndex >= 0)
		{
			DestroyParticle(FlexParticleIndex);
		}
	}
}

bool FFlexParticleEmitterInstance::IsDynamicDataRequired()
{
	if (Container)
	{
		auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(Emitter->SpriteTemplate);
		if (FlexEmitter && FlexEmitter->Phase.Fluid)
		{
			UFlexFluidSurfaceComponent* SurfaceComponent = Container->FluidSurfaceComponent;
			if (SurfaceComponent && SurfaceComponent->ShouldDisableEmitterRendering())
			{
				return false;
			}
		}
	}
	return true;
}

void FFlexParticleEmitterInstance::AddPendingComponentToAttach(USceneComponent* Component, float Radius)
{
	FlexComponentAttachment PendingAttach(Component, Radius);
	PendingAttachments.Add(PendingAttach);
}

void FFlexParticleEmitterInstance::ExecutePendingComponentsToAttach()
{
	for (int32 i = 0; i < PendingAttachments.Num(); i++)
	{
		AttachToComponent(PendingAttachments[i].Component, PendingAttachments[i].Radius);
	}
	PendingAttachments.Empty();
}

void FFlexParticleEmitterInstance::AttachToComponent(USceneComponent* Component, float Radius)
{
	const FTransform ComponentTransform = Component->GetComponentTransform();
	const FVector ComponentPos = Component->GetComponentTransform().GetTranslation();

	for (int32 i = 0; i < Emitter->ActiveParticles; i++)
	{
		DECLARE_PARTICLE(Particle, Emitter->ParticleData + Emitter->ParticleStride * Emitter->ParticleIndices[i]);

		int32 CurrentOffset = FlexDataOffset;

		const uint8* ParticleBase = (const uint8*)&Particle;
		PARTICLE_ELEMENT(int32, FlexParticleIndex);

		FVector4 ParticlePos = Container->Particles[FlexParticleIndex];

		// skip infinite mass particles as they may already be attached to another component
		if (ParticlePos.W == 0.0f)
			continue;

		// test distance from component origin
		//FVector Delta = FVector(ParticlePos) - Transform.GetTranslation();

		//if (Delta.Size() < Radius)
		if (FVector::DistSquared(FVector(ParticlePos), ComponentPos) < Radius * Radius)
		{
			// calculate local space position of particle in component
			FVector LocalPos = ComponentTransform.InverseTransformPosition(ParticlePos);

			FlexParticleAttachment Attachment;
			Attachment.Primitive = Component;
			Attachment.ParticleIndex = FlexParticleIndex;
			Attachment.OldMass = ParticlePos.W;
			Attachment.LocalPos = LocalPos;
			Attachment.Velocity = FVector(0.0f);

			Attachments.Add(Attachment);
		}
	}
}

void FFlexParticleEmitterInstance::SynchronizeAttachments(float DeltaTime)
{
	// process attachments
	for (int32 AttachmentIndex = 0; AttachmentIndex < Attachments.Num(); )
	{
		FlexParticleAttachment& Attachment = Attachments[AttachmentIndex];
		const USceneComponent* SceneComp = Attachment.Primitive.Get();

		// index into the simulation data, we need to modify the container's copy
		// of the data so that the new positions get sent back to the sim
		const int ParticleIndex = Attachment.ParticleIndex; //AssetInstance->mParticleIndices[Attachment.ParticleIndex];

		if (SceneComp)
		{
			FTransform AttachTransform;

			const UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(SceneComp);

			if (PrimComp)
			{
				// primitive component attachments use the physics bodies
				AttachTransform = PrimComp->GetComponentToWorld();
			}
			else
			{
				// regular components attach to the actor transform
				AttachTransform = SceneComp->GetComponentTransform();
			}

			const FVector AttachedPos = AttachTransform.TransformPosition(Attachment.LocalPos);

			// keep the velocity so the particles can be "thrown" by their attachment 
			Attachment.Velocity = (AttachedPos - FVector(Container->Particles[ParticleIndex])) / DeltaTime;

			Container->Particles[ParticleIndex] = FVector4(AttachedPos, 0.0f);
			Container->Velocities[ParticleIndex] = FVector(0.0f);

			++AttachmentIndex;
		}
		else // process detachments
		{
			Container->Particles[ParticleIndex].W = Attachment.OldMass;
			// Allow the particles to keep their current velocity
			Container->Velocities[ParticleIndex] = Attachment.Velocity;

			Attachments.RemoveAtSwap(AttachmentIndex);
		}
	}
}

void FFlexParticleEmitterInstance::DestroyParticle(int32 FlexParticleIndex)
{
	Container->DestroyParticle(FlexParticleIndex);
	RemoveAttachmentForParticle(FlexParticleIndex);
}

void FFlexParticleEmitterInstance::RemoveAttachmentForParticle(int32 ParticleIndex)
{
	for (int32 AttachmentIndex = 0; AttachmentIndex < Attachments.Num(); ++AttachmentIndex)
	{
		const FlexParticleAttachment& Attachment = Attachments[AttachmentIndex];

		if (ParticleIndex == Attachment.ParticleIndex)
		{
			Container->Particles[ParticleIndex].W = Attachment.OldMass;
			Container->Velocities[ParticleIndex] = FVector(0.0f);

			Attachments.RemoveAtSwap(AttachmentIndex);
			break;
		}
	}
}
