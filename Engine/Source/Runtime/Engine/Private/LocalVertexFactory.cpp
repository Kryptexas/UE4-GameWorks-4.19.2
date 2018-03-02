// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LocalVertexFactory.cpp: Local vertex factory implementation
=============================================================================*/

#include "LocalVertexFactory.h"
#include "SceneView.h"
#include "MeshBatch.h"
#include "SpeedTreeWind.h"
#include "ShaderParameterUtils.h"
#include "Rendering/ColorVertexBuffer.h"

class FSpeedTreeWindNullUniformBuffer : public TUniformBuffer<FSpeedTreeUniformParameters>
{
	typedef TUniformBuffer< FSpeedTreeUniformParameters > Super;
public:
	virtual void InitDynamicRHI() override;
};

void FSpeedTreeWindNullUniformBuffer::InitDynamicRHI()
{
	FSpeedTreeUniformParameters Parameters;
	FMemory::Memzero(Parameters);
	SetContentsNoUpdate(Parameters);
	
	Super::InitDynamicRHI();
}

static TGlobalResource< FSpeedTreeWindNullUniformBuffer > GSpeedTreeWindNullUniformBuffer;

void FLocalVertexFactoryShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	LODParameter.Bind(ParameterMap, TEXT("SpeedTreeLODInfo"));
	bAnySpeedTreeParamIsBound = LODParameter.IsBound() || ParameterMap.ContainsParameterAllocation(TEXT("SpeedTreeData"));

	VertexFetch_VertexFetchParameters.Bind(ParameterMap, TEXT("VertexFetch_Parameters"));
	VertexFetch_PositionBufferParameter.Bind(ParameterMap, TEXT("VertexFetch_PositionBuffer"));
	VertexFetch_TexCoordBufferParameter.Bind(ParameterMap, TEXT("VertexFetch_TexCoordBuffer"));
	VertexFetch_PackedTangentsBufferParameter.Bind(ParameterMap, TEXT("VertexFetch_PackedTangentsBuffer"));
	VertexFetch_ColorComponentsBufferParameter.Bind(ParameterMap, TEXT("VertexFetch_ColorComponentsBuffer"));
}

void FLocalVertexFactoryShaderParameters::Serialize(FArchive& Ar)
{
	Ar << bAnySpeedTreeParamIsBound;
	Ar << LODParameter;
	Ar << VertexFetch_VertexFetchParameters;
	Ar << VertexFetch_PositionBufferParameter;
	Ar << VertexFetch_TexCoordBufferParameter;
	Ar << VertexFetch_PackedTangentsBufferParameter;
	Ar << VertexFetch_ColorComponentsBufferParameter;
}

