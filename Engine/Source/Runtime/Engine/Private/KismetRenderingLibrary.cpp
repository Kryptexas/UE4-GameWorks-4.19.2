// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"

//////////////////////////////////////////////////////////////////////////
// UKismetRenderingLibrary

#define LOCTEXT_NAMESPACE "KismetRenderingLibrary"

UKismetRenderingLibrary::UKismetRenderingLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UCanvas* GCanvasForRenderingToTarget = NULL;

void UKismetRenderingLibrary::DrawMaterialToRenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, UMaterialInterface* Material)
{
	check(WorldContextObject);
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);

	if (TextureRenderTarget 
		&& TextureRenderTarget->Resource 
		&& World 
		&& Material)
	{
		if (!GCanvasForRenderingToTarget)
		{
			GCanvasForRenderingToTarget = NewObject<UCanvas>(GetTransientPackage(), NAME_None);
			GCanvasForRenderingToTarget->AddToRoot();
		}

		UCanvas* Canvas = GCanvasForRenderingToTarget;

		Canvas->Init(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY, nullptr);
		Canvas->Update();

		FCanvas RenderCanvas(
			TextureRenderTarget->GameThread_GetRenderTargetResource(), 
			nullptr, 
			FApp::GetCurrentTime() - GStartTime, 
			FApp::GetDeltaTime(), 
			FApp::GetCurrentTime() - GStartTime, 
			World->FeatureLevel);

		Canvas->Canvas = &RenderCanvas;

		TDrawEvent<FRHICommandList>* DrawMaterialToTargetEvent = new TDrawEvent<FRHICommandList>();

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			BeginDrawEventCommand,
			FName,RTName,TextureRenderTarget->GetFName(),
			TDrawEvent<FRHICommandList>*,DrawMaterialToTargetEvent,DrawMaterialToTargetEvent,
		{
			BEGIN_DRAW_EVENTF(
				RHICmdList, 
				DrawCanvasToTarget, 
				(*DrawMaterialToTargetEvent), 
				*RTName.ToString());
		});

		Canvas->K2_DrawMaterial(Material, FVector2D(0, 0), FVector2D(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY), FVector2D(0, 0));

		RenderCanvas.Flush_GameThread();
		Canvas->Canvas = NULL;

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			CanvasRenderTargetResolveCommand, FTextureRenderTargetResource*, RenderTargetResource, TextureRenderTarget->GameThread_GetRenderTargetResource(),
			TDrawEvent<FRHICommandList>*,DrawMaterialToTargetEvent,DrawMaterialToTargetEvent,
			{
				RHICmdList.CopyToResolveTarget(RenderTargetResource->GetRenderTargetTexture(), RenderTargetResource->TextureRHI, true, FResolveParams());
				STOP_DRAW_EVENT((*DrawMaterialToTargetEvent));
				delete DrawMaterialToTargetEvent;
			}
		);
	}
	else if (!Material)
	{
		FMessageLog("Blueprint").Warning(LOCTEXT("InvalidMaterial", "DrawMaterialToRenderTarget: Material must be non-null."));
	}
	else if (!TextureRenderTarget)
	{
		FMessageLog("Blueprint").Warning(LOCTEXT("InvalidTextureRenderTarget", "DrawMaterialToRenderTarget: TextureRenderTarget must be non-null."));
	}
}

void UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, UCanvas*& Canvas, FVector2D& Size, FDrawToRenderTargetContext& Context)
{
	check(WorldContextObject);
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);

	Canvas = NULL;
	Size = FVector2D(0, 0);
	Context = FDrawToRenderTargetContext();

	if (TextureRenderTarget 
		&& TextureRenderTarget->Resource 
		&& World)
	{
		Context.RenderTarget = TextureRenderTarget;

		if (!GCanvasForRenderingToTarget)
		{
			GCanvasForRenderingToTarget = NewObject<UCanvas>(GetTransientPackage(), NAME_None);
			GCanvasForRenderingToTarget->AddToRoot();
		}

		Canvas = GCanvasForRenderingToTarget;
		Size = FVector2D(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY);

		Canvas->Init(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY, nullptr);
		Canvas->Update();

		Canvas->Canvas = new FCanvas(
			TextureRenderTarget->GameThread_GetRenderTargetResource(), 
			nullptr, 
			FApp::GetCurrentTime() - GStartTime, 
			FApp::GetDeltaTime(), 
			FApp::GetCurrentTime() - GStartTime, 
			World->FeatureLevel, 
			// Draw immediately so that interleaved SetVectorParameter (etc) function calls work as expected
			FCanvas::CDM_ImmediateDrawing);

		Context.DrawEvent = new TDrawEvent<FRHICommandList>();

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			BeginDrawEventCommand,
			FName,RTName,TextureRenderTarget->GetFName(),
			TDrawEvent<FRHICommandList>*,DrawEvent,Context.DrawEvent,
		{
			BEGIN_DRAW_EVENTF(
				RHICmdList, 
				DrawCanvasToTarget, 
				(*DrawEvent), 
				*RTName.ToString());
		});
	}
	else if (!TextureRenderTarget)
	{
		FMessageLog("Blueprint").Warning(LOCTEXT("InvalidTextureRenderTarget", "BeginDrawCanvasToRenderTarget: TextureRenderTarget must be non-null."));
	}
}

void UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(UObject* WorldContextObject, const FDrawToRenderTargetContext& Context)
{
	check(WorldContextObject);
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);

	if (World)
	{
		if (GCanvasForRenderingToTarget && GCanvasForRenderingToTarget->Canvas)
		{
			GCanvasForRenderingToTarget->Canvas->Flush_GameThread();
			delete GCanvasForRenderingToTarget->Canvas;
			GCanvasForRenderingToTarget->Canvas = nullptr;
		}
		
		if (Context.RenderTarget)
		{
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				CanvasRenderTargetResolveCommand, FTextureRenderTargetResource*, RenderTargetResource, Context.RenderTarget->GameThread_GetRenderTargetResource(),
				TDrawEvent<FRHICommandList>*,DrawEvent,Context.DrawEvent,
				{
					RHICmdList.CopyToResolveTarget(RenderTargetResource->GetRenderTargetTexture(), RenderTargetResource->TextureRHI, true, FResolveParams());
					STOP_DRAW_EVENT((*DrawEvent));
					delete DrawEvent;
				}
			);
		}
		else if (!Context.RenderTarget)
		{
			FMessageLog("Blueprint").Warning(LOCTEXT("InvalidContext", "EndDrawCanvasToRenderTarget: Context must be valid."));
		}
	}
}

#undef LOCTEXT_NAMESPACE