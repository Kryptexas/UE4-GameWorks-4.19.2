// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
LandscapeRenderMobile.cpp: Landscape Rendering without using vertex texture fetch
=============================================================================*/

#include "EnginePrivate.h"
#include "ShaderParameters.h"
#include "EngineTerrainClasses.h"
#include "Landscape/LandscapeRender.h"
#include "Landscape/LandscapeRenderMobile.h"

void FLandscapeVertexFactoryMobile::InitRHI()
{
	// list of declaration items
	FVertexDeclarationElementList Elements;

	// position decls
	Elements.Add(AccessStreamComponent(MobileData.PositionComponent,0));

	if (MobileData.LODHeightsComponent.Num())
	{
		const int32 BaseAttribute = 1;
		for(int32 Index = 0;Index < MobileData.LODHeightsComponent.Num();Index++)
		{
			Elements.Add(AccessStreamComponent(MobileData.LODHeightsComponent[Index], BaseAttribute + Index));
		}
	}

	// create the actual device decls
	InitDeclaration(Elements,FVertexFactory::DataType());
}

/** Shader parameters for use with FLandscapeVertexFactory */
class FLandscapeVertexFactoryMobileVertexShaderParameters : public FVertexFactoryShaderParameters
{
public:
	/**
	* Bind shader constants by name
	* @param	ParameterMap - mapping of named shader constants to indices
	*/
	virtual void Bind(const FShaderParameterMap& ParameterMap) OVERRIDE
	{
		LodValuesParameter.Bind(ParameterMap,TEXT("LodValues"));
		NeighborSectionLodParameter.Bind(ParameterMap,TEXT("NeighborSectionLod"));
		LodBiasParameter.Bind(ParameterMap,TEXT("LodBias"));
		SectionLodsParameter.Bind(ParameterMap,TEXT("SectionLods"));
	}

	/**
	* Serialize shader params to an archive
	* @param	Ar - archive to serialize to
	*/
	virtual void Serialize(FArchive& Ar) OVERRIDE
	{
		Ar << LodValuesParameter;
		Ar << NeighborSectionLodParameter;
		Ar << LodBiasParameter;
		Ar << SectionLodsParameter;
	}
	/**
	* Set any shader data specific to this vertex factory
	*/
	virtual void SetMesh(FShader* VertexShader,const class FVertexFactory* VertexFactory,const class FSceneView& View,const struct FMeshBatchElement& BatchElement,uint32 DataFlags) const OVERRIDE
	{
		SCOPE_CYCLE_COUNTER(STAT_LandscapeVFDrawTime);

		FLandscapeBatchElementParams* BatchElementParams = (FLandscapeBatchElementParams*)BatchElement.UserData;
		check(BatchElementParams);

		const FLandscapeComponentSceneProxy* SceneProxy = BatchElementParams->SceneProxy;
		SetUniformBufferParameter(VertexShader->GetVertexShader(),VertexShader->GetUniformBufferParameter<FLandscapeUniformShaderParameters>(),*BatchElementParams->LandscapeUniformShaderParametersResource);

		FVector CameraLocalPos3D = SceneProxy->WorldToLocal.TransformPosition(View.ViewMatrices.ViewOrigin); 
		FVector2D CameraLocalPos = FVector2D(CameraLocalPos3D.X, CameraLocalPos3D.Y);

		if( LodBiasParameter.IsBound() )
		{
			FVector4 LodBias(
				SceneProxy->LODDistanceFactor,
				1.f / ( 1.f - SceneProxy->LODDistanceFactor ),
				CameraLocalPos3D.X + SceneProxy->SectionBase.X,
				CameraLocalPos3D.Y + SceneProxy->SectionBase.Y 
				);
			SetShaderValue(VertexShader->GetVertexShader(), LodBiasParameter, LodBias);
		}

		// Calculate LOD params
		FVector4 fCurrentLODs;
		FVector4 CurrentNeighborLODs[4];

		if( BatchElementParams->SubX == -1 )
		{
			for( int32 SubY = 0; SubY < SceneProxy->NumSubsections; SubY++ )
			{
				for( int32 SubX = 0; SubX < SceneProxy->NumSubsections; SubX++ )
				{
					int32 SubIndex = SubX + 2 * SubY;
					SceneProxy->CalcLODParamsForSubsection(View, CameraLocalPos, SubX, SubY, fCurrentLODs[SubIndex], CurrentNeighborLODs[SubIndex]);
				}
			}
		}
		else
		{
			int32 SubIndex = BatchElementParams->SubX + 2 * BatchElementParams->SubY;
			SceneProxy->CalcLODParamsForSubsection(View, CameraLocalPos, BatchElementParams->SubX, BatchElementParams->SubY, fCurrentLODs[SubIndex], CurrentNeighborLODs[SubIndex]);
		}

		if( SectionLodsParameter.IsBound() )
		{
			SetShaderValue(VertexShader->GetVertexShader(), SectionLodsParameter, fCurrentLODs);
		}

		if( NeighborSectionLodParameter.IsBound() )
		{
			SetShaderValue(VertexShader->GetVertexShader(), NeighborSectionLodParameter, CurrentNeighborLODs);
		}

		if( LodValuesParameter.IsBound() )
		{
			FVector4 LodValues(
				0,
				// convert current LOD coordinates into highest LOD coordinates
				(float)SceneProxy->SubsectionSizeQuads / (float)(((SceneProxy->SubsectionSizeVerts) >> BatchElementParams->CurrentLOD)-1),
				(float)SceneProxy->SubsectionSizeQuads, 
				1.f / (float)SceneProxy->SubsectionSizeQuads );

			SetShaderValue(VertexShader->GetVertexShader(),LodValuesParameter,LodValues);
		}
	}
protected:
	FShaderParameter LodValuesParameter;
	FShaderParameter NeighborSectionLodParameter;
	FShaderParameter LodBiasParameter;
	FShaderParameter SectionLodsParameter;
	TShaderUniformBufferParameter<FLandscapeUniformShaderParameters> LandscapeShaderParameters;
};

