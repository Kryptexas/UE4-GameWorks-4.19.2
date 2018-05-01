// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "RenderCommandFence.h"
#include "Materials/MaterialInterface.h"
#include "StaticParameterSet.h"
#include "MaterialShared.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceBasePropertyOverrides.h"
#include "Misc/App.h"
#include "MaterialInstance.generated.h"

class ITargetPlatform;
class UPhysicalMaterial;
class USubsurfaceProfile;
class UTexture;

//
// Forward declarations.
//
class FMaterialShaderMap;
class FMaterialShaderMapId;
class FSHAHash;

/** Editable scalar parameter. */
USTRUCT(BlueprintType)
struct FScalarParameterValue
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName ParameterName_DEPRECATED;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ScalarParameterValue)
	FMaterialParameterInfo ParameterInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ScalarParameterValue)
	float ParameterValue;

	UPROPERTY()
	FGuid ExpressionGUID;

	FScalarParameterValue()
		: ParameterValue(0)
	{
	}

	bool operator==(const FScalarParameterValue& Other) const
	{
		return
			ParameterInfo == Other.ParameterInfo &&
			ParameterValue == Other.ParameterValue &&
			ExpressionGUID == Other.ExpressionGUID;
	}
	bool operator!=(const FScalarParameterValue& Other) const
	{
		return !((*this) == Other);
	}
	
	typedef float ValueType;
	static ValueType GetValue(const FScalarParameterValue& Parameter) { return Parameter.ParameterValue; }
};

/** Editable vector parameter. */
USTRUCT(BlueprintType)
struct FVectorParameterValue
{
	GENERATED_USTRUCT_BODY()
		
	UPROPERTY()
	FName ParameterName_DEPRECATED;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=VectorParameterValue)
	FMaterialParameterInfo ParameterInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=VectorParameterValue)
	FLinearColor ParameterValue;

	UPROPERTY()
	FGuid ExpressionGUID;

	FVectorParameterValue()
		: ParameterValue(ForceInit)
	{
	}

	bool operator==(const FVectorParameterValue& Other) const
	{
		return
			ParameterInfo == Other.ParameterInfo &&
			ParameterValue == Other.ParameterValue &&
			ExpressionGUID == Other.ExpressionGUID;
	}
	bool operator!=(const FVectorParameterValue& Other) const
	{
		return !((*this) == Other);
	}
	
	typedef FLinearColor ValueType;
	static ValueType GetValue(const FVectorParameterValue& Parameter) { return Parameter.ParameterValue; }
};

/** Editable texture parameter. */
USTRUCT(BlueprintType)
struct FTextureParameterValue
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName ParameterName_DEPRECATED;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TextureParameterValue)
	FMaterialParameterInfo ParameterInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TextureParameterValue)
	class UTexture* ParameterValue;

	UPROPERTY()
	FGuid ExpressionGUID;

	FTextureParameterValue()
		: ParameterValue(NULL)
	{
	}

	bool operator==(const FTextureParameterValue& Other) const
	{
		return
			ParameterInfo == Other.ParameterInfo &&
			ParameterValue == Other.ParameterValue &&
			ExpressionGUID == Other.ExpressionGUID;
	}
	bool operator!=(const FTextureParameterValue& Other) const
	{
		return !((*this) == Other);
	}

	typedef const UTexture* ValueType;
	static ValueType GetValue(const FTextureParameterValue& Parameter) { return Parameter.ParameterValue; }
};

/** Editable font parameter. */
USTRUCT(BlueprintType)
struct FFontParameterValue
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName ParameterName_DEPRECATED;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=FontParameterValue)
	FMaterialParameterInfo ParameterInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=FontParameterValue)
	class UFont* FontValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=FontParameterValue)
	int32 FontPage;

	UPROPERTY()
	FGuid ExpressionGUID;

	FFontParameterValue()
		: FontValue(nullptr)
		, FontPage(0)
	{
	}

	bool operator==(const FFontParameterValue& Other) const
	{
		return
			ParameterInfo == Other.ParameterInfo &&
			FontValue == Other.FontValue &&
			FontPage == Other.FontPage &&
			ExpressionGUID == Other.ExpressionGUID;
	}
	bool operator!=(const FFontParameterValue& Other) const
	{
		return !((*this) == Other);
	}
	
	typedef const UTexture* ValueType;
	static ValueType GetValue(const FFontParameterValue& Parameter);
};

