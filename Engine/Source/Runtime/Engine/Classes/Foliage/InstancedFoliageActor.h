// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InstancedFoliageActor.generated.h"

UCLASS(notplaceable, HeaderGroup = Foliage, hidecategories = Object, MinimalAPI, NotBlueprintable)
class AInstancedFoliageActor : public AActor
{
	GENERATED_UCLASS_BODY()

	/** The static mesh type that will be used to show the widget */
	UPROPERTY(transient)
	class UStaticMesh* SelectedMesh;


public:
	TMap<class UStaticMesh*, struct FFoliageMeshInfo> FoliageMeshes;

	// Begin UObject interface. 	
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// Begin UObject interface. 

	// Begin AActor interface.
	// we don't want to have our components automatically destroyed by the Blueprint code
	virtual void RerunConstructionScripts() OVERRIDE {}
	// Add world origin offset
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) OVERRIDE;
	// End AActor interface.

#if WITH_EDITOR

	// Called in response to FEditorDelegates::MapChange.Broadcast(MapChangeEventFlags::MapRebuild ) so BSP rebuilds migrate foliage from obsolete to new components.
	ENGINE_API void MapRebuild();

	// Called from editor code to manage instances for components
	ENGINE_API void SnapInstancesForLandscape( class ULandscapeHeightfieldCollisionComponent* InComponent, const FBox& InInstanceBox );

	// Moves instances based on the specified component to the current streaming level
	ENGINE_API void MoveInstancesForComponentToCurrentLevel( class UActorComponent* InComponent );

	// Change all instances based on one component to a new component.
	void MoveInstancesToNewComponent( class UActorComponent* InOldComponent, class UActorComponent* InNewComponent );

	// Move instances based on a component that has just been moved.
	void MoveInstancesForMovedComponent( class UActorComponent* InComponent );

	// Returns a map of Static Meshes and their placed instances attached to a component.
	ENGINE_API TMap<class UStaticMesh*,TArray<const struct FFoliageInstancePlacementInfo*> > GetInstancesForComponent( class UActorComponent* InComponent );

	// Deletes the instances attached to a component
	ENGINE_API void DeleteInstancesForComponent( class UActorComponent* InComponent );

	// Add a new static mesh.
	ENGINE_API struct FFoliageMeshInfo* AddMesh( class UStaticMesh* InMesh );

	// Remove the static mesh from the mesh list, and all its instances.
	ENGINE_API void RemoveMesh( class UStaticMesh* InMesh );

	// Select an individual instance.
	ENGINE_API void SelectInstance( class UInstancedStaticMeshComponent* InComponent, int32 InComponentInstanceIndex, bool bToggle );

	// Propagate the selected instances to the actual render components
	ENGINE_API void ApplySelectionToComponents( bool bApply );

	// Updates the SelectedMesh property of the actor based on the actual selected instances
	ENGINE_API void CheckSelection();

	// Returns the location for the widget
	ENGINE_API FVector GetSelectionLocation();

	/*
 	* Get the instanced foliage actor for the current streaming level.
 	*
 	* @param InCreationWorldIfNone			World to create the foliage instance in
	* @param bCreateIfNone					Create if doesnt already exist
 	* returns								pointer to foliage object instance
 	*/
 	static ENGINE_API AInstancedFoliageActor* GetInstancedFoliageActor(UWorld* InWorld,bool bCreateIfNone=true);
	

	// Get the instanced foliage actor for the specified streaming level. Never creates a new IFA.
	static ENGINE_API AInstancedFoliageActor* GetInstancedFoliageActorForLevel(ULevel* Level);
#endif	//WITH_EDITOR
};



