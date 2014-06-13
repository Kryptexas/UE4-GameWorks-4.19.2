// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Materials/MaterialInterface.h"
#include "StaticParameterSet.h"
#include "MaterialInstance.generated.h"

//
// Forward declarations.
//
struct FMaterialInstanceBasePropertyOverrides;
class FMaterialShaderMap;
class FMaterialShaderMapId;
class FSHAHash;

/** Editable font parameter. */
USTRUCT()
struct FFontParameterValue
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=FontParameterValue)
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=FontParameterValue)
	class UFont* FontValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=FontParameterValue)
	int32 FontPage;

	UPROPERTY()
	FGuid ExpressionGUID;


	FFontParameterValue()
		: FontValue(NULL)
		, FontPage(0)
	{
	}


	typedef const UTexture* ValueType;
	static ValueType GetValue(const FFontParameterValue& Parameter);
};

/** Editable scalar parameter. */
USTRUCT()
struct FScalarParameterValue
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ScalarParameterValue)
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ScalarParameterValue)
	float ParameterValue;

	UPROPERTY()
	FGuid ExpressionGUID;


	FScalarParameterValue()
		: ParameterValue(0)
	{
	}


	typedef float ValueType;
	static ValueType GetValue(const FScalarParameterValue& Parameter) { return Parameter.ParameterValue; }
};

/** Editable texture parameter. */
USTRUCT()
struct FTextureParameterValue
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TextureParameterValue)
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TextureParameterValue)
	class UTexture* ParameterValue;

	UPROPERTY()
	FGuid ExpressionGUID;


	FTextureParameterValue()
		: ParameterValue(NULL)
	{
	}


	typedef const UTexture* ValueType;
	static ValueType GetValue(const FTextureParameterValue& Parameter) { return Parameter.ParameterValue; }
};

/** Editable vector parameter. */
USTRUCT()
struct FVectorParameterValue
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=VectorParameterValue)
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=VectorParameterValue)
	FLinearColor ParameterValue;

	UPROPERTY()
	FGuid ExpressionGUID;


	FVectorParameterValue()
		: ParameterValue(ForceInit)
	{
	}


	typedef FLinearColor ValueType;
	static ValueType GetValue(const FVectorParameterValue& Parameter) { return Parameter.ParameterValue; }
};

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
	 * Indicates whether the instance has static permutation resources (which are required when static parameters are present) 
	 * Read directly from the rendering thread, can only be modified with the use of a FMaterialUpdateContext.
	 * When true, StaticPermutationMaterialResources will always be valid and non-null.
	 */
	UPROPERTY()
	uint32 bHasStaticPermutationResource:1;

	/** Flag to detect cycles in the material instance graph. */
	uint32 ReentrantFlag:1;

	/** Unique ID for this material, used for caching during distributed lighting */
	UPROPERTY()
	FGuid ParentLightingGuid;

	/** Font parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MaterialInstance)
	TArray<struct FFontParameterValue> FontParameterValues;

	/** Scalar parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MaterialInstance)
	TArray<struct FScalarParameterValue> ScalarParameterValues;

	/** Texture parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MaterialInstance)
	TArray<struct FTextureParameterValue> TextureParameterValues;

	/** Vector parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MaterialInstance)
	TArray<struct FVectorParameterValue> VectorParameterValues;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MaterialInstance)
	bool bOverrideBaseProperties;

	FMaterialInstanceBasePropertyOverrides* BasePropertyOverrides;

	/** 
	 * FMaterialRenderProxy derivatives that represent this material instance to the renderer, when the renderer needs to fetch parameter values. 
	 * Second instance is used when selected, third when hovered.
	 */
	class FMaterialInstanceResource* Resources[3];

private:

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TArray<FGuid> ReferencedTextureGuids;
#endif // WITH_EDITORONLY_DATA

	/** Static parameter values that are overridden in this instance. */
	FStaticParameterSet StaticParameters;

	/** 
	 * Material resources used for rendering this material instance, in the case of static parameters being present.
	 * These will always be valid and non-null when bHasStaticPermutationResource is true,
	 * But only the entries affected by CacheResourceShadersForRendering will be valid for rendering.
	 * There need to be as many entries in this array as can be used simultaneously for rendering.  
	 * For example the material instance needs to support being rendered at different quality levels and feature levels within the same process.
	 */
	FMaterialResource* StaticPermutationMaterialResources[EMaterialQualityLevel::Num][ERHIFeatureLevel::Num];

	/** Material resources being cached for cooking. */
	TMap<const class ITargetPlatform*, TArray<FMaterialResource*>> CachedMaterialResourcesForCooking;

	/** Fence used to guarantee that the RT is finished using various resources in this UMaterial before cleanup. */
	FRenderCommandFence ReleaseFence;