template<class T>
bool CompareValueArraysByExpressionGUID(const TArray<T>& InA, const TArray<T>& InB)
{
	if (InA.Num() != InB.Num())
	{
		return false;
	}
	if (!InA.Num())
	{
		return true;
	}
	TArray<T> AA(InA);
	TArray<T> BB(InB);
	AA.Sort([](const T& A, const T& B) { return B.ExpressionGUID < A.ExpressionGUID; });
	BB.Sort([](const T& A, const T& B) { return B.ExpressionGUID < A.ExpressionGUID; });
	return AA == BB;
}


UCLASS(abstract, BlueprintType,MinimalAPI)
class UMaterialInstance : public UMaterialInterface
{
	GENERATED_UCLASS_BODY()

	/** Physical material to use for this graphics material. Used for sounds, effects etc.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MaterialInstance)
	class UPhysicalMaterial* PhysMaterial;

	/** Parent material. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MaterialInstance, AssetRegistrySearchable)
	class UMaterialInterface* Parent;

	/**
	 * Delegate for custom static parameters getter.
	 *
	 * @param OutStaticParameterSet Parameter set to append.
	 * @param Material Material instance to collect parameters.
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FCustomStaticParametersGetterDelegate, FStaticParameterSet&, UMaterialInstance*);

	/**
	 * Delegate for custom static parameters updater.
	 *
	 * @param StaticParameterSet Parameter set to update.
	 * @param Material Material to update.
	 *
	 * @returns True if any parameter been updated. False otherwise.
	 */
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FCustomParameterSetUpdaterDelegate, FStaticParameterSet&, UMaterial*);

	// Custom static parameters getter delegate.
	ENGINE_API static FCustomStaticParametersGetterDelegate CustomStaticParametersGetters;

	// An array of custom parameter set updaters.
	ENGINE_API static TArray<FCustomParameterSetUpdaterDelegate> CustomParameterSetUpdaters;

	/**
	 * Gets static parameter set for this material.
	 *
	 * @returns Static parameter set.
	 */
	ENGINE_API const FStaticParameterSet& GetStaticParameters() const;

	/**
	 * Indicates whether the instance has static permutation resources (which are required when static parameters are present) 
	 * Read directly from the rendering thread, can only be modified with the use of a FMaterialUpdateContext.
	 * When true, StaticPermutationMaterialResources will always be valid and non-null.
	 */
	UPROPERTY()
	uint32 bHasStaticPermutationResource:1;

	/** Defines if SubsurfaceProfile from this instance is used or it uses the parent one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MaterialInstance)
	uint32 bOverrideSubsurfaceProfile:1;
	
	/** Flag to detect cycles in the material instance graph. */
	bool ReentrantFlag[2];

	FORCEINLINE bool GetReentrantFlag() const
	{
		return ReentrantFlag[IsInGameThread() ? 0 : 1];
	}

	FORCEINLINE void SetReentrantFlag(const bool bValue)
	{
		ReentrantFlag[IsInGameThread() ? 0 : 1] = bValue;
	}

	/** Scalar parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MaterialInstance)
	TArray<struct FScalarParameterValue> ScalarParameterValues;

	/** Vector parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MaterialInstance)
	TArray<struct FVectorParameterValue> VectorParameterValues;

	/** Texture parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MaterialInstance)
	TArray<struct FTextureParameterValue> TextureParameterValues;

	/** Font parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MaterialInstance)
	TArray<struct FFontParameterValue> FontParameterValues;

	UPROPERTY()
	bool bOverrideBaseProperties_DEPRECATED;

	UPROPERTY(EditAnywhere, Category=MaterialInstance)
	struct FMaterialInstanceBasePropertyOverrides BasePropertyOverrides;

	//Cached copies of the base property overrides or the value from the parent to avoid traversing the parent chain for each access.
	float OpacityMaskClipValue;
	TEnumAsByte<EBlendMode> BlendMode;
	TEnumAsByte<EMaterialShadingModel> ShadingModel;
	uint32 TwoSided : 1;
	uint32 DitheredLODTransition : 1;
	uint32 bCastDynamicShadowAsMasked : 1;

	// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
	FVxgiMaterialProperties VxgiMaterialProperties;
#endif
	// NVCHANGE_END: Add VXGI

	/** 
	 * FMaterialRenderProxy derivatives that represent this material instance to the renderer, when the renderer needs to fetch parameter values. 
	 * Second instance is used when selected, third when hovered.
	 */
	class FMaterialInstanceResource* Resources[3];

	/** 
	 * Cached texture references from all expressions in the material (including nested functions). 
	 * This is used to link uniform texture expressions which were stored in the DDC with the UTextures that they reference.
	 */
	UPROPERTY(transient)
	TArray<UTexture*> PermutationTextureReferences;

	ENGINE_API virtual void PreSave(const class ITargetPlatform* TargetPlatform) override;
	ENGINE_API virtual float GetTextureDensity(FName TextureName, const struct FMeshUVChannelInfo& UVChannelData) const override;

	ENGINE_API bool Equivalent(const UMaterialInstance* CompareTo) const;

