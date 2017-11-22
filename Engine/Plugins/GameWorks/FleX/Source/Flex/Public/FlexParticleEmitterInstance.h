#pragma once

#include "NvFlex.h"
#include "NvFlexExt.h"

#include "FlexContainer.h"
#include "FlexParticleEmitterInstance.h"

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

	FFlexParticleEmitterInstance(FParticleEmitterInstance* Instance);
	virtual ~FFlexParticleEmitterInstance();

	void Tick(float DeltaTime, bool bSuppressSpawning);
	uint32 GetRequiredBytes(uint32 uiBytes);
	bool SpawnParticle(struct FBaseParticle* Particle, uint32 CurrentParticleIndex);
	void KillParticle(int32 KillIndex);
	bool IsDynamicDataRequired();

	virtual bool IsEnabled() { return Container != NULL; }
	virtual FBoxSphereBounds GetBounds() { return FBoxSphereBounds(Emitter->GetBoundingBox()); }
	virtual void Synchronize() {}

	void AddPendingComponentToAttach(USceneComponent* Component, float Radius);

	void ExecutePendingComponentsToAttach();

	void AttachToComponent(USceneComponent* Component, float Radius);

	void SynchronizeAttachments(float DeltaTime);

	void DestroyParticle(int32 FlexParticleIndex);

	void RemoveAttachmentForParticle(int32 ParticleIndex);


	FParticleEmitterInstance* Emitter;
	FFlexContainerInstance* Container;
	int32 Phase;

	//Currently only parented emitters will use theres for particle localization
	float LinearInertialScale;
	float AngularInertialScale;

	NvFlexExtMovingFrame MeshFrame;

	/* Attachments to force components */
	TArray<FlexParticleAttachment> Attachments;

	/* Pending "attachment to component" calls to process */
	TArray<FlexComponentAttachment> PendingAttachments;

	/** The offset to the index of the associated flex particle			*/
	int32 FlexDataOffset;
};
