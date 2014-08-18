// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GameEngine.cpp: Unreal game engine.
=============================================================================*/

#include "EnginePrivate.h"
#include "ParticleDefinitions.h"
#include "SoundDefinitions.h"
#include "Net/UnrealNetwork.h"
#include "AllocatorFixedSizeFreeList.h"
#include "Database.h"
#include "MallocProfiler.h"
#include "Net/NetworkProfiler.h"
#include "ConfigCacheIni.h"
#include "EngineModule.h"
#include "Engine/GameInstance.h"

#include "AVIWriter.h"

#include "Slate.h"
#include "Slate/SceneViewport.h"
#include "SVirtualJoystick.h"

#include "AVIWriter.h"
#include "AssetRegistryModule.h"
#include "SynthBenchmark.h"

#include "IHeadMountedDisplay.h"

DEFINE_LOG_CATEGORY_STATIC(LogEngine, Log, All);

ENGINE_API bool GDisallowNetworkTravel = false;

/** Benchmark results to the log */
static void RunSynthBenchmark(const TArray<FString>& Args)
{
	uint32 WorkScale = 10;
	bool bDebugOut = false;

	// 2 arguments mean with debug out
	if(Args.Num() > 1)
	{
		bDebugOut = true;
	}

	if ( Args.Num() > 0 )
	{
		WorkScale = FCString::Atoi(*Args[0]);
		WorkScale = FMath::Clamp(WorkScale, (uint32)1, (uint32)1000);
	}

	FSynthBenchmarkResults Result;
	ISynthBenchmark::Get().Run(Result, true, WorkScale, bDebugOut);
}

static FAutoConsoleCommand GDumpDrawListStatsCmd(
	TEXT("SynthBenchmark"),
	TEXT("Run simple benchmark to get some metrics to find reasonable game settings automatically."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&RunSynthBenchmark)
	);


int32 GetBoundFullScreenModeCVar()
{
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FullScreenMode")); 

	if (FPlatformProperties::SupportsWindowedMode())
	{
		int32 Value = CVar->GetValueOnGameThread();

		if (Value >= 0 && Value < EWindowMode::NumWindowModes)
		{
			return Value;
		}
	}

	// every other value behaves like 0
	return 0;
}

// depending on WindowMode and the console variable r.FullScreenMode
EWindowMode::Type GetWindowModeType(EWindowMode::Type WindowMode)
{
	if (FPlatformProperties::SupportsWindowedMode())
	{
		if (WindowMode == EWindowMode::Windowed)
		{
			return WindowMode;
		}

		if (GEngine && GEngine->HMDDevice.IsValid() && GEngine->HMDDevice->IsFullScreenAllowed())
		{
			return EWindowMode::Fullscreen;
		}
		
		// Figure out which full screen mode we should be
		EWindowMode::Type DesiredFullScreenWindowMode = EWindowMode::ConvertIntToWindowMode(GetBoundFullScreenModeCVar());
		return DesiredFullScreenWindowMode;
	}

	return EWindowMode::Fullscreen;
}