private:

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TArray<FGuid> ReferencedTextureGuids;
#endif // WITH_EDITORONLY_DATA

	/** Static parameter values that are overridden in this instance. */
	UPROPERTY()
	FStaticParameterSet StaticParameters;

	/** Inline material resources serialized from disk. To be processed on game thread in PostLoad. */
	TArray<FMaterialResource> LoadedMaterialResources;

	/** 
	 * Material resources used for rendering this material instance, in the case of static parameters being present.
	 * These will always be valid and non-null when bHasStaticPermutationResource is true,
	 * But only the entries affected by CacheResourceShadersForRendering will be valid for rendering.
	 * There need to be as many entries in this array as can be used simultaneously for rendering.  
	 * For example the material instance needs to support being rendered at different quality levels and feature levels within the same process.
	 */
	FMaterialResource* StaticPermutationMaterialResources[EMaterialQualityLevel::Num][ERHIFeatureLevel::Num];
#if WITH_EDITOR
	/** Material resources being cached for cooking. */
	TMap<const class ITargetPlatform*, TArray<FMaterialResource*>> CachedMaterialResourcesForCooking;
#endif
	/** Fence used to guarantee that the RT is finished using various resources in this UMaterial before cleanup. */
	FRenderCommandFence ReleaseFence;

public:
	// Begin UMaterialInterface interface.
	virtual ENGINE_API UMaterial* GetMaterial() override;
	virtual ENGINE_API const UMaterial* GetMaterial() const override;
	virtual ENGINE_API const UMaterial* GetMaterial_Concurrent(TMicRecursionGuard& RecursionGuard) const override;
	virtual ENGINE_API FMaterialResource* AllocatePermutationResource();
	virtual ENGINE_API FMaterialResource* GetMaterialResource(ERHIFeatureLevel::Type InFeatureLevel, EMaterialQualityLevel::Type QualityLevel = EMaterialQualityLevel::Num) override;
	virtual ENGINE_API const FMaterialResource* GetMaterialResource(ERHIFeatureLevel::Type InFeatureLevel, EMaterialQualityLevel::Type QualityLevel = EMaterialQualityLevel::Num) const override;
	virtual ENGINE_API bool GetScalarParameterSliderMinMax(const FMaterialParameterInfo& ParameterInfo, float& OutSliderMin, float& OutSliderMax) const override;
	virtual ENGINE_API bool GetScalarParameterValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue, bool bOveriddenOnly = false) const override;
	virtual ENGINE_API bool GetVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly = false) const override;
	virtual ENGINE_API bool IsVectorParameterUsedAsChannelMask(const FMaterialParameterInfo& ParameterInfo, bool& OutValue) const override;
	virtual ENGINE_API bool GetTextureParameterValue(const FMaterialParameterInfo& ParameterInfo, class UTexture*& OutValue, bool bOveriddenOnly = false) const override;
	virtual ENGINE_API bool GetFontParameterValue(const FMaterialParameterInfo& ParameterInfo, class UFont*& OutFontValue, int32& OutFontPage, bool bOveriddenOnly = false) const override;
	virtual ENGINE_API void GetUsedTextures(TArray<UTexture*>& OutTextures, EMaterialQualityLevel::Type QualityLevel, bool bAllQualityLevels, ERHIFeatureLevel::Type FeatureLevel, bool bAllFeatureLevels) const override;
	virtual ENGINE_API void GetUsedTexturesAndIndices(TArray<UTexture*>& OutTextures, TArray< TArray<int32> >& OutIndices, EMaterialQualityLevel::Type QualityLevel, ERHIFeatureLevel::Type FeatureLevel) const;
	virtual ENGINE_API void OverrideTexture(const UTexture* InTextureToOverride, UTexture* OverrideTexture, ERHIFeatureLevel::Type InFeatureLevel) override;
	virtual ENGINE_API void OverrideVectorParameterDefault(const FMaterialParameterInfo& ParameterInfo, const FLinearColor& Value, bool bOverride, ERHIFeatureLevel::Type FeatureLevel) override;
	virtual ENGINE_API void OverrideScalarParameterDefault(const FMaterialParameterInfo& ParameterInfo, float Value, bool bOverride, ERHIFeatureLevel::Type FeatureLevel) override;
	virtual ENGINE_API bool CheckMaterialUsage(const EMaterialUsage Usage) override;
	virtual ENGINE_API bool CheckMaterialUsage_Concurrent(const EMaterialUsage Usage) const override;
	virtual ENGINE_API bool GetStaticSwitchParameterValue(const FMaterialParameterInfo& ParameterInfo, bool &OutValue, FGuid &OutExpressionGuid, bool bOveriddenOnly = false) const override;
	virtual ENGINE_API bool GetStaticComponentMaskParameterValue(const FMaterialParameterInfo& ParameterInfo, bool &R, bool &G, bool &B, bool &A, FGuid &OutExpressionGuid, bool bOveriddenOnly = false) const override;
	virtual ENGINE_API bool GetTerrainLayerWeightParameterValue(const FMaterialParameterInfo& ParameterInfo, int32& OutWeightmapIndex, FGuid &OutExpressionGuid) const override;
	virtual ENGINE_API bool GetMaterialLayersParameterValue(const FMaterialParameterInfo& ParameterInfo, FMaterialLayersFunctions& OutLayers, FGuid &OutExpressionGuid) const override;
			ENGINE_API bool UpdateMaterialLayersParameterValue(const FMaterialParameterInfo& ParameterInfo, const FMaterialLayersFunctions& LayersValue, const bool bOverridden, const FGuid& GUID);
	virtual ENGINE_API bool IsDependent(UMaterialInterface* TestDependency) override;
	virtual ENGINE_API FMaterialRenderProxy* GetRenderProxy(bool Selected, bool bHovered = false) const override;
	virtual ENGINE_API UPhysicalMaterial* GetPhysicalMaterial() const override;
	virtual ENGINE_API bool UpdateLightmassTextureTracking() override;
	virtual ENGINE_API bool GetCastShadowAsMasked() const override;
	virtual ENGINE_API float GetEmissiveBoost() const override;
	virtual ENGINE_API float GetDiffuseBoost() const override;
	virtual ENGINE_API float GetExportResolutionScale() const override;
	virtual ENGINE_API bool GetParameterDesc(const FMaterialParameterInfo& ParameterInfo, FString& OutDesc, const TArray<struct FStaticMaterialLayersParameter>* MaterialLayersParameters = nullptr) const;	
