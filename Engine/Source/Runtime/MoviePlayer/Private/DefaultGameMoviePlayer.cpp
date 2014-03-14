// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MoviePlayer.h"

#include "Engine.h"
#include "Slate.h"
#include "RenderingCommon.h"
#include "Slate/SlateTextures.h"
#include "Slate/SceneViewport.h"
#include "GlobalShader.h"

#include "Spinlock.h"
#include "MoviePlayerThreading.h"
#include "DefaultGameMoviePlayer.h"



TSharedPtr<FDefaultGameMoviePlayer> FDefaultGameMoviePlayer::MoviePlayer;

TSharedPtr<FDefaultGameMoviePlayer> FDefaultGameMoviePlayer::Get()
{
	if (!MoviePlayer.IsValid())
	{
		MoviePlayer = MakeShareable(new FDefaultGameMoviePlayer);
	}
	return MoviePlayer;
}

FDefaultGameMoviePlayer::FDefaultGameMoviePlayer()
	: SyncMechanism(NULL)
	, MovieStreamingIsDone(0)
	, LoadingIsDone(0)
	, bUserCalledFinish(false)
	, LoadingScreenAttributes()
{
}

FDefaultGameMoviePlayer::~FDefaultGameMoviePlayer() {}

void FDefaultGameMoviePlayer::RegisterMovieStreamer(TSharedPtr<IMovieStreamer> InMovieStreamer)
{
	MovieStreamer = InMovieStreamer;
}

void FDefaultGameMoviePlayer::SetSlateRenderer(TSharedPtr<FSlateRenderer> InSlateRenderer)
{
	RendererPtr = InSlateRenderer;
}

void FDefaultGameMoviePlayer::Initialize()
{
	// Initialize shaders, because otherwise they might not be guaranteed to exist at this point
	if (!FPlatformProperties::RequiresCookedData())
	{
		TArray<int32> ShaderMapIds;
		ShaderMapIds.Add(GlobalShaderMapId);
		GShaderCompilingManager->FinishCompilation(TEXT("Global"), ShaderMapIds);
	}

	// Add a delegate to start playing movies when we start loading a map
	FCoreDelegates::PreLoadMap.AddSP( this, &FDefaultGameMoviePlayer::OnPreLoadMap );

	FPlatformSplash::Hide();

	TSharedRef<SWindow> GameWindow = UGameEngine::CreateGameWindow();

	TSharedPtr<SViewport> MovieViewport;
	LoadingScreenContents = SNew(SBorder)
		.BorderImage( FCoreStyle::Get().GetBrush(TEXT("BlackBrush")) )
		.OnMouseButtonDown(this, &FDefaultGameMoviePlayer::OnLoadingScreenMouseButtonDown)
		.Padding(0)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(this, &FDefaultGameMoviePlayer::GetMovieWidth)
				.HeightOverride(this, &FDefaultGameMoviePlayer::GetMovieHeight)
				[
					SAssignNew(MovieViewport, SViewport)
					.EnableGammaCorrection(false)
					.Visibility(this, &FDefaultGameMoviePlayer::GetViewportVisibility)
				]
			]
			+SOverlay::Slot()
			[
				SAssignNew(LoadingScreenWidgetHolder, SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("NoBorder")))
				.Padding(0)
			]
		];

	if (MovieStreamer.IsValid())
	{
		MovieViewport->SetViewportInterface( MovieStreamer->GetViewportInterface().ToSharedRef() );
	}
	
	// Register the movie viewport so that it can receive user input.
	if (!FPlatformProperties::SupportsWindowedMode())
	{
		FSlateApplication::Get().RegisterGameViewport( MovieViewport.ToSharedRef() );
	}

	LoadingScreenWindowPtr = GameWindow;
}

void FDefaultGameMoviePlayer::PassLoadingScreenWindowBackToGame() const
{
	auto GameEngine = Cast<UGameEngine>(GEngine);
	if (LoadingScreenWindowPtr.IsValid() && GameEngine)
	{
		GameEngine->GameViewportWindow = LoadingScreenWindowPtr;
	}
}

