// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Landscape.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "UnrealEngine.h"
#include "EngineUtils.h"
#include "CanvasTypes.h"
#include "Shader.h"
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
protected:

	FLandscapeGrassWeightVS() {}
	FLandscapeGrassWeightVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{}

public:

	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		// Only compile for LandscapeVertexFactory
		return (Material->IsUsedWithLandscape() || Material->IsSpecialEngineMaterial()) && IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4) && (VertexFactoryType == FindVertexFactoryType(FName(TEXT("FLandscapeVertexFactory"), FNAME_Find))) && !IsConsolePlatform(Platform);
	}

	void SetParameters(FRHICommandList& RHICmdList, const FMaterialRenderProxy* MaterialRenderProxy, const FMaterial& MaterialResource, const FSceneView& View)
	{
		FMeshMaterialShader::SetParameters(RHICmdList, GetVertexShader(), MaterialRenderProxy, MaterialResource, View, ESceneRenderTargetsMode::DontSet);
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(RHICmdList, GetVertexShader(), VertexFactory, View, Proxy, BatchElement);
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FLandscapeGrassWeightVS, TEXT("LandscapeGrassWeight"), TEXT("VSMain"), SF_Vertex);


class FLandscapeGrassWeightPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FLandscapeGrassWeightPS, MeshMaterial);
public:

	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		// Only compile for LandscapeVertexFactory
		return (Material->IsUsedWithLandscape() || Material->IsSpecialEngineMaterial()) && IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4) && (VertexFactoryType == FindVertexFactoryType(FName(TEXT("FLandscapeVertexFactory"), FNAME_Find))) && !IsConsolePlatform(Platform);
	}

	FLandscapeGrassWeightPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{}

	FLandscapeGrassWeightPS() {}

	void SetParameters(FRHICommandList& RHICmdList, const FMaterialRenderProxy* MaterialRenderProxy, const FMaterial& MaterialResource, const FSceneView* View)
	{
		FMeshMaterialShader::SetParameters(RHICmdList, GetPixelShader(), MaterialRenderProxy, MaterialResource, *View, ESceneRenderTargetsMode::DontSet);
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(RHICmdList, GetPixelShader(), VertexFactory, View, Proxy, BatchElement);
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

	void SetSharedState(FRHICommandList& RHICmdList, const FSceneView* View, const ContextDataType PolicyContext) const
	{
		// Set the depth-only shader parameters for the material.
		VertexShader->SetParameters(RHICmdList, MaterialRenderProxy, *MaterialResource, *View);
		PixelShader->SetParameters(RHICmdList, MaterialRenderProxy, *MaterialResource, View);

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


class FLandscapeGrassWeightExporter
{
	static void RenderLandscapeComponentToTexture(
		ULandscapeComponent* LandscapeComponent,
		const FMatrix& ViewMatrix,
		const FMatrix& ProjectionMatrix,
		FIntPoint TargetSize,
		float TargetGamma,
		TArray<FColor>& OutSamples)
	{
		UTextureRenderTarget2D* RenderTargetTexture = new UTextureRenderTarget2D(FObjectInitializer());
		check(RenderTargetTexture);
		RenderTargetTexture->AddToRoot();
		RenderTargetTexture->ClearColor = FLinearColor::Transparent;
		RenderTargetTexture->TargetGamma = TargetGamma;
		RenderTargetTexture->InitCustomFormat(TargetSize.X, TargetSize.Y, PF_B8G8R8A8, false); 
		FTextureRenderTarget2DResource* RenderTargetResource = RenderTargetTexture->GameThread_GetRenderTargetResource()->GetTextureRenderTarget2DResource();
		RenderTargetResource->GetRenderTargetTexture();

		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(RenderTargetResource, NULL, FEngineShowFlags(ESFIM_Game))
			.SetWorldTimes(FApp::GetCurrentTime() - GStartTime, FApp::GetDeltaTime(), FApp::GetCurrentTime() - GStartTime)
			);

		// To enable visualization mode
		ViewFamily.EngineShowFlags.SetPostProcessing(true);
		ViewFamily.EngineShowFlags.SetVisualizeBuffer(true);
		ViewFamily.EngineShowFlags.SetTonemapper(false);

		// Force LOD render
#if WITH_EDITOR
		ViewFamily.LandscapeLODOverride = 0;
#endif
		FSceneViewInitOptions ViewInitOptions;
		ViewInitOptions.SetViewRectangle(FIntRect(0, 0, TargetSize.X, TargetSize.Y));
		ViewInitOptions.ViewFamily = &ViewFamily;
		ViewInitOptions.ViewMatrix = ViewMatrix;
		ViewInitOptions.ProjectionMatrix = ProjectionMatrix;

		FCanvas Canvas(RenderTargetResource, NULL, FApp::GetCurrentTime() - GStartTime, FApp::GetDeltaTime(), FApp::GetCurrentTime() - GStartTime, GMaxRHIFeatureLevel);
		Canvas.Clear(FLinearColor::Transparent);

		GetRendererModule().CreateAndInitSingleView(&ViewFamily, &ViewInitOptions);
		
		// render
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FDrawSceneCommand,
			FSceneViewFamily&, ViewFamily, ViewFamily,
			FPrimitiveSceneInfo*, PrimitiveSceneInfo, LandscapeComponent->SceneProxy->GetPrimitiveSceneInfo(),
			{
				RenderLandscapeComponentToTexture_RenderThread(RHICmdList, ViewFamily, PrimitiveSceneInfo);
				FlushPendingDeleteRHIResources_RenderThread();
			});

		// Copy the contents of the remote texture to system memory
		OutSamples.Init(TargetSize.X*TargetSize.Y);
		FReadSurfaceDataFlags ReadSurfaceDataFlags;
		ReadSurfaceDataFlags.SetLinearToGamma(false);
		RenderTargetResource->ReadPixels(OutSamples, ReadSurfaceDataFlags, FIntRect(0, 0, TargetSize.X, TargetSize.Y));

		RenderTargetTexture->RemoveFromRoot();
		RenderTargetTexture = nullptr;
	}

	static void RenderLandscapeComponentToTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily, FPrimitiveSceneInfo* PrimitiveSceneInfo)
	{
		const FSceneView* View = ViewFamily.Views[0];
		RHICmdList.SetViewport(View->ViewRect.Min.X, View->ViewRect.Min.Y, 0.0f, View->ViewRect.Max.X, View->ViewRect.Max.Y, 1.0f);
		RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
		RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
		RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
		RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

		for (int32 StaticMeshIdx = 0; StaticMeshIdx < PrimitiveSceneInfo->StaticMeshes.Num(); StaticMeshIdx++)
		{
			FMeshBatch& Mesh = *(FMeshBatch*)(&PrimitiveSceneInfo->StaticMeshes[StaticMeshIdx]);

			FLandscapeGrassWeightDrawingPolicy DrawingPolicy(Mesh.VertexFactory, Mesh.MaterialRenderProxy, *Mesh.MaterialRenderProxy->GetMaterial(GMaxRHIFeatureLevel));
			RHICmdList.BuildAndSetLocalBoundShaderState(DrawingPolicy.GetBoundShaderStateInput(GMaxRHIFeatureLevel));
			DrawingPolicy.SetSharedState(RHICmdList, View, FLandscapeGrassWeightDrawingPolicy::ContextDataType());

			// The first batch element contains the grass batch for the entire component
			DrawingPolicy.SetMeshRenderState(RHICmdList, *View, PrimitiveSceneInfo->Proxy, Mesh, 0, false, FMeshDrawingPolicy::ElementDataType(), FLandscapeGrassWeightDrawingPolicy::ContextDataType());
			DrawingPolicy.DrawMesh(RHICmdList, Mesh, 0);
		}
	}