#if WITH_EDITOR
	virtual ENGINE_API bool GetParameterSortPriority(const FMaterialParameterInfo& ParameterInfo, int32& OutSortPriority, const TArray<struct FStaticMaterialLayersParameter>* MaterialLayersParameters = nullptr) const override;
	virtual ENGINE_API bool GetGroupSortPriority(const FString& InGroupName, int32& OutSortPriority) const override;
	virtual ENGINE_API bool GetTexturesInPropertyChain(EMaterialProperty InProperty, TArray<UTexture*>& OutTextures,
		TArray<FName>* OutTextureParamNames, struct FStaticParameterSet* InStaticParameterSet) override;
#endif
	virtual ENGINE_API void RecacheUniformExpressions() const override;
	virtual ENGINE_API bool GetRefractionSettings(float& OutBiasValue) const override;
	ENGINE_API virtual void ForceRecompileForRendering() override;
	
	ENGINE_API virtual float GetOpacityMaskClipValue() const override;
	ENGINE_API virtual EBlendMode GetBlendMode() const override;
	ENGINE_API virtual EMaterialShadingModel GetShadingModel() const override;
	ENGINE_API virtual bool IsTwoSided() const override;
	ENGINE_API virtual bool IsDitheredLODTransition() const override;
	ENGINE_API virtual bool IsMasked() const override;;
	
	ENGINE_API virtual USubsurfaceProfile* GetSubsurfaceProfile_Internal() const override;

	// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
	ENGINE_API virtual FVxgiMaterialProperties GetVxgiMaterialProperties() const override { return VxgiMaterialProperties; }
#endif
	// NVCHANGE_END: Add VXGI

	/** Checks to see if an input property should be active, based on the state of the material */
	ENGINE_API virtual bool IsPropertyActive(EMaterialProperty InProperty) const override;
#if WITH_EDITOR
	/** Allows material properties to be compiled with the option of being overridden by the material attributes input. */
	ENGINE_API virtual int32 CompilePropertyEx(class FMaterialCompiler* Compiler, const FGuid& AttributeID) override;