UGameEngine::UGameEngine(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

/*-----------------------------------------------------------------------------
	cleanup!!
-----------------------------------------------------------------------------*/

void UGameEngine::CreateGameViewportWidget( UGameViewportClient* GameViewportClient )
{
	TSharedRef<SOverlay> ViewportOverlayWidgetRef = SNew( SOverlay );
	TSharedRef<SViewport> GameViewportWidgetRef = 
		SNew( SViewport )
			// Render directly to the window backbuffer unless capturing a movie or getting screenshots
			// @todo TEMP
			.RenderDirectlyToWindow( !GEngine->bStartWithMatineeCapture && GIsDumpingMovie == 0 )
			[
				ViewportOverlayWidgetRef
			];

	GameViewportWidget = GameViewportWidgetRef;

	GameViewportClient->SetViewportOverlayWidget( GameViewportWindow.Pin(), ViewportOverlayWidgetRef );
}

void UGameEngine::CreateGameViewport( UGameViewportClient* GameViewportClient )
{
	check(GameViewportWindow.IsValid());

	if( !GameViewportWidget.IsValid() )
	{
		CreateGameViewportWidget( GameViewportClient );
	}
	TSharedRef<SViewport> GameViewportWidgetRef = GameViewportWidget.ToSharedRef();

	auto Window = GameViewportWindow.Pin();

	Window->SetWidgetToFocusOnActivate( GameViewportWidgetRef );
	Window->SetOnWindowClosed( FOnWindowClosed::CreateUObject( this, &UGameEngine::OnGameWindowClosed ) );

	// SAVEWINPOS tells us to load/save window positions to user settings (this is disabled by default)
	int32 SaveWinPos;
	if (FParse::Value(FCommandLine::Get(), TEXT("SAVEWINPOS="), SaveWinPos) && SaveWinPos > 0 )
	{
		// Get WinX/WinY from GameSettings, apply them if valid.
		FIntPoint PiePosition = GetGameUserSettings()->GetWindowPosition();
		if (PiePosition.X >= 0 && PiePosition.Y >= 0)
		{
			int32 WinX = GetGameUserSettings()->GetWindowPosition().X;
			int32 WinY = GetGameUserSettings()->GetWindowPosition().Y;
			Window->MoveWindowTo(FVector2D(WinX, WinY));
		}
		Window->SetOnWindowMoved( FOnWindowMoved::CreateUObject( this, &UGameEngine::OnGameWindowMoved ) );
	}

	SceneViewport = MakeShareable( new FSceneViewport( GameViewportClient, GameViewportWidgetRef ) );
	GameViewportClient->Viewport = SceneViewport.Get();

	//GameViewportClient->CreateHighresScreenshotCaptureRegionWidget(); //  Disabled until mouse based input system can be made to work correctly.

	// The viewport widget needs an interface so it knows what should render
	GameViewportWidgetRef->SetViewportInterface( SceneViewport.ToSharedRef() );

	FViewportFrame* ViewportFrame = SceneViewport.Get();

	GameViewport->SetViewportFrame(ViewportFrame);
}

void UGameEngine::ConditionallyOverrideSettings(int32& ResolutionX, int32& ResolutionY, EWindowMode::Type& WindowMode)
{
	if (FParse::Param(FCommandLine::Get(),TEXT("Windowed")) || FParse::Param(FCommandLine::Get(), TEXT("SimMobile")))
	{
		// -Windowed or -SimMobile
		WindowMode = EWindowMode::Windowed;
	}
	else if (FParse::Param(FCommandLine::Get(),TEXT("FullScreen")))
	{
		// -FullScreen
#if PLATFORM_MAC // @TODO: Fullscreen mode isn't working fully on OS X as it requires explicit CGL back-buffer size and mouse<->display coordinate conversions.
		// For now use the windowed fullscreen mode which merely stretches to fill the display, which is what OS X does for us by default.
		WindowMode = EWindowMode::WindowedFullscreen;
#else
		WindowMode = EWindowMode::Fullscreen;
#endif
	}

	//fullscreen is always supported, but don't allow windowed mode on platforms that dont' support it.
	WindowMode = (!FPlatformProperties::SupportsWindowedMode() && (WindowMode == EWindowMode::Windowed || WindowMode == EWindowMode::WindowedMirror || WindowMode == EWindowMode::WindowedFullscreen)) ? EWindowMode::Fullscreen : WindowMode;

	FParse::Value(FCommandLine::Get(), TEXT("ResX="), ResolutionX);
	FParse::Value(FCommandLine::Get(), TEXT("ResY="), ResolutionY);

	if (!IsRunningDedicatedServer() && ((ResolutionX <= 0) || (ResolutionY <= 0)))
	{
		// consume available desktop area
		FDisplayMetrics DisplayMetrics;
		FSlateApplication::Get().GetDisplayMetrics( DisplayMetrics );
		
		ResolutionX = DisplayMetrics.PrimaryDisplayWidth;
		ResolutionY = DisplayMetrics.PrimaryDisplayHeight;

		// If we're in windowed mode, attempt to choose a suitable starting resolution that is smaller than the desktop, with a matching aspect ratio
		if (WindowMode == EWindowMode::Windowed)
		{
			TArray<FIntPoint> WindowedResolutions;
			GenerateConvenientWindowedResolutions(DisplayMetrics, WindowedResolutions);

			if (WindowedResolutions.Num() > 0)
			{
				// We'll default to the largest one we have
				ResolutionX = WindowedResolutions[WindowedResolutions.Num() - 1].X;
				ResolutionY = WindowedResolutions[WindowedResolutions.Num() - 1].Y;

				// Attempt to find the largest one with the same aspect ratio
				float DisplayAspect = (float)DisplayMetrics.PrimaryDisplayWidth / (float)DisplayMetrics.PrimaryDisplayHeight;
				for (int32 i = WindowedResolutions.Num() - 1; i >= 0; --i)
				{
					float Aspect = (float)WindowedResolutions[i].X / (float)WindowedResolutions[i].Y;
					if (FMath::Abs(Aspect - DisplayAspect) < KINDA_SMALL_NUMBER)
					{
						ResolutionX = WindowedResolutions[i].X;
						ResolutionY = WindowedResolutions[i].Y;
						break;
					}
				}
			}
		}
	}

	// Check the platform to see if we should override the user settings.
	if (FPlatformProperties::HasFixedResolution())
	{
		// Always use the device's actual resolution that has been setup earlier
		FDisplayMetrics DisplayMetrics;
		FSlateApplication::Get().GetDisplayMetrics( DisplayMetrics );

		// We need to pass the resolution back out to GameUserSettings, or it will just override it again
		ResolutionX = DisplayMetrics.PrimaryDisplayWorkAreaRect.Right - DisplayMetrics.PrimaryDisplayWorkAreaRect.Left;
		ResolutionY = DisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom - DisplayMetrics.PrimaryDisplayWorkAreaRect.Top;
		FSystemResolution::RequestResolutionChange(ResolutionX, ResolutionY, EWindowMode::Fullscreen);
	}


	if (FParse::Param(FCommandLine::Get(),TEXT("Portrait")))
	{
		Swap(ResolutionX, ResolutionY);
	}
}

TSharedRef<SWindow> UGameEngine::CreateGameWindow()
{
	int32 ResX = GSystemResolution.ResX;
	int32 ResY = GSystemResolution.ResY;
	EWindowMode::Type WindowMode = GSystemResolution.WindowMode;
	ConditionallyOverrideSettings(ResX, ResY, WindowMode);

	// If the current settings have been overridden, apply them back into the system
	if (ResX != GSystemResolution.ResX || ResY != GSystemResolution.ResY || WindowMode != GSystemResolution.WindowMode)
	{
		FSystemResolution::RequestResolutionChange(ResX, ResY, WindowMode);
		GSystemResolution.ResX = ResX;
		GSystemResolution.ResY = ResY;
		GSystemResolution.WindowMode = WindowMode;
	}

#if PLATFORM_64BITS
	//These are invariant strings so they don't need to be localized
	const FText PlatformBits = FText::FromString( TEXT( "64" ) );
#else	//PLATFORM_64BITS
	const FText PlatformBits = FText::FromString( TEXT( "32" ) );
#endif	//PLATFORM_64BITS

	FFormatNamedArguments Args;
	Args.Add( TEXT("GameName"), FText::FromString( FApp::GetGameName() ) );
	Args.Add( TEXT("PlatformArchitecture"), PlatformBits );
	Args.Add( TEXT("RHIName"), FText::FromName( LegacyShaderPlatformToShaderFormat( GRHIShaderPlatform ) ) );

	const FText AppName = FText::Format( NSLOCTEXT("UnrealEd", "GameWindowTitle", "{GameName} ({PlatformArchitecture}-bit, {RHIName})"), Args );

	// Allow optional winX/winY parameters to set initial window position
	EAutoCenter::Type AutoCenterType = EAutoCenter::PrimaryWorkArea;
	int32 WinX=0;
	int32 WinY=0;
	if (FParse::Value(FCommandLine::Get(), TEXT("WinX="), WinX) && FParse::Value(FCommandLine::Get(), TEXT("WinY="), WinY))
	{
		AutoCenterType = EAutoCenter::None;
	}

	TSharedRef<SWindow> Window = SNew(SWindow)
	.ClientSize(FVector2D( ResX, ResY ))
	.Title( AppName )
	.AutoCenter(AutoCenterType)
	.ScreenPosition(FVector2D(WinX, WinY))
	.FocusWhenFirstShown(true)
	.UseOSWindowBorder(true);

	const bool bShowImmediately = false;

	FSlateApplication::Get().AddWindow( Window, bShowImmediately );
	
	// Do not set fullscreen mode here, since it doesn't take 
	// HMDDevice into account. The window mode will be set properly later
	// from SwitchGameWindowToUseGameViewport() method (see ResizeWindow call).
	Window->SetWindowMode(EWindowMode::Windowed);

	Window->ShowWindow();

	// Tick now to force a redraw of the window and ensure correct fullscreen application
	FSlateApplication::Get().Tick();

	return Window;
}

void UGameEngine::SwitchGameWindowToUseGameViewport()
{
	if ( ensure(GameViewportWindow.IsValid()) && GameViewportWindow.Pin()->GetContent() != GameViewportWidget )
	{
		if( !GameViewportWidget.IsValid() )
		{
			CreateGameViewport( GameViewport );
		}
		GameViewportWindow.Pin()->SetContent( GameViewportWidget.ToSharedRef() );
		SceneViewport->ResizeFrame((uint32)GSystemResolution.ResX, (uint32)GSystemResolution.ResY, GSystemResolution.WindowMode, 0, 0);

		// Move the registration of the game viewport to that messages are correctly received.
		if (!FPlatformProperties::SupportsWindowedMode())
		{
			FSlateApplication::Get().RegisterGameViewport( GameViewportWidget.ToSharedRef() );
		}
	}
}

void UGameEngine::OnGameWindowClosed( const TSharedRef<SWindow>& WindowBeingClosed )
{
	FSlateApplication::Get().UnregisterGameViewport();
	// This will shutdown the game
	GameViewport->CloseRequested( SceneViewport->GetViewport() );
	SceneViewport.Reset();
}

void UGameEngine::OnGameWindowMoved( const TSharedRef<SWindow>& WindowBeingMoved )
{
	const FSlateRect WindowRect = WindowBeingMoved->GetWindowGeometryInScreen().GetRect();
	GetGameUserSettings()->SetWindowPosition(WindowRect.Left, WindowRect.Top);
	GetGameUserSettings()->SaveConfig();
}

void UGameEngine::RedrawViewports( bool bShouldPresent /*= true*/ )
{
	SCOPE_CYCLE_COUNTER(STAT_RedrawViewports);

	if ( GameViewport != NULL )
	{
		GameViewport->LayoutPlayers();
		if ( GameViewport->Viewport != NULL )
		{
			GameViewport->Viewport->Draw(bShouldPresent);
		}
	}
}

/*-----------------------------------------------------------------------------
	Game init and exit.
-----------------------------------------------------------------------------*/
UEngine::UEngine(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	AsyncLoadingTimeLimit = 5.0f;
	bAsyncLoadingUseFullTimeLimit = true;
	PriorityAsyncLoadingExtraTime = 20.0f;

	C_WorldBox = FColor(0, 0, 40, 255);
	C_BrushWire = FColor(192, 0, 0, 255);
	C_AddWire = FColor(127, 127, 255, 255);
	C_SubtractWire = FColor(255, 127, 127, 255);
	C_SemiSolidWire = FColor(127, 255, 0, 255);
	C_NonSolidWire = FColor(63, 192, 32, 255);
	C_WireBackground = FColor(0, 0, 0, 255);
	C_ScaleBoxHi = FColor(223, 149, 157, 255);
	C_VolumeCollision = FColor(149, 223, 157, 255);
	C_BSPCollision = FColor(149, 157, 223, 255);
	C_OrthoBackground = FColor(30, 30, 30, 255);
	C_Volume = FColor(255, 196, 255, 255);
	C_BrushShape = FColor(128, 255, 128, 255);

	SelectionHighlightIntensity = 0.0f;
	BSPSelectionHighlightIntensity = 0.0f;
	HoverHighlightIntensity = 10.f;

	SelectionHighlightIntensityBillboards = 0.25f;

	bUseSound = true;
	bPendingHardwareSurveyResults = false;
	bIsInitialized = false;

	BeginStreamingPauseDelegate = NULL;
	EndStreamingPauseDelegate = NULL;
}

void UGameEngine::Init(IEngineLoop* InEngineLoop)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGameEngine Init"), STAT_GameEngineStartup, STATGROUP_LoadTime);

	// Call base.
	UEngine::Init(InEngineLoop);

#if USE_NETWORK_PROFILER
	FString NetworkProfilerTag;
	if( FParse::Value(FCommandLine::Get(), TEXT("NETWORKPROFILER="), NetworkProfilerTag ) )
	{
		GNetworkProfiler.EnableTracking(true);
	}
#endif

	// Load all of the engine modules that we need at startup that are not editor-related
	UGameEngine::LoadRuntimeEngineStartupModules();

	// Load and apply user game settings
	GetGameUserSettings()->LoadSettings();
	GetGameUserSettings()->ApplySettings(true);

	// Create game instance.  For GameEngine, this should be the only GameInstance that ever gets created.
	{
		FStringClassReference GameInstanceClassName = GetDefault<UGameMapsSettings>()->GameInstanceClass;
		UClass* GameInstanceClass = (GameInstanceClassName.IsValid() ? LoadObject<UClass>(NULL, *GameInstanceClassName.ToString()) : UGameInstance::StaticClass());
		
		GameInstance = ConstructObject<UGameInstance>(GameInstanceClass, this);

		GameInstance->Init();
	}
 
//  	// Creates the initial world context. For GameEngine, this should be the only WorldContext that ever gets created.
//  	FWorldContext& InitialWorldContext = CreateNewWorldContext(EWorldType::Game);

	// Initialize the viewport client.
	UGameViewportClient* ViewportClient = NULL;
	if(GIsClient)
	{
		ViewportClient = ConstructObject<UGameViewportClient>(GameViewportClientClass,this);
		ViewportClient->Init(*GameInstance->GetWorldContext(), GameInstance);
		GameViewport = ViewportClient;
 		GameInstance->GetWorldContext()->GameViewport = ViewportClient;
	}

	bCheckForMovieCapture = true;

	// Attach the viewport client to a new viewport.
	if(ViewportClient)
	{
		// This must be created before any gameplay code adds widgets
		bool bWindowAlreadyExists = GameViewportWindow.IsValid();
		if (!bWindowAlreadyExists)
		{
			GameViewportWindow = CreateGameWindow();
		}

		CreateGameViewport( ViewportClient );

		if( !bWindowAlreadyExists )
		{
			SwitchGameWindowToUseGameViewport();
		}
		FString Error;
		if(ViewportClient->SetupInitialLocalPlayer(Error) == NULL)
		{
			UE_LOG(LogEngine, Fatal,TEXT("%s"),*Error);
		}
	}

	GameInstance->StartGameInstance();


	UE_LOG(LogInit, Display, TEXT("Game Engine Initialized.") );

	// for IsInitialized()
	bIsInitialized = true;
}


