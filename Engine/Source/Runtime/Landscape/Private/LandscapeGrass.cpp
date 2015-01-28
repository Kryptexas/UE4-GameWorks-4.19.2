// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Landscape.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "UnrealEngine.h"
#include "EngineUtils.h"
#include "Shader.h"
#include "ShaderParameterUtils.h"
#include "MeshMaterialShader.h"
#include "EngineModule.h"
#include "RendererInterface.h"
#include "DrawingPolicy.h"
#include "GenericOctreePublic.h"
#include "PrimitiveSceneInfo.h"
#include "MaterialCompiler.h"
#include "Materials/MaterialExpressionLandscapeGrassOutput.h"
#include "EdGraph/EdGraphNode.h"
#include "Engine/TextureRenderTarget2D.h"

#define LOCTEXT_NAMESPACE "Landscape"

class FLandscapeGrassWeightVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FLandscapeGrassWeightVS, MeshMaterial);

	FShaderParameter RenderOffsetParameter;

protected:

	FLandscapeGrassWeightVS()
	{}

	FLandscapeGrassWeightVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer)
	: FMeshMaterialShader(Initializer)
	{
		RenderOffsetParameter.Bind(Initializer.ParameterMap, TEXT("RenderOffset"));
	}

public:

	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		// Only compile for LandscapeVertexFactory
		return (Material->IsUsedWithLandscape() || Material->IsSpecialEngineMaterial()) && IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4) && (VertexFactoryType == FindVertexFactoryType(FName(TEXT("FLandscapeVertexFactory"), FNAME_Find))) && !IsConsolePlatform(Platform);
	}

	void SetParameters(FRHICommandList& RHICmdList, const FMaterialRenderProxy* MaterialRenderProxy, const FMaterial& MaterialResource, const FSceneView& View, const FVector2D& RenderOffset)
	{
		FMeshMaterialShader::SetParameters(RHICmdList, GetVertexShader(), MaterialRenderProxy, MaterialResource, View, ESceneRenderTargetsMode::DontSet);
		SetShaderValue(RHICmdList, GetVertexShader(), RenderOffsetParameter, RenderOffset);
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(RHICmdList, GetVertexShader(), VertexFactory, View, Proxy, BatchElement);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		Ar << RenderOffsetParameter;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FLandscapeGrassWeightVS, TEXT("LandscapeGrassWeight"), TEXT("VSMain"), SF_Vertex);


class FLandscapeGrassWeightPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FLandscapeGrassWeightPS, MeshMaterial);
	FShaderParameter OutputPassParameter;
public:

	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		// Only compile for LandscapeVertexFactory
		return (Material->IsUsedWithLandscape() || Material->IsSpecialEngineMaterial()) && IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4) && (VertexFactoryType == FindVertexFactoryType(FName(TEXT("FLandscapeVertexFactory"), FNAME_Find))) && !IsConsolePlatform(Platform);
	}

	FLandscapeGrassWeightPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	: FMeshMaterialShader(Initializer)
	{
		OutputPassParameter.Bind(Initializer.ParameterMap, TEXT("OutputPass"));
	}

	FLandscapeGrassWeightPS()
	{}

	void SetParameters(FRHICommandList& RHICmdList, const FMaterialRenderProxy* MaterialRenderProxy, const FMaterial& MaterialResource, const FSceneView* View, int32 OutputPass)
	{
		FMeshMaterialShader::SetParameters(RHICmdList, GetPixelShader(), MaterialRenderProxy, MaterialResource, *View, ESceneRenderTargetsMode::DontSet);
		if (OutputPassParameter.IsBound())
		{
			SetShaderValue(RHICmdList, GetPixelShader(), OutputPassParameter, OutputPass);
		}
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(RHICmdList, GetPixelShader(), VertexFactory, View, Proxy, BatchElement);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		Ar << OutputPassParameter;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FLandscapeGrassWeightPS, TEXT("LandscapeGrassWeight"), TEXT("PSMain"), SF_Pixel);