#endif // WITH_EDITOR
	//~ End UMaterialInterface Interface.

	//~ Begin UObject Interface.
	virtual ENGINE_API void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	virtual ENGINE_API void PostInitProperties() override;	
#if WITH_EDITOR
	virtual ENGINE_API void BeginCacheForCookedPlatformData(const ITargetPlatform *TargetPlatform) override;
	virtual ENGINE_API bool IsCachedCookedPlatformDataLoaded(const ITargetPlatform* TargetPlatform) override;
	virtual ENGINE_API void ClearCachedCookedPlatformData(const ITargetPlatform *TargetPlatform) override;
	virtual ENGINE_API void ClearAllCachedCookedPlatformData() override;
#endif
	virtual ENGINE_API void Serialize(FArchive& Ar) override;
	virtual ENGINE_API void PostLoad() override;
	virtual ENGINE_API void BeginDestroy() override;
	virtual ENGINE_API bool IsReadyForFinishDestroy() override;
	virtual ENGINE_API void FinishDestroy() override;
	ENGINE_API static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	ENGINE_API virtual void NotifyObjectReferenceEliminated() const override;
#if WITH_EDITOR
	virtual ENGINE_API void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/**
	 * Sets new static parameter overrides on the instance and recompiles the static permutation resources if needed (can be forced with bForceRecompile).
	 * Can be passed either a minimal parameter set (overridden parameters only) or the entire set generated by GetStaticParameterValues().
	 * Can also trigger recompile based on new set of FMaterialInstanceBasePropertyOverrides 
	 */
	ENGINE_API void UpdateStaticPermutation(const FStaticParameterSet& NewParameters, FMaterialInstanceBasePropertyOverrides& NewBasePropertyOverrides, const bool bForceStaticPermutationUpdate = false);
	/**
	* Sets new static parameter overrides on the instance and recompiles the static permutation resources if needed.
	* Can be passed either a minimal parameter set (overridden parameters only) or the entire set generated by GetStaticParameterValues().
	*/
	ENGINE_API void UpdateStaticPermutation(const FStaticParameterSet& NewParameters);
	/** Ensure's static permutations for current parameters and overrides are upto date. */
	ENGINE_API void UpdateStaticPermutation();