void UGameEngine::PreExit()
{
	FAVIWriter* AVIWriter = FAVIWriter::GetInstance();
	if (AVIWriter)
	{
		AVIWriter->Close();
	}

	Super::PreExit();

	// Stop tracking, automatically flushes.
	NETWORK_PROFILER(GNetworkProfiler.EnableTracking(false));

	CancelAllPending();

	// Clean up all worlds
	for (int32 WorldIndex = 0; WorldIndex < WorldList.Num(); ++WorldIndex)
	{
		UWorld* const World = WorldList[WorldIndex].World();
		if ( World != NULL )
		{
			World->FlushLevelStreaming( NULL, true );
			World->CleanupWorld();
		}
	}
}

void UGameEngine::FinishDestroy()
{
	if ( !HasAnyFlags(RF_ClassDefaultObject) )
	{
		// Game exit.
		UE_LOG(LogExit, Log, TEXT("Game engine shut down") );
	}

	Super::FinishDestroy();
}


void UGameEngine::LoadRuntimeEngineStartupModules()
{
	// NOTE: These modules will be loaded when the game starts up, and also when the editor starts up.

	// We only want live streaming support if we're actually in a game
	if( !IsRunningDedicatedServer() && !IsRunningCommandlet() )
	{
		FModuleManager::Get().LoadModule( TEXT("GameLiveStreaming") );
	}

	// ... load other required engine runtime modules here (but NOT editor modules) ...
}


