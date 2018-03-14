// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ThumbnailRendering/MaterialFunctionThumbnailRenderer.h"
#include "Misc/App.h"
#include "ShowFlags.h"
#include "SceneView.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialFunctionInstance.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "ThumbnailHelpers.h"


UMaterialFunctionThumbnailRenderer::UMaterialFunctionThumbnailRenderer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ThumbnailScene = nullptr;
}

void UMaterialFunctionThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas)
{
	UMaterialFunctionInterface* MatFunc = Cast<UMaterialFunctionInterface>(Object);
	UMaterialFunctionInstance* MatFuncInst = Cast<UMaterialFunctionInstance>(Object);
	const bool bIsFunctionInstancePreview = MatFuncInst && MatFuncInst->GetBaseFunction();

	if (MatFunc || bIsFunctionInstancePreview)
	{
		if (ThumbnailScene == nullptr || ensure(ThumbnailScene->GetWorld() != nullptr) == false)
		{
			if (ThumbnailScene)
			{
				FlushRenderingCommands();
				delete ThumbnailScene;
			}

			ThumbnailScene = new FMaterialThumbnailScene();
		}

		UMaterialInterface* PreviewMaterial = bIsFunctionInstancePreview ? MatFuncInst->GetPreviewMaterial() : MatFunc->GetPreviewMaterial();
		EMaterialFunctionUsage FunctionUsage = bIsFunctionInstancePreview ? MatFuncInst->GetMaterialFunctionUsage() : MatFunc->GetMaterialFunctionUsage();
		UThumbnailInfo* ThumbnailInfo = bIsFunctionInstancePreview ? MatFuncInst->ThumbnailInfo : MatFunc->ThumbnailInfo;

		if (PreviewMaterial)
		{
				PreviewMaterial->ThumbnailInfo = ThumbnailInfo;
				if (FunctionUsage == EMaterialFunctionUsage::MaterialLayerBlend)
				{
					PreviewMaterial->SetShouldForcePlanePreview(true);
				}
				ThumbnailScene->SetMaterialInterface(PreviewMaterial);
	
			FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues( RenderTarget, ThumbnailScene->GetScene(), FEngineShowFlags(ESFIM_Game) )
				.SetWorldTimes(FApp::GetCurrentTime() - GStartTime, FApp::GetDeltaTime(), FApp::GetCurrentTime() - GStartTime));

			ViewFamily.EngineShowFlags.DisableAdvancedFeatures();
			ViewFamily.EngineShowFlags.MotionBlur = 0;

			ThumbnailScene->GetView(&ViewFamily, X, Y, Width, Height);

			if (ViewFamily.Views.Num() > 0)
			{
				RenderViewFamily(Canvas,&ViewFamily);
			}

			ThumbnailScene->SetMaterialInterface(nullptr);
		}
	}
}

void UMaterialFunctionThumbnailRenderer::BeginDestroy()
{ 	
	if (ThumbnailScene)
	{
		delete ThumbnailScene;
		ThumbnailScene = nullptr;
	}

	Super::BeginDestroy();
}