#endif // WITH_EDITOR

	//~ End UObject Interface.

	/**
	 * Recompiles static permutations if necessary.
	 * Note: This modifies material variables used for rendering and is assumed to be called within a FMaterialUpdateContext!
	 */
	ENGINE_API void InitStaticPermutation();

	ENGINE_API void UpdateOverridableBaseProperties();

	/** 
	 * Cache resource shaders for rendering on the given shader platform. 
	 * If a matching shader map is not found in memory or the DDC, a new one will be compiled.
	 * The results will be applied to this FMaterial in the renderer when they are finished compiling.
	 * Note: This modifies material variables used for rendering and is assumed to be called within a FMaterialUpdateContext!
	 */
	void CacheResourceShadersForCooking(EShaderPlatform ShaderPlatform, TArray<FMaterialResource*>& OutCachedMaterialResources);

	/** 
	 * Gathers actively used shader maps from all material resources used by this material instance
	 * Note - not refcounting the shader maps so the references must not be used after material resources are modified (compilation, loading, etc)
	 */
	void GetAllShaderMaps(TArray<FMaterialShaderMap*>& OutShaderMaps);

	/**
	 * Builds a composited set of static parameters, including inherited and overridden values
	 */
	ENGINE_API void GetStaticParameterValues(FStaticParameterSet& OutStaticParameters);

	/**
	 * Builds a composited set of parameter names, including inherited and overridden values
	 */
	template<typename ExpressionType>
	void GetAllParameterInfo(TArray<FMaterialParameterInfo>& OutParameterInfo, TArray<FGuid>& OutParameterIds) const
	{
		if (const UMaterial* Material = GetMaterial())
		{
			Material->GetAllParameterInfo<ExpressionType>(OutParameterInfo, OutParameterIds);
		}
	}

	ENGINE_API virtual void GetAllScalarParameterInfo(TArray<FMaterialParameterInfo>& OutParameterInfo, TArray<FGuid>& OutParameterIds) const override;
	ENGINE_API virtual void GetAllVectorParameterInfo(TArray<FMaterialParameterInfo>& OutParameterInfo, TArray<FGuid>& OutParameterIds) const override;
	ENGINE_API virtual void GetAllTextureParameterInfo(TArray<FMaterialParameterInfo>& OutParameterInfo, TArray<FGuid>& OutParameterIds) const override;
	ENGINE_API virtual void GetAllFontParameterInfo(TArray<FMaterialParameterInfo>& OutParameterInfo, TArray<FGuid>& OutParameterIds) const override;
	ENGINE_API virtual void GetAllMaterialLayersParameterInfo(TArray<FMaterialParameterInfo>& OutParameterInfo, TArray<FGuid>& OutParameterIds) const override;
	ENGINE_API virtual void GetAllStaticSwitchParameterInfo(TArray<FMaterialParameterInfo>& OutParameterInfo, TArray<FGuid>& OutParameterIds) const override;
	ENGINE_API virtual void GetAllStaticComponentMaskParameterInfo(TArray<FMaterialParameterInfo>& OutParameterInfo, TArray<FGuid>& OutParameterIds) const override;

	ENGINE_API virtual void GetDependentFunctions(TArray<class UMaterialFunctionInterface*>& DependentFunctions) const override;
	
	ENGINE_API virtual bool GetScalarParameterDefaultValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue, bool bOveriddenOnly = false, bool bCheckOwnedGlobalOverrides = false) const override;
	ENGINE_API virtual bool GetVectorParameterDefaultValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly = false, bool bCheckOwnedGlobalOverrides = false) const override;
	ENGINE_API virtual bool GetTextureParameterDefaultValue(const FMaterialParameterInfo& ParameterInfo, UTexture*& OutValue, bool bCheckOwnedGlobalOverrides = false) const override;
	ENGINE_API virtual bool GetFontParameterDefaultValue(const FMaterialParameterInfo& ParameterInfo, UFont*& OutFontValue, int32& OutFontPage, bool bCheckOwnedGlobalOverrides = false) const override;
	ENGINE_API virtual bool GetStaticSwitchParameterDefaultValue(const FMaterialParameterInfo& ParameterInfo, bool& OutValue, FGuid& OutExpressionGuid, bool bCheckOwnedGlobalOverrides = false) const override;
	ENGINE_API virtual bool GetStaticComponentMaskParameterDefaultValue(const FMaterialParameterInfo& ParameterInfo, bool& OutR, bool& OutG, bool& OutB, bool& OutA, FGuid& OutExpressionGuid, bool bCheckOwnedGlobalOverrides = false) const override;

	ENGINE_API virtual bool GetGroupName(const FMaterialParameterInfo& ParameterInfo, FName& OutGroup) const override;

	/** Appends textures referenced by expressions, including nested functions. */
	ENGINE_API virtual void AppendReferencedTextures(TArray<UTexture*>& InOutTextures) const override;

	void GetBasePropertyOverridesHash(FSHAHash& OutHash)const;
	ENGINE_API virtual bool HasOverridenBaseProperties()const;

	// For all materials instances, UMaterialInstance::CacheResourceShadersForRendering
	ENGINE_API static void AllMaterialsCacheResourceShadersForRendering();

	/** 
	 * Determine whether this Material Instance is a child of another Material
	 *
	 * @param	Material	Material to check against
	 * @return	true if this Material Instance is a child of the other Material.
	 */
	ENGINE_API bool IsChildOf(const UMaterialInterface* Material) const;


#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	/**
	 * Output to the log which materials and textures are used by this material.
	 * @param Indent	Number of tabs to put before the log.
	 */
	ENGINE_API virtual void LogMaterialsAndTextures(FOutputDevice& Ar, int32 Indent) const override;
#endif

	/**
	 *	Returns all the Guids related to this material. For material instances, this includes the parent hierarchy.
	 *  Used for versioning as parent changes don't update the child instance Guids.
	 *
	 *	@param	bIncludeTextures	Whether to include the referenced texture Guids.
	 *	@param	OutGuids			The list of all resource guids affecting the precomputed lighting system and texture streamer.
	 */
	ENGINE_API virtual void GetLightingGuidChain(bool bIncludeTextures, TArray<FGuid>& OutGuids) const override;

protected:
	/**
	 * Updates parameter names on the material instance, returns true if parameters have changed.
	 */
	bool UpdateParameters();

	ENGINE_API void SetParentInternal(class UMaterialInterface* NewParent, bool RecacheShaders);

	void GetTextureExpressionValues(const FMaterialResource* MaterialResource, TArray<UTexture*>& OutTextures, TArray< TArray<int32> >* OutIndices = nullptr) const;

	/** 
	 * Updates StaticPermutationMaterialResources based on the value of bHasStaticPermutationResource
	 * This is a helper used when recompiling MI's with static parameters.  
	 * Assumes that the rendering thread command queue has been flushed by the caller.
	 */
	void UpdatePermutationAllocations();

