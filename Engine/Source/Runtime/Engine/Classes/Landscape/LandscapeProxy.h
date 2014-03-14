// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "LandscapeInfo.h"
#include "PhysicsEngine/BodyInstance.h"

#include "LandscapeProxy.generated.h"

/** Structure storing channel usage for weightmap textures */
USTRUCT()
struct FLandscapeWeightmapUsage
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	class ULandscapeComponent* ChannelUsage[4];

	FLandscapeWeightmapUsage()
	{
		ChannelUsage[0] = NULL;
		ChannelUsage[1] = NULL;
		ChannelUsage[2] = NULL;
		ChannelUsage[3] = NULL;
	}
	friend FArchive& operator<<( FArchive& Ar, FLandscapeWeightmapUsage& U );
	int32 FreeChannelCount() const
	{
		return	((ChannelUsage[0] == NULL) ? 1 : 0) + 
				((ChannelUsage[1] == NULL) ? 1 : 0) + 
				((ChannelUsage[2] == NULL) ? 1 : 0) + 
				((ChannelUsage[3] == NULL) ? 1 : 0);
	}
};

USTRUCT()
struct FLandscapeEditorLayerSettings
{
	GENERATED_USTRUCT_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class ULandscapeLayerInfoObject* LayerInfoObj;

	UPROPERTY()
	FString ReimportLayerFilePath;

	FLandscapeEditorLayerSettings()
		: LayerInfoObj(NULL)
		, ReimportLayerFilePath()
	{
	}

	FLandscapeEditorLayerSettings(ULandscapeLayerInfoObject* InLayerInfo, const FString& InFilePath = FString())
		: LayerInfoObj(InLayerInfo)
		, ReimportLayerFilePath(InFilePath)
	{
	}

	bool operator==(const FLandscapeEditorLayerSettings& rhs)
	{
		return LayerInfoObj == rhs.LayerInfoObj;
	}

	bool operator==(const class ULandscapeLayerInfoObject*& rhs)
	{
		return LayerInfoObj == rhs;
	}
#endif // WITH_EDITORONLY_DATA
};

USTRUCT()
struct FLandscapeLayerStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	class ULandscapeLayerInfoObject* LayerInfoObj;

#if WITH_EDITORONLY_DATA
	UPROPERTY(transient)
	class UMaterialInstanceConstant* ThumbnailMIC;

	UPROPERTY()
	class ALandscapeProxy* Owner;

	UPROPERTY(transient)
	int32 DebugColorChannel;

	UPROPERTY(transient)
	uint32 bSelected:1;

	UPROPERTY()
	FString SourceFilePath;
#endif // WITH_EDITORONLY_DATA

	FLandscapeLayerStruct()
		: LayerInfoObj(NULL)
#if WITH_EDITORONLY_DATA
		, ThumbnailMIC(NULL)
		, Owner(NULL)
		, DebugColorChannel(0)
		, bSelected(false)
		, SourceFilePath()
#endif // WITH_EDITORONLY_DATA
	{
	}
};

/** Structure storing Layer Data for import */
USTRUCT()
struct FLandscapeImportLayerInfo
{
	GENERATED_USTRUCT_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(Category="Import", VisibleAnywhere)
	FName LayerName;

	UPROPERTY(Category="Import", EditAnywhere)
	ULandscapeLayerInfoObject* LayerInfo;

	UPROPERTY(Category="Import", VisibleAnywhere)
	class UMaterialInstanceConstant* ThumbnailMIC; // Optional

	UPROPERTY(Category="Import", EditAnywhere)
	FString SourceFilePath; // Optional
#endif

#if WITH_EDITOR
	FLandscapeImportLayerInfo(FName InLayerName = NAME_None)
	:	LayerName(InLayerName)
	,	LayerInfo(NULL)
	,	ThumbnailMIC(NULL)
	,	SourceFilePath("")
	{
	}

	FLandscapeImportLayerInfo(const struct FLandscapeInfoLayerSettings& InLayerSettings)
	:	LayerName(InLayerSettings.GetLayerName())
	,	LayerInfo(InLayerSettings.LayerInfoObj)
	,	ThumbnailMIC(NULL)
	,	SourceFilePath(InLayerSettings.GetEditorSettings().ReimportLayerFilePath)
	{
	}
#endif
};

UENUM()
namespace ELandscapeLayerPaintingRestriction
{
	enum Type
	{
		// No restriction, can paint anywhere (default)
		None         UMETA(DisplayName="None"),

		// Uses the MaxPaintedLayersPerComponent setting from the LandscapeProxy
		UseMaxLayers UMETA(DisplayName="Limit Layer Count"),

		// Restricts painting to only components that already have this layer
		ExistingOnly UMETA(DisplayName="Existing Layers Only"),
	};
}