void FLocalVertexFactoryShaderParameters::SetMesh(FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const
{
	const auto* LocalVertexFactory = static_cast<const FLocalVertexFactory*>(VertexFactory);
	
	FVertexShaderRHIParamRef VS = Shader->GetVertexShader();
	if (LocalVertexFactory->SupportsManualVertexFetch(View.GetShaderPlatform()))
	{
		SetSRVParameter(RHICmdList, VS, VertexFetch_PositionBufferParameter, LocalVertexFactory->GetPositionsSRV());
		SetSRVParameter(RHICmdList, VS, VertexFetch_PackedTangentsBufferParameter, LocalVertexFactory->GetTangentsSRV());
		SetSRVParameter(RHICmdList, VS, VertexFetch_TexCoordBufferParameter, LocalVertexFactory->GetTextureCoordinatesSRV());

		int ColorIndexMask = 0;
		if (BatchElement.bUserDataIsColorVertexBuffer)
		{
			FColorVertexBuffer* OverrideColorVertexBuffer = (FColorVertexBuffer*)BatchElement.UserData;
			check(OverrideColorVertexBuffer);

			SetSRVParameter(RHICmdList, VS, VertexFetch_ColorComponentsBufferParameter, OverrideColorVertexBuffer->GetColorComponentsSRV());
			ColorIndexMask = OverrideColorVertexBuffer->GetNumVertices() > 1 ? ~0 : 0;
		}
		else
		{
			SetSRVParameter(RHICmdList, VS, VertexFetch_ColorComponentsBufferParameter, LocalVertexFactory->GetColorComponentsSRV());
			ColorIndexMask = (int)LocalVertexFactory->GetColorIndexMask();
		}

		const int NumTexCoords = LocalVertexFactory->GetNumTexcoords();
		const int LightMapCoordinateIndex = LocalVertexFactory->GetLightMapCoordinateIndex();
		FIntVector Parameters = { ColorIndexMask, NumTexCoords, LightMapCoordinateIndex };
		SetShaderValue(RHICmdList, VS, VertexFetch_VertexFetchParameters, Parameters);
	}

	if (BatchElement.bUserDataIsColorVertexBuffer)
	{
		FColorVertexBuffer* OverrideColorVertexBuffer = (FColorVertexBuffer*)BatchElement.UserData;
		check(OverrideColorVertexBuffer);

		if (!LocalVertexFactory->SupportsManualVertexFetch(View.GetShaderPlatform()))
		{
			LocalVertexFactory->SetColorOverrideStream(RHICmdList, OverrideColorVertexBuffer);
		}	
	}

	if (bAnySpeedTreeParamIsBound && View.Family != NULL && View.Family->Scene != NULL)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FLocalVertexFactoryShaderParameters_SetMesh_SpeedTree);
		FUniformBufferRHIParamRef SpeedTreeUniformBuffer = View.Family->Scene->GetSpeedTreeUniformBuffer(VertexFactory);
		if (SpeedTreeUniformBuffer == NULL)
		{
			SpeedTreeUniformBuffer = GSpeedTreeWindNullUniformBuffer.GetUniformBufferRHI();
		}
		check(SpeedTreeUniformBuffer != NULL);

		SetUniformBufferParameter(RHICmdList, VS, Shader->GetUniformBufferParameter<FSpeedTreeUniformParameters>(), SpeedTreeUniformBuffer);

		if (LODParameter.IsBound())
		{
			FVector LODData(BatchElement.MinScreenSize, BatchElement.MaxScreenSize, BatchElement.MaxScreenSize - BatchElement.MinScreenSize);
			SetShaderValue(RHICmdList, VS, LODParameter, LODData);
		}
	}
}

/**
 * Should we cache the material's shadertype on this platform with this vertex factory? 
 */
bool FLocalVertexFactory::ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	return true; 
}

void FLocalVertexFactory::SetData(const FDataType& InData)
{
	check(IsInRenderingThread());

	{
		//const int NumTexCoords = InData.NumTexCoords;
		//const int LightMapCoordinateIndex = InData.LightMapCoordinateIndex;
		//check(NumTexCoords > 0);
		//check(LightMapCoordinateIndex < NumTexCoords && LightMapCoordinateIndex >= 0);
		//check(InData.PositionComponentSRV);
		//check(InData.TangentsSRV);
		//check(InData.TextureCoordinatesSRV);
		//check(InData.ColorComponentsSRV);
	}

	// The shader code makes assumptions that the color component is a FColor, performing swizzles on ES2 and Metal platforms as necessary
	// If the color is sent down as anything other than VET_Color then you'll get an undesired swizzle on those platforms
	check((InData.ColorComponent.Type == VET_None) || (InData.ColorComponent.Type == VET_Color));

	Data = InData;
	UpdateRHI();
}

/**
* Copy the data from another vertex factory
* @param Other - factory to copy from
*/
void FLocalVertexFactory::Copy(const FLocalVertexFactory& Other)
{
	FLocalVertexFactory* VertexFactory = this;
	const FDataType* DataCopy = &Other.Data;
	ENQUEUE_RENDER_COMMAND(FLocalVertexFactoryCopyData)(
		[VertexFactory, DataCopy](FRHICommandListImmediate& RHICmdList)
		{
			VertexFactory->Data = *DataCopy;
		});
	BeginUpdateResourceRHI(this);
}

