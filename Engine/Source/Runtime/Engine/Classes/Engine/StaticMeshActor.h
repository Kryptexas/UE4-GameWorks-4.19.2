// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "StaticMeshActor.generated.h"

/**
 * An instance of a StaticMesh in a level
 * Note that PostInitializeComponents() is not called for StaticMeshActors
 */
UCLASS(hidecategories=(Input), ConversionRoot, meta=(ChildCanTick))
class ENGINE_API AStaticMeshActor : public AActor, public INavRelevantActorInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=StaticMeshActor, VisibleAnywhere, BlueprintReadOnly, meta=(ExposeFunctionCategories="Mesh,Rendering,Physics,Components|StaticMesh"))
	TSubobjectPtr<class UStaticMeshComponent> StaticMeshComponent;

	/** This static mesh should replicate movement. Automatically sets the RemoteRole and bReplicateMovement flags. Meant to be edited on placed actors (those other two proeprties are not) */
	UPROPERTY(Category=Actor, EditAnywhere, AdvancedDisplay)
	bool bStaticMeshReplicateMovement;

	/** Function to change mobility type of light */
	void SetMobility(EComponentMobility::Type InMobility);

#if WITH_EDITOR
	// Begin AActor Interface
	virtual void CheckForErrors() OVERRIDE;
	virtual bool GetReferencedContentObjects( TArray<UObject*>& Objects ) const OVERRIDE;
	// End AActor Interface
#endif // WITH_EDITOR	

protected:
	// Begin UObject interface.
	virtual FString GetDetailedInfoInternal() const OVERRIDE;
#if WITH_EDITOR
	virtual void LoadedFromAnotherClass(const FName& OldClassName) OVERRIDE;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR	
	// End UObject interface.
};