public:
	// Begin interface IBlendableInterface
	virtual void OverrideBlendableSettings(class FSceneView& View, float Weight) const;
	// End interface IBlendableInterface

	// Begin UMaterialInterface interface.
	virtual UMaterial* GetMaterial() override;
	virtual const UMaterial* GetMaterial() const override;
	virtual const UMaterial* GetMaterial_Concurrent(TMicRecursionGuard& RecursionGuard) const override;
	virtual FMaterialResource* GetMaterialResource(ERHIFeatureLevel::Type InFeatureLevel, EMaterialQualityLevel::Type QualityLevel = EMaterialQualityLevel::Num) override;
	virtual const FMaterialResource* GetMaterialResource(ERHIFeatureLevel::Type InFeatureLevel, EMaterialQualityLevel::Type QualityLevel = EMaterialQualityLevel::Num) const override;
	virtual bool GetFontParameterValue(FName ParameterName,class UFont*& OutFontValue, int32& OutFontPage) const override;
	virtual bool GetScalarParameterValue(FName ParameterName,float& OutValue) const override;
	virtual bool GetTextureParameterValue(FName ParameterName,class UTexture*& OutValue) const override;
	virtual bool GetVectorParameterValue(FName ParameterName,FLinearColor& OutValue) const override;
	virtual void GetUsedTextures(TArray<UTexture*>& OutTextures, EMaterialQualityLevel::Type QualityLevel, bool bAllQualityLevels) const override;
	virtual void OverrideTexture( const UTexture* InTextureToOverride, UTexture* OverrideTexture ) override;
	virtual bool CheckMaterialUsage(const EMaterialUsage Usage, const bool bSkipPrim = false) override;
	virtual bool CheckMaterialUsage_Concurrent(const EMaterialUsage Usage, const bool bSkipPrim = false) const override;
	virtual bool GetStaticSwitchParameterValue(FName ParameterName,bool &OutValue,FGuid &OutExpressionGuid) override;
	virtual bool GetStaticComponentMaskParameterValue(FName ParameterName, bool &R, bool &G, bool &B, bool &A,FGuid &OutExpressionGuid) override;
	virtual bool GetTerrainLayerWeightParameterValue(FName ParameterName, int32& OutWeightmapIndex, FGuid &OutExpressionGuid) override;
	virtual bool IsDependent(UMaterialInterface* TestDependency) override;
	virtual FMaterialRenderProxy* GetRenderProxy(bool Selected, bool bHovered=false) const override;
	virtual UPhysicalMaterial* GetPhysicalMaterial() const override;
	virtual bool UpdateLightmassTextureTracking() override;
	virtual bool GetCastShadowAsMasked() const override;
	virtual float GetEmissiveBoost() const override;
	virtual float GetDiffuseBoost() const override;
	virtual float GetExportResolutionScale() const override;
	virtual float GetDistanceFieldPenumbraScale() const override;
	virtual bool GetTexturesInPropertyChain(EMaterialProperty InProperty, TArray<UTexture*>& OutTextures, 
		TArray<FName>* OutTextureParamNames, class FStaticParameterSet* InStaticParameterSet) override;
	virtual void RecacheUniformExpressions() const override;
	virtual bool GetRefractionSettings(float& OutBiasValue) const override;
	ENGINE_API virtual void ForceRecompileForRendering() override;
	
	ENGINE_API virtual float GetOpacityMaskClipValue_Internal() const;
	ENGINE_API virtual EBlendMode GetBlendMode_Internal() const;
	ENGINE_API virtual EMaterialShadingModel GetShadingModel_Internal() const;
	ENGINE_API virtual bool IsTwoSided_Internal() const;

	/** Returns true and sets Result if the property was overridden in the instance. Otherwise, returns false and the base material property should be used. */
	ENGINE_API virtual bool GetOpacityMaskClipValueOverride(float& OutResult) const;
	/** Returns true and sets Result if the property was overridden in the instance. Otherwise, returns false and the base material property should be used. */
	ENGINE_API virtual bool GetBlendModeOverride(EBlendMode& OutResult) const;
	/** Returns true and sets Result if the property was overridden in the instance. Otherwise, returns false and the base material property should be used. */
	ENGINE_API virtual bool GetShadingModelOverride(EMaterialShadingModel& OutResult) const;
	/** Returns true and sets Result if the property was overridden in the instance. Otherwise, returns false and the base material property should be used. */
	ENGINE_API virtual bool IsTwoSidedOverride(bool& OutResult) const;

	/** Checks to see if an input property should be active, based on the state of the material */
	ENGINE_API virtual bool IsPropertyActive(EMaterialProperty InProperty) const;
	/** Allows material properties to be compiled with the option of being overridden by the material attributes input. */
	ENGINE_API virtual int32 CompileProperty(class FMaterialCompiler* Compiler, EMaterialProperty Property, float DefaultFloat, FLinearColor DefaultColor, const FVector4& DefaultVector);
	// End UMaterialInterface interface.

	// Begin UObject interface.
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) override;
	virtual void PostInitProperties() override;	
	virtual void BeginCacheForCookedPlatformData(  const ITargetPlatform *TargetPlatform ) override;
	virtual void ClearCachedCookedPlatformData( const ITargetPlatform *TargetPlatform ) override;
	virtual void ClearAllCachedCookedPlatformData() override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual void BeginDestroy() override;
	virtual bool IsReadyForFinishDestroy() override;
	virtual void FinishDestroy() override;
	ENGINE_API static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/**
	 * Sets new static parameter overrides on the instance and recompiles the static permutation resources if needed (can be forced with bForceRecompile).
	 * Can be passed either a minimal parameter set (overridden parameters only) or the entire set generated by GetStaticParameterValues().
	 */
	ENGINE_API void UpdateStaticPermutation(const FStaticParameterSet& NewParameters, bool bForceRecompile = false);

