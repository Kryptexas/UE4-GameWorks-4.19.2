// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "GameFramework/Actor.h"
#include "Templates/UniqueObj.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "InstancedFoliageActor.generated.h"

// Forward declarations
class UFoliageType;
struct FFoliageInstancePlacementInfo;
struct FFoliageMeshInfo;
class UFoliageType_InstancedStaticMesh;

UCLASS(notplaceable, hidecategories = Object, MinimalAPI, NotBlueprintable)
class AInstancedFoliageActor : public AActor
{
	GENERATED_UCLASS_BODY()

	/** The static mesh type that will be used to show the widget */
	UPROPERTY(transient)
	UFoliageType* SelectedMesh;

public:
	TMap<UFoliageType*, TUniqueObj<FFoliageMeshInfo>> FoliageMeshes;

public:
	// Begin UObject interface.
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject interface. 

	// Begin AActor interface.
	// we don't want to have our components automatically destroyed by the Blueprint code
	virtual void RerunConstructionScripts() override {}
	virtual void PostRegisterAllComponents() override;
	virtual void PostUnregisterAllComponents() override;
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;
	// End AActor interface.


	// Performs a reverse lookup from a mesh to its settings
	FOLIAGE_API UFoliageType* GetSettingsForMesh(const UStaticMesh* InMesh, FFoliageMeshInfo** OutMeshInfo = nullptr);

	// Finds the number of instances overlapping with the sphere. 
	FOLIAGE_API int32 GetOverlappingSphereCount(const UFoliageType* FoliageType, const FSphere& Sphere) const;

	// Finds a mesh entry
	FOLIAGE_API FFoliageMeshInfo* FindMesh(const UFoliageType* InType);

	// Finds a mesh entry
	FOLIAGE_API const FFoliageMeshInfo* FindMesh(const UFoliageType* InType) const;

	/**
	* Get the instanced foliage actor for the current streaming level.
	*
	* @param InCreationWorldIfNone			World to create the foliage instance in
	* @param bCreateIfNone					Create if doesnt already exist
	* returns								pointer to foliage object instance
	*/
	static FOLIAGE_API AInstancedFoliageActor* GetInstancedFoliageActorForCurrentLevel(UWorld* InWorld, bool bCreateIfNone = true);


	/**
	* Get the instanced foliage actor for the specified streaming level.
	* @param bCreateIfNone					Create if doesnt already exist
	* returns								pointer to foliage object instance
	*/
	static FOLIAGE_API AInstancedFoliageActor* GetInstancedFoliageActorForLevel(ULevel* Level, bool bCreateIfNone = true);

#if WITH_EDITOR
	virtual void PostEditUndo() override;

	// Called in response to BSP rebuilds to migrate foliage from obsolete to new components.
	FOLIAGE_API void MapRebuild();

	// Moves instances based on the specified component to the current streaming level
	FOLIAGE_API void MoveInstancesForComponentToCurrentLevel(UActorComponent* InComponent);

	// Change all instances based on one component to a new component (possible in another level).
	// The instances keep the same world locations
	FOLIAGE_API void MoveInstancesToNewComponent(UPrimitiveComponent* InOldComponent, UPrimitiveComponent* InNewComponent);

	// Move instances based on a component that has just been moved.
	void MoveInstancesForMovedComponent(UActorComponent* InComponent);
	
	// Returns a map of Static Meshes and their placed instances attached to a component.
	FOLIAGE_API TMap<UFoliageType*, TArray<const FFoliageInstancePlacementInfo*>> GetInstancesForComponent(UActorComponent* InComponent);

	// Deletes the instances attached to a component
	FOLIAGE_API void DeleteInstancesForComponent(UActorComponent* InComponent);

	// Deletes the instances spawned by a component
	FOLIAGE_API void DeleteInstancesForSpawner(UActorComponent* InComponent);

	// Finds a mesh entry or adds it if it doesn't already exist
	FOLIAGE_API FFoliageMeshInfo* FindOrAddMesh(UFoliageType* InType);

	// Add a new static mesh.
	FOLIAGE_API FFoliageMeshInfo* AddMesh(UStaticMesh* InMesh, UFoliageType** OutSettings = nullptr, const UFoliageType_InstancedStaticMesh* DefaultSettings = nullptr);
	FOLIAGE_API FFoliageMeshInfo* AddMesh(UFoliageType* InType);

	// Remove the static mesh from the mesh list, and all its instances.
	FOLIAGE_API void RemoveMesh(UFoliageType* InFoliageType);

	// Select an individual instance.
	FOLIAGE_API void SelectInstance(UInstancedStaticMeshComponent* InComponent, int32 InComponentInstanceIndex, bool bToggle);

	// Propagate the selected instances to the actual render components
	FOLIAGE_API void ApplySelectionToComponents(bool bApply);

	// Updates the SelectedMesh property of the actor based on the actual selected instances
	FOLIAGE_API void CheckSelection();

	// Returns the location for the widget
	FOLIAGE_API FVector GetSelectionLocation();

	// Transforms Editor specific data which is stored in world space
	FOLIAGE_API void ApplyLevelTransform(const FTransform& LevelTransform);

	/* Called to notify InstancedFoliageActor that a UFoliageType has been modified */
	void NotifyFoliageTypeChanged(UFoliageType* FoliageType);

#endif	//WITH_EDITOR

private:
#if WITH_EDITOR
	void OnLevelActorMoved(AActor* InActor);
#endif
private:
#if WITH_EDITOR
	FDelegateHandle OnLevelActorMovedDelegateHandle;
	FDelegateHandle OnApplyLevelTransformDelegateHandle;
#endif

};
