// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
StreamingPauseRendering.cpp: Streaming pause implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "StreamingPauseRendering.h"
#include "SceneViewport.h"
#include "MoviePlayer.h"

IMPLEMENT_MODULE(FStreamingPauseRenderingModule, StreamingPauseRenderingModule);

void FStreamingPauseRenderingModule::StartupModule()
{
	BeginDelegate.BindRaw(this, &FStreamingPauseRenderingModule::BeginStreamingPause);
	EndDelegate.BindRaw(this, &FStreamingPauseRenderingModule::EndStreamingPause);
	check(GEngine);
	GEngine->RegisterBeginStreamingPauseRenderingDelegate(&BeginDelegate);
	GEngine->RegisterEndStreamingPauseRenderingDelegate(&EndDelegate);
}

void FStreamingPauseRenderingModule::ShutdownModule()
{
	BeginDelegate.Unbind();
	EndDelegate.Unbind();
	if( GEngine )
	{
		GEngine->RegisterBeginStreamingPauseRenderingDelegate(NULL);
		GEngine->RegisterEndStreamingPauseRenderingDelegate(NULL);
	}
}

/** Enqueue the streaming pause to suspend rendering during blocking load. */
void FStreamingPauseRenderingModule::BeginStreamingPause( FViewport* GameViewport )
{
#if WITH_STREAMING_PAUSE

	//Create the viewport widget and add a throbber.
	ViewportWidget = SNew( SViewport )
		.EnableGammaCorrection(false);

	ViewportWidget->SetContent
		(
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.VAlign(VAlign_Bottom)
		.HAlign(HAlign_Right)
		.Padding(FMargin(10.0f))
		[
			SNew(SThrobber)
		]
	);

	//Render the current scene to a new viewport.
	static const auto* CVarRenderLastFrameInStreamingPause = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.RenderLastFrameInStreamingPause"));
	if( CVarRenderLastFrameInStreamingPause && CVarRenderLastFrameInStreamingPause->GetValueOnGameThread() != 0 ) 
	{
		ViewportClient = GameViewport->GetClient();

		Viewport = MakeShareable(new FSceneViewport(ViewportClient, ViewportWidget));

		ViewportWidget->SetViewportInterface(Viewport.ToSharedRef());

		FIntPoint SizeXY = GameViewport->GetSizeXY();	
		Viewport->UpdateViewportRHI(false,SizeXY.X,SizeXY.Y, EWindowMode::Fullscreen);

		Viewport->EnqueueBeginRenderFrame();

		FCanvas Canvas( Viewport.Get(), NULL, ViewportClient->GetWorld());
		{
			ViewportClient->Draw(Viewport.Get(), &Canvas);
		}
		Canvas.Flush();

		//Don't need debug canvas I presume?
		//Viewport->GetDebugCanvas()->Flush(true);

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			EndDrawingCommand,
			FViewport*,Viewport,Viewport.Get(),
			//FIntPoint,InRestoreSize,RestoreSize,
		{
			Viewport->EndRenderFrame( false, false );
		});
	}

	//Create the loading screen and begin rendering the widget.
	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = true; 
	LoadingScreen.WidgetLoadingScreen = ViewportWidget; // SViewport from above
	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);			
	GetMoviePlayer()->PlayMovie();

#endif

	FlushRenderingCommands();
}

/** Enqueue the streaming pause to resume rendering after blocking load is completed. */
void FStreamingPauseRenderingModule::EndStreamingPause()
{
	//Stop rendering the loading screen and resume
	GetMoviePlayer()->WaitForMovieToFinish();

	ViewportWidget = NULL;
	Viewport = NULL;
	ViewportClient = NULL;
	
	FlushRenderingCommands();
}