/**
* Used to write out depth for opaque and masked materials during the depth-only pass.
*/
class FLandscapeGrassWeightDrawingPolicy : public FMeshDrawingPolicy
{
public:
	FLandscapeGrassWeightDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& InMaterialResource
		)
		:
		FMeshDrawingPolicy(InVertexFactory, InMaterialRenderProxy, InMaterialResource, false, false)
	{
		PixelShader = InMaterialResource.GetShader<FLandscapeGrassWeightPS>(InVertexFactory->GetType());
		VertexShader = InMaterialResource.GetShader<FLandscapeGrassWeightVS>(VertexFactory->GetType());
	}

	// FMeshDrawingPolicy interface.
	bool Matches(const FLandscapeGrassWeightDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other)
			&& VertexShader == Other.VertexShader
			&& PixelShader == Other.PixelShader;
	}

	void SetSharedState(FRHICommandList& RHICmdList, const FSceneView* View, const ContextDataType PolicyContext, int32 OutputPass, const FVector2D& RenderOffset) const
	{
		// Set the shader parameters for the material.
		VertexShader->SetParameters(RHICmdList, MaterialRenderProxy, *MaterialResource, *View, RenderOffset);
		PixelShader->SetParameters(RHICmdList, MaterialRenderProxy, *MaterialResource, View, OutputPass);

		// Set the shared mesh resources.
		FMeshDrawingPolicy::SetSharedState(RHICmdList, View, PolicyContext);
	}

	FBoundShaderStateInput GetBoundShaderStateInput(ERHIFeatureLevel::Type InFeatureLevel)
	{
		return FBoundShaderStateInput(
			FMeshDrawingPolicy::GetVertexDeclaration(),
			VertexShader->GetVertexShader(),
			FHullShaderRHIRef(),
			FDomainShaderRHIRef(),
			PixelShader->GetPixelShader(),
			NULL);
	}

	void SetMeshRenderState(
		FRHICommandList& RHICmdList,
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		bool bBackFace,
		const ElementDataType& ElementData,
		const ContextDataType PolicyContext
		) const
	{
		const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];
		VertexShader->SetMesh(RHICmdList, VertexFactory, View, PrimitiveSceneProxy, BatchElement);
		PixelShader->SetMesh(RHICmdList, VertexFactory, View, PrimitiveSceneProxy, BatchElement);
		FMeshDrawingPolicy::SetMeshRenderState(RHICmdList, View, PrimitiveSceneProxy, Mesh, BatchElementIndex, bBackFace, ElementData, PolicyContext);
	}

	friend int32 CompareDrawingPolicy(const FLandscapeGrassWeightDrawingPolicy& A, const FLandscapeGrassWeightDrawingPolicy& B)
	{
		COMPAREDRAWINGPOLICYMEMBERS(VertexShader);
		COMPAREDRAWINGPOLICYMEMBERS(PixelShader);
		COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
		COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
		COMPAREDRAWINGPOLICYMEMBERS(bIsTwoSidedMaterial);
		return 0;
	}
private:
	FLandscapeGrassWeightVS* VertexShader;
	FLandscapeGrassWeightPS* PixelShader;
};

// data also accessible by render thread
class FLandscapeGrassWeightExporter_RenderThread
{
	FLandscapeGrassWeightExporter_RenderThread(int32 InNumPasses)
	: RenderTargetResource(nullptr)
	, NumPasses(InNumPasses)
	{}

	friend class FLandscapeGrassWeightExporter;

public:
	virtual ~FLandscapeGrassWeightExporter_RenderThread()
	{}

	struct FComponentInfo
	{
		ULandscapeComponent* Component;
		FVector2D ViewOffset;
		int32 PixelOffsetX;
		FPrimitiveSceneInfo* PrimitiveSceneInfo;

		FComponentInfo(ULandscapeComponent* InComponent, FVector2D& InViewOffset, int32 InPixelOffsetX)
			: Component(InComponent)
			, ViewOffset(InViewOffset)
			, PixelOffsetX(InPixelOffsetX)
			, PrimitiveSceneInfo(InComponent->SceneProxy->GetPrimitiveSceneInfo())
		{}
	};

	FTextureRenderTarget2DResource* RenderTargetResource;
	TArray<FComponentInfo> ComponentInfos;
	FIntPoint TargetSize;
	int32 NumPasses;
	int32 PassOffsetX;
	FMatrix ViewMatrix;
	FMatrix ProjectionMatrix;

