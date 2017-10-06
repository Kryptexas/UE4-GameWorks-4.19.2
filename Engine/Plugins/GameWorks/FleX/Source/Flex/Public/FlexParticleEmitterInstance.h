#pragma once

#include "NvFlex.h"
#include "NvFlexExt.h"

#include "PhysXIncludes.h"

#include "FlexParticleSpriteEmitter.h"

/*-----------------------------------------------------------------------------
FFlexParticleEmitterInstance
-----------------------------------------------------------------------------*/

struct FFlexParticleEmitterInstance : public IFlexContainerClient
{
	// Copied from UFlexComponent
	struct FlexParticleAttachment
	{
		TWeakObjectPtr<USceneComponent> Primitive;
		int32 ParticleIndex;
		float OldMass;
		FVector LocalPos;
		FVector Velocity;
	};

	struct FlexComponentAttachment
	{
		FlexComponentAttachment(USceneComponent* InComponent, float InRadius)
			: Component(InComponent)
			, Radius(InRadius)
		{}

		USceneComponent* Component;
		float Radius;
	};

	FFlexParticleEmitterInstance(FParticleEmitterInstance* Instance)
	{
		Emitter = Instance;

		auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(Emitter->SpriteTemplate);
		if (FlexEmitter && FlexEmitter->FlexContainerTemplate)
		{
			FPhysScene* Scene = Emitter->Component->GetWorld()->GetPhysicsScene();

			Container = FFlexManager::get().GetFlexContainer(Scene, FlexEmitter->FlexContainerTemplate);
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
		}
	}

	virtual ~FFlexParticleEmitterInstance()
	{
		if (Container)
			Container->Unregister(this);
	}

	virtual bool IsEnabled() { return Container != NULL; }
	virtual FBoxSphereBounds GetBounds() { return FBoxSphereBounds(Emitter->GetBoundingBox()); }
	virtual void Synchronize() {}

	FParticleEmitterInstance* Emitter;
	FFlexContainerInstance* Container;
	int32 Phase;

	//Currently only parented emitters will use theres for particle localization
	float LinearInertialScale;
	float AngularInertialScale;

	NvFlexExtMovingFrame MeshFrame;

	void AddPendingComponentToAttach(USceneComponent* Component, float Radius)
	{
		FlexComponentAttachment PendingAttach(Component, Radius);
		PendingAttachments.Add(PendingAttach);
	}

	void ExecutePendingComponentsToAttach()
	{
		for (int32 i = 0; i < PendingAttachments.Num(); i++)
		{
			AttachToComponent(PendingAttachments[i].Component, PendingAttachments[i].Radius);
		}
		PendingAttachments.Empty();
	}

	void AttachToComponent(USceneComponent* Component, float Radius)
	{
		const FTransform ComponentTransform = Component->GetComponentTransform();
		const FVector ComponentPos = Component->GetComponentTransform().GetTranslation();

		for (int32 i = 0; i < Emitter->ActiveParticles; i++)
		{
			DECLARE_PARTICLE(Particle, Emitter->ParticleData + Emitter->ParticleStride * Emitter->ParticleIndices[i]);

			int32 CurrentOffset = Emitter->FlexDataOffset;

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

	void SynchronizeAttachments(float DeltaTime)
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

	void DestroyParticle(int32 FlexParticleIndex)
	{
		Container->DestroyParticle(FlexParticleIndex);
		RemoveAttachmentForParticle(FlexParticleIndex);
	}

	void RemoveAttachmentForParticle(int32 ParticleIndex)
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

	/* Attachments to force components */
	TArray<FlexParticleAttachment> Attachments;

	/* Pending "attachment to component" calls to process */
	TArray<FlexComponentAttachment> PendingAttachments;
};