#if WITH_EDITOR
	/**
	* Refresh parameter names using the stored reference to the expression object for the parameter.
	*/
	ENGINE_API void UpdateParameterNames();

#endif

	/**
	 * Internal interface for setting / updating values for material instances.
	 */
	void SetVectorParameterValueInternal(const FMaterialParameterInfo& ParameterInfo, FLinearColor Value);
	bool SetVectorParameterByIndexInternal(int32 ParameterIndex, FLinearColor Value);
	bool SetScalarParameterByIndexInternal(int32 ParameterIndex, float Value);
	void SetScalarParameterValueInternal(const FMaterialParameterInfo& ParameterInfo, float Value);
	void SetTextureParameterValueInternal(const FMaterialParameterInfo& ParameterInfo, class UTexture* Value);
	void SetFontParameterValueInternal(const FMaterialParameterInfo& ParameterInfo, class UFont* FontValue, int32 FontPage);
	void ClearParameterValuesInternal(const bool bAllParameters = true);

	/** Initialize the material instance's resources. */
	ENGINE_API void InitResources();

	/** 
	 * Cache resource shaders for rendering on the given shader platform. 
	 * If a matching shader map is not found in memory or the DDC, a new one will be compiled.
	 * The results will be applied to this FMaterial in the renderer when they are finished compiling.
	 * Note: This modifies material variables used for rendering and is assumed to be called within a FMaterialUpdateContext!
	 */
	void CacheResourceShadersForRendering();

	/** Caches shader maps for an array of material resources. */
	void CacheShadersForResources(EShaderPlatform ShaderPlatform, const TArray<FMaterialResource*>& ResourcesToCache, bool bApplyCompletedShaderMapForRendering);

	/** 
	 * Copies over material instance parameters from the base material given a material interface.
	 * This is a slow operation that is needed for the editor.
	 * @param Source silently ignores the case if 0
	 */
	ENGINE_API void CopyMaterialInstanceParameters(UMaterialInterface* Source);

	// to share code between PostLoad() and PostEditChangeProperty()
	void PropagateDataToMaterialProxy();

	/** Allow resource to access private members. */
	friend class FMaterialInstanceResource;
	/** Editor-only access to private members. */
	friend class UMaterialEditingLibrary;
	/** Class that knows how to update MI's */
	friend class FMaterialUpdateContext;
};

/** 4.19.2 workaround - Similar to base call but evaluates all expressions found, not just the first */
template<typename ExpressionType>
void FindClosestExpressionByGUIDRecursive(const FName& InName, const FGuid& InGUID, const TArray<UMaterialExpression*>& InMaterialExpression, ExpressionType*& OutExpression)
{
	for (int32 ExpressionIndex = 0; ExpressionIndex < InMaterialExpression.Num(); ExpressionIndex++)
	{
		UMaterialExpression* ExpressionPtr = InMaterialExpression[ExpressionIndex];
		UMaterialExpressionMaterialFunctionCall* MaterialFunctionCall = Cast<UMaterialExpressionMaterialFunctionCall>(ExpressionPtr);
		UMaterialExpressionMaterialAttributeLayers* MaterialLayers = Cast<UMaterialExpressionMaterialAttributeLayers>(ExpressionPtr);

		if (ExpressionPtr && ExpressionPtr->GetParameterExpressionId() == InGUID)
		{
			check(ExpressionPtr->bIsParameterExpression);
			if (ExpressionType* ParamExpression = Cast<ExpressionType>(ExpressionPtr))
			{
				// UE-57086, 4.19.2 workaround - To deal with duplicated parameters with matching GUIDs we walk
				// through every parameter rather than taking the first. Either we return the first matching GUID
				// we encounter (as before), or if we find another with the same name that can take precedence.
				// Only taking the first parameter means we can incorrectly treat the parameter as a rename and
				// lose/move data when we encounter an illegal GUID duplicate.
				// Properly fixing duplicate GUIDs is beyond the scope of a hotfix, see UE-47863 for more info.
				// NOTE: The case where a parameter in a function is renamed but another function in the material
				// contains a duplicate GUID is still broken and may lose the data. This still leaves us in a
				// more consistent state than 4.18 and should minimize the impact to a rarer occurrence.
				if (!OutExpression || InName == ParamExpression->ParameterName)
				{
					OutExpression = ParamExpression;
				}
			}
		}
		else if (MaterialFunctionCall && MaterialFunctionCall->MaterialFunction)
		{
			if (const TArray<UMaterialExpression*>* FunctionExpressions = MaterialFunctionCall->MaterialFunction->GetFunctionExpressions())
			{
				FindClosestExpressionByGUIDRecursive<ExpressionType>(InName, InGUID, *FunctionExpressions, OutExpression);
			}
		}
		else if (MaterialLayers)
		{
			const TArray<UMaterialFunctionInterface*>& Layers = MaterialLayers->GetLayers();
			const TArray<UMaterialFunctionInterface*>& Blends = MaterialLayers->GetBlends();

			for (const auto* Layer : Layers)
			{
				if (Layer)
				{
					if (const TArray<UMaterialExpression*>* FunctionExpressions = Layer->GetFunctionExpressions())
					{
						FindClosestExpressionByGUIDRecursive<ExpressionType>(InName, InGUID, *FunctionExpressions, OutExpression);
					}
				}
			}

			for (const auto* Blend : Blends)
			{
				if (Blend)
				{
					if (const TArray<UMaterialExpression*>* FunctionExpressions = Blend->GetFunctionExpressions())
					{
						FindClosestExpressionByGUIDRecursive<ExpressionType>(InName, InGUID, *FunctionExpressions, OutExpression);
					}
				}
			}
		}
	}
}

