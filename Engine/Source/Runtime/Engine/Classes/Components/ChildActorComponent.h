// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ChildActorComponent.generated.h"

class FChildActorComponentInstanceData;

/** A component that spawns an Actor when registered, and destroys it when unregistered.*/
UCLASS(ClassGroup=Utility, hidecategories=(Object,LOD,Physics,Lighting,TextureStreaming,Activation,"Components|Activation",Collision), meta=(BlueprintSpawnableComponent), MinimalAPI)
class UChildActorComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** The class of Actor to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=ChildActorComponent)
	TSubclassOf<AActor>	ChildActorClass;

	/** The actor that we spawned and own */
	UPROPERTY(BlueprintReadOnly, Category = ChildActorComponent, TextExportTransient)
	AActor*	ChildActor;

	/** We try to keep the child actor's name as best we can, so we store it off here when destroying */
	FName ChildActorName;

	/** Cached copy of the instance data when the ChildActor is destroyed to be available when needed */
	TSharedPtr<FChildActorComponentInstanceData> CachedInstanceData;

	// Begin ActorComponent interface.
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed() override;
	virtual TSharedPtr<FComponentInstanceDataBase> GetComponentInstanceData() const override;
	virtual FName GetComponentInstanceDataType() const override;
	virtual void ApplyComponentInstanceData(TSharedPtr<FComponentInstanceDataBase> ComponentInstanceData) override;

	// End ActorComponent interface.

	/** Create the child actor */
	void CreateChildActor();

	/** Kill any currently present child actor */
	void DestroyChildActor();
};



