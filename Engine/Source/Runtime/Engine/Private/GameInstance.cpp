// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GameInstance.cpp: Implementation of GameInstance class
=============================================================================*/


#include "EnginePrivate.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#endif


UGameInstance::UGameInstance(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

UWorld* UGameInstance::GetWorld() const
{
	return WorldContext ? WorldContext->World() : NULL;
}

UEngine* UGameInstance::GetEngine() const
{
	return CastChecked<UEngine>(GetOuter());
}

void UGameInstance::Init()
{
	UEngine* const Engine = GetEngine();

	// Creates the world context. This should be the only WorldContext that ever gets created for this GameInstance.
	WorldContext = &Engine->CreateNewWorldContext(EWorldType::Game);
	WorldContext->OwningGameInstance = this;
}

#if WITH_EDITOR
bool UGameInstance::InitPIE(bool bAnyBlueprintErrors, int32 PIEInstance)
{
	UEditorEngine* const EditorEngine = CastChecked<UEditorEngine>(GetEngine());

	WorldContext = &EditorEngine->CreateNewWorldContext(EWorldType::PIE);
	WorldContext->OwningGameInstance = this;
	WorldContext->PIEInstance = PIEInstance;

	const FString WorldPackageName = EditorEngine->EditorWorld->GetOutermost()->GetName();

	// Establish World Context for PIE World
	WorldContext->LastURL.Map = WorldPackageName;
	WorldContext->PIEPrefix = UWorld::BuildPIEPackagePrefix(PIEInstance);

	const ULevelEditorPlaySettings* PlayInSettings = GetDefault<ULevelEditorPlaySettings>();

	// We always need to create a new PIE world unless we're using the editor world for SIE
	UWorld* NewWorld = nullptr;

	if (PlayInSettings->PlayNetMode == PIE_Client)
	{
		// We are going to connect, so just load an empty world
		NewWorld = EditorEngine->CreatePIEWorldFromEntry(*WorldContext, EditorEngine->EditorWorld, PIEMapName);
	}
	else if (PlayInSettings->PlayNetMode == PIE_ListenServer && !PlayInSettings->RunUnderOneProcess)
	{
		// We *have* to save the world to disk in order to be a listen server that allows other processes to connect.
		// Otherwise, clients would not be able to load the world we are using
		NewWorld = EditorEngine->CreatePIEWorldBySavingToTemp(*WorldContext, EditorEngine->EditorWorld, PIEMapName);
	}
	else
	{
		// Standard PIE path: just duplicate the EditorWorld
		NewWorld = EditorEngine->CreatePIEWorldByDuplication(*WorldContext, EditorEngine->EditorWorld, PIEMapName);
	}

	// failed to create the world!
	if (NewWorld == nullptr)
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Error_FailedCreateEditorPreviewWorld", "Failed to create editor preview world."));
		return false;
	}

	NewWorld->SetGameInstance(this);
	WorldContext->SetCurrentWorld(NewWorld);
	WorldContext->AddRef(EditorEngine->PlayWorld);	// Tie this context to this UEngine::PlayWorld*		// @fixme, needed still?

	// make sure we can clean up this world!
	NewWorld->ClearFlags(RF_Standalone);
	NewWorld->bKismetScriptError = bAnyBlueprintErrors;

	return true;
}


/** Initializes streaming levels when PIE starts up based on the players start location */
static void InitStreamingLevelsForPIEStartup(UGameViewportClient* GameViewportClient, UWorld* PlayWorld)
{
	check(PlayWorld);
	ULocalPlayer* const Player = PlayWorld->GetFirstLocalPlayerFromController();
	if (Player)
	{
		// Create a view family for the game viewport
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
			GameViewportClient->Viewport,
			PlayWorld->Scene,
			GameViewportClient->EngineShowFlags)
			.SetRealtimeUpdate(true));


		// Calculate a view where the player is to update the streaming from the players start location
		FVector ViewLocation;
		FRotator ViewRotation;
		Player->CalcSceneView(&ViewFamily, /*out*/ ViewLocation, /*out*/ ViewRotation, GameViewportClient->Viewport);

		// Update level streaming.
		PlayWorld->FlushLevelStreaming(&ViewFamily);
	}
}


