// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ThumbnailRendering/MaterialFunctionThumbnailRenderer.h"
#include "Misc/App.h"
#include "ShowFlags.h"
#include "SceneView.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialFunctionInstance.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "ThumbnailHelpers.h"


// FPreviewScene derived helpers for rendering
#include "RendererInterface.h"
#include "EngineModule.h"

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

		UMaterial* PreviewMaterial = bIsFunctionInstancePreview ? MatFuncInst->GetPreviewMaterial() : MatFunc->GetPreviewMaterial();
		UMaterialInstanceConstant* FunctionInstanceProxy = nullptr;

		if (PreviewMaterial)
		{
			if (bIsFunctionInstancePreview)
			{
				FunctionInstanceProxy = NewObject<UMaterialInstanceConstant>((UObject*)GetTransientPackage(), FName(TEXT("None")), RF_Transactional);
				FunctionInstanceProxy->SetParentEditorOnly(PreviewMaterial);
				MatFuncInst->OverrideMaterialInstanceParameterValues(FunctionInstanceProxy);
				FunctionInstanceProxy->PreEditChange(NULL);
				FunctionInstanceProxy->PostEditChange();
				PreviewMaterial->ThumbnailInfo = MatFuncInst->ThumbnailInfo;
				if (MatFuncInst->GetMaterialFunctionUsage() == EMaterialFunctionUsage::MaterialLayerBlend)
				{
					FunctionInstanceProxy->SetShouldForcePlanePreview(true);
				}
				ThumbnailScene->SetMaterialInterface((UMaterialInterface*)FunctionInstanceProxy);
			}
			else
			{
				PreviewMaterial->ThumbnailInfo = MatFunc->ThumbnailInfo;
				if (MatFunc->GetMaterialFunctionUsage() == EMaterialFunctionUsage::MaterialLayerBlend)
				{
					PreviewMaterial->SetShouldForcePlanePreview(true);
				}
				ThumbnailScene->SetMaterialInterface((UMaterialInterface*)PreviewMaterial);
			}
	
			FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues( RenderTarget, ThumbnailScene->GetScene(), FEngineShowFlags(ESFIM_Game) )
				.SetWorldTimes(FApp::GetCurrentTime() - GStartTime, FApp::GetDeltaTime(), FApp::GetCurrentTime() - GStartTime));

			ViewFamily.EngineShowFlags.DisableAdvancedFeatures();
			ViewFamily.EngineShowFlags.MotionBlur = 0;

			ThumbnailScene->GetView(&ViewFamily, X, Y, Width, Height);

			if (ViewFamily.Views.Num() > 0)
			{
				GetRendererModule().BeginRenderingViewFamily(Canvas,&ViewFamily);
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