FVertexFactoryShaderParameters* FLandscapeVertexFactoryMobile::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	switch( ShaderFrequency )
	{
	case SF_Vertex:
		return new FLandscapeVertexFactoryMobileVertexShaderParameters();
	case SF_Pixel:
		return new FLandscapeVertexFactoryPixelShaderParameters();
	default:
		return NULL;
	}
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FLandscapeVertexFactoryMobile, "LandscapeVertexFactory", true, true, true, false, false);

/** 
* Initialize the RHI for this rendering resource 
*/
void FLandscapeVertexBufferMobile::InitRHI()
{
	// create a static vertex buffer
	VertexBufferRHI = RHICreateVertexBuffer(DataSize, NULL, BUF_Static);
	void* VertexData = RHILockVertexBuffer(VertexBufferRHI, 0, DataSize, RLM_WriteOnly);
	// Copy stored platform data
	FMemory::Memcpy(VertexData, (uint8*)Data, DataSize);
	RHIUnlockVertexBuffer(VertexBufferRHI);
}

FLandscapeComponentSceneProxyMobile::FLandscapeComponentSceneProxyMobile(ULandscapeComponent* InComponent, FLandscapeEditToolRenderData* InEditToolRenderData)
	:	FLandscapeComponentSceneProxy(InComponent, InEditToolRenderData)
{
		check(InComponent && InComponent->PlatformData.HasValidPlatformData());
		InComponent->PlatformData.GetUncompressedData(PlatformData);
#if WITH_EDITOR
	if (InComponent->HeightmapTexture != NULL)
	{
		MaterialInterface = InComponent->GeneratePlatformPixelData(WeightmapTextures, false);
		InComponent->MobileMaterialInterface = MaterialInterface;
		InComponent->MobileNormalmapTexture = WeightmapTextures[0];
	}
#endif
	NormalmapTexture = WeightmapTextures[0]; // Use Weightmap0 as Normal Texture
}

FLandscapeComponentSceneProxyMobile::~FLandscapeComponentSceneProxyMobile()
{
	if (VertexFactory)
	{
		delete VertexFactory;
		VertexFactory = NULL;
	}
}

void FLandscapeComponentSceneProxyMobile::CreateRenderThreadResources()
{
	// Use only Index buffers
	SharedBuffers = FLandscapeComponentSceneProxy::SharedBuffersMap.FindRef(SharedBuffersKey);
	if( SharedBuffers == NULL )
	{
		SharedBuffers = new FLandscapeSharedBuffers(SharedBuffersKey, SubsectionSizeQuads, NumSubsections);
		FLandscapeComponentSceneProxy::SharedBuffersMap.Add(SharedBuffersKey, SharedBuffers);
	}

	SharedBuffers->AddRef();

	int32 VertexBufferSize = PlatformData.Num();
	// Copy platform data into vertex buffer
	VertexBuffer = new FLandscapeVertexBufferMobile(PlatformData.GetTypedData(), VertexBufferSize);

	FLandscapeVertexFactoryMobile* LandscapeVertexFactory = new FLandscapeVertexFactoryMobile();
	LandscapeVertexFactory->MobileData.PositionComponent = FVertexStreamComponent(VertexBuffer, STRUCT_OFFSET(FLandscapeMobileVertex,Position), sizeof(FLandscapeMobileVertex), VET_UByte4N);
	for( uint32 Index = 0; Index < LANDSCAPE_MAX_ES_LOD_COMP; ++Index )
	{
		LandscapeVertexFactory->MobileData.LODHeightsComponent.Add
			(FVertexStreamComponent(VertexBuffer, STRUCT_OFFSET(FLandscapeMobileVertex,LODHeights) + sizeof(uint8) * 4 * Index, sizeof(FLandscapeMobileVertex), VET_UByte4N));
	}

	LandscapeVertexFactory->InitResource();
	VertexFactory = LandscapeVertexFactory;
	DynamicMesh.VertexFactory = VertexFactory;
#if WITH_EDITOR
	DynamicMeshTools.VertexFactory = VertexFactory;
#endif

	// Assign LandscapeUniformShaderParameters
	LandscapeUniformShaderParameters.InitResource();

	for( int32 ElementIdx=0;ElementIdx<DynamicMesh.Elements.Num();ElementIdx++ )
	{
		FMeshBatchElement& BatchElement = DynamicMesh.Elements[ElementIdx];
		FLandscapeBatchElementParams* BatchElementParams = (FLandscapeBatchElementParams*)BatchElement.UserData;
		BatchElementParams->LandscapeUniformShaderParametersResource = &LandscapeUniformShaderParameters;
	}

	PlatformData.Empty();
}