UCLASS(HeaderGroup=Terrain, dependson=UEngineTypes, NotPlaceable, hidecategories=(Display, Attachment, Physics, Debug, Lighting, LOD), showcategories=(Rendering, "Utilities|Orientation"), MinimalAPI)
class ALandscapeProxy : public AInfo
{
	GENERATED_UCLASS_BODY()

protected:
	/** Guid for LandscapeEditorInfo **/
	UPROPERTY()
	FGuid LandscapeGuid;

public:
	/** Offset in quads from landscape actor origin **/
	UPROPERTY()
	FIntPoint LandscapeSectionOffset;

	/** Max LOD level to use when rendering */
	UPROPERTY(EditAnywhere, Category=LOD)
	int32 MaxLODLevel;

#if WITH_EDITORONLY_DATA
	/** LOD level to use when exporting the landscape to obj or FBX */
	UPROPERTY(EditAnywhere, Category=LOD)
	int32 ExportLOD;
#endif

	/** LOD level to use when running lightmass (increase to 1 or 2 for large landscapes to stop lightmass crashing) */
	UPROPERTY(EditAnywhere, Category=Lighting)
	int32 StaticLightingLOD;

	/** Default physical material, used when no per-layer values physical materials */
	UPROPERTY(EditAnywhere, Category=Landscape)
	class UPhysicalMaterial* DefaultPhysMaterial;

	/**
	 * Allows artists to adjust the distance where textures using UV 0 are streamed in/out.
	 * 1.0 is the default, whereas a higher value increases the streamed-in resolution.
	 */
	UPROPERTY(EditAnywhere, Category=Landscape)
	float StreamingDistanceMultiplier;

	/** Combined material used to render the landscape */
	UPROPERTY(EditAnywhere, Category=Landscape)
	class UMaterialInterface* LandscapeMaterial;

	/** Material used to render landscape components with holes. Should be a BLEND_Masked version of LandscapeMaterial. */
	UPROPERTY(EditAnywhere, Category=Landscape)
	class UMaterialInterface* LandscapeHoleMaterial;

	UPROPERTY(EditAnywhere, Category=LOD)
	float LODDistanceFactor;

	/** The array of LandscapeComponent that are used by the landscape */
	UPROPERTY()
	TArray<class ULandscapeComponent*> LandscapeComponents;

	/** Array of LandscapeHeightfieldCollisionComponent */
	UPROPERTY()
	TArray<class ULandscapeHeightfieldCollisionComponent*> CollisionComponents;

	/**
	 *	The resolution to cache lighting at, in texels/quad in one axis
	 *  Total resolution would be changed by StaticLightingResolution*StaticLightingResolution
	 *	Automatically calculate proper value for removing seams
	 */
	UPROPERTY(EditAnywhere, Category=Lighting)
	float StaticLightingResolution;

	UPROPERTY(EditAnywhere, Category=LandscapeProxy)
	TLazyObjectPtr<class ALandscape> LandscapeActor;

	UPROPERTY(EditAnywhere, Category=Lighting)
	uint32 bCastStaticShadow:1;

	UPROPERTY()
	uint32 bIsProxy:1;

#if WITH_EDITORONLY_DATA
	UPROPERTY(transient)
	uint32 bIsMovingToLevel:1;    // Check for the Move to Current Level case
#endif // WITH_EDITORONLY_DATA

	/** The Lightmass settings for this object. */
	UPROPERTY(EditAnywhere, Category=Lightmass)
	struct FLightmassPrimitiveSettings LightmassSettings;

	// Landscape LOD to use for collision tests. Higher numbers use less memory and process faster, but are much less accurate
	UPROPERTY(EditAnywhere, Category=Landscape)
	int32 CollisionMipLevel;

	/** Thickness of the collision surface, in unreal units */
	UPROPERTY(EditAnywhere, Category=Landscape)
	float CollisionThickness;

	/** Collision profile settings for this landscape */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Collision, meta=(ShowOnlyInnerProperties))
	FBodyInstance BodyInstance;

	UPROPERTY()
	TArray<struct FLandscapeLayerStruct> LayerInfoObjs_DEPRECATED;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TArray<ULandscapeLayerInfoObject*> EditorCachedLayerInfos_DEPRECATED;

	UPROPERTY()
	FString ReimportHeightmapFilePath;

	UPROPERTY()
	TArray<struct FLandscapeEditorLayerSettings> EditorLayerSettings;
#endif

	/** Data set at creation time */
	UPROPERTY()
	int32 ComponentSizeQuads;    // Total number of quads in each component

	UPROPERTY()
	int32 SubsectionSizeQuads;    // Number of quads for a subsection of a component. SubsectionSizeQuads+1 must be a power of two.

	UPROPERTY()
	int32 NumSubsections;    // Number of subsections in X and Y axis

	/** Change the Level of Detail distance factor */
	UFUNCTION(BlueprintCallable, Category="Rendering")
	virtual void ChangeLODDistanceFactor(float InLODDistanceFactor);

	/** Hints navigation system whether this landscape will ever be navigated on. true by default, but make sure to set it to false for faraway, background landscapes */
	UPROPERTY(EditAnywhere, Category=Landscape)
	uint32 bUsedForNavigation:1;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category=Landscape)
	int32 MaxPaintedLayersPerComponent; // 0 = disabled