	void RenderLandscapeComponentToTexture_RenderThread(FRHICommandListImmediate& RHICmdList)
	{
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(RenderTargetResource, NULL, FEngineShowFlags(ESFIM_Game)).SetWorldTimes(FApp::GetCurrentTime() - GStartTime, FApp::GetDeltaTime(), FApp::GetCurrentTime() - GStartTime));

#if WITH_EDITOR
		ViewFamily.LandscapeLODOverride = 0; 		// Force LOD render
#endif

		FSceneViewInitOptions ViewInitOptions;
		ViewInitOptions.SetViewRectangle(FIntRect(0, 0, TargetSize.X, TargetSize.Y));
		ViewInitOptions.ViewMatrix = ViewMatrix;
		ViewInitOptions.ProjectionMatrix = ProjectionMatrix;
		ViewInitOptions.ViewFamily = &ViewFamily;

		GetRendererModule().CreateAndInitSingleView(RHICmdList, &ViewFamily, &ViewInitOptions);
		
		const FSceneView* View = ViewFamily.Views[0];
		RHICmdList.SetViewport(View->ViewRect.Min.X, View->ViewRect.Min.Y, 0.0f, View->ViewRect.Max.X, View->ViewRect.Max.Y, 1.0f);
		RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
		RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
		RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
		RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

		for (auto& ComponentInfo : ComponentInfos)
		{
			FPrimitiveSceneInfo* PrimitiveSceneInfo = ComponentInfo.PrimitiveSceneInfo;
			for (int32 PassIdx = 0; PassIdx < NumPasses; PassIdx++)
			{
				for (int32 StaticMeshIdx = 0; StaticMeshIdx < PrimitiveSceneInfo->StaticMeshes.Num(); StaticMeshIdx++)
				{
					FMeshBatch& Mesh = *(FMeshBatch*)(&PrimitiveSceneInfo->StaticMeshes[StaticMeshIdx]);

					FLandscapeGrassWeightDrawingPolicy DrawingPolicy(Mesh.VertexFactory, Mesh.MaterialRenderProxy, *Mesh.MaterialRenderProxy->GetMaterial(GMaxRHIFeatureLevel));
					RHICmdList.BuildAndSetLocalBoundShaderState(DrawingPolicy.GetBoundShaderStateInput(GMaxRHIFeatureLevel));
					DrawingPolicy.SetSharedState(RHICmdList, View, FLandscapeGrassWeightDrawingPolicy::ContextDataType(), PassIdx, ComponentInfo.ViewOffset + FVector2D(PassOffsetX * PassIdx, 0));

					// The first batch element contains the grass batch for the entire component
					DrawingPolicy.SetMeshRenderState(RHICmdList, *View, PrimitiveSceneInfo->Proxy, Mesh, 0, false, FMeshDrawingPolicy::ElementDataType(), FLandscapeGrassWeightDrawingPolicy::ContextDataType());
					DrawingPolicy.DrawMesh(RHICmdList, Mesh, 0);
				}
			}
		}
	}
};



class FLandscapeGrassWeightExporter : public FLandscapeGrassWeightExporter_RenderThread
{
	ALandscapeProxy* LandscapeProxy;
	int32 ComponentSizeQuads;
	int32 ComponentSizeVerts;
	TArray<ULandscapeGrassType*> GrassTypes;
	UTextureRenderTarget2D* RenderTargetTexture;

public:

	FLandscapeGrassWeightExporter(ALandscapeProxy* InLandscapeProxy, const TArray<ULandscapeComponent*>& InLandscapeComponents, const TArray<ULandscapeGrassType*> InGrassTypes) 
	:	FLandscapeGrassWeightExporter_RenderThread((InGrassTypes.Num() + 5) / 4)
	,	LandscapeProxy(InLandscapeProxy)
	,	ComponentSizeQuads(InLandscapeProxy->ComponentSizeQuads)
	,	ComponentSizeVerts(ComponentSizeQuads + 1)
	,	GrassTypes(InGrassTypes)
	,	RenderTargetTexture(nullptr)
	{
		check(InLandscapeComponents.Num());

		TargetSize = FIntPoint(ComponentSizeVerts * NumPasses * InLandscapeComponents.Num(), ComponentSizeVerts);
		FIntPoint TargetSizeMinusOne(TargetSize - FIntPoint(1,1));
		PassOffsetX = 2.0f * ComponentSizeVerts / TargetSize.X;

		for (int32 Idx = 0; Idx<InLandscapeComponents.Num();Idx++)
		{
			ULandscapeComponent* Component = InLandscapeComponents[Idx];

			FIntPoint ComponentOffset = (Component->GetSectionBase() - LandscapeProxy->LandscapeSectionOffset);
			int32 PixelOffsetX = Idx * NumPasses * ComponentSizeVerts;

			FVector2D ViewOffset(-ComponentOffset.X,ComponentOffset.Y);
			ViewOffset.X += PixelOffsetX;
			ViewOffset /= (FVector2D(TargetSize) * 0.5f);

			new(ComponentInfos)FComponentInfo(Component, ViewOffset, PixelOffsetX);
		}
		
		// center of target area in world
		FVector TargetCenter = LandscapeProxy->GetTransform().TransformPosition(FVector(TargetSizeMinusOne, 0.f)*0.5f);

		// extent of target in world space
		FVector TargetExtent = FVector(TargetSize, 0.f)*LandscapeProxy->GetActorScale()*0.5f;

		ViewMatrix = FTranslationMatrix(-TargetCenter);
		ViewMatrix *= FInverseRotationMatrix(LandscapeProxy->GetActorRotation());
		ViewMatrix *= FMatrix(FPlane(1, 0, 0, 0),
			FPlane(0, -1, 0, 0),
			FPlane(0, 0, -1, 0),
			FPlane(0, 0, 0, 1));

		const float ZOffset = WORLD_MAX;
		ProjectionMatrix = FReversedZOrthoMatrix(
			TargetExtent.X,
			TargetExtent.Y,
			0.5f / ZOffset,
			ZOffset);

		RenderTargetTexture = new UTextureRenderTarget2D(FObjectInitializer());
		check(RenderTargetTexture);
		RenderTargetTexture->ClearColor = FLinearColor::Transparent;
		RenderTargetTexture->TargetGamma = 1.f;
		RenderTargetTexture->InitCustomFormat(TargetSize.X, TargetSize.Y, PF_B8G8R8A8, false);
		RenderTargetResource = RenderTargetTexture->GameThread_GetRenderTargetResource()->GetTextureRenderTarget2DResource();

		// render
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FDrawSceneCommand,
			FLandscapeGrassWeightExporter_RenderThread*, Exporter, this,
			{
				Exporter->RenderLandscapeComponentToTexture_RenderThread(RHICmdList);
				FlushPendingDeleteRHIResources_RenderThread();
			});
	}

	void ApplyResults()
	{
		TArray<FColor> Samples;
		Samples.Init(TargetSize.X*TargetSize.Y);

		// Copy the contents of the remote texture to system memory
		FReadSurfaceDataFlags ReadSurfaceDataFlags;
		ReadSurfaceDataFlags.SetLinearToGamma(false);
		RenderTargetResource->ReadPixels(Samples, ReadSurfaceDataFlags, FIntRect(0, 0, TargetSize.X, TargetSize.Y));

		for (auto& ComponentInfo : ComponentInfos)
		{
			FLandscapeComponentGrassData* NewGrassData = new FLandscapeComponentGrassData();

			NewGrassData->HeightData.Empty(FMath::Square(ComponentSizeVerts));

			TArray<TArray<uint8>*> GrassWeightArrays;
			GrassWeightArrays.Empty(GrassTypes.Num());
			for (auto GrassType : GrassTypes)
			{
				int32 Index = GrassWeightArrays.Add(&NewGrassData->WeightData.Add(GrassType));
				GrassWeightArrays[Index]->Empty(FMath::Square(ComponentSizeVerts));
			}

			for (int32 PassIdx = 0; PassIdx < NumPasses; PassIdx++)
			{
				FColor* SampleData = &Samples[ComponentInfo.PixelOffsetX + PassIdx*ComponentSizeVerts];
				if (PassIdx == 0)
				{
					NewGrassData->HeightData.Empty(FMath::Square(ComponentSizeVerts));

					for (int32 y = 0; y < ComponentSizeVerts; y++)
					{
						for (int32 x = 0; x < ComponentSizeVerts; x++)
						{
							FColor& Sample = SampleData[x + y * TargetSize.X];
							uint16 Height = (((uint16)Sample.R) << 8) + (uint16)(Sample.G);
							NewGrassData->HeightData.Add(Height);
							GrassWeightArrays[0]->Add(Sample.B);
							if (GrassTypes.Num() > 1)
							{
								GrassWeightArrays[1]->Add(Sample.A);
							}
						}
					}
				}
				else
				{								
					for (int32 y = 0; y < ComponentSizeVerts; y++)
					{
						for (int32 x = 0; x < ComponentSizeVerts; x++)
						{
							FColor& Sample = SampleData[x + y * TargetSize.X];

							int32 TypeIdx = PassIdx * 4 - 2;
							GrassWeightArrays[TypeIdx++]->Add(Sample.R);
							if (TypeIdx < GrassTypes.Num())
							{
								GrassWeightArrays[TypeIdx++]->Add(Sample.G);
								if (TypeIdx < GrassTypes.Num())
								{
									GrassWeightArrays[TypeIdx++]->Add(Sample.B);
									if (TypeIdx < GrassTypes.Num())
									{
										GrassWeightArrays[TypeIdx++]->Add(Sample.A);
									}
								}
							}
						}
					}
				}
			}

			// Assign the new data (thread-safe)
			ComponentInfo.Component->GrassData = MakeShareable(NewGrassData);
		}
	}

	void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
	{
		if (RenderTargetTexture)
		{
			Collector.AddReferencedObject(RenderTargetTexture);
		}

		if (LandscapeProxy)
		{
			Collector.AddReferencedObject(LandscapeProxy);
		}

		for (auto& Info : ComponentInfos)
		{
			if (Info.Component)
			{
				Collector.AddReferencedObject(Info.Component);
			}
		}

		for (auto GrassType : GrassTypes)
		{
			if (GrassType)
			{
				Collector.AddReferencedObject(GrassType);
			}
		}
	}
};