/*-----------------------------------------------------------------------------
	Command line executor.
-----------------------------------------------------------------------------*/

bool UGameEngine::Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar )
{
	if( FParse::Command( &Cmd,TEXT("REATTACHCOMPONENTS")) || FParse::Command( &Cmd,TEXT("REREGISTERCOMPONENTS")))
	{
		return HandleReattachComponentsCommand( Cmd, Ar );
	}
	// exec to 
	else if( FParse::Command( &Cmd,TEXT("MOVIE")) )
	{
		return HandleMovieCommand( Cmd, Ar );
	}
	else if( FParse::Command( &Cmd,TEXT("EXIT")) || FParse::Command(&Cmd,TEXT("QUIT")))
	{
		if ( FPlatformProperties::SupportsQuit() )
		{
			return HandleExitCommand( Cmd, Ar );
		}
		else
		{
			// ignore command on xbox one and ps4 as it will cause a crash
			// ttp:321126
			return true;
		}
	}
	else if( FParse::Command( &Cmd, TEXT("GETMAXTICKRATE") ) )
	{
		return HandleGetMaxTickRateCommand( Cmd, Ar );
	}
	else if( FParse::Command( &Cmd, TEXT("CANCEL") ) )
	{
		return HandleCancelCommand( Cmd, Ar, InWorld );	
	}
#if !UE_BUILD_SHIPPING
	else if( FParse::Command( &Cmd, TEXT("ApplyUserSettings") ) )
	{
		return HandleApplyUserSettingsCommand( Cmd, Ar );
	}
#endif // !UE_BUILD_SHIPPING
	else if( InWorld && InWorld->Exec( InWorld, Cmd, Ar ) )
	{
		return true;
	}
	else if( InWorld && InWorld->GetAuthGameMode() && InWorld->GetAuthGameMode()->ProcessConsoleExec(Cmd,Ar,NULL) )
	{
		return true;
	}
	else
	{
#if UE_BUILD_SHIPPING
		// disallow set of actor properties if network game
		if ((FParse::Command( &Cmd, TEXT("SET")) || FParse::Command( &Cmd, TEXT("SETNOPEC"))))
		{
			FWorldContext &Context = GetWorldContextFromWorldChecked(InWorld);
			if( Context.PendingNetGame != NULL || InWorld->GetNetMode() != NM_Standalone)
			{
				return true;
			}
			// the effects of this cannot be easily reversed, so prevent the user from playing network games without restarting to avoid potential exploits
			GDisallowNetworkTravel = true;
		}
#endif // UE_BUILD_SHIPPING
		if (UEngine::Exec(InWorld, Cmd, Ar))
		{
			return true;
		}
		else if (UPlatformInterfaceBase::StaticExec(Cmd, Ar))
		{
			return true;
		}
	
		return false;
	}
}