public:
	
	static bool GetLandscapeComponentGrassMap(ULandscapeComponent* LandscapeComponent, TArray<FColor>& OutSamples)
	{
		check(LandscapeComponent);

		if (!LandscapeComponent->SceneProxy)
		{
			return false;
		}
		ALandscapeProxy* LandscapeProxy = LandscapeComponent->GetLandscapeProxy();

		FIntRect LandscapeRect = FIntRect(0, 0, LandscapeComponent->ComponentSizeQuads, LandscapeComponent->ComponentSizeQuads);
		LandscapeRect += LandscapeComponent->GetSectionBase() - LandscapeProxy->LandscapeSectionOffset;

		FVector MidPoint = FVector(LandscapeRect.Min, 0.f) + FVector(LandscapeRect.Size(), 0.f)*0.5f;
		FVector LandscapeCenter = LandscapeProxy->GetTransform().TransformPosition(MidPoint);
		FVector LandscapeExtent = FVector(LandscapeRect.Size() + FIntPoint(1,1), 0.f)*LandscapeProxy->GetActorScale()*0.5f;

		FMatrix ViewMatrix = FTranslationMatrix(-LandscapeCenter);
		ViewMatrix *= FInverseRotationMatrix(LandscapeProxy->GetActorRotation());
		ViewMatrix *= FMatrix(FPlane(1, 0, 0, 0),
			FPlane(0, -1, 0, 0),
			FPlane(0, 0, -1, 0),
			FPlane(0, 0, 0, 1));

		const float ZOffset = WORLD_MAX;
		FMatrix ProjectionMatrix = FReversedZOrthoMatrix(
			LandscapeExtent.X,
			LandscapeExtent.Y,
			0.5f / ZOffset,
			ZOffset);

		FIntPoint TargetSize(LandscapeComponent->ComponentSizeQuads + 1, LandscapeComponent->ComponentSizeQuads + 1);

		//TargetSize *= 8;

		const float Gamma = 1.0f;
		RenderLandscapeComponentToTexture(LandscapeComponent, ViewMatrix, ProjectionMatrix, TargetSize, Gamma, OutSamples);
		return true;
	}
};

