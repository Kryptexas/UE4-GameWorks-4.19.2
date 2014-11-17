// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AI/Navigation/NavRelevantActorInterface.h"
#include "StaticMeshActor.generated.h"

/**
 * An instance of a StaticMesh in a level
 * Note that PostInitializeComponents() is not called for StaticMeshActors
 */
UCLASS(hidecategories=(Input), showcategories=("Input|MouseInput", "Input|TouchInput"), ConversionRoot, meta=(ChildCanTick))
class ENGINE_API AStaticMeshActor : public AActor, public INavRelevantActorInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=StaticMeshActor, VisibleAnywhere, BlueprintReadOnly, meta=(ExposeFunctionCategories="Mesh,Rendering,Physics,Components|StaticMesh"))
	TSubobjectPtr<class UStaticMeshComponent> StaticMeshComponent;
	
	virtual void BeginPlay() override;

	/** This static mesh should replicate movement. Automatically sets the RemoteRole and bReplicateMovement flags. Meant to be edited on placed actors (those other two proeprties are not) */
	UPROPERTY(Category=Actor, EditAnywhere, AdvancedDisplay)
	bool bStaticMeshReplicateMovement;

	/** Function to change mobility type of light */
	void SetMobility(EComponentMobility::Type InMobility);

#if WITH_EDITOR
	// Begin AActor Interface
	virtual void CheckForErrors() override;
	virtual bool GetReferencedContentObjects( TArray<UObject*>& Objects ) const override;
	// End AActor Interface
#endif // WITH_EDITOR	

protected:
	// Begin UObject interface.
	virtual FString GetDetailedInfoInternal() const override;
#if WITH_EDITOR
	virtual void LoadedFromAnotherClass(const FName& OldClassName) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR	
	// End UObject interface.
};