void FDefaultGameMoviePlayer::SetupLoadingScreen(const FLoadingScreenAttributes& InLoadingScreenAttributes)
{
	LoadingScreenAttributes = InLoadingScreenAttributes;
}

bool FDefaultGameMoviePlayer::PlayMovie()
{
	bool bBeganPlaying = false;
	if (LoadingScreenIsPrepared() && !IsMovieCurrentlyPlaying() )
	{
		check(LoadingScreenAttributes.IsValid());

		MovieStreamingIsDone.Set(MovieStreamingIsPrepared() ? 0 : 1);
		LoadingIsDone.Set(0);
		bUserCalledFinish = false;
		
		if (MovieStreamingIsPrepared())
		{
			MovieStreamer->Init(LoadingScreenAttributes.MoviePaths);
		}
		LoadingScreenWidgetHolder->SetContent(LoadingScreenAttributes.WidgetLoadingScreen.IsValid() ? LoadingScreenAttributes.WidgetLoadingScreen.ToSharedRef() : SNullWidget::NullWidget);
		LoadingScreenWindowPtr.Pin()->SetContent(LoadingScreenContents.ToSharedRef());
		
		SyncMechanism = new FSlateLoadingSynchronizationMechanism();
		SyncMechanism->Initialize();

		bBeganPlaying = true;
	}

	return bBeganPlaying;
}

void FDefaultGameMoviePlayer::WaitForMovieToFinish()
{
	if (LoadingScreenIsPrepared() && IsMovieCurrentlyPlaying())
	{
		SyncMechanism->DestroySlateThread();
		delete SyncMechanism;
		SyncMechanism = NULL;
		
		LoadingIsDone.Set(1);
		
		const bool bAutoCompleteWhenLoadingCompletes = LoadingScreenAttributes.bAutoCompleteWhenLoadingCompletes;
		while (!IsMovieStreamingFinished() && !bAutoCompleteWhenLoadingCompletes && !bUserCalledFinish)
		{
			if (FSlateApplication::IsInitialized())
			{
				FPlatformMisc::PumpMessages(true);
				FSlateApplication::Get().Tick();
				
				// Synchronize the game thread and the render thread so that the render thread doesn't get too far behind.
				FSlateApplication::Get().GetRenderer()->Sync();
			}
		}

		MovieStreamingIsDone.Set(1);
		if( MovieStreamer.IsValid() )
		{
			MovieStreamer->ForceCompletion();
		}

		FlushRenderingCommands();

		// Allow the movie streamer to clean up any resources it uses once there are no movies to play.
		if( MovieStreamer.IsValid() )
		{
			MovieStreamer->Cleanup();
		}
	

		// Finally, clear out the loading screen attributes, forcing users to always
		// explicitly set the loading screen they want (rather than have stale loading screens)
		LoadingScreenAttributes = FLoadingScreenAttributes();
	}
	else
	{	
		UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
		if (GameEngine)
		{
			GameEngine->SwitchGameWindowToUseGameViewport();
		}

	}
}

bool FDefaultGameMoviePlayer::IsLoadingFinished() const
{
	return LoadingIsDone.GetValue() != 0;
}

bool FDefaultGameMoviePlayer::IsMovieCurrentlyPlaying() const
{
	return SyncMechanism != NULL;
}

bool FDefaultGameMoviePlayer::IsMovieStreamingFinished() const
{
	return MovieStreamingIsDone.GetValue() != 0;
}

void FDefaultGameMoviePlayer::Tick( float DeltaTime )
{
	if (LoadingScreenWindowPtr.IsValid() && RendererPtr.IsValid())
	{
		if (MovieStreamingIsPrepared() && !IsMovieStreamingFinished())
		{
			const bool bMovieIsDone = MovieStreamer->Tick(DeltaTime);
			if (bMovieIsDone)
			{
				MovieStreamingIsDone.Set(1);
			}
		}

		if (!IsLoadingFinished() && SyncMechanism)
		{
			if (SyncMechanism->IsSlateDrawPassEnqueued())
			{
				RendererPtr.Pin()->DrawWindows();
				SyncMechanism->ResetSlateDrawPassEnqueued();
			}
		}
	}
}