#if WITH_EDITOR
void ULandscapeComponent::RenderGrassMap()
{
	UMaterialInterface* Material = GetLandscapeMaterial();
	if (GIsEditor && !GUsingNullRHI && SceneProxy && Material)
	{
		FLandscapeComponentGrassData* NewGrassData = new FLandscapeComponentGrassData();

		TArray<FColor> Samples;
		FLandscapeGrassWeightExporter::GetLandscapeComponentGrassMap(this, Samples);

		TArray<const UMaterialExpressionLandscapeGrassOutput*> GrassExpressions;
		Material->GetMaterial()->GetAllExpressionsOfType<UMaterialExpressionLandscapeGrassOutput>(GrassExpressions);
		if (GrassExpressions.Num() > 0)
		{
			int NumTypes = FMath::Min<int32>(GrassExpressions[0]->GrassTypes.Num(), 2); // Only 2 layers currently supported
			if (NumTypes > 0)
			{
				TArray<uint8>* GrassWeightArrays[2] = { nullptr, nullptr };

				// Initialize arrays
				NewGrassData->HeightData.Empty(Samples.Num());
				for (int32 Index = 0; Index < NumTypes; Index++)
				{
					GrassWeightArrays[Index] = &NewGrassData->WeightData.Add(GrassExpressions[0]->GrassTypes[Index].GrassType);
					GrassWeightArrays[Index]->Empty(Samples.Num());
				}

				// Fill data
				for (FColor& Sample : Samples)
				{
					uint16 Height = (((uint16)Sample.R) << 8) + (uint16)(Sample.G);
					check((Height >> 8) == Sample.R);
					check((Height & 255) == Sample.G);
					NewGrassData->HeightData.Add(Height);
					GrassWeightArrays[0]->Add(Sample.B);
					if (NumTypes>1)
					{
						GrassWeightArrays[1]->Add(Sample.A);
					}
				}
			}
		}

		FArchive* Ar = IFileManager::Get().CreateFileWriter(TEXT("D:\\Heightmaps\\Rendered.r16"), FILEWRITE_EvenIfReadOnly);
		if (Ar)
		{
			Ar->Serialize(NewGrassData->HeightData.GetData(), NewGrassData->HeightData.Num() * sizeof(uint16));
			delete Ar;
		}

		// Assign the new data (thread-safe)
		GrassData = MakeShareable(NewGrassData);
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
	GrassDensity = 400;
	StartCullDistance = 10000.0f;
	EndCullDistance = 10000.0f;
	PlacementJitter = 1.0f;
	RandomRotation = true;
	AlignToSurface = true;
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