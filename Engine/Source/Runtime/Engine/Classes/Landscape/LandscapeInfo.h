// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LandscapeLayerInfoObject.h"

#include "LandscapeInfo.generated.h"

/** Structure storing Collision for LandscapeComponent Add */
USTRUCT()
struct FLandscapeAddCollision
{
	GENERATED_USTRUCT_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FVector Corners[4];

#endif // WITH_EDITORONLY_DATA


		FLandscapeAddCollision()
		{
			#if WITH_EDITORONLY_DATA
			Corners[0] = Corners[1] = Corners[2] = Corners[3] = FVector::ZeroVector;
			#endif // WITH_EDITORONLY_DATA
		}
	
};

USTRUCT()
struct FLandscapeInfoLayerSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	class ULandscapeLayerInfoObject* LayerInfoObj;

	UPROPERTY()
	FName LayerName;

#if WITH_EDITORONLY_DATA
	UPROPERTY(transient)
	class UMaterialInstanceConstant* ThumbnailMIC;

	UPROPERTY()
	class ALandscapeProxy* Owner;

	UPROPERTY(transient)
	int32 DebugColorChannel;

	UPROPERTY(transient)
	uint32 bValid:1;
#endif // WITH_EDITORONLY_DATA

	FLandscapeInfoLayerSettings()
		: LayerInfoObj(NULL)
		, LayerName(NAME_None)
#if WITH_EDITORONLY_DATA
		, ThumbnailMIC(NULL)
		, Owner(NULL)
		, DebugColorChannel(0)
		, bValid(false)
#endif // WITH_EDITORONLY_DATA
	{
	}

	FLandscapeInfoLayerSettings(ULandscapeLayerInfoObject* InLayerInfo, class ALandscapeProxy* InProxy)
		: LayerInfoObj(InLayerInfo)
		, LayerName((InLayerInfo != NULL) ? InLayerInfo->LayerName : NAME_None)
#if WITH_EDITORONLY_DATA
		, ThumbnailMIC(NULL)
		, Owner(InProxy)
		, DebugColorChannel(0)
		, bValid(false)
#endif
	{
	}

	FLandscapeInfoLayerSettings(FName InPlaceholderLayerName, class ALandscapeProxy* InProxy)
		: LayerInfoObj(NULL)
		, LayerName(InPlaceholderLayerName)
#if WITH_EDITORONLY_DATA
		, ThumbnailMIC(NULL)
		, Owner(InProxy)
		, DebugColorChannel(0)
		, bValid(false)
#endif
	{
	}

	ENGINE_API FName GetLayerName() const;

#if WITH_EDITORONLY_DATA
	ENGINE_API struct FLandscapeEditorLayerSettings& GetEditorSettings() const;
#endif
};

UCLASS(HeaderGroup=Terrain)
class ULandscapeInfo : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TLazyObjectPtr<class ALandscape> LandscapeActor;

	UPROPERTY()
	FGuid LandscapeGuid;

	UPROPERTY()
	int32 ComponentSizeQuads;
	
	UPROPERTY()
	int32 SubsectionSizeQuads;

	UPROPERTY()
	int32 ComponentNumSubsections;
	
	UPROPERTY()
	FVector DrawScale;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(transient)
	uint32 bIsValid:1;

	UPROPERTY()
	TArray<struct FLandscapeInfoLayerSettings> Layers;
#endif // WITH_EDITORONLY_DATA

public:
	struct FLandscapeDataInterface* DataInterface;

	/** Map of the offsets (in component space) to the component. Valid in editor only. */
	TMap<FIntPoint,class ULandscapeComponent*> XYtoComponentMap;

	/** Map of the offsets to the newly added collision components. Only available near valid LandscapeComponents. Valid in editor only. */
	TMap<FIntPoint, FLandscapeAddCollision> XYtoAddCollisionMap;

	TSet<class ALandscapeProxy*> Proxies;

private:
	TSet<class ULandscapeComponent*> SelectedComponents;
	TSet<class ULandscapeComponent*> SelectedRegionComponents;

