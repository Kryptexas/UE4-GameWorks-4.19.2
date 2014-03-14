// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MaterialShared.h: Shared material definitions.
=============================================================================*/

#pragma once

#include "Engine.h"
#include "RefCounting.h"
#include "ShaderCore.h"
#include "Shader.h"
#include "VertexFactory.h"
#include "RenderResource.h"
#include "ShaderCompiler.h"
#include "UniformBuffer.h"
class FMeshMaterialShaderMap;
class FMaterialShaderMap;
class FMaterialShaderType;
class FMaterial;
class UMaterialInstance;

#define ME_CAPTION_HEIGHT		18
#define ME_STD_VPADDING			16
#define ME_STD_HPADDING			32
#define ME_STD_BORDER			8
#define ME_STD_THUMBNAIL_SZ		96
#define ME_PREV_THUMBNAIL_SZ	256
#define ME_STD_LABEL_PAD		16
#define ME_STD_TAB_HEIGHT		21

DECLARE_LOG_CATEGORY_EXTERN(LogMaterial,Log,Verbose);

/** Quality levels that a material can be compiled for. */
namespace EMaterialQualityLevel
{
	enum Type
	{
		Low,
		High,
		Num
	};
}

/** Creates a string that represents the given quality level. */
extern void GetMaterialQualityLevelName(EMaterialQualityLevel::Type InMaterialQualityLevel, FString& OutName);

/**
 * The types which can be used by materials.
 */
enum EMaterialValueType
{
	/** 
	 * A scalar float type.  
	 * Note that MCT_Float1 will not auto promote to any other float types, 
	 * So use MCT_Float instead for scalar expression return types.
	 */
	MCT_Float1		= 1,
	MCT_Float2		= 2,
	MCT_Float3		= 4,
	MCT_Float4		= 8,

	/** 
	 * Any size float type by definition, but this is treated as a scalar which can auto convert (by replication) to any other size float vector.
	 * Use this as the type for any scalar expressions.
	 */
	MCT_Float		= 8|4|2|1,
	MCT_Texture2D	= 16,
	MCT_TextureCube	= 32,
	MCT_Texture		= 16|32,
	MCT_StaticBool	= 64,
	MCT_Unknown		= 128,
	MCT_MaterialAttributes	= 256
};

/**
 * The context of a material being rendered.
 */
struct ENGINE_API FMaterialRenderContext
{
	/** material instance used for the material shader */
	const FMaterialRenderProxy* MaterialRenderProxy;
	/** Material resource to use. */
	const FMaterial& Material;
	/** Whether or not selected objects should use their selection color. */
	bool bShowSelection;
	/** Whether to modify sampler state set with this context to work around mip map artifacts that show up in deferred passes. */
	bool bWorkAroundDeferredMipArtifacts;

	/** 
	* Constructor
	*/
	FMaterialRenderContext(
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& InMaterial,
		const FSceneView* InView,
		bool bInWorkAroundDeferredMipArtifacts = false);
};

/**
 * Represents a subclass of FMaterialUniformExpression.
 */
class FMaterialUniformExpressionType
{
public:

	typedef class FMaterialUniformExpression* (*SerializationConstructorType)();

	/**
	 * @return The global uniform expression type list.  The list is used to temporarily store the types until
	 *			the name subsystem has been initialized.
	 */
	static TLinkedList<FMaterialUniformExpressionType*>*& GetTypeList();

	/**
	 * Should not be called until the name subsystem has been initialized.
	 * @return The global uniform expression type map.
	 */
	static TMap<FName,FMaterialUniformExpressionType*>& GetTypeMap();

	/**
	 * Minimal initialization constructor.
	 */
	FMaterialUniformExpressionType(const TCHAR* InName,SerializationConstructorType InSerializationConstructor);

	/**
	 * Serializer for references to uniform expressions.
	 */
	friend FArchive& operator<<(FArchive& Ar,class FMaterialUniformExpression*& Ref);
	friend FArchive& operator<<(FArchive& Ar,class FMaterialUniformExpressionTexture*& Ref);

	const TCHAR* GetName() const { return Name; }

private:

	const TCHAR* Name;
	SerializationConstructorType SerializationConstructor;
};

#define DECLARE_MATERIALUNIFORMEXPRESSION_TYPE(Name) \
	public: \
	static FMaterialUniformExpressionType StaticType; \
	static FMaterialUniformExpression* SerializationConstructor() { return new Name(); } \
	virtual FMaterialUniformExpressionType* GetType() const { return &StaticType; }