bool UGameEngine::HandleReattachComponentsCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	UClass* Class=NULL;
	if( ParseObject<UClass>( Cmd, TEXT("CLASS="), Class, ANY_PACKAGE ) &&
		Class->IsChildOf(UActorComponent::StaticClass()) )
	{
		for( FObjectIterator It(Class); It; ++It )
		{
			UActorComponent* ActorComponent = Cast<UActorComponent>(*It);
			if( ActorComponent )
			{
				FComponentReregisterContext Reregister(ActorComponent);
			}
		}
	}
	return true;
}

bool UGameEngine::HandleMovieCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	FString MovieCmd = FParse::Token(Cmd,0);
	if( MovieCmd.Contains(TEXT("PLAY")) )
	{
		for( TObjectIterator<UTextureMovie> It; It; ++It )
		{
			UTextureMovie* Movie = *It;
			Movie->Play();
		}
	}
	else if( MovieCmd.Contains(TEXT("PAUSE")) )
	{
		for( TObjectIterator<UTextureMovie> It; It; ++It )
		{
			UTextureMovie* Movie = *It;
			Movie->Pause();
		}
	}
	else if( MovieCmd.Contains(TEXT("STOP")) )
	{
		for( TObjectIterator<UTextureMovie> It; It; ++It )
		{
			UTextureMovie* Movie = *It;
			Movie->Stop();
		}
	}
	return true;
}