/**
 * This function takes a array of parameter structs and attempts to establish a reference to the expression object each parameter represents.
 * If a reference exists, the function checks to see if the parameter has been renamed.
 *
 * @param Parameters		Array of parameters to operate on.
 * @param ParentMaterial	Parent material to search in for expressions.
 *
 * @return Returns whether or not any of the parameters was changed.
 */
template <typename ParameterType, typename ExpressionType>
bool UpdateParameterSet(TArray<ParameterType>& Parameters, UMaterial* ParentMaterial)
{
	bool bChanged = false;

	// Loop through all of the parameters and try to either establish a reference to the 
	// expression the parameter represents, or check to see if the parameter's name has changed.
	for (int32 ParameterIdx = 0; ParameterIdx < Parameters.Num(); ParameterIdx++)
	{
		bool bTryToFindByName = true;

		ParameterType& Parameter = Parameters[ParameterIdx];

		if (Parameter.ExpressionGUID.IsValid())
		{
			ExpressionType* Expression = nullptr;
			FindClosestExpressionByGUIDRecursive<ExpressionType>(Parameter.ParameterInfo.Name, Parameter.ExpressionGUID, ParentMaterial->Expressions, Expression);

			// Check to see if the parameter name was changed.
			if (Expression)
			{
				bTryToFindByName = false;

				if (Parameter.ParameterInfo.Name != Expression->ParameterName)
				{
					Parameter.ParameterInfo.Name = Expression->ParameterName;
					bChanged = true;
				}
			}
		}

		// No reference to the material expression exists, so try to find one in the material expression's array if we are in the editor.
		if (bTryToFindByName && GIsEditor && !FApp::IsGame())
		{
			for (UMaterialExpression* Expression : ParentMaterial->Expressions)
			{
				if (Expression->IsA<ExpressionType>())
				{
					ExpressionType* ParameterExpression = CastChecked<ExpressionType>(Expression);
					if (ParameterExpression->ParameterName == Parameter.ParameterInfo.Name)
					{
						Parameter.ExpressionGUID = ParameterExpression->ExpressionGUID;
						bChanged = true;
						break;
					}
				}
				else if (UMaterialExpressionMaterialFunctionCall* FunctionCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expression))
				{
					if (FunctionCall->MaterialFunction && FunctionCall->MaterialFunction->UpdateParameterSet<ParameterType, ExpressionType>(Parameter))
					{
						bChanged = true;
						break;
					}
				}
				else if (UMaterialExpressionMaterialAttributeLayers* LayersExpression = Cast<UMaterialExpressionMaterialAttributeLayers>(Expression))
				{
					const TArray<UMaterialFunctionInterface*> Layers = LayersExpression->GetLayers();
					const TArray<UMaterialFunctionInterface*> Blends = LayersExpression->GetBlends();

					for (auto* Layer : Layers)
					{
						if (Layer && Layer->UpdateParameterSet<ParameterType, ExpressionType>(Parameter))
						{
							bChanged = true;
							break;
						}
					}

					if (!bChanged)
					{
						for (auto* Blend : Blends)
						{
							if (Blend && Blend->UpdateParameterSet<ParameterType, ExpressionType>(Parameter))
							{
								bChanged = true;
								break;
							}
						}
					}

					if (bChanged)
					{
						break;
					}		
				}
			}
		}
	}

	return bChanged;
}
