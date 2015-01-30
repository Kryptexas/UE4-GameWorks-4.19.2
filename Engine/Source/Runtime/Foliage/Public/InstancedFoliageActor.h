// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "GameFramework/Actor.h"
#include "Templates/UniqueObj.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "FoliageInstanceBase.h"

#include "InstancedFoliageActor.generated.h"

// Forward declarations
class UFoliageType;
struct FFoliageInstancePlacementInfo;
struct FFoliageMeshInfo;
class UFoliageType_InstancedStaticMesh;
class UProceduralFoliageComponent;
struct FFoliageInstance;
struct FHitResult;
struct FDesiredFoliageInstance;

UCLASS(notplaceable, hidecategories = Object, MinimalAPI, NotBlueprintable)
class AInstancedFoliageActor : public AActor
{
	GENERATED_UCLASS_BODY()

	/** The static mesh type that will be used to show the widget */
	UPROPERTY(transient)
	UFoliageType* SelectedMesh;

public:
#if WITH_EDITORONLY_DATA
	// Cross level references cache for instances base
	FFoliageInstanceBaseCache InstanceBaseCache;
#endif// WITH_EDITORONLY_DATA

	TMap<UFoliageType*, TUniqueObj<FFoliageMeshInfo>> FoliageMeshes;

public:
	// Begin UObject interface.
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;
	virtual void PostLoad() override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject interface. 

	// Begin AActor interface.
	// we don't want to have our components automatically destroyed by the Blueprint code
	virtual void RerunConstructionScripts() override {}
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;
	// End AActor interface.


	// Performs a reverse lookup from a mesh to its settings
	FOLIAGE_API UFoliageType* GetSettingsForMesh(const UStaticMesh* InMesh, FFoliageMeshInfo** OutMeshInfo = nullptr);

	// Finds the number of instances overlapping with the sphere. 
	FOLIAGE_API int32 GetOverlappingSphereCount(const UFoliageType* FoliageType, const FSphere& Sphere) const;
	// Finds the number of instances overlapping with the box. 
	FOLIAGE_API int32 GetOverlappingBoxCount(const UFoliageType* FoliageType, const FBox& Box) const;
	// Finds all instances in the provided box and get their transforms
	FOLIAGE_API void GetOverlappingBoxTransforms(const UFoliageType* FoliageType, const FBox& Box, TArray<FTransform>& OutTransforms) const;

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

	static FOLIAGE_API bool FoliageTrace(const UWorld* InWorld, FHitResult& OutHit, const FDesiredFoliageInstance& DesiredInstance, const AInstancedFoliageActor* IgnoreIFA = nullptr, FName InTraceTag = NAME_None, bool InbReturnFaceIndex = false);
	FOLIAGE_API bool CheckCollisionWithWorld(const UFoliageType* Settings, const FFoliageInstance& Inst, const FVector& HitNormal, const FVector& HitLocation);

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

	// Deletes the instances spawned by a procedural component
	void DeleteInstancesForProceduralFoliageComponent(const UProceduralFoliageComponent* ProceduralComponent);

	// Finds a mesh entry or adds it if it doesn't already exist
	FOLIAGE_API FFoliageMeshInfo* FindOrAddMesh(UFoliageType* InType);

	// Add a new static mesh.
	FOLIAGE_API FFoliageMeshInfo* AddMesh(UStaticMesh* InMesh, UFoliageType** OutSettings = nullptr, const UFoliageType_InstancedStaticMesh* DefaultSettings = nullptr);
	FOLIAGE_API FFoliageMeshInfo* AddMesh(UFoliageType* InType);
	FOLIAGE_API FFoliageMeshInfo* UpdateMeshSettings(const UStaticMesh* InMesh, const UFoliageType_InstancedStaticMesh* DefaultSettings);

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

	/* Called to notify InstancedFoliageActor that a UFoliageType has been modified */
	void NotifyFoliageTypeChanged(UFoliageType* FoliageType);

#endif	//WITH_EDITOR

private:
#if WITH_EDITORONLY_DATA
	// Deprecated data, will be converted and cleaned up in PostLoad
	TMap<UFoliageType*, TUniqueObj<struct FFoliageMeshInfo_Deprecated>> FoliageMeshes_Deprecated;
#endif//WITH_EDITORONLY_DATA
	
#if WITH_EDITOR
	void OnLevelActorMoved(AActor* InActor);
	void OnLevelActorDeleted(AActor* InActor);
#endif
private:
#if WITH_EDITOR
	FDelegateHandle OnLevelActorMovedDelegateHandle;
	FDelegateHandle OnLevelActorDeletedDelegateHandle;
#endif

};