bool UGameEngine::HandleExitCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	for (int32 WorldIndex = 0; WorldIndex < WorldList.Num(); ++WorldIndex)
	{
		UWorld* const World = WorldList[WorldIndex].World();

		// Cancel any pending connection to a server
		CancelPending(WorldList[WorldIndex]);

		// Shut down any existing game connections
		ShutdownWorldNetDriver(World);
	}

	Ar.Log( TEXT("Closing by request") );
	FPlatformMisc::RequestExit( 0 );
	return true;
}

bool UGameEngine::HandleGetMaxTickRateCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	Ar.Logf( TEXT("%f"), GetMaxTickRate(0,false) );
	return true;
}

bool UGameEngine::HandleCancelCommand( const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld )
{
	CancelPending(GetWorldContextFromWorldChecked(InWorld));
	return true;
}

#if !UE_BUILD_SHIPPING
bool UGameEngine::HandleApplyUserSettingsCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	GetGameUserSettings()->ApplySettings(false);
	return true;
}
#endif // !UE_BUILD_SHIPPING

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UGameEngine::PostLoadMap()
{
}

float UGameEngine::GetMaxTickRate( float DeltaTime, bool bAllowFrameRateSmoothing )
{
	float MaxTickRate = 0.f;

	if (FPlatformProperties::SupportsWindowedMode() == false && !IsRunningDedicatedServer())
	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.VSync"));
		// Limit framerate on console if VSYNC is enabled to avoid jumps from 30 to 60 and back.
		if( CVar->GetValueOnGameThread() != 0 )
		{
			if (SmoothedFrameRateRange.HasUpperBound())
			{
				MaxTickRate = SmoothedFrameRateRange.GetUpperBoundValue();
			}
		}
	}
	else 
	{
		UWorld* World = NULL;

		for (int32 WorldIndex = 0; WorldIndex < WorldList.Num(); ++WorldIndex)
		{
			if (WorldList[WorldIndex].WorldType == EWorldType::Game)
			{
				World = WorldList[WorldIndex].World();
				break;
			}
		}

		if( World )
		{
			UNetDriver* NetDriver = World->GetNetDriver();
			// In network games, limit framerate to not saturate bandwidth.
			if( NetDriver && (NetDriver->GetNetMode() == NM_DedicatedServer || (NetDriver->GetNetMode() == NM_ListenServer && NetDriver->bClampListenServerTickRate)))
			{
				// We're a dedicated server, use the LAN or Net tick rate.
				MaxTickRate = FMath::Clamp( NetDriver->NetServerMaxTickRate, 10, 120 );
			}
			/*else if( NetDriver && NetDriver->ServerConnection )
			{
				if( NetDriver->ServerConnection->CurrentNetSpeed <= 10000 )
				{
					MaxTickRate = FMath::Clamp( MaxTickRate, 10.f, 90.f );
				}
			}*/
		}
	}

	// See if the code in the base class wants to replace this
	float SuperTickRate = Super::GetMaxTickRate(DeltaTime, bAllowFrameRateSmoothing);
	if(SuperTickRate != 0.0)
	{
		MaxTickRate = SuperTickRate;
	}

	return MaxTickRate;
}