#endif // WITH_EDITOR

	// End UObject interface.

	/**
	 * Recompiles static permutations if necessary.
	 * Note: This modifies material variables used for rendering and is assumed to be called within a FMaterialUpdateContext!
	 */
	ENGINE_API void InitStaticPermutation();

	/** 
	 * Cache resource shaders for rendering on the given shader platform. 
	 * If a matching shader map is not found in memory or the DDC, a new one will be compiled.
	 * The results will be applied to this FMaterial in the renderer when they are finished compiling.
	 * Note: This modifies material variables used for rendering and is assumed to be called within a FMaterialUpdateContext!
	 */
	void CacheResourceShadersForCooking(EShaderPlatform ShaderPlatform, TArray<FMaterialResource*>& OutCachedMaterialResources);

	/** Builds a FMaterialShaderMapId to represent this material instance for the given platform and quality level. */
	ENGINE_API void GetMaterialResourceId(EShaderPlatform ShaderPlatform, EMaterialQualityLevel::Type QualityLevel, FMaterialShaderMapId& OutId);

	/** 
	 * Gathers actively used shader maps from all material resources used by this material instance
	 * Note - not refcounting the shader maps so the references must not be used after material resources are modified (compilation, loading, etc)
	 */
	void GetAllShaderMaps(TArray<FMaterialShaderMap*>& OutShaderMaps);

	/**
	 * Builds a composited set of static parameters, including inherited and overidden values
	 */
	ENGINE_API void GetStaticParameterValues(FStaticParameterSet& OutStaticParameters);

	void GetBasePropertyOverridesHash(FSHAHash& OutHash)const;

	// For all materials instances, UMaterialInstance::CacheResourceShadersForRendering
	static void AllMaterialsCacheResourceShadersForRendering();

protected:
	/**
	 * Updates parameter names on the material instance, returns true if parameters have changed.
	 */
	bool UpdateParameters();

	ENGINE_API void SetParentInternal(class UMaterialInterface* NewParent);

	void GetTextureExpressionValues(const FMaterialResource* MaterialResource, TArray<UTexture*>& OutTextures) const;

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
	void SetVectorParameterValueInternal(FName ParameterName, FLinearColor Value);
	void SetScalarParameterValueInternal(FName ParameterName, float Value);
	void SetTextureParameterValueInternal(FName ParameterName, class UTexture* Value);
	void SetFontParameterValueInternal(FName ParameterName, class UFont* FontValue, int32 FontPage);
	void ClearParameterValuesInternal();

	/** Initialize the material instance's resources. */
	ENGINE_API void InitResources();

	/** Builds a FMaterialShaderMapId for the material instance. */
	void GetMaterialResourceId(const FMaterialResource* Resource, EShaderPlatform ShaderPlatform, const FStaticParameterSet& CompositedStaticParameters, FMaterialShaderMapId& OutId);

	/** 
	 * Cache resource shaders for rendering on the given shader platform. 
	 * If a matching shader map is not found in memory or the DDC, a new one will be compiled.
	 * The results will be applied to this FMaterial in the renderer when they are finished compiling.
	 * Note: This modifies material variables used for rendering and is assumed to be called within a FMaterialUpdateContext!
	 */
	void CacheResourceShadersForRendering();

	/** Caches shader maps for an array of material resources. */
	void CacheShadersForResources(EShaderPlatform ShaderPlatform, const TArray<FMaterialResource*>& ResourcesToCache, bool bApplyCompletedShaderMapForRendering);

	/** Copies over parameters given a material interface */
	ENGINE_API void CopyMaterialInstanceParameters(UMaterialInterface* MaterialInterface);

	/** Allow resource to access private members. */
	friend class FMaterialInstanceResource;
	/** Editor-only access to private members. */
	friend class FMaterialEditor;
	/** Class that knows how to update MI's */
	friend class FMaterialUpdateContext;
};