#endif

public:
	ENGINE_API static class ULandscapeLayerInfoObject* DataLayer;

	/** Map of material instance constants used to for the components. Key is generated with ULandscapeComponent::GetLayerAllocationKey() */
	TMap<FString, class UMaterialInstanceConstant*> MaterialInstanceConstantMap;

	/** Map of weightmap usage */
	TMap<UTexture2D*, struct FLandscapeWeightmapUsage> WeightmapUsageMap;

	// Begin AActor Interface
	virtual void UnregisterAllComponents() OVERRIDE;
	virtual void RegisterAllComponents() OVERRIDE;
	virtual void RerunConstructionScripts() OVERRIDE {}
	virtual bool IsLevelBoundsRelevant() const OVERRIDE { return true; }
	virtual bool UpdateNavigationRelevancy() OVERRIDE;
	virtual FBox GetComponentsBoundingBox(bool bNonColliding = false) const OVERRIDE;
#if WITH_EDITOR
	virtual void Destroyed() OVERRIDE;
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) OVERRIDE;
	virtual void PostEditMove(bool bFinished) OVERRIDE;
	virtual bool ShouldImport(FString* ActorPropString, bool IsMovingLevel) OVERRIDE;
	virtual bool ShouldExport() OVERRIDE;
	virtual bool GetSelectedComponents(TArray<UObject*>& SelectedObjects) OVERRIDE;
	// End AActor Interface
#endif	//WITH_EDITOR

	FGuid GetLandscapeGuid() const { return LandscapeGuid; }
	virtual class ALandscape* GetLandscapeActor();

	// Begin UObject interface.
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	virtual void PostLoad() OVERRIDE;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) OVERRIDE;
	virtual void PreEditUndo() OVERRIDE;
	virtual void PostEditUndo() OVERRIDE;
	virtual void PostEditImport() OVERRIDE;
	// End UObject Interface

	FLandscapeLayerStruct* GetLayerInfo_Deprecated(FName LayerName);
	ENGINE_API static TArray<FName> GetLayersFromMaterial(UMaterialInterface* Material);
	ENGINE_API TArray<FName> GetLayersFromMaterial() const;
	ENGINE_API static ULandscapeLayerInfoObject* CreateLayerInfo(const TCHAR* LayerName, ULevel* Level);
	ENGINE_API ULandscapeLayerInfoObject* CreateLayerInfo(const TCHAR* LayerName);

	ENGINE_API class ULandscapeInfo* GetLandscapeInfo(bool bSpawnNewActor = true);

	// Get Landscape Material assigned to this Landscape
	virtual UMaterialInterface* GetLandscapeMaterial() const;

	// Get Hole Landscape Material assigned to this Landscape
	virtual UMaterialInterface* GetLandscapeHoleMaterial() const;

	// Remove Invalid weightmaps
	ENGINE_API void RemoveInvalidWeightmaps();

	// Changed Physical Material
	ENGINE_API void ChangedPhysMaterial();

	// Check input Landscape actor is match for this LandscapeProxy (by GUID)
	ENGINE_API bool IsValidLandscapeActor(class ALandscape* Landscape);

	// Copy properties from parent Landscape actor
	ENGINE_API void GetSharedProperties(class ALandscapeProxy* Landscape);

	/** Get the LandcapeActor-to-world transform with respect to landscape section offset*/
	ENGINE_API FTransform LandscapeActorToWorld() const;
	
	/** Set landscape absolute location in section space */
	ENGINE_API void SetAbsoluteSectionBase(FIntPoint SectionOffset);
	
	/** Get landscape position in section space */
	ENGINE_API FIntPoint GetSectionBaseOffset() const;
	
	/** Recreate all components rendering and collision states */
	ENGINE_API void RecreateComponentsState();

	/** Recreate all collision components based on render component */
	ENGINE_API void RecreateCollisionComponents();

	ENGINE_API static ULandscapeMaterialInstanceConstant* GetLayerThumbnailMIC(UMaterialInterface* LandscapeMaterial, FName LayerName, UTexture2D* ThumbnailWeightmap, UTexture2D* ThumbnailHeightmap, ALandscapeProxy* Proxy);

	ENGINE_API void Import(FGuid Guid, int32 VertsX, int32 VertsY, 
							int32 ComponentSizeQuads, int32 NumSubsections, int32 SubsectionSizeQuads, 
							uint16* HeightData, const TCHAR* HeightmapFileName, 
							TArray<FLandscapeImportLayerInfo> ImportLayerInfos, uint8* AlphaDataPointers[] );

	/**
	 * Exports landscape into raw mesh
	 * 
	 * @param OutRawMesh - Resulting raw mesh
	 * @return true if successful
	 */
	ENGINE_API bool ExportToRawMesh(struct FRawMesh& OutRawMesh) const;


	/** @return Current size of bounding rectangle in quads space */
	ENGINE_API FIntRect GetBoundingRect() const;
#endif


};



