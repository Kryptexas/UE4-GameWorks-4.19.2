// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShaderParameters.h"
#include "Components.h"
#include "VertexFactory.h"

class FMaterial;
class FSceneView;
struct FMeshBatchElement;

/*=============================================================================
	LocalVertexFactory.h: Local vertex factory definitions.
=============================================================================*/

/**
 * A vertex factory which simply transforms explicit vertex attributes from local to world space.
 */
class ENGINE_API FLocalVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FLocalVertexFactory);
public:

	FLocalVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, const char* InDebugName, const FStaticMeshDataType* InStaticMeshDataType = nullptr)
		: FVertexFactory(InFeatureLevel)
		, ColorStreamIndex(-1)
		, DebugName(InDebugName)
	{
		StaticMeshDataType = InStaticMeshDataType ? InStaticMeshDataType : &Data;
		bSupportsManualVertexFetch = true;
	}

	struct FDataType : public FStaticMeshDataType
	{
	};

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	static bool ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_SPEEDTREE_WIND"),TEXT("1"));

		const bool ContainsManualVertexFetch = OutEnvironment.GetDefinitions().Contains("MANUAL_VERTEX_FETCH");
		if (!ContainsManualVertexFetch && RHISupportsManualVertexFetch(Platform))
		{
			OutEnvironment.SetDefine(TEXT("MANUAL_VERTEX_FETCH"), TEXT("1"));
		}
	}

	/**
	 * An implementation of the interface used by TSynchronizedResource to update the resource with new data from the game thread.
	 */
	void SetData(const FDataType& InData);

	/**
	* Copy the data from another vertex factory
	* @param Other - factory to copy from
	*/
	void Copy(const FLocalVertexFactory& Other);

	// FRenderResource interface.
	virtual void InitRHI() override;

	static bool SupportsTessellationShaders() { return true; }

	static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency);

	FORCEINLINE_DEBUGGABLE void SetColorOverrideStream(FRHICommandList& RHICmdList, const FVertexBuffer* ColorVertexBuffer) const
	{
		checkf(ColorVertexBuffer->IsInitialized(), TEXT("Color Vertex buffer was not initialized! Name %s"), *ColorVertexBuffer->GetFriendlyName());
		checkf(IsInitialized() && EnumHasAnyFlags(EVertexStreamUsage::Overridden, Data.ColorComponent.VertexStreamUsage) && ColorStreamIndex > 0, TEXT("Per-mesh colors with bad stream setup! Name %s"), *ColorVertexBuffer->GetFriendlyName());
		RHICmdList.SetStreamSource(ColorStreamIndex, ColorVertexBuffer->VertexBufferRHI, 0);
	}

	inline const FShaderResourceViewRHIParamRef GetPositionsSRV() const
	{
		return StaticMeshDataType->PositionComponentSRV;
	}

	inline const FShaderResourceViewRHIParamRef GetTangentsSRV() const
	{
		return StaticMeshDataType->TangentsSRV;
	}

	inline const FShaderResourceViewRHIParamRef GetTextureCoordinatesSRV() const
	{
		return StaticMeshDataType->TextureCoordinatesSRV;
	}

	inline const FShaderResourceViewRHIParamRef GetColorComponentsSRV() const
	{
		return StaticMeshDataType->ColorComponentsSRV;
	}

	inline const uint32 GetColorIndexMask() const
	{
		return StaticMeshDataType->ColorIndexMask;
	}

	inline const int GetLightMapCoordinateIndex() const
	{
		return StaticMeshDataType->LightMapCoordinateIndex;
	}

	inline const int GetNumTexcoords() const
	{
		return StaticMeshDataType->NumTexCoords;
	}

protected:
	const FDataType& GetData() const { return Data; }

	FDataType Data;
	const FStaticMeshDataType* StaticMeshDataType;

	int32 ColorStreamIndex;

	struct FDebugName
	{
		FDebugName(const char* InDebugName)
#if !UE_BUILD_SHIPPING
			: DebugName(InDebugName)
#endif
		{}
	private:
#if !UE_BUILD_SHIPPING
		const char* DebugName;
#endif
	} DebugName;
};

/**
 * Shader parameters for LocalVertexFactory.
 */
class FLocalVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	virtual void Bind(const FShaderParameterMap& ParameterMap) override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void SetMesh(FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const override;

	FLocalVertexFactoryShaderParameters()
		: bAnySpeedTreeParamIsBound(false)
	{
	}

	// SpeedTree LOD parameter
	FShaderParameter LODParameter;

	//Parameters to manually load TexCoords
	FShaderParameter VertexFetch_VertexFetchParameters;
	FShaderResourceParameter VertexFetch_PositionBufferParameter;
	FShaderResourceParameter VertexFetch_TexCoordBufferParameter;
	FShaderResourceParameter VertexFetch_PackedTangentsBufferParameter;
	FShaderResourceParameter VertexFetch_ColorComponentsBufferParameter;

	// True if LODParameter is bound, which puts us on the slow path in SetMesh
	bool bAnySpeedTreeParamIsBound;
};