void FLocalVertexFactory::InitRHI()
{
	// If the vertex buffer containing position is not the same vertex buffer containing the rest of the data,
	// then initialize PositionStream and PositionDeclaration.
	if(Data.PositionComponent.VertexBuffer != Data.TangentBasisComponents[0].VertexBuffer)
	{
		FVertexDeclarationElementList PositionOnlyStreamElements;
		PositionOnlyStreamElements.Add(AccessPositionStreamComponent(Data.PositionComponent,0));
		InitPositionDeclaration(PositionOnlyStreamElements);
	}

	FVertexDeclarationElementList Elements;
	if(Data.PositionComponent.VertexBuffer != NULL)
	{
		Elements.Add(AccessStreamComponent(Data.PositionComponent,0));
	}

	// only tangent,normal are used by the stream. the binormal is derived in the shader
	uint8 TangentBasisAttributes[2] = { 1, 2 };
	for(int32 AxisIndex = 0;AxisIndex < 2;AxisIndex++)
	{
		if(Data.TangentBasisComponents[AxisIndex].VertexBuffer != NULL)
		{
			Elements.Add(AccessStreamComponent(Data.TangentBasisComponents[AxisIndex],TangentBasisAttributes[AxisIndex]));
		}
	}

	if (Data.ColorComponentsSRV == nullptr)
	{
		Data.ColorComponentsSRV = GNullColorVertexBuffer.VertexBufferSRV;
		Data.ColorIndexMask = 0;
	}

	ColorStreamIndex = -1;
	if(Data.ColorComponent.VertexBuffer)
	{
		Elements.Add(AccessStreamComponent(Data.ColorComponent,3));
		ColorStreamIndex = Elements.Last().StreamIndex;
	}
	else
	{
		//If the mesh has no color component, set the null color buffer on a new stream with a stride of 0.
		//This wastes 4 bytes of bandwidth per vertex, but prevents having to compile out twice the number of vertex factories.
		FVertexStreamComponent NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color, EVertexStreamUsage::ManualFetch);
		Elements.Add(AccessStreamComponent(NullColorComponent, 3));
		ColorStreamIndex = Elements.Last().StreamIndex;
	}

	if(Data.TextureCoordinates.Num())
	{
		const int32 BaseTexCoordAttribute = 4;
		for(int32 CoordinateIndex = 0;CoordinateIndex < Data.TextureCoordinates.Num();CoordinateIndex++)
		{
			Elements.Add(AccessStreamComponent(
				Data.TextureCoordinates[CoordinateIndex],
				BaseTexCoordAttribute + CoordinateIndex
				));
		}

		for (int32 CoordinateIndex = Data.TextureCoordinates.Num(); CoordinateIndex < MAX_STATIC_TEXCOORDS / 2; CoordinateIndex++)
		{
			Elements.Add(AccessStreamComponent(
				Data.TextureCoordinates[Data.TextureCoordinates.Num() - 1],
				BaseTexCoordAttribute + CoordinateIndex
				));
		}
	}

	if(Data.LightMapCoordinateComponent.VertexBuffer)
	{
		Elements.Add(AccessStreamComponent(Data.LightMapCoordinateComponent,15));
	}
	else if(Data.TextureCoordinates.Num())
	{
		Elements.Add(AccessStreamComponent(Data.TextureCoordinates[0],15));
	}

	check(Streams.Num() > 0);

	InitDeclaration(Elements);

	check(IsValidRef(GetDeclaration()));
}

FVertexFactoryShaderParameters* FLocalVertexFactory::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	if (ShaderFrequency == SF_Vertex)
	{
		return new FLocalVertexFactoryShaderParameters();
	}

	return NULL;
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FLocalVertexFactory,"/Engine/Private/LocalVertexFactory.ush",true,true,true,true,true);