#if WITH_EDITOR

bool ULandscapeComponent::CanRenderGrassMap()
{
	// Check we can render
	if (!GIsEditor || GUsingNullRHI || !GetWorld() || GetWorld()->FeatureLevel < ERHIFeatureLevel::SM4 || !SceneProxy)
	{
		return false;
	}
		
	// Check we can render the material
	if (!MaterialInstance->GetMaterialResource(GetWorld()->FeatureLevel)->HasValidGameThreadShaderMap())
	{
		return false;
	}
	
	// Check for valid heightmap that is fully streamed in
	if (!HeightmapTexture || HeightmapTexture->ResidentMips != HeightmapTexture->GetNumMips())
	{
		return false;
	}

	return true;
}

void ULandscapeComponent::RenderGrassMap()
{
	UMaterialInterface* Material = GetLandscapeMaterial();
	if (CanRenderGrassMap())
	{
		TArray<const UMaterialExpressionLandscapeGrassOutput*> GrassExpressions;
		Material->GetMaterial()->GetAllExpressionsOfType<UMaterialExpressionLandscapeGrassOutput>(GrassExpressions);
		if (GrassExpressions.Num() > 0)
		{
			TArray<ULandscapeGrassType*> GrassTypes;
			GrassTypes.Empty(GrassExpressions[0]->GrassTypes.Num());
			for (auto& GrassTypeInput : GrassExpressions[0]->GrassTypes)
			{
				if (GrassTypeInput.GrassType)
				{
					GrassTypes.Add(GrassTypeInput.GrassType);
				}
			}

			TArray<ULandscapeComponent*> LandscapeComponents;
			LandscapeComponents.Add(this);

			FLandscapeGrassWeightExporter Exporter(GetLandscapeProxy(), LandscapeComponents, GrassTypes);
			Exporter.ApplyResults();
		}

	}
}

void ULandscapeComponent::RemoveGrassMap()
{
	GrassData = MakeShareable(new FLandscapeComponentGrassData());
}

#endif


//
// UMaterialExpressionLandscapeGrassOutput
//
UMaterialExpressionLandscapeGrassOutput::UMaterialExpressionLandscapeGrassOutput(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FString STRING_Landscape;
		FName NAME_Grass;
		FConstructorStatics()
			: STRING_Landscape(LOCTEXT("Landscape", "Landscape").ToString())
			, NAME_Grass("Grass")
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	MenuCategories.Add(ConstructorStatics.STRING_Landscape);

	// No outputs
	Outputs.Reset();

	// Default input
	new(GrassTypes)FGrassInput(ConstructorStatics.NAME_Grass);
}