bool UGameInstance::StartPIEGameInstance(ULocalPlayer* LocalPlayer, bool bInSimulateInEditor, bool bAnyBlueprintErrors, bool bStartInSpectatorMode)
{
	UEditorEngine* const EditorEngine = CastChecked<UEditorEngine>(GetEngine());
	ULevelEditorPlaySettings const* PlayInSettings = GetDefault<ULevelEditorPlaySettings>();

	// for clients, just connect to the server
	if (PlayInSettings->PlayNetMode == PIE_Client)
	{
		FString Error;
		FURL BaseURL = WorldContext->LastURL;
		if (EditorEngine->Browse(*WorldContext, FURL(&BaseURL, TEXT("127.0.0.1"), (ETravelType)TRAVEL_Absolute), Error) == EBrowseReturnVal::Pending)
		{
			EditorEngine->TransitionType = TT_WaitingToConnect;
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("UnrealEd", "Error_CouldntLaunchPIEClient", "Couldn't Launch PIE Client: {0}"), FText::FromString(Error)));
			return false;
		}
	}
	else
	{
		// we're going to be playing in the current world, get it ready for play
		UWorld* const PlayWorld = GetWorld();

		// make a URL
		FURL URL;
		// If the user wants to start in spectator mode, do not use the custom play world for now
		if (EditorEngine->UserEditedPlayWorldURL.Len() > 0 && !bStartInSpectatorMode)
		{
			// If the user edited the play world url. Verify that the map name is the same as the currently loaded map.
			URL = FURL(NULL, *EditorEngine->UserEditedPlayWorldURL, TRAVEL_Absolute);
			if (URL.Map != PIEMapName)
			{
				// Ensure the URL map name is the same as the generated play world map name.
				URL.Map = PIEMapName;
			}
		}
		else
		{
			// The user did not edit the url, just build one from scratch.
			URL = FURL(NULL, *EditorEngine->BuildPlayWorldURL(*PIEMapName, bStartInSpectatorMode), TRAVEL_Absolute);
		}

		// If a start location is specified, spawn a temporary PlayerStartPIE actor at the start location and use it as the portal.
		AActor* PlayerStart = NULL;
		if (EditorEngine->SpawnPlayFromHereStart(PlayWorld, PlayerStart, EditorEngine->PlayWorldLocation, EditorEngine->PlayWorldRotation) == false)
		{
			// failed to create "play from here" playerstart
			FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Error_FailedCreatePlayFromHerePlayerStart", "Failed to create PlayerStart at desired starting location."));
			return false;
		}

		if (!PlayWorld->SetGameMode(URL))
		{
			// Setting the game mode failed so bail 
			FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Error_FailedCreateEditorPreviewWorld", "Failed to create editor preview world."));
			return false;
		}

		// Navigate PIE world to the Editor world origin location
		// and stream in all relevant levels around that location
		if (PlayWorld->WorldComposition)
		{
			PlayWorld->NavigateTo(PlayWorld->GlobalOriginOffset);
		}

		UNavigationSystem::InitializeForWorld(PlayWorld, WorldContext->GamePlayers.Num() > 0 ? FNavigationSystem::PIEMode : FNavigationSystem::SimulationMode);
		PlayWorld->CreateAISystem();

		PlayWorld->InitializeActorsForPlay(URL);

		// @todo, just use WorldContext.GamePlayer[0]?
		if (LocalPlayer)
		{
			FString Error;
			if (!LocalPlayer->SpawnPlayActor(URL.ToString(1), Error, PlayWorld))
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("UnrealEd", "Error_CouldntSpawnPlayer", "Couldn't spawn player: {0}"), FText::FromString(Error)));
				return nullptr;
			}
		}

		static bool bDoBeginPlayHere = true;
		if (bDoBeginPlayHere)
			PlayWorld->BeginPlay();

		UGameViewportClient* const GameViewport = GetGameViewportClient();
		if (GameViewport != NULL && GameViewport->Viewport != NULL)
		{
			// Stream any levels now that need to be loaded before the game starts
			InitStreamingLevelsForPIEStartup(GameViewport, PlayWorld);
		}
		
		if (PlayInSettings->PlayNetMode == PIE_ListenServer)
		{
			// start listen server with the built URL
			PlayWorld->Listen(URL);
		}
	}

	return true;
}
#endif