TStatId FDefaultGameMoviePlayer::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FDefaultGameMoviePlayer, STATGROUP_Tickables);
}

bool FDefaultGameMoviePlayer::IsTickable() const
{
	return true;
}

bool FDefaultGameMoviePlayer::LoadingScreenIsPrepared() const
{
	return LoadingScreenAttributes.WidgetLoadingScreen.IsValid() || MovieStreamingIsPrepared();
}

void FDefaultGameMoviePlayer::SetupLoadingScreenFromIni()
{
	// We may have already setup a movie from a startup module
	if( !LoadingScreenAttributes.IsValid() )
	{
		// look in the .ini for a list of loading movies
		TArray<FString> StartupMovies;
		GConfig->GetArray(TEXT("StartupLoadingSequence"), TEXT("StartupMovies"), StartupMovies, GEngineIni);

		// if we didn't have any, nothing to do here
		if (StartupMovies.Num() > 0)
		{
		
			// fill out the attributes
			FLoadingScreenAttributes LoadingScreen;
			LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
			LoadingScreen.MoviePaths = StartupMovies;

			// now setup the actual loading screen
			SetupLoadingScreen(LoadingScreen);
		}
	}
}

bool FDefaultGameMoviePlayer::MovieStreamingIsPrepared() const
{
	return MovieStreamer.IsValid() && LoadingScreenAttributes.MoviePaths.Num() > 0;
}

FVector2D FDefaultGameMoviePlayer::GetMovieSize() const
{
	const FVector2D ScreenSize = LoadingScreenWindowPtr.Pin()->GetClientSizeInScreen();
	if (MovieStreamingIsPrepared())
	{
		const float MovieAspectRatio = MovieStreamer->GetAspectRatio();
		const float ScreenAspectRatio = ScreenSize.X / ScreenSize.Y;
		if (MovieAspectRatio < ScreenAspectRatio)
		{
			return FVector2D(ScreenSize.Y * MovieAspectRatio, ScreenSize.Y);
		}
		else
		{
			return FVector2D(ScreenSize.X, ScreenSize.X / MovieAspectRatio);
		}
	}

	// No movie, so simply return the size of the window
	return ScreenSize;
}

FOptionalSize FDefaultGameMoviePlayer::GetMovieWidth() const
{
	return GetMovieSize().X;
}

FOptionalSize FDefaultGameMoviePlayer::GetMovieHeight() const
{
	return GetMovieSize().Y;
}

EVisibility FDefaultGameMoviePlayer::GetSlateBackgroundVisibility() const
{
	return MovieStreamingIsPrepared() && !IsMovieStreamingFinished() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility FDefaultGameMoviePlayer::GetViewportVisibility() const
{
	return MovieStreamingIsPrepared() && !IsMovieStreamingFinished() ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply FDefaultGameMoviePlayer::OnLoadingScreenMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& PointerEvent)
{
	if (IsLoadingFinished())
	{
		if (LoadingScreenAttributes.bMoviesAreSkippable)
		{
			MovieStreamingIsDone.Set(1);
			MovieStreamer->ForceCompletion();
		}

		if (IsMovieStreamingFinished())
		{
			bUserCalledFinish = true;
		}
	}

	return FReply::Handled();
}


void FDefaultGameMoviePlayer::OnPreLoadMap()
{
	FCoreDelegates::PostLoadMap.RemoveAll(this);

	if( PlayMovie() )
	{
		FCoreDelegates::PostLoadMap.AddSP(this, &FDefaultGameMoviePlayer::OnPostLoadMap );
	}
}

void FDefaultGameMoviePlayer::OnPostLoadMap()
{
	WaitForMovieToFinish();
}