int32 UMaterialExpressionLandscapeGrassOutput::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex)
{
	if (GrassTypes.IsValidIndex(OutputIndex))
	{
		if (GrassTypes[OutputIndex].Input.Expression)
		{
			return Compiler->CustomOutput(this, OutputIndex, GrassTypes[OutputIndex].Input.Compile(Compiler, MultiplexIndex));
		}
		else
		{
			return CompilerError(Compiler, TEXT("Input missing"));
		}
	}

	return INDEX_NONE;
}

void UMaterialExpressionLandscapeGrassOutput::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Grass"));
}

const TArray<FExpressionInput*> UMaterialExpressionLandscapeGrassOutput::GetInputs()
{
	TArray<FExpressionInput*> OutInputs;
	for (auto& GrassType : GrassTypes)
	{
		OutInputs.Add(&GrassType.Input);
	}
	return OutInputs;
}

FExpressionInput* UMaterialExpressionLandscapeGrassOutput::GetInput(int32 InputIndex)
{
	return &GrassTypes[InputIndex].Input;
}

FString UMaterialExpressionLandscapeGrassOutput::GetInputName(int32 InputIndex) const
{
	return GrassTypes[InputIndex].Name.ToString();
}

#if WITH_EDITOR
void UMaterialExpressionLandscapeGrassOutput::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty)
	{
		const FName PropertyName = PropertyChangedEvent.MemberProperty->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UMaterialExpressionLandscapeGrassOutput, GrassTypes))
		{
			if (GraphNode)
			{
				GraphNode->ReconstructNode();
			}
		}
	}
}
#endif

//
// ULandscapeGrassType
//

ULandscapeGrassType::ULandscapeGrassType(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GrassDensity_DEPRECATED = 400;
	StartCullDistance_DEPRECATED = 10000.0f;
	EndCullDistance_DEPRECATED = 10000.0f;
	PlacementJitter_DEPRECATED = 1.0f;
	RandomRotation_DEPRECATED = true;
	AlignToSurface_DEPRECATED = true;
}

void ULandscapeGrassType::PostLoad()
{
	Super::PostLoad();
	if (GrassMesh_DEPRECATED && !GrassVarieties.Num())
	{
		FGrassVariety Grass;
		Grass.GrassMesh = GrassMesh_DEPRECATED;
		Grass.GrassDensity = GrassDensity_DEPRECATED;
		Grass.StartCullDistance = StartCullDistance_DEPRECATED;
		Grass.EndCullDistance = EndCullDistance_DEPRECATED;
		Grass.PlacementJitter = PlacementJitter_DEPRECATED;
		Grass.RandomRotation = RandomRotation_DEPRECATED;
		Grass.AlignToSurface = AlignToSurface_DEPRECATED;

		GrassVarieties.Add(Grass);
		GrassMesh_DEPRECATED = nullptr;
	}
}


#if WITH_EDITOR
void ULandscapeGrassType::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (GIsEditor)
	{
		// Only care current world object
		for (TActorIterator<ALandscapeProxy> It(GWorld); It; ++It)
		{
			ALandscapeProxy* Proxy = *It;
			const UMaterialInterface* MaterialInterface = Proxy->LandscapeMaterial;
			if (MaterialInterface)
			{
				TArray<const UMaterialExpressionLandscapeGrassOutput*> GrassExpressions;
				MaterialInterface->GetMaterial()->GetAllExpressionsOfType<UMaterialExpressionLandscapeGrassOutput>(GrassExpressions);

				// Should only be one grass type node
				if (GrassExpressions.Num() > 0)
				{
					for (auto& Output : GrassExpressions[0]->GrassTypes)
					{
						if (Output.GrassType == this)
						{
							Proxy->FlushFoliageComponents();
							break;
						}
					}
				}
			}
		}
	}
}
#endif

//
// FLandscapeComponentGrassData
//
FArchive& operator<<(FArchive& Ar, FLandscapeComponentGrassData& Data)
{
	Data.HeightData.BulkSerialize(Ar);
	// Each weight data array, being 1 byte will be serialized in bulk.
	return Ar << Data.WeightData;
}

#undef LOCTEXT_NAMESPACE