// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

// FPreviewScene derived helpers for rendering
#include "ThumbnailHelpers.h"

UMaterialInstanceThumbnailRenderer::UMaterialInstanceThumbnailRenderer(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ThumbnailScene = nullptr;
}

void UMaterialInstanceThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas)
{
	UMaterialInterface* MatInst = Cast<UMaterialInterface>(Object);
	if (MatInst != nullptr)
	{
		if ( ThumbnailScene == nullptr )
		{
			ThumbnailScene = new FMaterialThumbnailScene();
		}

		ThumbnailScene->SetMaterialInterface(MatInst);
		FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues( RenderTarget, ThumbnailScene->GetScene(), FEngineShowFlags(ESFIM_Game) )
			.SetWorldTimes(GCurrentTime - GStartTime,GDeltaTime,GCurrentTime - GStartTime) );

		ViewFamily.EngineShowFlags.DisableAdvancedFeatures();
		ViewFamily.EngineShowFlags.MotionBlur = 0;
		ViewFamily.EngineShowFlags.AntiAliasing = 0;

		ThumbnailScene->GetView(&ViewFamily, X, Y, Width, Height);

		if (ViewFamily.Views.Num() > 0)
		{
			GetRendererModule().BeginRenderingViewFamily(Canvas, &ViewFamily);
		}

		ThumbnailScene->SetMaterialInterface(nullptr);
	}
}

void UMaterialInstanceThumbnailRenderer::BeginDestroy()
{
	if ( ThumbnailScene != nullptr )
	{
		delete ThumbnailScene;
		ThumbnailScene = nullptr;
	}

	Super::BeginDestroy();
}