UGameViewportClient* UGameInstance::GetGameViewportClient() const
{
	FWorldContext* const WC = GetWorldContext();
	return WC ? WC->GameViewport : nullptr;
}

void UGameInstance::StartGameInstance()
{
	UEngine* const Engine = GetEngine();

	// Create default URL.
	// @note: if we change how we determine the valid start up map update LaunchEngineLoop's GetStartupMap()
	FURL DefaultURL;
	DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);

	// Enter initial world.
	EBrowseReturnVal::Type BrowseRet = EBrowseReturnVal::Failure;
	FString Error;
	TCHAR Parm[4096] = TEXT("");
	const TCHAR* Tmp = FCommandLine::Get();

#if UE_BUILD_SHIPPING
	// In shipping don't allow an override
	Tmp = TEXT("");
#endif // UE_BUILD_SHIPPING

	const UGameMapsSettings* GameMapsSettings = GetDefault<UGameMapsSettings>();
	const FString& DefaultMap = GameMapsSettings->GetGameDefaultMap();
	if (!FParse::Token(Tmp, Parm, ARRAY_COUNT(Parm), 0) || Parm[0] == '-')
	{
		FCString::Strcpy(Parm, *(DefaultMap + GameMapsSettings->LocalMapOptions));
	}

	FURL URL(&DefaultURL, Parm, TRAVEL_Partial);
	if (URL.Valid)
	{
		BrowseRet = Engine->Browse(*WorldContext, URL, Error);
	}

	// If waiting for a network connection, go into the starting level.
	if (BrowseRet != EBrowseReturnVal::Success && FCString::Stricmp(Parm, *DefaultMap) != 0)
	{
		const FText Message = FText::Format(NSLOCTEXT("Engine", "MapNotFound", "The map specified on the commandline '{0}' could not be found. Would you like to load the default map instead?"), FText::FromString(URL.Map));

		// the map specified on the command-line couldn't be loaded.  ask the user if we should load the default map instead
		if (FCString::Stricmp(*URL.Map, *DefaultMap) != 0 &&
			FMessageDialog::Open(EAppMsgType::OkCancel, Message) != EAppReturnType::Ok)
		{
			// user canceled (maybe a typo while attempting to run a commandlet)
			FPlatformMisc::RequestExit(false);
			return;
		}
		else
		{
			BrowseRet = Engine->Browse(*WorldContext, FURL(&DefaultURL, *(DefaultMap + GameMapsSettings->LocalMapOptions), TRAVEL_Partial), Error);
		}
	}

	// Handle failure.
	if (BrowseRet != EBrowseReturnVal::Success)
	{
		UE_LOG(LogLoad, Fatal, TEXT("%s"), *FString::Printf(TEXT("Failed to enter %s: %s. Please check the log for errors."), Parm, *Error));
	}

}


bool UGameInstance::HandleOpenCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld)
{
	check(WorldContext && WorldContext->World() == InWorld);

	UEngine* const Engine = GetEngine();

	FURL TestURL(&WorldContext->LastURL, Cmd, TRAVEL_Absolute);
	if (TestURL.IsLocalInternal())
	{
		// make sure the file exists if we are opening a local file
		if (!Engine->MakeSureMapNameIsValid(TestURL.Map))
		{
			Ar.Logf(TEXT("ERROR: The map '%s' does not exist."), *TestURL.Map);
			return true;
		}
	}

	Engine->SetClientTravel(InWorld, Cmd, TRAVEL_Absolute);
	return true;
}

bool UGameInstance::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	// @todo a bunch of stuff in UEngine probably belongs here as well
	if (FParse::Command(&Cmd, TEXT("OPEN")))
	{
		return HandleOpenCommand(Cmd, Ar, InWorld);
	}

	return false;
}


APlayerController* UGameInstance::GetFirstLocalPlayerController() const
{
	if (WorldContext)
	{
		for (ULocalPlayer* Player : WorldContext->GamePlayers)
		{
			if (Player && Player->PlayerController)
			{
				// return first non-null entry
				return Player->PlayerController;
			}
		}
	}

	// didn't find one
	return nullptr;
}

ULocalPlayer* UGameInstance::GetFirstGamePlayer() const
{
	return (WorldContext && WorldContext->GamePlayers.Num() > 0) ? WorldContext->GamePlayers[0] : nullptr;
}