#define IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(Name) \
	FMaterialUniformExpressionType Name::StaticType(TEXT(#Name),&Name::SerializationConstructor);

/**
 * Represents an expression which only varies with uniform inputs.
 */
class FMaterialUniformExpression : public FRefCountedObject
{
public:

	virtual ~FMaterialUniformExpression() {}

	virtual FMaterialUniformExpressionType* GetType() const = 0;
	virtual void Serialize(FArchive& Ar) = 0;
	virtual void GetNumberValue(const struct FMaterialRenderContext& Context,FLinearColor& OutValue) const {}
	virtual class FMaterialUniformExpressionTexture* GetTextureUniformExpression() { return NULL; }
	virtual bool IsConstant() const { return false; }
	virtual bool IsIdentical(const FMaterialUniformExpression* OtherExpression) const { return false; }

	friend FArchive& operator<<(FArchive& Ar,class FMaterialUniformExpression*& Ref);
};

/**
 * A texture expression.
 */
class FMaterialUniformExpressionTexture: public FMaterialUniformExpression
{
	DECLARE_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionTexture);
public:

	FMaterialUniformExpressionTexture();

	FMaterialUniformExpressionTexture(int32 InTextureIndex);

	// FMaterialUniformExpression interface.
	virtual void Serialize(FArchive& Ar);
	virtual void GetTextureValue(const FMaterialRenderContext& Context,const FMaterial& Material,const UTexture*& OutValue) const;
	/** Accesses the texture used for rendering this uniform expression. */
	virtual void GetGameThreadTextureValue(const class UMaterialInterface* MaterialInterface,const FMaterial& Material,UTexture*& OutValue,bool bAllowOverride=true) const;
	virtual class FMaterialUniformExpressionTexture* GetTextureUniformExpression() { return this; }
	void SetTransientOverrideTextureValue( UTexture* InOverrideTexture );

	virtual bool IsConstant() const
	{
		return false;
	}
	virtual bool IsIdentical(const FMaterialUniformExpression* OtherExpression) const;

	friend FArchive& operator<<(FArchive& Ar,class FMaterialUniformExpressionTexture*& Ref);

protected:
	/** Index into FMaterial::GetReferencedTextures */
	int32 TextureIndex;
	/** Texture that may be used in the editor for overriding the texture but never saved to disk, accessible only by the game thread! */
	UTexture* TransientOverrideValue_GameThread;
	/** Texture that may be used in the editor for overriding the texture but never saved to disk, accessible only by the rendering thread! */
	UTexture* TransientOverrideValue_RenderThread;
};

/** Stores all uniform expressions for a material generated from a material translation. */
class FUniformExpressionSet : public FRefCountedObject
{
public:
	FUniformExpressionSet(): UniformBufferStruct(NULL) {}

	ENGINE_API void Serialize(FArchive& Ar);
	bool IsEmpty() const;
	bool operator==(const FUniformExpressionSet& ReferenceSet) const;
	FString GetSummaryString() const;
	void GetResourcesString(EShaderPlatform ShaderPlatform,FString& InputsString) const;

	void SetParameterCollections(const TArray<class UMaterialParameterCollection*>& Collections);
	void CreateBufferStruct();
	const FUniformBufferStruct& GetUniformBufferStruct() const;
	ENGINE_API FUniformBufferRHIRef CreateUniformBuffer(const FMaterialRenderContext& MaterialRenderContext) const;

	uint32 GetAllocatedSize() const
	{
		return UniformVectorExpressions.GetAllocatedSize()
			+ UniformScalarExpressions.GetAllocatedSize()
			+ Uniform2DTextureExpressions.GetAllocatedSize()
			+ UniformCubeTextureExpressions.GetAllocatedSize()
			+ ParameterCollections.GetAllocatedSize()
			+ UniformBufferStruct ? (sizeof(FUniformBufferStruct) + UniformBufferStruct->GetMembers().GetAllocatedSize()) : 0;
	}

protected:
	
	TArray<TRefCountPtr<FMaterialUniformExpression> > UniformVectorExpressions;
	TArray<TRefCountPtr<FMaterialUniformExpression> > UniformScalarExpressions;
	TArray<TRefCountPtr<FMaterialUniformExpressionTexture> > Uniform2DTextureExpressions;
	TArray<TRefCountPtr<FMaterialUniformExpressionTexture> > UniformCubeTextureExpressions;

	/** Ids of parameter collections referenced by the material that was translated. */
	TArray<FGuid> ParameterCollections;

	/** The structure of a uniform buffer containing values for these uniform expressions. */
	TScopedPointer<FUniformBufferStruct> UniformBufferStruct;

	friend class FMaterial;
	friend class FHLSLMaterialTranslator;
	friend class FMaterialShaderMap;
	friend class FMaterialShader;
	friend class FMaterialRenderProxy;
	friend class FDebugUniformExpressionSet;
};

/** Stores outputs from the material compile that need to be saved. */
class FMaterialCompilationOutput
{
public:
	FMaterialCompilationOutput() :
		bUsesSceneColor(false),
		bNeedsSceneTextures(false),
		bUsesEyeAdaptation(false)
	{}

	ENGINE_API void Serialize(FArchive& Ar);

	FUniformExpressionSet UniformExpressionSet;

	/** 
	 * Indicates whether the material uses scene color. 
	 */
	bool bUsesSceneColor;

	/** true if the material needs the scenetexture lookups. */
	bool bNeedsSceneTextures;

	/** true if the material uses the EyeAdaptationLookup */
	bool bUsesEyeAdaptation;
};

/**
* Holds the information for a static switch parameter
*/
class FStaticSwitchParameter
{
public:
	FName ParameterName;
	bool Value;
	bool bOverride;
	FGuid ExpressionGUID;

	FStaticSwitchParameter() :
		ParameterName(TEXT("None")),
		Value(false),
		bOverride(false),
		ExpressionGUID(0,0,0,0)
	{ }

	FStaticSwitchParameter(FName InName, bool InValue, bool InOverride, FGuid InGuid) :
		ParameterName(InName),
		Value(InValue),
		bOverride(InOverride),
		ExpressionGUID(InGuid)
	{ }

	friend FArchive& operator<<(FArchive& Ar,FStaticSwitchParameter& P)
	{
		Ar << P.ParameterName << P.Value << P.bOverride;
		Ar << P.ExpressionGUID;
		return Ar;
	}
};

/**
* Holds the information for a static component mask parameter
*/
class FStaticComponentMaskParameter
{
public:
	FName ParameterName;
	bool R, G, B, A;
	bool bOverride;
	FGuid ExpressionGUID;

	FStaticComponentMaskParameter() :
		ParameterName(TEXT("None")),
		R(false),
		G(false),
		B(false),
		A(false),
		bOverride(false),
		ExpressionGUID(0,0,0,0)
	{ }

	FStaticComponentMaskParameter(FName InName, bool InR, bool InG, bool InB, bool InA, bool InOverride, FGuid InGuid) :
		ParameterName(InName),
		R(InR),
		G(InG),
		B(InB),
		A(InA),
		bOverride(InOverride),
		ExpressionGUID(InGuid)
	{ }

	friend FArchive& operator<<(FArchive& Ar,FStaticComponentMaskParameter& P)
	{
		Ar << P.ParameterName << P.R << P.G << P.B << P.A << P.bOverride;
		Ar << P.ExpressionGUID;
		return Ar;
	}
};

/**
* Holds the information for a static switch parameter
*/
class FStaticTerrainLayerWeightParameter
{
public:
	FName ParameterName;
	bool bOverride;
	FGuid ExpressionGUID;

	int32 WeightmapIndex;

	FStaticTerrainLayerWeightParameter() :
		ParameterName(TEXT("None")),
		bOverride(false),
		ExpressionGUID(0,0,0,0),
		WeightmapIndex(INDEX_NONE)
	{ }

	FStaticTerrainLayerWeightParameter(FName InName, int32 InWeightmapIndex, bool InOverride, FGuid InGuid) :
		ParameterName(InName),
		bOverride(InOverride),
		ExpressionGUID(InGuid),
		WeightmapIndex(InWeightmapIndex)
	{ }

	friend FArchive& operator<<(FArchive& Ar,FStaticTerrainLayerWeightParameter& P)
	{
		Ar << P.ParameterName << P.WeightmapIndex << P.bOverride;
		Ar << P.ExpressionGUID;
		return Ar;
	}
};

/** Contains all the information needed to identify a single permutation of static parameters. */
class FStaticParameterSet
{
public:
	/** An array of static switch parameters in this set */
	TArray<FStaticSwitchParameter> StaticSwitchParameters;

	/** An array of static component mask parameters in this set */
	TArray<FStaticComponentMaskParameter> StaticComponentMaskParameters;

	/** An array of terrain layer weight parameters in this set */
	TArray<FStaticTerrainLayerWeightParameter> TerrainLayerWeightParameters;

	FStaticParameterSet() {}

	/** 
	* Checks if this set contains any parameters
	* 
	* @return	true if this set has no parameters
	*/
	bool IsEmpty() const
	{
		return StaticSwitchParameters.Num() == 0 && StaticComponentMaskParameters.Num() == 0 && TerrainLayerWeightParameters.Num() == 0;
	}

	void Serialize(FArchive& Ar)
	{
		// Note: FStaticParameterSet is saved both in packages (UMaterialInstance) and the DDC (FMaterialShaderMap)
		// Backwards compatibility only works with FStaticParameterSet's stored in packages.  
		// You must bump MATERIALSHADERMAP_DERIVEDDATA_VER as well if changing the serialization of FStaticParameterSet.

		Ar << StaticSwitchParameters << StaticComponentMaskParameters;
		Ar << TerrainLayerWeightParameters;
	}

	/** Updates the hash state with the static parameter names and values. */
	void UpdateHash(FSHA1& HashState) const;

	/** 
	* Indicates whether this set is equal to another, copying override settings.
	* 
	* @param ReferenceSet	The set to compare against
	* @return				true if the sets are not equal
	*/
	bool ShouldMarkDirty(const FStaticParameterSet* ReferenceSet);

	/** 
	* Tests this set against another for equality, disregarding override settings.
	* 
	* @param ReferenceSet	The set to compare against
	* @return				true if the sets are equal
	*/
	bool operator==(const FStaticParameterSet& ReferenceSet) const;

	bool operator!=(const FStaticParameterSet& ReferenceSet) const
	{
		return !(*this == ReferenceSet);
	}

	FString GetSummaryString() const;

	void AppendKeyString(FString& KeyString) const;
};

/** 
 * Usage options for a shader map.
 * The purpose of EMaterialShaderMapUsage is to allow creating a unique yet deterministic (no appCreateGuid) Id,
 * For a shader map corresponding to any UMaterial or UMaterialInstance, for different use cases.
 * As an example, when exporting a material to Lightmass we want to compile a shader map with FLightmassMaterialProxy,
 * And generate a FMaterialShaderMapId for it that allows reuse later, so it must be deterministic.
 */
namespace EMaterialShaderMapUsage
{
	enum Type
	{
		Default,
		LightmassExportEmissive,
		LightmassExportDiffuse,
		LightmassExportOpacity,
		LightmassExportNormal
	};
}

/** Contains all the information needed to uniquely identify a FMaterialShaderMap. */
class FMaterialShaderMapId
{
public:

	/** 
	 * The base material's StateId.  
	 * This guid represents all the state of a UMaterial that is not covered by the other members of FMaterialShaderMapId.
	 * Any change to the UMaterial that modifes that state (for example, adding an expression) must modify this guid.
	 */
	FGuid BaseMaterialId;

	/** 
	 * Quality level that this shader map is going to be compiled at.  
	 * Can be a value of EMaterialQualityLevel::Num if quality level doesn't matter to the compiled result.
	 */
	EMaterialQualityLevel::Type QualityLevel;

	/** Feature level that the shader map is going to be compiled for. */
	ERHIFeatureLevel::Type FeatureLevel;

	/** 
	 * Indicates what use case this shader map will be for.
	 * This allows the same UMaterial / UMaterialInstance to be compiled with multiple FMaterial derived classes,
	 * While still creating an Id that is deterministic between runs (no appCreateGuid used).
	 */
	EMaterialShaderMapUsage::Type Usage;

	/** Static parameters and base Id. */
	FStaticParameterSet ParameterSet;

	/** Guids of any functions the material was dependent on. */
	TArray<FGuid> ReferencedFunctions;

	/** Guids of any Parameter Collections the material was dependent on. */
	TArray<FGuid> ReferencedParameterCollections;

	/** Shader types of shaders that are inlined in this shader map in the DDC. */
	TArray<FShaderTypeDependency> ShaderTypeDependencies;

	/** Vertex factory types of shaders that are inlined in this shader map in the DDC. */
	TArray<FVertexFactoryTypeDependency> VertexFactoryTypeDependencies;

	/** 
	 * Hash of the textures referenced by the uniform expressions in the shader map.
	 * This is stored in the shader map Id to gracefully handle situations where code changes
	 * that generates the array of textures that the uniform expressions use to link up after being loaded from the DDC.
	 */
	FSHAHash TextureReferencesHash;
	
	/** A hash of the base property overrides for this material instance. */
	FSHAHash BasePropertyOverridesHash;
	
	FMaterialShaderMapId()
		: BaseMaterialId(0, 0, 0, 0)
		, QualityLevel(EMaterialQualityLevel::High)
		, FeatureLevel(ERHIFeatureLevel::SM4)
		, Usage(EMaterialShaderMapUsage::Default)
	{
	}
	
	void SetShaderDependencies(const TArray<FShaderType*>& ShaderTypes, const TArray<FVertexFactoryType*>& VFTypes);

	void Serialize(FArchive& Ar);

	friend uint32 GetTypeHash(const FMaterialShaderMapId& Ref)
	{
		return Ref.BaseMaterialId.A;
	}

	SIZE_T GetSizeBytes() const
	{
		return sizeof(*this)
			+ ReferencedFunctions.GetAllocatedSize()
			+ ReferencedParameterCollections.GetAllocatedSize()
			+ ShaderTypeDependencies.GetAllocatedSize()
			+ VertexFactoryTypeDependencies.GetAllocatedSize();
	}

	/** Hashes the material-specific part of this shader map Id. */
	void GetMaterialHash(FSHAHash& OutHash) const;

	/** 
	* Tests this set against another for equality, disregarding override settings.
	* 
	* @param ReferenceSet	The set to compare against
	* @return				true if the sets are equal
	*/
	bool operator==(const FMaterialShaderMapId& ReferenceSet) const;

	bool operator!=(const FMaterialShaderMapId& ReferenceSet) const
	{
		return !(*this == ReferenceSet);
	}

	/** Appends string representations of this Id to a key string. */
	void AppendKeyString(FString& KeyString) const;

	/** Returns true if the requested shader type is a dependency of this shader map Id. */
	bool ContainsShaderType(const FShaderType* ShaderType) const
	{
		for (int32 TypeIndex = 0; TypeIndex < ShaderTypeDependencies.Num(); TypeIndex++)
		{
			if (ShaderTypeDependencies[TypeIndex].ShaderType == ShaderType)
			{
				return true;
			}
		}

		return false;
	}

	/** Returns true if the requested vertex factory type is a dependency of this shader map Id. */
	bool ContainsVertexFactoryType(const FVertexFactoryType* VFType) const
	{
		for (int32 TypeIndex = 0; TypeIndex < VertexFactoryTypeDependencies.Num(); TypeIndex++)
		{
			if (VertexFactoryTypeDependencies[TypeIndex].VertexFactoryType == VFType)
			{
				return true;
			}
		}

		return false;
	}
};

/**
 * The shaders which the render the material on a mesh generated by a particular vertex factory type.
 */
class FMeshMaterialShaderMap : public TShaderMap<FMeshMaterialShaderType>
{
public:

	FMeshMaterialShaderMap(FVertexFactoryType* InVFType) :
		VertexFactoryType(InVFType)
	{}

	/**
	 * Enqueues compilation for all shaders for a material and vertex factory type.
	 * @param Material - The material to compile shaders for.
	 * @param VertexFactoryType - The vertex factory type to compile shaders for.
	 * @param Platform - The platform to compile for.
	 */
	uint32 BeginCompile(
		uint32 ShaderMapId,
		const FMaterialShaderMapId& InShaderMapId, 
		const FMaterial* Material,
		FShaderCompilerEnvironment* MaterialEnvironment,
		EShaderPlatform Platform,
		TArray<FShaderCompileJob*>& NewJobs
		);

	/**
	 * Creates shaders for all of the compile jobs and caches them in this shader map.
	 * @param Material - The material to compile shaders for.
	 * @param CompilationResults - The compile results that were enqueued by BeginCompile.
	 */
	void FinishCompile(uint32 ShaderMapId, const FUniformExpressionSet& UniformExpressionSet, const FSHAHash& MaterialShaderMapHash, const TArray<FShaderCompileJob*>& CompilationResults, const FString& InDebugDescription);

	/**
	 * Checks whether a material shader map is missing any shader types necessary for the given material.
	 * May be called with a NULL FMeshMaterialShaderMap, which is equivalent to a FMeshMaterialShaderMap with no shaders cached.
	 * @param MeshShaderMap - The FMeshMaterialShaderMap to check for the necessary shaders.
	 * @param Material - The material which is checked.
	 * @return True if the shader map has all of the shader types necessary.
	 */
	static bool IsComplete(
		const FMeshMaterialShaderMap* MeshShaderMap,
		EShaderPlatform Platform,
		const FMaterial* Material,
		FVertexFactoryType* InVertexFactoryType,
		bool bSilent
		);

	void LoadMissingShadersFromMemory(
		const FSHAHash& MaterialShaderMapHash, 
		const FMaterial* Material, 
		EShaderPlatform Platform);

	/**
	 * Removes all entries in the cache with exceptions based on a shader type
	 * @param ShaderType - The shader type to flush
	 */
	void FlushShadersByShaderType(FShaderType* ShaderType);

	// Accessors.
	FVertexFactoryType* GetVertexFactoryType() const { return VertexFactoryType; }

private:
	/** The vertex factory type these shaders are for. */
	FVertexFactoryType* VertexFactoryType;
};

/**
 * The set of material shaders for a single material.
 */
class FMaterialShaderMap : public TShaderMap<FMaterialShaderType>, public FDeferredCleanupInterface
{
public:

	/**
	 * Finds the shader map for a material.
	 * @param StaticParameterSet - The static parameter set identifying the shader map
	 * @param Platform - The platform to lookup for
	 * @return NULL if no cached shader map was found.
	 */
	static FMaterialShaderMap* FindId(const FMaterialShaderMapId& ShaderMapId, EShaderPlatform Platform);

	/** Flushes the given shader types from any loaded FMaterialShaderMap's. */
	static void FlushShaderTypes(TArray<FShaderType*>& ShaderTypesToFlush, TArray<const FVertexFactoryType*>& VFTypesToFlush);

	static void FixupShaderTypes(EShaderPlatform Platform, const TMap<FShaderType*, FString>& ShaderTypeNames, const TMap<FVertexFactoryType*, FString>& VertexFactoryTypeNames);

	/** 
	 * Attempts to load the shader map for the given material from the Derived Data Cache.
	 * If InOutShaderMap is valid, attempts to load the individual missing shaders instead.
	 */
	static void LoadFromDerivedDataCache(const FMaterial* Material, const FMaterialShaderMapId& ShaderMapId, EShaderPlatform Platform, TRefCountPtr<FMaterialShaderMap>& InOutShaderMap);

	FMaterialShaderMap();

	// Destructor.
	~FMaterialShaderMap();

	/**
	 * Compiles the shaders for a material and caches them in this shader map.
	 * @param Material - The material to compile shaders for.
	 * @param ShaderMapId - the set of static parameters to compile for
	 * @param Platform - The platform to compile to
	 */
	void Compile(
		FMaterial* Material,
		const FMaterialShaderMapId& ShaderMapId, 
		TRefCountPtr<FShaderCompilerEnvironment> MaterialEnvironment,
		const FMaterialCompilationOutput& InMaterialCompilationOutput,
		EShaderPlatform Platform,
		bool bSynchronousCompile,
		bool bApplyCompletedShaderMapForRendering
		);

	/** Sorts the incoming compiled jobs into the appropriate mesh shader maps, and finalizes this shader map so that it can be used for rendering. */
	bool ProcessCompilationResults(const TArray<FShaderCompileJob*>& InCompilationResults, int32& ResultIndex, float& TimeBudget);

	/**
	 * Checks whether the material shader map is missing any shader types necessary for the given material.
	 * @param Material - The material which is checked.
	 * @return True if the shader map has all of the shader types necessary.
	 */
	bool IsComplete(const FMaterial* Material, bool bSilent);

	/** Attempts to load missing shaders from memory. */
	void LoadMissingShadersFromMemory(const FMaterial* Material);

	/** Builds a list of the shaders in a shader map. */
	ENGINE_API void GetShaderList(TMap<FShaderId,FShader*>& OutShaders) const;

	/** Registers a material shader map in the global map so it can be used by materials. */
	void Register();

	// Reference counting.
	ENGINE_API void AddRef();
	ENGINE_API void Release();

	// FDeferredCleanupInterface
	virtual void FinishCleanup()
	{
		bDeletedThroughDeferredCleanup = true;
		delete this;
	}

	/**
	 * Removes all entries in the cache with exceptions based on a shader type
	 * @param ShaderType - The shader type to flush
	 */
	void FlushShadersByShaderType(FShaderType* ShaderType);

	/**
	 * Removes all entries in the cache with exceptions based on a vertex factory type
	 * @param ShaderType - The shader type to flush
	 */
	void FlushShadersByVertexFactoryType(const FVertexFactoryType* VertexFactoryType);
	
	/** Removes a material from ShaderMapsBeingCompiled. */
	static void RemovePendingMaterial(FMaterial* Material);

	/** Finds a shader map currently being compiled that was enqueued for the given material. */
	static const FMaterialShaderMap* GetShaderMapBeingCompiled(const FMaterial* Material);

	/** Serializes the shader map. */
	void Serialize(FArchive& Ar, bool bInlineShaderResources=true);

	/** Saves this shader map to the derived data cache. */
	void SaveToDerivedDataCache();

	/** Backs up any FShaders in this shader map to memory through serialization and clears FShader references. */
	TArray<uint8>* BackupShadersToMemory();
	/** Recreates FShaders from the passed in memory, handling shader key changes. */
	void RestoreShadersFromMemory(const TArray<uint8>& ShaderData);

	/** Serializes a shader map to an archive (used with recompiling shaders for a remote console) */
	ENGINE_API static void SaveForRemoteRecompile(FArchive& Ar, const TMap<FString, TArray<TRefCountPtr<FMaterialShaderMap> > >& CompiledShaderMaps, const TArray<FShaderResourceId>& ClientResourceIds);
	ENGINE_API static void LoadForRemoteRecompile(FArchive& Ar, EShaderPlatform ShaderPlatform, const TArray<FString>& MaterialsForShaderMaps);

	/** Computes the memory used by this shader map without counting the shaders themselves. */
	uint32 GetSizeBytes() const
	{
		return sizeof(*this)
			+ MeshShaderMaps.GetAllocatedSize()
			+ OrderedMeshShaderMaps.GetAllocatedSize()
			+ FriendlyName.GetAllocatedSize()
			+ VertexFactoryMap.GetAllocatedSize()
			+ MaterialCompilationOutput.UniformExpressionSet.GetAllocatedSize()
			+ DebugDescription.GetAllocatedSize();
	}

	/** Returns the maximum number of texture samplers used by any shader in this shader map. */
	uint32 GetMaxTextureSamplers() const;

	// Accessors.
	ENGINE_API const FMeshMaterialShaderMap* GetMeshShaderMap(FVertexFactoryType* VertexFactoryType) const;
	const FMaterialShaderMapId& GetShaderMapId() const { return ShaderMapId; }
	EShaderPlatform GetShaderPlatform() const { return Platform; }
	const FString& GetFriendlyName() const { return FriendlyName; }
	uint32 GetCompilingId() const { return CompilingId; }
	bool IsCompilationFinalized() const { return bCompilationFinalized; }
	bool CompiledSuccessfully() const { return bCompiledSuccessfully; }
	const FString& GetDebugDescription() const { return DebugDescription; }
	bool UsesSceneColor() const { return MaterialCompilationOutput.bUsesSceneColor; }
	bool NeedsSceneTextures() const { return MaterialCompilationOutput.bNeedsSceneTextures; }
	bool UsesEyeAdaptation() const { return MaterialCompilationOutput.bUsesEyeAdaptation; }

	bool IsValidForRendering() const
	{
		return bCompilationFinalized && bCompiledSuccessfully && !bDeletedThroughDeferredCleanup;
	}

	const FUniformExpressionSet& GetUniformExpressionSet() const { return MaterialCompilationOutput.UniformExpressionSet; }

private:

	/** 
	 * A global map from a material's static parameter set to any shader map cached for that material. 
	 * Note: this does not necessarily contain all material shader maps in memory.  Shader maps with the same key can evict each other.
	 * No ref counting needed as these are removed on destruction of the shader map.
	 */
	static TMap<FMaterialShaderMapId,FMaterialShaderMap*> GIdToMaterialShaderMap[SP_NumPlatforms];

	/** 
	 * All material shader maps in memory. 
	 * No ref counting needed as these are removed on destruction of the shader map.
	 */
	static TArray<FMaterialShaderMap*> AllMaterialShaderMaps;

	/** The material's cached shaders for vertex factory type dependent shaders. */
	TIndirectArray<FMeshMaterialShaderMap> MeshShaderMaps;

	/** The material's mesh shader maps, indexed by VFType->GetId(), for fast lookup at runtime. */
	TArray<FMeshMaterialShaderMap*> OrderedMeshShaderMaps;

	/** The material's user friendly name, typically the object name. */
	FString FriendlyName;

	/** The platform this shader map was compiled with */
	EShaderPlatform Platform;

	/** The static parameter set that this shader map was compiled with */
	FMaterialShaderMapId ShaderMapId;

	/** A map from vertex factory type to the material's cached shaders for that vertex factory type. */
	TMap<FVertexFactoryType*,FMeshMaterialShaderMap*> VertexFactoryMap;

	/** Uniform expressions generated from the material compile. */
	FMaterialCompilationOutput MaterialCompilationOutput;

	/** Next value for CompilingId. */
	static uint32 NextCompilingId;

	/** Tracks material resources and their shader maps that need to be compiled but whose compilation is being deferred. */
	static TMap<TRefCountPtr<FMaterialShaderMap>, TArray<FMaterial*> > ShaderMapsBeingCompiled;

	/** Uniquely identifies this shader map during compilation, needed for deferred compilation where shaders from multiple shader maps are compiled together. */
	uint32 CompilingId;

	mutable uint32 NumRefs;

	/** Used to catch errors where the shader map is deleted directly. */
	bool bDeletedThroughDeferredCleanup;

	/** Indicates whether this shader map has been registered in GIdToMaterialShaderMap */
	uint32 bRegistered : 1;

	/** 
	 * Indicates whether this shader map has had ProcessCompilationResults called after Compile.
	 * The shader map must not be used on the rendering thread unless bCompilationFinalized is true.
	 */
	uint32 bCompilationFinalized : 1;

	uint32 bCompiledSuccessfully : 1;

	/** Indicates whether the shader map should be stored in the shader cache. */
	uint32 bIsPersistent : 1;

	/** Debug information about how the material shader map was compiled. */
	FString DebugDescription;

	/** Initializes OrderedMeshShaderMaps from the contents of MeshShaderMaps. */
	void InitOrderedMeshShaderMaps();

	friend ENGINE_API void DumpMaterialStats( EShaderPlatform Platform );
	friend class FShaderCompilingManager;
};

//
//	EMaterialProperty
//

enum EMaterialProperty
{
	MP_EmissiveColor = 0,
	MP_Opacity,
	MP_OpacityMask,
	MP_DiffuseColor,
	MP_SpecularColor,
	MP_BaseColor,
	MP_Metallic,
	MP_Specular,
	MP_Roughness,
	MP_Normal,
	MP_WorldPositionOffset,
	MP_WorldDisplacement,
	MP_TessellationMultiplier,
	MP_SubsurfaceColor,
	MP_AmbientOcclusion,
	MP_Refraction,
	MP_CustomizedUVs0,
	MP_CustomizedUVs1,
	MP_CustomizedUVs2,
	MP_CustomizedUVs3,
	MP_CustomizedUVs4,
	MP_CustomizedUVs5,
	MP_CustomizedUVs6,
	MP_CustomizedUVs7,
	MP_MaterialAttributes,
	MP_MAX,
};

/** 
 * Enum that contains entries for the ways that material properties need to be compiled.
 * This 'inherits' from EMaterialProperty in the sense that all of its values start after the values in EMaterialProperty.
 * Each material property is compiled once for its usual shader frequency, determined by GetMaterialPropertyShaderFrequency(),
 * And then this enum contains entries for extra compiles of a material property with a different shader frequency.
 * This is necessary for material properties which need to be evaluated in multiple shader frequencies.
 */
enum ECompiledMaterialProperty
{
	CompiledMP_EmissiveColorCS = MP_MAX,
	CompiledMP_MAX
};

/**
 * Uniquely identifies a material expression output. 
 * Used by the material compiler to keep track of which output it is compiling.
 */
class FMaterialExpressionKey
{
public:
	UMaterialExpression* Expression;
	int32 OutputIndex;

	/** An index used by some expressions to send multiple values across a single connection.*/
	int32 MultiplexIndex;

	FMaterialExpressionKey(UMaterialExpression* InExpression, int32 InOutputIndex) :
		Expression(InExpression),
		OutputIndex(InOutputIndex),
		MultiplexIndex(INDEX_NONE)
	{}

	FMaterialExpressionKey(UMaterialExpression* InExpression, int32 InOutputIndex, int32 InMultiplexIndex) :
		Expression(InExpression),
		OutputIndex(InOutputIndex),
		MultiplexIndex(InMultiplexIndex)
	{}


	friend bool operator==(const FMaterialExpressionKey& X, const FMaterialExpressionKey& Y)
	{
		return X.Expression == Y.Expression && X.OutputIndex == Y.OutputIndex && X.MultiplexIndex == Y.MultiplexIndex;
	}

	friend uint32 GetTypeHash(const FMaterialExpressionKey& ExpressionKey)
	{
		return PointerHash(ExpressionKey.Expression);
	}
};

/** Function specific compiler state. */
class FMaterialFunctionCompileState
{
public:

	class UMaterialExpressionMaterialFunctionCall* FunctionCall;

	// Stack used to avoid re-entry within this function
	TArray<FMaterialExpressionKey> ExpressionStack;

	/** A map from material expression to the index into CodeChunks of the code for the material expression. */
	TMap<FMaterialExpressionKey,int32> ExpressionCodeMap[MP_MAX][SF_NumFrequencies];

	explicit FMaterialFunctionCompileState(UMaterialExpressionMaterialFunctionCall* InFunctionCall) :
		FunctionCall(InFunctionCall)
	{}
};

/**
 * @return The type of value expected for the given material property.
 */
extern ENGINE_API EMaterialValueType GetMaterialPropertyType(EMaterialProperty Property);

/** Returns the shader frequency corresponding to the given material input. */
extern ENGINE_API EShaderFrequency GetMaterialPropertyShaderFrequency(EMaterialProperty Property);

/** Returns whether the given expression class is allowed. */
extern ENGINE_API bool IsAllowedExpressionType(UClass* Class, bool bMaterialFunction);

/** Parses a string into multiple lines, for use with tooltips. */
extern ENGINE_API void ConvertToMultilineToolTip(const FString& InToolTip, int32 TargetLineLength, TArray<FString>& OutToolTip);

/** Given a combination of EMaterialValueType flags, get text descriptions of all types */
extern ENGINE_API void GetMaterialValueTypeDescriptions(uint32 MaterialValueType, TArray<FText>& OutDescriptions);

/** Check whether a combination of EMaterialValueType flags can be connected */
extern ENGINE_API bool CanConnectMaterialValueTypes(uint32 InputType, uint32 OutputType);

/**
 * FMaterial serves 3 intertwined purposes:
 *   Represents a material to the material compilation process, and provides hooks for extensibility (CompileProperty, etc)
 *   Represents a material to the renderer, with functions to access material properties
 *   Stores a cached shader map, and other transient output from a compile, which is necessary with async shader compiling
 *      (when a material finishes async compilation, the shader map and compile errors need to be stored somewhere)
 */
class FMaterial
{
public:

	/**
	 * Minimal initialization constructor.
	 */
	FMaterial():
		RenderingThreadShaderMap(NULL),
		Id_DEPRECATED(0,0,0,0),
		OutstandingCompileShaderMapId(INDEX_NONE),
		QualityLevel(EMaterialQualityLevel::High),
		bQualityLevelHasDifferentNodes(false),
		FeatureLevel(ERHIFeatureLevel::SM4),
		bContainsInlineShaders(false)
	{}

	/**
	 * Destructor
	 */
	ENGINE_API virtual ~FMaterial();

	/**
	 * Caches the material shaders for this material with no static parameters on the given platform.
	 * This is used by material resources of UMaterials.
	 */
	ENGINE_API bool CacheShaders(EShaderPlatform Platform, bool bApplyCompletedShaderMapForRendering);

	/**
	 * Caches the material shaders for the given static parameter set and platform.
	 * This is used by material resources of UMaterialInstances.
	 */
	ENGINE_API bool CacheShaders(const FMaterialShaderMapId& ShaderMapId, EShaderPlatform Platform, bool bApplyCompletedShaderMapForRendering);

	/**
	 * Should the shader for this material with the given platform, shader type and vertex 
	 * factory type combination be compiled
	 *
	 * @param Platform		The platform currently being compiled for
	 * @param ShaderType	Which shader is being compiled
	 * @param VertexFactory	Which vertex factory is being compiled (can be NULL)
	 *
	 * @return true if the shader should be compiled
	 */
	ENGINE_API virtual bool ShouldCache(EShaderPlatform Platform, const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const;

	/** Serializes the material. */
	ENGINE_API virtual void LegacySerialize(FArchive& Ar);

	/** Serializes the shader map inline in this material, including any shader dependencies. */
	void SerializeInlineShaderMap(FArchive& Ar);

	/** Releases this material's shader map.  Must only be called on materials not exposed to the rendering thread! */
	void ReleaseShaderMap();

	// Material properties.
	ENGINE_API virtual void GetShaderMapId(EShaderPlatform Platform, FMaterialShaderMapId& OutId) const;
	virtual int32 GetMaterialDomain() const = 0; // See EMaterialDomain.
	virtual bool IsTwoSided() const = 0;
	virtual bool IsTangentSpaceNormal() const { return false; }
	virtual bool ShouldInjectEmissiveIntoLPV() const { return false; }
	virtual bool ShouldGenerateSphericalParticleNormals() const { return false; }
	virtual	bool ShouldDisableDepthTest() const { return false; }
	virtual	bool ShouldEnableResponsiveAA() const { return false; }
	virtual bool IsLightFunction() const = 0;
	virtual bool IsUsedWithEditorCompositing() const { return false; }
	virtual bool IsUsedWithDeferredDecal() const = 0;
	virtual bool IsWireframe() const = 0;
	virtual bool IsSpecialEngineMaterial() const = 0;
	virtual bool IsUsedWithSkeletalMesh() const { return false; }
	virtual bool IsUsedWithLandscape() const { return false; }
	virtual bool IsUsedWithParticleSystem() const { return false; }
	virtual bool IsUsedWithParticleSprites() const { return false; }
	virtual bool IsUsedWithBeamTrails() const { return false; }
	virtual bool IsUsedWithMeshParticles() const { return false; }
	virtual bool IsUsedWithStaticLighting() const { return false; }
	virtual	bool IsUsedWithMorphTargets() const { return false; }
	virtual bool IsUsedWithSplineMeshes() const { return false; }
	virtual bool IsUsedWithInstancedStaticMeshes() const { return false; }
	virtual bool IsUsedWithAPEXCloth() const { return false; }
	ENGINE_API virtual enum EMaterialTessellationMode GetTessellationMode() const;
	virtual bool IsCrackFreeDisplacementEnabled() const { return false; }
	virtual bool IsAdaptiveTessellationEnabled() const { return false; }
	virtual bool IsFullyRough() const { return false; }
	virtual bool UseLmDirectionality() const { return true; }
	virtual bool IsMasked() const = 0;
	virtual enum EBlendMode GetBlendMode() const = 0;
	virtual enum EMaterialLightingModel GetLightingModel() const = 0;
	virtual enum ETranslucencyLightingMode GetTranslucencyLightingMode() const { return TLM_VolumetricNonDirectional; };
	virtual float GetOpacityMaskClipValue() const = 0;
	virtual bool IsDistorted() const { return false; };
	virtual float GetTranslucencyDirectionalLightingIntensity() const { return 1.0f; }
	virtual float GetTranslucentShadowDensityScale() const { return 1.0f; }
	virtual float GetTranslucentSelfShadowDensityScale() const { return 1.0f; }
	virtual float GetTranslucentSelfShadowSecondDensityScale() const { return 1.0f; }
	virtual float GetTranslucentSelfShadowSecondOpacity() const { return 1.0f; }
	virtual float GetTranslucentBackscatteringExponent() const { return 1.0f; }
	virtual bool IsSeparateTranslucencyEnabled() const { return false; }
	virtual FLinearColor GetTranslucentMultipleScatteringExtinction() const { return FLinearColor::White; }
	virtual float GetTranslucentShadowStartOffset() const { return 0.0f; }
	virtual float GetRefractionDepthBiasValue() const { return 0.0f; }
	virtual bool UseTranslucencyVertexFog() const { return false; }
	virtual FString GetFriendlyName() const = 0;
	virtual bool HasVertexPositionOffsetConnected() const { return false; }
	virtual uint32 GetDecalBlendMode() const { return 0; }
	virtual bool HasNormalConnected() const { return false; }
	virtual bool RequiresSynchronousCompilation() const { return false; };
	virtual bool IsDefaultMaterial() const { return false; };

	/**
	 * Should shaders compiled for this material be saved to disk?
	 */
	virtual bool IsPersistent() const = 0;

	/**
	* Called when compilation of an FMaterial finishes, after the GameThreadShaderMap is set and the render command to set the RenderThreadShaderMap is queued
	*/
	virtual void NotifyCompilationFinished() { }

	/** 
	 * Blocks until compilation has completed. Returns immediately if a compilation is not outstanding.
	 */
	ENGINE_API void FinishCompilation();

	EMaterialQualityLevel::Type GetQualityLevel() const 
	{
		return QualityLevel;
	}

	// Accessors.
	const TArray<FString>& GetCompileErrors() const { return CompileErrors; }
	void SetCompileErrors(const TArray<FString>& InCompileErrors) { CompileErrors = InCompileErrors; }
	const TArray<UMaterialExpression*>& GetErrorExpressions() const { return ErrorExpressions; }
	ENGINE_API const TArray<TRefCountPtr<FMaterialUniformExpressionTexture> >& GetUniform2DTextureExpressions() const;
	ENGINE_API const TArray<TRefCountPtr<FMaterialUniformExpressionTexture> >& GetUniformCubeTextureExpressions() const;
	const FGuid& GetLegacyId() const { return Id_DEPRECATED; }

	ERHIFeatureLevel::Type GetFeatureLevel() const { return FeatureLevel; }
	bool GetUsesDynamicParameter() const 
	{ 
		//@todo - remove non-dynamic parameter particle VF and always support dynamic parameter
		return true; 
	}
	ENGINE_API bool UsesSceneColor() const;
	ENGINE_API bool NeedsSceneTextures() const;
	ENGINE_API bool UsesEyeAdaptation() const;	
	ENGINE_API bool MaterialModifiesMeshPosition() const;

	class FMaterialShaderMap* GetGameThreadShaderMap() const 
	{ 
		checkSlow(IsInGameThread());
		return GameThreadShaderMap; 
	}

	/** Note: SetRenderingThreadShaderMap must also be called with the same value, but from the rendering thread. */
	void SetGameThreadShaderMap(FMaterialShaderMap* InMaterialShaderMap)
	{
		checkSlow(IsInGameThread());
		GameThreadShaderMap = InMaterialShaderMap;
	}

	void SetInlineShaderMap(FMaterialShaderMap* InMaterialShaderMap)
	{
		checkSlow(IsInGameThread());
		GameThreadShaderMap = InMaterialShaderMap;
		bContainsInlineShaders = true;
	}

	ENGINE_API class FMaterialShaderMap* GetRenderingThreadShaderMap() const;

	/** Note: SetGameThreadShaderMap must also be called with the same value, but from the game thread. */
	ENGINE_API void SetRenderingThreadShaderMap(FMaterialShaderMap* InMaterialShaderMap);

	void ClearOutstandingCompileId()
	{
		OutstandingCompileShaderMapId = INDEX_NONE;
	}

	void AddReferencedObjects(FReferenceCollector& Collector);

	virtual const TArray<UTexture*>& GetReferencedTextures() const = 0;

	/**
	 * Finds the shader matching the template type and the passed in vertex factory, asserts if not found.
	 * Note - Only implemented for FMeshMaterialShaderTypes
	 */
	template<typename ShaderType>
	ShaderType* GetShader(FVertexFactoryType* VertexFactoryType) const
	{
		return (ShaderType*)GetShader(&ShaderType::StaticType, VertexFactoryType);
	}

	/** Returns a string that describes the material's usage for debugging purposes. */
	virtual FString GetMaterialUsageDescription() const = 0;

	/**
	* Get user source code for the material, with a list of code snippets to highlight representing the code for each MaterialExpression
	* @param OutSource - generated source code
	* @param OutHighlightMap - source code highlight list
	* @return - true on Success
	*/
	ENGINE_API bool GetMaterialExpressionSource( FString& OutSource, TMap<FMaterialExpressionKey,int32> OutExpressionCodeMap[][SF_NumFrequencies] );

	/** 
	 * Adds an FMaterial to the global list.
	 * Any FMaterials that don't belong to a UMaterialInterface need to be registered in this way to work correctly with runtime recompiling of outdated shaders.
	 */
	static void AddEditorLoadedMaterialResource(FMaterial* Material)
	{
		EditorLoadedMaterialResources.Add(Material);
	}

	/** Recompiles any materials in the EditorLoadedMaterialResources list if they are not complete. */
	static void UpdateEditorLoadedMaterialResources();

	/** Backs up any FShaders in editor loaded materials to memory through serialization and clears FShader references. */
	static void BackupEditorLoadedMaterialShadersToMemory(TMap<FMaterialShaderMap*, TScopedPointer<TArray<uint8> > >& ShaderMapToSerializedShaderData);
	/** Recreates FShaders in editor loaded materials from the passed in memory, handling shader key changes. */
	static void RestoreEditorLoadedMaterialShadersFromMemory(const TMap<FMaterialShaderMap*, TScopedPointer<TArray<uint8> > >& ShaderMapToSerializedShaderData);

protected:

	/** Entry point for compiling a specific material property.  This must call SetMaterialProperty. */
	virtual int32 CompileProperty(EMaterialProperty Property,EShaderFrequency InShaderFrequency,class FMaterialCompiler* Compiler) const = 0;

	/** Returns the index to the Expression in the Expressions array, or -1 if not found. */
	int32 FindExpression(const TArray<TRefCountPtr<FMaterialUniformExpressionTexture> >&Expressions, const FMaterialUniformExpressionTexture &Expression);

	/** Useful for debugging. */
	virtual FString GetBaseMaterialPathName() const { return TEXT(""); }

	void SetQualityLevelProperties(EMaterialQualityLevel::Type InQualityLevel, bool bInQualityLevelHasDifferentNodes, ERHIFeatureLevel::Type InFeatureLevel)
	{
		QualityLevel = InQualityLevel;
		bQualityLevelHasDifferentNodes = bInQualityLevelHasDifferentNodes;
		FeatureLevel = InFeatureLevel;
	}

	/** 
	 * Gets the shader map usage of the material, which will be included in the DDC key.
	 * This mechanism allows derived material classes to create different DDC keys with the same base material.
	 * For example lightmass exports diffuse and emissive, each of which requires a material resource with the same base material.
	 */
	virtual EMaterialShaderMapUsage::Type GetShaderMapUsage() const { return EMaterialShaderMapUsage::Default; }

	/** Gets the Guid that represents this material. */
	virtual FGuid GetMaterialId() const = 0;

private:

	/** 
	 * Tracks FMaterials without a corresponding UMaterialInterface in the editor, for example FExpressionPreviews.
	 * Used to handle the 'recompileshaders changed' command in the editor.
	 * This doesn't have to use a reference counted pointer because materials are removed on destruction.
	 */
	ENGINE_API static TSet<FMaterial*> EditorLoadedMaterialResources;

	TArray<FString> CompileErrors;

	/** List of material expressions which generated a compiler error during the last compile. */
	TArray<UMaterialExpression*> ErrorExpressions;

	/** 
	 * Game thread tracked shader map, which is ref counted and manages shader map lifetime. 
	 * The shader map uses deferred deletion so that the rendering thread has a chance to process a release command when the shader map is no longer referenced.
	 * Code that sets this is responsible for updating RenderingThreadShaderMap in a thread safe way.
	 * During an async compile, this will be NULL and will not contain the actual shader map until compilation is complete.
	 */
	TRefCountPtr<FMaterialShaderMap> GameThreadShaderMap;

	/** 
	 * Shader map for this material resource which is accessible by the rendering thread. 
	 * This must be updated along with GameThreadShaderMap, but on the rendering thread.
	 */
	FMaterialShaderMap* RenderingThreadShaderMap;

	/** 
	 * Legacy unique identifier of this material resource.
	 * This functionality is now provided by UMaterial::StateId.
	 */
	FGuid Id_DEPRECATED;

	/** 
	 * Contains the compiling id of this shader map when it is being compiled asynchronously. 
	 * This can be used to access the shader map during async compiling, since GameThreadShaderMap will not have been set yet.
	 */
	int32 OutstandingCompileShaderMapId;

	/** Quality level that this material is representing. */
	EMaterialQualityLevel::Type QualityLevel;

	/** Whether this material has quality level specific nodes. */
	bool bQualityLevelHasDifferentNodes;

	/** Feature level that this material is representing. */
	ERHIFeatureLevel::Type FeatureLevel;

	/** 
	 * Whether this material was loaded with shaders inlined. 
	 * If true, GameThreadShaderMap will contain a reference to the inlined shader map between Serialize and PostLoad.
	 */
	uint32 bContainsInlineShaders : 1;

	/**
	* Compiles this material for Platform, storing the result in OutShaderMap if the compile was synchronous
	*/
	bool BeginCompileShaderMap(
		const FMaterialShaderMapId& ShaderMapId,
		EShaderPlatform Platform, 
		TRefCountPtr<class FMaterialShaderMap>& OutShaderMap, 
		bool bApplyCompletedShaderMapForRendering);

	/** Populates OutEnvironment with defines needed to compile shaders for this material. */
	void SetupMaterialEnvironment(
		EShaderPlatform Platform,
		const FUniformExpressionSet& InUniformExpressionSet,
		FShaderCompilerEnvironment& OutEnvironment
		) const;

	/**
	 * Finds the shader matching the template type and the passed in vertex factory, asserts if not found.
	 */
	ENGINE_API FShader* GetShader(class FMeshMaterialShaderType* ShaderType, FVertexFactoryType* VertexFactoryType) const;

	void GetReferencedTexturesHash(FSHAHash& OutHash) const;

	/** Produces arrays of any shader and vertex factory type that this material is dependent on. */
	ENGINE_API void GetDependentShaderAndVFTypes(EShaderPlatform Platform, TArray<FShaderType*>& OutShaderTypes, TArray<FVertexFactoryType*>& OutVFTypes) const;

	EMaterialQualityLevel::Type GetQualityLevelForShaderMapId() const 
	{
		return bQualityLevelHasDifferentNodes ? QualityLevel : EMaterialQualityLevel::Num;
	}

	friend class FMaterialShaderMap;
	friend class FShaderCompilingManager;
	friend class FHLSLMaterialTranslator;
};

/**
 * Cached uniform expression values.
 */
struct FUniformExpressionCache
{
	/** Material uniform buffer. */
	FUniformBufferRHIRef UniformBuffer;
	/** Textures referenced by uniform expressions. */
	TArray<const UTexture*> Textures;
	/** Cube textures referenced by uniform expressions. */
	TArray<const UTexture*> CubeTextures;
	/** Ids of parameter collections needed for rendering. */
	TArray<FGuid> ParameterCollections;
	/** True if the cache is up to date. */
	bool bUpToDate;

	/** Shader map that was used to cache uniform expressions on this material.  This is used for debugging and verifying correct behavior. */
	const FMaterialShaderMap* CachedUniformExpressionShaderMap;

	FUniformExpressionCache() :
		bUpToDate(false),
		CachedUniformExpressionShaderMap(NULL)
	{}

	/** Destructor. */
	~FUniformExpressionCache()
	{
		UniformBuffer.SafeRelease();
	}
};

/**
 * A material render proxy used by the renderer.
 */
class FMaterialRenderProxy : public FRenderResource
{
public:

	/** Cached uniform expressions. */
	mutable FUniformExpressionCache UniformExpressionCache[ERHIFeatureLevel::Num];

	/** Default constructor. */
	ENGINE_API FMaterialRenderProxy();

	/** Initialization constructor. */
	ENGINE_API FMaterialRenderProxy(bool bInSelected, bool bInHovered);

	/** Destructor. */
	ENGINE_API virtual ~FMaterialRenderProxy();

	/**
	 * Evaluates uniform expressions and stores them in OutUniformExpressionCache.
	 * @param OutUniformExpressionCache - The uniform expression cache to build.
	 * @param MaterialRenderContext - The context for which to cache expressions.
	 */
	void ENGINE_API EvaluateUniformExpressions(FUniformExpressionCache& OutUniformExpressionCache, const FMaterialRenderContext& Context) const;

	/**
	 * Caches uniform expressions for efficient runtime evaluation.
	 */
	void ENGINE_API CacheUniformExpressions();

	/**
	 * Enqueues a rendering command to cache uniform expressions for efficient runtime evaluation.
	 */
	void ENGINE_API CacheUniformExpressions_GameThread();

	/**
	 * Invalidates the uniform expression cache.
	 */
	void ENGINE_API InvalidateUniformExpressionCache();

	// These functions should only be called by the rendering thread.

	/** Returns the effective FMaterial, which can be a fallback if this material's shader map is invalid.  Always returns a valid material pointer. */
	virtual const class FMaterial* GetMaterial(ERHIFeatureLevel::Type InFeatureLevel) const = 0;
	/** Returns the FMaterial, without using a fallback if the FMaterial doesn't have a valid shader map. Can return NULL. */
	virtual FMaterial* GetMaterialNoFallback(ERHIFeatureLevel::Type InFeatureLevel) const { return NULL; }
	virtual bool GetVectorValue(const FName ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const = 0;
	virtual bool GetScalarValue(const FName ParameterName, float* OutValue, const FMaterialRenderContext& Context) const = 0;
	virtual bool GetTextureValue(const FName ParameterName,const UTexture** OutValue, const FMaterialRenderContext& Context) const = 0;
	virtual float GetDistanceFieldPenumbraScale() const { return 1.0f; }
	bool IsSelected() const { return bSelected; }
	bool IsHovered() const { return bHovered; }

	// FRenderResource interface.
	ENGINE_API virtual void InitDynamicRHI();
	ENGINE_API virtual void ReleaseDynamicRHI();

	ENGINE_API static const TSet<FMaterialRenderProxy*>& GetMaterialRenderProxyMap() 
	{
		return MaterialRenderProxyMap;
	}

private:

	/** true if the material is selected. */
	bool bSelected : 1;
	/** true if the material is hovered. */
	bool bHovered : 1;

	/** 
	 * Tracks all material render proxies in all scenes, can only be accessed on the rendering thread.
	 * This is used to propagate new shader maps to materials being used for rendering.
	 */
	ENGINE_API static TSet<FMaterialRenderProxy*> MaterialRenderProxyMap;
};

/**
 * An material render proxy which overrides the material's Color vector parameter.
 */
class FColoredMaterialRenderProxy : public FMaterialRenderProxy
{
public:

	const FMaterialRenderProxy* const Parent;
	const FLinearColor Color;

	/** Initialization constructor. */
	FColoredMaterialRenderProxy(const FMaterialRenderProxy* InParent,const FLinearColor& InColor):
		Parent(InParent),
		Color(InColor)
	{}

	// FMaterialRenderProxy interface.
	ENGINE_API virtual const class FMaterial* GetMaterial(ERHIFeatureLevel::Type InFeatureLevel) const;
	ENGINE_API virtual bool GetVectorValue(const FName ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const;
	ENGINE_API virtual bool GetScalarValue(const FName ParameterName, float* OutValue, const FMaterialRenderContext& Context) const;
	ENGINE_API virtual bool GetTextureValue(const FName ParameterName,const UTexture** OutValue, const FMaterialRenderContext& Context) const;
};

/**
 * An material render proxy which overrides the material's Color and Lightmap resolution vector parameter.
 */
class FLightingDensityMaterialRenderProxy : public FColoredMaterialRenderProxy
{
public:
	const FVector2D LightmapResolution;

	/** Initialization constructor. */
	FLightingDensityMaterialRenderProxy(const FMaterialRenderProxy* InParent,const FLinearColor& InColor, const FVector2D& InLightmapResolution) :
		FColoredMaterialRenderProxy(InParent, InColor), 
		LightmapResolution(InLightmapResolution)
	{}

	// FMaterialRenderProxy interface.
	virtual bool GetVectorValue(const FName ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const;
};

/**
* A material render proxy for font rendering
*/
class FFontMaterialRenderProxy : public FMaterialRenderProxy
{
public:

	/** parent material instance for fallbacks */
	const FMaterialRenderProxy* const Parent;
	/** font which supplies the texture pages */
	const class UFont* Font;
	/** index to the font texture page to use by the intance */
	const int32 FontPage;
	/** font parameter name for finding the matching parameter */
	const FName& FontParamName;

	/** Initialization constructor. */
	FFontMaterialRenderProxy(const FMaterialRenderProxy* InParent,const class UFont* InFont,const int32 InFontPage, const FName& InFontParamName)
	:	Parent(InParent)
	,	Font(InFont)
	,	FontPage(InFontPage)
	,	FontParamName(InFontParamName)
	{
		check(Parent);
		check(Font);
	}

	// FMaterialRenderProxy interface.
	virtual const class FMaterial* GetMaterial(ERHIFeatureLevel::Type InFeatureLevel) const;
	virtual bool GetVectorValue(const FName ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const;
	virtual bool GetScalarValue(const FName ParameterName, float* OutValue, const FMaterialRenderContext& Context) const;
	virtual bool GetTextureValue(const FName ParameterName,const UTexture** OutValue, const FMaterialRenderContext& Context) const;
};

/**
 * A material render proxy which overrides the selection color
 */
class FOverrideSelectionColorMaterialRenderProxy : public FMaterialRenderProxy
{
public:

	const FMaterialRenderProxy* const Parent;
	const FLinearColor SelectionColor;

	/** Initialization constructor. */
	FOverrideSelectionColorMaterialRenderProxy(const FMaterialRenderProxy* InParent,const FLinearColor& InSelectionColor) :
		Parent(InParent), 
		SelectionColor(InSelectionColor)
	{}

	// FMaterialRenderProxy interface.
	virtual const class FMaterial* GetMaterial(ERHIFeatureLevel::Type InFeatureLevel) const;
	virtual bool GetVectorValue(const FName ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const;
	virtual bool GetScalarValue(const FName ParameterName, float* OutValue, const FMaterialRenderContext& Context) const;
	virtual bool GetTextureValue(const FName ParameterName,const UTexture** OutValue, const FMaterialRenderContext& Context) const;
};

//
//	FExpressionInput
//

//@warning: FExpressionInput is mirrored in MaterialExpression.h and manually "subclassed" in Material.h (FMaterialInput)
struct FExpressionInput
{
	/** Material expression that this input is connected to, or NULL if not connected. */
	class UMaterialExpression*	Expression;

	/** Index into Expression's outputs array that this input is connected to. */
	int32						OutputIndex;

	/** 
	 * Optional name of the input.  
	 * Note that this is the only member which is not derived from the output currently connected. 
	 */
	FString						InputName;

	int32						Mask,
								MaskR,
								MaskG,
								MaskB,
								MaskA;
	uint32						GCC64Padding; // @todo 64: if the C++ didn't mismirror this structure, we might not need this

	FExpressionInput()
		: Expression(NULL)
		, OutputIndex(0)
		, Mask(0)
		, MaskR(0)
		, MaskG(0)
		, MaskB(0)
		, MaskA(0)
		, GCC64Padding(0)
	{
	}

	int32 Compile(class FMaterialCompiler* Compiler, int32 MultiplexIndex=INDEX_NONE);

	/**
	 * Tests if the input has a material expression connected to it
	 *
	 * @return	true if an expression is connected, otherwise false
	 */
	bool IsConnected() const { return (NULL != Expression); }

	/** Connects output of InExpression to this input */
	ENGINE_API void Connect( int32 InOutputIndex, class UMaterialExpression* InExpression );
};

//
//	FMaterialInput
//

template<class InputType> struct FMaterialInput: FExpressionInput
{
	uint32	UseConstant:1;
	InputType	Constant;
};

struct FColorMaterialInput: FMaterialInput<FColor>
{
	ENGINE_API int32 Compile(class FMaterialCompiler* Compiler,const FColor& Default);
};
struct FScalarMaterialInput: FMaterialInput<float>
{
	ENGINE_API int32 Compile(class FMaterialCompiler* Compiler,float Default);
};

struct FVectorMaterialInput: FMaterialInput<FVector>
{
	ENGINE_API int32 Compile(class FMaterialCompiler* Compiler,const FVector& Default);
};

struct FVector2MaterialInput: FMaterialInput<FVector2D>
{
	int32 Compile(class FMaterialCompiler* Compiler,const FVector2D& Default);
};

struct FMaterialAttributesInput: FMaterialInput<int32>
{
	ENGINE_API int32 Compile(class FMaterialCompiler* Compiler, EMaterialProperty Property, float DefaultFloat, const FColor& DefaultColor, const FVector& DefaultVector);
};

//
//	FExpressionOutput
//

struct FExpressionOutput
{
	FString	OutputName;
	int32	Mask,
			MaskR,
			MaskG,
			MaskB,
			MaskA;

	FExpressionOutput(int32 InMask = 0,int32 InMaskR = 0,int32 InMaskG = 0,int32 InMaskB = 0,int32 InMaskA = 0):
		Mask(InMask),
		MaskR(InMaskR),
		MaskG(InMaskG),
		MaskB(InMaskB),
		MaskA(InMaskA)
	{}

	FExpressionOutput(FString InOutputName, int32 InMask = 0,int32 InMaskR = 0,int32 InMaskG = 0,int32 InMaskB = 0,int32 InMaskA = 0):
		OutputName(InOutputName),
		Mask(InMask),
		MaskR(InMaskR),
		MaskG(InMaskG),
		MaskB(InMaskB),
		MaskA(InMaskA)
	{}
};

/**
 * @return True if BlendMode is translucent (should be part of the translucent rendering).
 */
extern ENGINE_API bool IsTranslucentBlendMode(enum EBlendMode BlendMode);

/**
 * Implementation of the FMaterial interface for a UMaterial or UMaterialInstance.
 */
class FMaterialResource : public FMaterial
{
public:

	ENGINE_API FMaterialResource();
	virtual ~FMaterialResource() {}

	void SetMaterial(UMaterial* InMaterial, EMaterialQualityLevel::Type InQualityLevel, bool bInQualityLevelHasDifferentNodes, ERHIFeatureLevel::Type InFeatureLevel, UMaterialInstance* InInstance = NULL)
	{
		Material = InMaterial;
		MaterialInstance = InInstance;
		SetQualityLevelProperties(InQualityLevel, bInQualityLevelHasDifferentNodes, InFeatureLevel);
	}

	/** Returns the number of samplers used in this material, or -1 if the material does not have a valid shader map (compile error or still compiling). */
	ENGINE_API int32 GetSamplerUsage() const;

	ENGINE_API virtual FString GetMaterialUsageDescription() const;

	// FMaterial interface.
	ENGINE_API virtual void GetShaderMapId(EShaderPlatform Platform, FMaterialShaderMapId& OutId) const;
	ENGINE_API virtual int32 GetMaterialDomain() const OVERRIDE;
	ENGINE_API virtual bool IsTwoSided() const;
	ENGINE_API virtual bool IsTangentSpaceNormal() const;
	ENGINE_API virtual bool ShouldInjectEmissiveIntoLPV() const;
	ENGINE_API virtual bool ShouldGenerateSphericalParticleNormals() const;
	ENGINE_API virtual bool ShouldDisableDepthTest() const;
	ENGINE_API virtual bool ShouldEnableResponsiveAA() const;
	ENGINE_API virtual bool IsLightFunction() const;
	ENGINE_API virtual bool IsUsedWithEditorCompositing() const;
	ENGINE_API virtual bool IsUsedWithDeferredDecal() const;
	ENGINE_API virtual bool IsWireframe() const;
	ENGINE_API virtual bool IsSpecialEngineMaterial() const;
	ENGINE_API virtual bool IsUsedWithSkeletalMesh() const;
	ENGINE_API virtual bool IsUsedWithLandscape() const;
	ENGINE_API virtual bool IsUsedWithParticleSystem() const;
	ENGINE_API virtual bool IsUsedWithParticleSprites() const;
	ENGINE_API virtual bool IsUsedWithBeamTrails() const;
	ENGINE_API virtual bool IsUsedWithMeshParticles() const;
	ENGINE_API virtual bool IsUsedWithStaticLighting() const;
	ENGINE_API virtual bool IsUsedWithMorphTargets() const;
	ENGINE_API virtual bool IsUsedWithSplineMeshes() const;
	ENGINE_API virtual bool IsUsedWithInstancedStaticMeshes() const;
	ENGINE_API virtual bool IsUsedWithAPEXCloth() const;
	ENGINE_API virtual enum EMaterialTessellationMode GetTessellationMode() const;
	ENGINE_API virtual bool IsCrackFreeDisplacementEnabled() const;
	ENGINE_API virtual bool IsAdaptiveTessellationEnabled() const;
	ENGINE_API virtual bool IsFullyRough() const;
	ENGINE_API virtual bool UseLmDirectionality() const;
	ENGINE_API virtual enum EBlendMode GetBlendMode() const;
	ENGINE_API virtual uint32 GetDecalBlendMode() const;
	ENGINE_API virtual bool HasNormalConnected() const;
	ENGINE_API virtual enum EMaterialLightingModel GetLightingModel() const;
	ENGINE_API virtual enum ETranslucencyLightingMode GetTranslucencyLightingMode() const;
	ENGINE_API virtual float GetOpacityMaskClipValue() const;
	ENGINE_API virtual bool IsDistorted() const;
	ENGINE_API virtual float GetTranslucencyDirectionalLightingIntensity() const;
	ENGINE_API virtual float GetTranslucentShadowDensityScale() const;
	ENGINE_API virtual float GetTranslucentSelfShadowDensityScale() const;
	ENGINE_API virtual float GetTranslucentSelfShadowSecondDensityScale() const;
	ENGINE_API virtual float GetTranslucentSelfShadowSecondOpacity() const;
	ENGINE_API virtual float GetTranslucentBackscatteringExponent() const;
	ENGINE_API virtual bool IsSeparateTranslucencyEnabled() const;
	ENGINE_API virtual FLinearColor GetTranslucentMultipleScatteringExtinction() const;
	ENGINE_API virtual float GetTranslucentShadowStartOffset() const;
	ENGINE_API virtual bool IsMasked() const;
	ENGINE_API virtual FString GetFriendlyName() const;
	ENGINE_API virtual bool RequiresSynchronousCompilation() const;
	ENGINE_API virtual bool IsDefaultMaterial() const;
	ENGINE_API virtual float GetRefractionDepthBiasValue() const;
	ENGINE_API virtual bool UseTranslucencyVertexFog() const;
	/**
	 * Should shaders compiled for this material be saved to disk?
	 */
	ENGINE_API virtual bool IsPersistent() const;
	ENGINE_API virtual FGuid GetMaterialId() const OVERRIDE;

	ENGINE_API virtual void NotifyCompilationFinished() OVERRIDE;

	/**
	 * Gets instruction counts that best represent the likely usage of this material based on lighting model and other factors.
	 * @param Descriptions - an array of descriptions to be populated
	 * @param InstructionCounts - an array of instruction counts matching the Descriptions.  
	 *		The dimensions of these arrays are guaranteed to be identical and all values are valid.
	 */
	ENGINE_API void GetRepresentativeInstructionCounts(TArray<FString> &Descriptions, TArray<int32> &InstructionCounts) const;

	ENGINE_API SIZE_T GetResourceSizeInclusive();

	ENGINE_API virtual void LegacySerialize(FArchive& Ar);

	ENGINE_API virtual const TArray<UTexture*>& GetReferencedTextures() const OVERRIDE;
protected:
	UMaterial* Material;
	UMaterialInstance* MaterialInstance;

	/** Entry point for compiling a specific material property.  This must call SetMaterialProperty. */
	ENGINE_API virtual int32 CompileProperty(EMaterialProperty Property,EShaderFrequency InShaderFrequency,class FMaterialCompiler* Compiler) const;
	ENGINE_API virtual bool HasVertexPositionOffsetConnected() const;
	/** Useful for debugging. */
	ENGINE_API virtual FString GetBaseMaterialPathName() const;
};

/**
 * This class takes care of all of the details you need to worry about when modifying a UMaterial
 * on the main thread. This class should *always* be used when doing so!
 */
class FMaterialUpdateContext
{
	/** Materials updated within this context. */
	TSet<UMaterial*> UpdatedMaterials;
	/** Active global component reregister context, if any. */
	TScopedPointer<class FGlobalComponentReregisterContext> ComponentReregisterContext;
	/** The shader platform that was being processed - can control if we need to update components */
	EShaderPlatform ShaderPlatform;
	/** True if the SyncWithRenderingThread option was specified. */
	bool bSyncWithRenderingThread;

public:

	/** Options controlling what is done before/after the material is updated. */
	struct EOptions
	{
		enum Type
		{
			/** Reregister all components while updating the material. */
			ReregisterComponents = 0x1,
			/**
			 * Sync with the rendering thread. This is necessary when modifying a
			 * material exposed to the rendering thread. You may omit this flag if
			 * you have already flushed rendering commands.
			 */
			SyncWithRenderingThread = 0x2,
			/** Default options: reregister components, sync with rendering thread. */
			Default = ReregisterComponents | SyncWithRenderingThread,
		};
	};

	/** Initialization constructor. */
	explicit ENGINE_API FMaterialUpdateContext(uint32 Options = EOptions::Default, EShaderPlatform InShaderPlatform=GRHIShaderPlatform);

	/** Destructor. */
	ENGINE_API ~FMaterialUpdateContext();

	/** Add a material that has been updated to the context. */
	void AddMaterial(UMaterial* Material)
	{
		UpdatedMaterials.Add(Material);
	}
};

/**
 * Check whether the specified texture is needed to render the material instance.
 * @param Texture	The texture to check.
 * @return bool - true if the material uses the specified texture.
 */
ENGINE_API bool DoesMaterialUseTexture(const UMaterialInterface* Material,const UTexture* CheckTexture);

/**
 * @return Gets the index for a material property to its EMaterialProperty.
 */
ENGINE_API EMaterialProperty GetMaterialPropertyFromInputOutputIndex(int32 Index);

/**
 * @return Gets the material property for a particular I/O index into the material attributes nodes.
 */
ENGINE_API int32 GetInputOutputIndexFromMaterialProperty(EMaterialProperty Property);

/**
 * @return Gets the default value for a material property
 */
ENGINE_API void GetDefaultForMaterialProperty(EMaterialProperty Property, float& OutDefaultFloat, FColor& OutDefaultColor, FVector& OutDefaultVector);

/**
 * @return Gets the name of a property.
 */
ENGINE_API FString GetNameOfMaterialProperty(EMaterialProperty Property);

/** TODO - This can be removed whenever VER_UE4_MATERIAL_ATTRIBUTES_REORDERING is no longer relevant. */
ENGINE_API void DoMaterialAttributeReorder(FExpressionInput* Input);