void UGameEngine::Tick( float DeltaSeconds, bool bIdleMode )
{
	SCOPE_CYCLE_COUNTER(STAT_GameEngineTick);
	NETWORK_PROFILER(GNetworkProfiler.TrackFrameBegin());

	int32 LocalTickCycles=0;
	CLOCK_CYCLES(LocalTickCycles);
	
	// -----------------------------------------------------
	// Non-World related stuff
	// -----------------------------------------------------

	if( DeltaSeconds < 0.0f )
	{
#if (UE_BUILD_SHIPPING && WITH_EDITOR)
		// End users don't have access to the secure parts of UDN.  Regardless, they won't
		// need the warning because the game ships with AMD drivers that address the issue.
		UE_LOG(LogEngine, Fatal,TEXT("Negative delta time!"));
#else
		// Send developers to the support list thread.
		UE_LOG(LogEngine, Fatal,TEXT("Negative delta time! Please see https://udn.epicgames.com/lists/showpost.php?list=ue3bugs&id=4364"));
#endif
	}

	// Tick the module manager
	FModuleManager::Get().Tick();

	if (!IsRunningDedicatedServer() && !IsRunningCommandlet())
	{
		// Clean up the game viewports that have been closed.
		CleanupGameViewport();
	}

	// If all viewports closed, time to exit.
	if(GIsClient && GameViewport == NULL )
	{
		UE_LOG(LogEngine, Log,  TEXT("All Windows Closed") );
		FPlatformMisc::RequestExit( 0 );
		return;
	}

	if ( GameViewport != NULL )
	{
		// Decide whether to drop high detail because of frame rate.
		GameViewport->SetDropDetail(DeltaSeconds);
	}

	// Update subsystems.
	{
		// This assumes that UObject::StaticTick only calls ProcessAsyncLoading.
		StaticTick(DeltaSeconds, bAsyncLoadingUseFullTimeLimit, AsyncLoadingTimeLimit / 1000.f);
	}

	// -----------------------------------------------------
	// Begin ticking worlds
	// -----------------------------------------------------

	FName OriginalGWorldContext = NAME_None;
	for (int32 i=0; i < WorldList.Num(); ++i)
	{
		if (WorldList[i].World() == GWorld)
		{
			OriginalGWorldContext = WorldList[i].ContextHandle;
			break;
		}
	}

	bool WorldWasPaused = false;

	for (int32 WorldIdx = 0; WorldIdx < WorldList.Num(); ++WorldIdx)
	{
		FWorldContext &Context = WorldList[WorldIdx];
		if (Context.World() == NULL)
		{
			continue;
		}

		WorldWasPaused |= Context.World()->IsPaused();

		GWorld = Context.World();

		// Tick all travel and Pending NetGames (Seamless, server, client)
		TickWorldTravel(Context, DeltaSeconds);

		if (!IsRunningDedicatedServer() && !IsRunningCommandlet())
		{
			// Only update reflection captures in game once all 'always loaded' levels have been loaded
			// This won't work with actual level streaming though
			if (Context.World()->AreAlwaysLoadedLevelsLoaded())
			{
				// Update sky light first because it's considered direct lighting, sky diffuse will be visible in reflection capture indirect specular
				USkyLightComponent::UpdateSkyCaptureContents(Context.World());
				UReflectionCaptureComponent::UpdateReflectionCaptureContents(Context.World());
			}
		}

		if (!bIdleMode)
		{
			// Tick the world.
			GameCycles=0;
			CLOCK_CYCLES(GameCycles);
			Context.World()->Tick( LEVELTICK_All, DeltaSeconds );
			UNCLOCK_CYCLES(GameCycles);
		}

		// Issue cause event after first tick to provide a chance for the game to spawn the player and such.
		if( Context.World()->bWorldWasLoadedThisTick )
		{
			Context.World()->bWorldWasLoadedThisTick = false;
			
			const TCHAR* InitialExec = Context.LastURL.GetOption(TEXT("causeevent="),NULL);
			ULocalPlayer* GamePlayer = Context.OwningGameInstance ? Context.OwningGameInstance->GetFirstGamePlayer() : NULL;
			if( InitialExec && GamePlayer )
			{
				UE_LOG(LogEngine, Log, TEXT("Issuing initial cause event passed from URL: %s"), InitialExec);
				GamePlayer->Exec( GamePlayer->GetWorld(), *(FString("CAUSEEVENT ") + InitialExec), *GLog );
			}

			Context.World()->bTriggerPostLoadMap = true;
		}

		// Tick the viewports.
		if ( GameViewport != NULL && !bIdleMode )
		{
			SCOPE_CYCLE_COUNTER(STAT_GameViewportTick);
			GameViewport->Tick(DeltaSeconds);
		}
	
		UpdateTransitionType(Context.World());
	
		// fixme: this will only happen once due to the static bool, but still need to figure out how to handle this for multiple worlds
		if (FPlatformProperties::SupportsWindowedMode())
		{
			// Hide the splashscreen and show the game window
			static bool bFirstTime = true;
			if ( bFirstTime )
			{
				bFirstTime = false;
				FPlatformSplash::Hide();
				if ( GameViewportWindow.IsValid() )
				{
					GameViewportWindow.Pin()->ShowWindow();
					FSlateApplication::Get().RegisterGameViewport( GameViewportWidget.ToSharedRef() );
				}
			}
		}

		if (!bIdleMode && !IsRunningDedicatedServer() && !IsRunningCommandlet())
		{
			// Render everything.
			RedrawViewports();
		}

		// Block on async loading if requested.
		if( Context.World()->bRequestedBlockOnAsyncLoading )
		{
			// Only perform work if there is anything to do. This ensures we are not syncronizing with the GPU
			// and suspending the device needlessly.
			bool bWorkToDo = IsAsyncLoading();
			if (!bWorkToDo)
			{
				Context.World()->UpdateLevelStreaming();
				bWorkToDo = Context.World()->IsVisibilityRequestPending();
			}
			if (bWorkToDo)
			{
				// tell clients to do the same so they don't fall behind
				for( FConstPlayerControllerIterator Iterator = Context.World()->GetPlayerControllerIterator(); Iterator; ++Iterator )
				{
					APlayerController* PlayerController = *Iterator;
					UNetConnection* Conn = Cast<UNetConnection>(PlayerController->Player);
					if (Conn != NULL && Conn->GetUChildConnection() == NULL)
					{
						// call the event to replicate the call
						PlayerController->ClientSetBlockOnAsyncLoading();
						// flush the connection to make sure it gets sent immediately
						Conn->FlushNet(true);
					}
				}

				if( GameViewport && BeginStreamingPauseDelegate && BeginStreamingPauseDelegate->IsBound() )
				{
					BeginStreamingPauseDelegate->Execute( GameViewport->Viewport );
				}	

				// Flushes level streaming requests, blocking till completion.
				Context.World()->FlushLevelStreaming();

				if( EndStreamingPauseDelegate && EndStreamingPauseDelegate->IsBound() )
				{
					EndStreamingPauseDelegate->Execute( );
				}	
			}
			Context.World()->bRequestedBlockOnAsyncLoading = false;
		}

		// streamingServer
		if( GIsServer == true )
		{
			SCOPE_CYCLE_COUNTER(STAT_UpdateLevelStreaming);
			Context.World()->UpdateLevelStreaming();
		}

		// Update Audio. This needs to occur after rendering as the rendering code updates the listener position.
		if( GetAudioDevice() )
		{
			GetAudioDevice()->Update( !Context.World()->IsPaused() );
		}

	

		if( GIsClient )
		{
			// IStreamingManager is updated outside of a world context. For now, assuming it needs to tick here, before possibly calling PostLoadMap. 
			// Will need to take another look when trying to support multiple worlds.

			// Update resource streaming after viewports have had a chance to update view information. Normal update.
			IStreamingManager::Get().Tick( DeltaSeconds );

			if ( Context.World()->bTriggerPostLoadMap )
			{
				Context.World()->bTriggerPostLoadMap = false;

				// Turns off the loading movie (if it was turned on by LoadMap) and other post-load cleanup.
				PostLoadMap();
			}
		}

		UNCLOCK_CYCLES(LocalTickCycles);
		TickCycles=LocalTickCycles;

		// See whether any map changes are pending and we requested them to be committed.
		ConditionalCommitMapChange(Context);
	}

	// ----------------------------
	//	End per-world ticking
	// ----------------------------

	// Restore original GWorld*. This will go away one day.
	if (OriginalGWorldContext != NAME_None)
	{
		GWorld = GetWorldContextFromHandleChecked(OriginalGWorldContext).World();
	}

	// tell renderer about GWorld->IsPaused(), before rendering
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			SetPaused,
			bool, bGamePaused, WorldWasPaused,
		{
			GRenderingRealtimeClock.SetGamePaused(bGamePaused);
		});
	}


	// rendering thread commands
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			TickRenderingTimer,
			bool, bPauseRenderingRealtimeClock, GPauseRenderingRealtimeClock,
			float, DeltaTime, DeltaSeconds,
		{
			if(!bPauseRenderingRealtimeClock)
			{
				// Tick the GRenderingRealtimeClock, unless it's paused
				GRenderingRealtimeClock.Tick(DeltaTime);
			}

			GetRendererModule().TickRenderTargetPool();
		});
	}
	
	// Skip AVI work in environments where there is no rendering
	if (!IsRunningDedicatedServer() && !IsRunningCommandlet())
	{
		FAVIWriter* AVIWriter = FAVIWriter::GetInstance();
		if (AVIWriter)
		{
			AVIWriter->Update(DeltaSeconds);
		}

		// Start the movie capture if needed
		if (bCheckForMovieCapture && GEngine->bStartWithMatineeCapture && GEngine->MatineeCaptureType == EMatineeCaptureType::AVI && GameViewport->Viewport->GetSizeXY() != FIntPoint::ZeroValue )
		{
			if (AVIWriter)
			{
				AVIWriter->StartCapture(GameViewport->Viewport);
			}
			bCheckForMovieCapture = false;
		}
	}
}


void UGameEngine::ProcessToggleFreezeCommand( UWorld* InWorld )
{
	if (GameViewport)
	{
		GameViewport->Viewport->ProcessToggleFreezeCommand();
	}
}


void UGameEngine::ProcessToggleFreezeStreamingCommand(UWorld* InWorld)
{
	// if not already frozen, then flush async loading before we freeze so that we don't mess up any in-process streaming
	if (!InWorld->bIsLevelStreamingFrozen)
	{
		FlushAsyncLoading();
	}

	// toggle the frozen state
	InWorld->bIsLevelStreamingFrozen = !InWorld->bIsLevelStreamingFrozen;
}


UWorld* UGameEngine::GetGameWorld()
{
	for (auto It = WorldList.CreateConstIterator(); It; ++It)
	{
		const FWorldContext& Context = *It;
        // Explicitly not checking for PIE worlds here, this should only 
        // be called outside of editor (and thus is in UGameEngine
		if (Context.WorldType == EWorldType::Game && Context.World())
		{
			return Context.World();
		}
	}

	return NULL;
}