public:

	/** True if we're currently editing this landscape */
	bool bCurrentlyEditing;

	TMap<FIntPoint,float> SelectedRegion;

	// Begin UObject Interface.
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void BeginDestroy() OVERRIDE;
#if WITH_EDITOR
	virtual void PostEditUndo() OVERRIDE;
	// End UObject Interface

	// @todo document 
	// all below.
	ENGINE_API struct FLandscapeDataInterface* GetDataInterface();

	ENGINE_API void GetComponentsInRegion(int32 X1, int32 Y1, int32 X2, int32 Y2, TSet<ULandscapeComponent*>& OutComponents);
	ENGINE_API bool GetLandscapeExtent(int32& MinX, int32& MinY, int32& MaxX, int32& MaxY);
	ENGINE_API void Export(const TArray<ULandscapeLayerInfoObject*>& LayerInfos, const TArray<FString>& Filenames);
	ENGINE_API void ExportHeightmap(const FString& Filename);
	ENGINE_API void ExportLayer(ULandscapeLayerInfoObject* LayerInfo, const FString& Filename);
	ENGINE_API bool ApplySplines(bool bOnlySelected);

	ENGINE_API bool GetSelectedExtent(int32& MinX, int32& MinY, int32& MaxX, int32& MaxY);
	FVector GetLandscapeCenterPos(float& LengthZ, int32 MinX = MAX_int32, int32 MinY = MAX_int32, int32 MaxX = MIN_int32, int32 MaxY = MIN_int32);
	ENGINE_API bool IsValidPosition(int32 X, int32 Y);
	ENGINE_API void DeleteLayer(ULandscapeLayerInfoObject* LayerInfo);
	ENGINE_API void ReplaceLayer(ULandscapeLayerInfoObject* FromLayerInfo, ULandscapeLayerInfoObject* ToLayerInfo);

	ENGINE_API void UpdateDebugColorMaterial();

	ENGINE_API TSet<class ULandscapeComponent*> GetSelectedComponents() const;
	ENGINE_API TSet<class ULandscapeComponent*> GetSelectedRegionComponents() const;
	ENGINE_API void UpdateSelectedComponents(TSet<class ULandscapeComponent*>& NewComponents, bool bIsComponentwise = true);
	ENGINE_API void SortSelectedComponents();
	ENGINE_API void ClearSelectedRegion(bool bIsComponentwise = true);

	ENGINE_API void UpdateAllAddCollisions();
	void UpdateAddCollision(FIntPoint LandscapeKey);

	ENGINE_API FLandscapeEditorLayerSettings& GetLayerEditorSettings(ULandscapeLayerInfoObject* LayerInfo) const;
	ENGINE_API void CreateLayerEditorSettingsFor(ULandscapeLayerInfoObject* LayerInfo);

	ENGINE_API ULandscapeLayerInfoObject* GetLayerInfoByName(FName LayerName, class ALandscapeProxy* Owner = NULL) const;
	ENGINE_API int32 GetLayerInfoIndex(FName LayerName, class ALandscapeProxy* Owner = NULL) const;
	ENGINE_API int32 GetLayerInfoIndex(ULandscapeLayerInfoObject* LayerInfo, class ALandscapeProxy* Owner = NULL) const;
	ENGINE_API bool UpdateLayerInfoMap(class ALandscapeProxy* Proxy = NULL, bool bInvalidate = false);

	/** 
	 *  Returns landscape which is spawned in the current level that was previously added to this landscape info object
	 *  @param	bRegistered		Whether to consider only registered(visible) landscapes
	 *	@return					Landscape or landscape proxy found in the current level 
	 */
	ENGINE_API ALandscapeProxy* GetCurrentLevelLandscapeProxy(bool bRegistered) const;
	
	/** 
	 *	returns shared landscape or landscape proxy, mostly for transformations
	 *	@todo: should be removed
	 */
	ENGINE_API ALandscapeProxy* GetLandscapeProxy() const;

	/** Associates passed actor with this info object
 	 *  @param	Proxy		Landscape actor to register
	 *  @param  bMapCheck	Whether to warn about landscape errors
	 */
	ENGINE_API void RegisterActor(ALandscapeProxy* Proxy, bool bMapCheck = false);
	
	/** Deassociates passed actor with this info object*/
	ENGINE_API void UnregisterActor(ALandscapeProxy* Proxy);

	/** Associates passed landscape component with this info object
	 *  @param	Component	Landscape component to register
	 *  @param  bMapCheck	Whether to warn about landscape errors
	 */
	ENGINE_API void RegisterActorComponent(ULandscapeComponent* Component, bool bMapCheck = false);
	
	/** Deassociates passed landscape component with this info object*/
	ENGINE_API void UnregisterActorComponent(ULandscapeComponent* Component);
		
	/** Resets all actors, proxies, components registrations */
	ENGINE_API void Reset();

	/** Recreate all LandscapeInfo objects in given world
	 *  @param  bMapCheck	Whether to warn about landscape errors
	 */
	ENGINE_API static void RecreateLandscapeInfo(UWorld* InWorld, bool bMapCheck);

	/** 
	 *  Fixes up proxies relative position to landscape actor
	 *  basically makes sure that each LandscapeProxy RootComponent transform reflects LandscapeSectionOffset value
	 *  requires LandscapeActor to be loaded
	 *  Does not work in World composition mode!
	 */
	ENGINE_API void FixupProxiesTransform();
	
	/** Fix up component layers, weightmaps for all registered proxies
	 */
	ENGINE_API void FixupProxiesWeightmaps();

#endif
};



