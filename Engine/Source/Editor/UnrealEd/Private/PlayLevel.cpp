// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "SoundDefinitions.h"
#include "LevelUtils.h"
#include "BusyCursor.h"
#include "ScopedTransaction.h"
#include "Database.h"
#include "PackageTools.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "BlueprintUtilities.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/LevelEditor/Public/SLevelViewport.h"
#include "Toolkits/AssetEditorManager.h"
#include "Toolkits/ToolkitManager.h"
#include "BlueprintEditorModule.h"
#include "TargetPlatform.h"
#include "MainFrame.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"
#include "LauncherServices.h"
#include "Settings.h"
#include "TargetDeviceServices.h"
#include "GameProjectGenerationModule.h"
#include "SourceCodeNavigation.h"

DEFINE_LOG_CATEGORY_STATIC(LogPlayLevel, Log, All);

#define LOCTEXT_NAMESPACE "PlayLevel"

void UEditorEngine::EndPlayMap()
{
	// Matinee must be closed before PIE can stop - matinee during PIE will be editing a PIE-world actor
	if( GEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_InterpEdit) )
	{
		FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "PIENeedsToCloseMatineeMessage", "Closing 'Play in Editor' must close UnrealMatinee.") );
		GEditorModeTools().DeactivateMode( FBuiltinEditorModes::EM_InterpEdit );
	}

	EndPlayOnLocalPc();

	const FScopedBusyCursor BusyCursor;
	check(PlayWorld);

	// Enable screensavers when ending PIE.
	EnableScreenSaver( true );

	// Make a list of all the actors that should be selected
	TArray<UObject *> SelectedActors;
	if ( ActorsThatWereSelected.Num() > 0 )
	{
		for ( int32 ActorIndex = 0; ActorIndex < ActorsThatWereSelected.Num(); ++ActorIndex )
		{
			TWeakObjectPtr<AActor> Actor = ActorsThatWereSelected[ ActorIndex ].Get();
			if (Actor.IsValid())
			{
				SelectedActors.Add( Actor.Get() );
			}
		}
		ActorsThatWereSelected.Empty();
	}
	else
	{
		for ( FSelectionIterator It( GetSelectedActorIterator() ); It; ++It )
		{
			AActor* Actor = static_cast<AActor*>( *It );
			if (Actor)
			{
				checkSlow( Actor->IsA(AActor::StaticClass()) );

				AActor* EditorActor = EditorUtilities::GetEditorWorldCounterpartActor(Actor);
				if (EditorActor)
				{
					SelectedActors.Add( EditorActor );
				}
			}
		}
	}

	// Deselect all objects, to avoid problems caused by property windows still displaying
	// properties for an object that gets garbage collected during the PIE clean-up phase.
	GEditor->SelectNone( true, true, false );
	GetSelectedActors()->DeselectAll();
	GetSelectedObjects()->DeselectAll();

	// For every actor that was selected previously, make sure it's editor equivalent is selected
	for ( int32 ActorIndex = 0; ActorIndex < SelectedActors.Num(); ++ActorIndex )
	{
		AActor* Actor = Cast<AActor>( SelectedActors[ ActorIndex ] );
		if (Actor)
		{
			SelectActor( Actor, true, false );
		}
	}

	// let the editor know
	FEditorDelegates::EndPIE.Broadcast(bIsSimulatingInEditor);

	// clean up any previous Play From Here sessions
	if ( GameViewport != NULL && GameViewport->Viewport != NULL )
	{
		GameViewport->CloseRequested(GameViewport->Viewport);
	}
	CleanupGameViewport();


	// find objects like Textures in the playworld levels that won't get garbage collected as they are marked RF_Standalone
	for( FObjectIterator It; It; ++It )
	{
		UObject* Object = *It;
		if( Object->HasAnyFlags(RF_Standalone) && (Object->GetOutermost()->PackageFlags & PKG_PlayInEditor)  )
		{
			// Clear RF_Standalone flag from objects in the levels used for PIE so they get cleaned up.
			Object->ClearFlags(RF_Standalone);
		}
	}

	// Clean up each world individually
	for (int32 WorldIdx = WorldList.Num()-1; WorldIdx >= 0; --WorldIdx)
	{
		FWorldContext &ThisContext = WorldList[WorldIdx];
		if (ThisContext.WorldType == EWorldType::PIE)
		{
			TeardownPlaySession(ThisContext);
			WorldList.RemoveAt(WorldIdx);
		}
	}
	
	{
		// Clear out viewport index
		PlayInEditorViewportIndex = -1; 

		// The undo system may have a reference to a SIE/PIE object that is about to be destroyed, so clear the transactions
		ResetTransaction( NSLOCTEXT("UnrealEd", "EndPlayMap", "End Play Map") );

		// We could have been toggling back and forth between simulate and pie before ending the play map
		// Make sure the property windows are cleared of any pie actors
		GUnrealEd->UpdateFloatingPropertyWindows();

		// Clean up any pie actors being referenced 
		GEngine->BroadcastLevelActorsChanged();
	}

	// Lose the EditorWorld pointer (this is only maintained while PIEing)
#if WITH_NAVIGATION_GENERATOR
	if (EditorWorld->GetNavigationSystem())
	{
		EditorWorld->GetNavigationSystem()->OnPIEEnd();
	}
#endif // WITH_NAVIGATION_GENERATOR

	EditorWorld->bAllowAudioPlayback = true;
	EditorWorld = NULL;

	// Garbage Collect
	CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );

	// Make sure that all objects in the temp levels were entirely garbage collected.
	for( FObjectIterator ObjectIt; ObjectIt; ++ObjectIt )
	{
		UObject* Object = *ObjectIt;
		if( Object->GetOutermost()->PackageFlags & PKG_PlayInEditor )
		{
			UWorld* TheWorld = UWorld::FindWorldInPackage(Object->GetOutermost());
			if ( TheWorld )
			{
				StaticExec(GWorld, *FString::Printf(TEXT("OBJ REFS CLASS=WORLD NAME=%s"), *TheWorld->GetPathName()));
			}
			else
			{
				UE_LOG(LogPlayLevel, Error, TEXT("No PIE world was found when attempting to gather references after GC."));
			}

			TMap<UObject*,UProperty*>	Route		= FArchiveTraceRoute::FindShortestRootPath( Object, true, GARBAGE_COLLECTION_KEEPFLAGS );
			FString						ErrorString	= FArchiveTraceRoute::PrintRootPath( Route, Object );

			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Path"), FText::FromString(ErrorString));
				
			// We cannot safely recover from this.
			FMessageLog("PIE").CriticalError()
				->AddToken(FUObjectToken::Create(Object, FText::FromString(Object->GetFullName())))
				->AddToken(FTextToken::Create(FText::Format(LOCTEXT("PIEObjectStillReferenced", "Object from PIE level still referenced. Shortest path from root: {Path}"), Arguments)));
		}
	}

	// Final cleanup/reseting
	FWorldContext& EditorWorldContext = GEditor->GetEditorWorldContext();
	UPackage* Package = EditorWorldContext.World()->GetOutermost();

	// Spawn note actors dropped in PIE.
	if(GEngine->PendingDroppedNotes.Num() > 0)
	{
		const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "CreatePIENoteActors", "Create PIE Notes") );

		for(int32 i=0; i<GEngine->PendingDroppedNotes.Num(); i++)
		{
			FDropNoteInfo& NoteInfo = GEngine->PendingDroppedNotes[i];
			ANote* NewNote = EditorWorldContext.World()->SpawnActor<ANote>(NoteInfo.Location, NoteInfo.Rotation);
			if(NewNote)
			{
				NewNote->Text = NoteInfo.Comment;
				if( NewNote->GetRootComponent() != NULL )
				{
					NewNote->GetRootComponent()->SetRelativeScale3D( FVector(2.f) );
				}
			}
		}
		Package->MarkPackageDirty();
		GEngine->PendingDroppedNotes.Empty();
	}

	// Restores realtime viewports that have been disabled for PIE.
	RestoreRealtimeViewports();

	// Don't actually need to reset this delegate but doing so allows is to check invalid attempts to execute the delegate
	FScopedConditionalWorldSwitcher::SwitchWorldForPIEDelegate = FOnSwitchWorldForPIE();

	// Set the autosave timer to have at least 10 seconds remaining before autosave
	const static float SecondsWarningTillAutosave = 10.0f;
	GUnrealEd->GetPackageAutoSaver().ForceMinimumTimeTillAutoSave(SecondsWarningTillAutosave);

	for(TObjectIterator<UAudioComponent> It; It; ++It)
	{
		UAudioComponent* AudioComp = *It;
		if (AudioComp->GetWorld() == EditorWorldContext.World())
		{
			AudioComp->ReregisterComponent();
		}
	}

	// no longer queued
	bIsPlayWorldQueued = false;
	bIsSimulateInEditorQueued = false;
	bRequestEndPlayMapQueued = false;

	// display any info if required.
	FMessageLog("PIE").Notify( LOCTEXT("PIEErrorsPresent", "Errors/warnings reported while playing in editor."), EMessageSeverity::Warning );
}

void UEditorEngine::TeardownPlaySession(FWorldContext &PieWorldContext)
{
	check(PieWorldContext.WorldType == EWorldType::PIE);
	PlayWorld = PieWorldContext.World();

	
	if (!PieWorldContext.RunAsDedicated)
	{
		// Slate data for this pie world
		FSlatePlayInEditorInfo &SlatePlayInEditorSession = SlatePlayInEditorMap.FindChecked(PieWorldContext.ContextHandle);

		// Destroy Viewport
		if ( PieWorldContext.GameViewport != NULL && PieWorldContext.GameViewport->Viewport != NULL )
		{
			PieWorldContext.GameViewport->CloseRequested(PieWorldContext.GameViewport->Viewport);
		}
		CleanupGameViewport();
	
		// Clean up the slate PIE viewport if we have one
		if( SlatePlayInEditorSession.DestinationSlateViewport.IsValid() )
		{
			TSharedPtr<ILevelViewport> Viewport = SlatePlayInEditorSession.DestinationSlateViewport.Pin();

			if( !bIsSimulatingInEditor)
			{
				// Set the editor viewport location to match that of Play in Viewport if we arent simulating in the editor, we have a valid player to get the location from 
				if( SlatePlayInEditorSession.EditorPlayer.IsValid() && SlatePlayInEditorSession.EditorPlayer.Get()->PlayerController )
				{
					FVector ViewLocation;
					FRotator ViewRotation;
					SlatePlayInEditorSession.EditorPlayer.Get()->PlayerController->GetPlayerViewPoint( ViewLocation, ViewRotation );
					Viewport->GetLevelViewportClient().SetViewLocation( ViewLocation );

					if( Viewport->GetLevelViewportClient().IsPerspective() )
					{
						// Rotation only matters for perspective viewports not orthographic
						Viewport->GetLevelViewportClient().SetViewRotation( ViewRotation );
					}
				}
			}

			// No longer simulating in the viewport
			Viewport->GetLevelViewportClient().SetIsSimulateInEditorViewport( false );

			// Clear out the hit proxies before GC'ing
			Viewport->GetLevelViewportClient().Viewport->InvalidateHitProxy();
		}
		else if( SlatePlayInEditorSession.SlatePlayInEditorWindow.IsValid() )
		{
			// Unregister the game viewport from slate.  This sends a final message to the viewport
			// so it can have a chance to release mouse capture, mouse lock, etc.		
			FSlateApplication::Get().UnregisterGameViewport();

			// Viewport client is cleaned up.  Make sure its not being accessed
			SlatePlayInEditorSession.SlatePlayInEditorWindowViewport->SetViewportClient( NULL );

			// The window may have already been destroyed in the case that the PIE window close box was pressed 
			if( SlatePlayInEditorSession.SlatePlayInEditorWindow.IsValid() )
			{
				// Destroy the SWindow
				FSlateApplication::Get().DestroyWindowImmediately( SlatePlayInEditorSession.SlatePlayInEditorWindow.Pin().ToSharedRef() );
			}
		}
	
		// Disassociate the players from their PlayerControllers.
		// This is done in the GameEngine path in UEngine::LoadMap.
		// But since PIE is just shutting down, and not loading a 
		// new map, we need to do it manually here for now.
		//for (auto It = GEngine->GetLocalPlayerIterator(PlayWorld); It; ++It)
		for (FLocalPlayerIterator It(GEngine, PlayWorld); It; ++It)
		{
			if(It->PlayerController)
			{
				if(It->PlayerController->GetPawn())
				{
					PlayWorld->DestroyActor(It->PlayerController->GetPawn(), true);
				}
				PlayWorld->DestroyActor(It->PlayerController, true);
				It->PlayerController = NULL;
			}
		}

	}

	// Change GWorld to be the play in editor world during cleanup.
	check( EditorWorld == GWorld );
	GWorld = PlayWorld;
	GIsPlayInEditorWorld = true;
	
	// Remember Simulating flag so that we know if OnSimulateSessionFinished is required after everything has been cleaned up. 
	bool bWasSimulatingInEditor = bIsSimulatingInEditor;
	// Clear Simulating In Editor bit
	bIsSimulatingInEditor = false;

	
	// Stop all audio and remove references to temp level.
	if (GetAudioDevice())
	{
		GetAudioDevice()->Flush( PlayWorld );
		GetAudioDevice()->ResetInterpolation();
	}

	// cleanup refs to any duplicated streaming levels
	for ( int32 LevelIndex=0; LevelIndex<PlayWorld->StreamingLevels.Num(); LevelIndex++ )
	{
		ULevelStreaming* StreamingLevel = PlayWorld->StreamingLevels[LevelIndex];
		if( StreamingLevel != NULL )
		{
			const ULevel* PlayWorldLevel = StreamingLevel->GetLoadedLevel();
			if ( PlayWorldLevel != NULL )
			{
				UWorld* World = Cast<UWorld>( PlayWorldLevel->GetOuter() );
				if( World != NULL )
				{
					// Attempt to move blueprint debugging references back to the editor world
					if( EditorWorld != NULL && EditorWorld->StreamingLevels.IsValidIndex(LevelIndex) )
					{
						const ULevel* EditorWorldLevel = EditorWorld->StreamingLevels[LevelIndex]->GetLoadedLevel();
						if ( EditorWorldLevel != NULL )
						{
							UWorld* SublevelEditorWorld  = Cast<UWorld>(EditorWorldLevel->GetOuter());
							if( SublevelEditorWorld != NULL )
							{
								World->TransferBlueprintDebugReferences(SublevelEditorWorld);
							}	
						}
					}

					World->CleanupWorld();
				}
			}
		}
	}

	// Clean up all streaming levels
	PlayWorld->bIsLevelStreamingFrozen = false;
	PlayWorld->bShouldForceUnloadStreamingLevels = true;
	PlayWorld->FlushLevelStreaming();

	// Construct a list of editors that are active for objects being debugged. We will refresh these when we have cleaned up to ensure no invalid objects exist in them
	TArray< IBlueprintEditor* > Editors;
	FAssetEditorManager& AssetEditorManager = FAssetEditorManager::Get();
	const UWorld::FBlueprintToDebuggedObjectMap& EditDebugObjectsPre = PlayWorld->GetBlueprintObjectsBeingDebugged();
	for (UWorld::FBlueprintToDebuggedObjectMap::TConstIterator EditIt(EditDebugObjectsPre); EditIt; ++EditIt)
	{
		if (UBlueprint* TargetBP = EditIt.Key().Get())
		{
			if(IBlueprintEditor* EachEditor = static_cast<IBlueprintEditor*>(AssetEditorManager.FindEditorForAsset(TargetBP, false)))
			{
				Editors.AddUnique( EachEditor );
			}
		}
	}
	
	// Move blueprint debugging pointers back to the objects in the editor world
	PlayWorld->TransferBlueprintDebugReferences(EditorWorld);

	FPhysScene* PhysScene = PlayWorld->GetPhysicsScene();
	if (PhysScene)
	{
		PhysScene->WaitPhysScenes();
	}

	// Clean up the temporary play level.
	PlayWorld->CleanupWorld();

	// Remove from root (Seamless travel may have done this)
	PlayWorld->RemoveFromRoot();
		
	PlayWorld = NULL;

	// Refresh any editors we had open in case they referenced objects that no longer exist.
	for (int32 iEditors = 0; iEditors <  Editors.Num(); iEditors++)
	{
		Editors[ iEditors ]->RefreshEditors();
	}
	
	// Restore GWorld.
	GWorld = EditorWorld;
	GIsPlayInEditorWorld = false;

	FWorldContext& EditorWorldContext = GEditor->GetEditorWorldContext();

	// Let the viewport know about leaving PIE/Simulate session. Do it after everything's been cleaned up
	// as the viewport will play exit sound here and this has to be done after GetAudioDevice()->Flush
	// otherwise all sounds will be immediately stopped.
	if (!PieWorldContext.RunAsDedicated)
	{
		// Slate data for this pie world
		FSlatePlayInEditorInfo &SlatePlayInEditorSession = SlatePlayInEditorMap.FindChecked(PieWorldContext.ContextHandle);
		if( SlatePlayInEditorSession.DestinationSlateViewport.IsValid() )
		{
			TSharedPtr<ILevelViewport> Viewport = SlatePlayInEditorSession.DestinationSlateViewport.Pin();

			if( Viewport->HasPlayInEditorViewport() )
			{
				Viewport->EndPlayInEditorSession();
			}

			// Let the Slate viewport know that we're leaving Simulate mode
			if( bWasSimulatingInEditor )
			{
				Viewport->OnSimulateSessionFinished();
			}

			Viewport->GetLevelViewportClient().SetReferenceToWorldContext(EditorWorldContext);
		}

		// Remove the slate info from the map (note that the UWorld* is long gone at this point, but the WorldContext still exists. It will be removed outside of this function)
		SlatePlayInEditorMap.Remove(PieWorldContext.ContextHandle);
	}
}

void UEditorEngine::PlayMap( const FVector* StartLocation, const FRotator* StartRotation, int32 Destination, int32 InPlayInViewportIndex, bool bUseMobilePreview, bool bMovieCapture )
{
	// queue up a Play From Here request, this way the load/save won't conflict with the TransBuffer, which doesn't like 
	// loading and saving to happen during a transaction

	// save the StartLocation if we have one
	if (StartLocation)
	{
		PlayWorldLocation = *StartLocation;
		PlayWorldRotation = StartRotation ? *StartRotation : FRotator::ZeroRotator;
		bHasPlayWorldPlacement = true;
	}
	else
	{
		bHasPlayWorldPlacement = false;
	}

	// remember where to send the play map request
	PlayWorldDestination = Destination;

	// Set whether or not we want to use mobile preview mode (PC platform only)
	bUseMobilePreviewForPlayWorld = bUseMobilePreview;

	// Set whether or not we want to start movie capturing immediately (PC platform only)
	bStartMovieCapture = bMovieCapture;

	// tell the editor to kick it off next Tick()
	bIsPlayWorldQueued = true;

	// Not wanting to simulate
	bIsSimulateInEditorQueued = false;

	// Unless we've been asked to play in a specific viewport window, this index will be -1
	PlayInEditorViewportIndex = InPlayInViewportIndex;
}


void UEditorEngine::RequestPlaySession( bool bAtPlayerStart, TSharedPtr<class ILevelViewport> DestinationViewport, bool bInSimulateInEditor, const FVector* StartLocation, const FRotator* StartRotation, int32 DestinationConsole, bool bUseMobilePreview )
{
	// Remember whether or not we were attempting to play from playerstart or from viewport
	GIsPIEUsingPlayerStart = bAtPlayerStart;

	// queue up a Play From Here request, this way the load/save won't conflict with the TransBuffer, which doesn't like 
	// loading and saving to happen during a transaction

	// save the StartLocation if we have one
	if (!bInSimulateInEditor && StartLocation != NULL)
	{
		PlayWorldLocation = *StartLocation;
		PlayWorldRotation = StartRotation ? *StartRotation : FRotator::ZeroRotator;
		bHasPlayWorldPlacement = true;
	}
	else
	{
		bHasPlayWorldPlacement = false;
	}

	// remember where to send the play map request
	PlayWorldDestination = DestinationConsole;

	RequestedDestinationSlateViewport = DestinationViewport;

	// Set whether or not we want to use mobile preview mode (PC platform only)
	bUseMobilePreviewForPlayWorld = bUseMobilePreview;

	// Not capturing a movie
	bStartMovieCapture = false;

	// tell the editor to kick it off next Tick()
	bIsPlayWorldQueued = true;

	// Store whether we want to play in editor, or only simulate in editor
	bIsSimulateInEditorQueued = bInSimulateInEditor;

	// Unless we have been asked to play in a specific viewport window, this index will be -1
	PlayInEditorViewportIndex = -1;

	// @todo gmp: temp hack for Rocket demo
	bPlayOnLocalPcSession = false;
	bPlayUsingLauncher = false;
}


// @todo gmp: temp hack for Rocket demo
void UEditorEngine::RequestPlaySession( const FVector* StartLocation, const FRotator* StartRotation, bool MobilePreview )
{
	bPlayOnLocalPcSession = true;
	bPlayUsingLauncher = false;
	bPlayUsingMobilePreview = MobilePreview;

	if (StartLocation != NULL)
	{
		PlayWorldLocation = *StartLocation;
		PlayWorldRotation = StartRotation ? *StartRotation : FRotator::ZeroRotator;
		bHasPlayWorldPlacement = true;
	}
	else
	{
		bHasPlayWorldPlacement = false;
	}

	bIsPlayWorldQueued = true;
}

void UEditorEngine::RequestPlaySession(const FString& DeviceId, const FString& DeviceName)
{
	bPlayOnLocalPcSession = false;
	bPlayUsingLauncher = true;

	// always use playerstart on remote devices (for now?)
	bHasPlayWorldPlacement = false;

	// remember the platform name to run on
	PlayUsingLauncherDeviceId = DeviceId;
	PlayUsingLauncherDeviceName = DeviceName;

	bIsPlayWorldQueued = true;
}

void UEditorEngine::PlaySessionPaused()
{
	FEditorDelegates::PausePIE.Broadcast(bIsSimulatingInEditor);
}


void UEditorEngine::PlaySessionResumed()
{
	FEditorDelegates::ResumePIE.Broadcast(bIsSimulatingInEditor);
}


void UEditorEngine::PlaySessionSingleStepped()
{
	FEditorDelegates::SingleStepPIE.Broadcast(bIsSimulatingInEditor);
}


void AdvanceWindowPositionsForNextPIEWindow(int32 &WinX, int32 &WinY, int32 Width, int32 Height)
{
	const FIntPoint WindowPadding(0, 0);//(15, 32);

	WinX += Width + WindowPadding.X;

	FSlateRect PreferredWorkArea = FSlateApplication::Get().GetPreferredWorkArea();
	
	// if no more windows fit horizontally, place them in a new row
	if ((WinX + WindowPadding.X + Width) > PreferredWorkArea.Right)
	{
		WinX = 0;
		WinY += Height + WindowPadding.X;
	}

	// if no more rows fit vertically, stack windows on top of each other
	if (WinY + WindowPadding.Y + Height >= PreferredWorkArea.Bottom)
	{
		WinX += WindowPadding.X;
		WinY = WindowPadding.Y;
	}
}


void GetMultipleInstancePositions(int32 index, int32 &LastX, int32 &LastY)
{
	ULevelEditorPlaySettings* PlayInSettings = Cast<ULevelEditorPlaySettings>(ULevelEditorPlaySettings::StaticClass()->GetDefaultObject());

	if (PlayInSettings->MultipleInstancePositions.IsValidIndex(index) &&
		(PlayInSettings->MultipleInstanceLastHeight == PlayInSettings->NewWindowHeight) &&
		(PlayInSettings->MultipleInstanceLastWidth == PlayInSettings->NewWindowWidth))
	{
		PlayInSettings->NewWindowPosition =	PlayInSettings->MultipleInstancePositions[index];

		LastX = PlayInSettings->NewWindowPosition.X;
		LastY = PlayInSettings->NewWindowPosition.Y;
	}
	else
	{
		PlayInSettings->NewWindowPosition = FIntPoint(LastX, LastY);
	}

	AdvanceWindowPositionsForNextPIEWindow(LastX, LastY, PlayInSettings->NewWindowWidth, PlayInSettings->NewWindowHeight);
}


FString GenerateCmdLineForNextPieInstance(int32 &WinX, int32 &WinY, int32 &InstanceNum, bool IsServer)
{
	// Get GameSettings INI override
	FString GameUserSettingsOverride = GGameUserSettingsIni.Replace(TEXT("GameUserSettings"), *FString::Printf(TEXT("PIEGameUserSettings%d"), InstanceNum++));
	const ULevelEditorPlaySettings* PlayInSettings = GetDefault<ULevelEditorPlaySettings>();

	// Construct parms:
	//	-Override GameUserSettings.ini
	//	-Force no steam
	//	-Allow saving of config files (since we are giving them an override INI)
	FString CmdLine = FString::Printf(TEXT("GameUserSettingsINI=%s -MultiprocessSaveConfig %s "), *GameUserSettingsOverride, *PlayInSettings->AdditionalLaunchOptions);

	if (IsServer && PlayInSettings->PlayNetDedicated)
	{
		// Append dedicated server options
		CmdLine += TEXT("-server -log ");
	}
	else
	{
		// hack: fudge window by WindowBorderSize, so it doesn't appear off-screen
		FMargin WindowBorderSize(8.0f, 32.0f);
		TSharedPtr<SWindow> SomeWindow = FSlateApplication::Get().GetActiveTopLevelWindow();

		if (SomeWindow.IsValid())
		{
			WindowBorderSize = SomeWindow->GetWindowBorderSize();
		}

		// Listen server or clients: specify default win position and SAVEWINPOS so the final positions are saved
		// in order to preserve PIE networking window setup
		CmdLine += FString::Printf(TEXT("WinX=%d WinY=%d SAVEWINPOS=1 "), WinX + WindowBorderSize.Left, WinY + WindowBorderSize.Top);

		// Advance window
		AdvanceWindowPositionsForNextPIEWindow(WinX, WinY, PlayInSettings->NewWindowWidth, PlayInSettings->NewWindowHeight );
	}
	
	return CmdLine;
}

void UEditorEngine::StartQueuedPlayMapRequest()
{
	const bool bWantSimulateInEditor = bIsSimulateInEditorQueued;

	// note that we no longer have a queued request
	bIsPlayWorldQueued = false;
	bIsSimulateInEditorQueued = false;

	//ULevelEditorPlaySettings* PlaySettings = GetMutableDefault<ULevelEditorPlaySettings>();
	//PlaySettings->LastExecutedPlayModeType = PlayMode;

	EndPlayOnLocalPc();

	const ULevelEditorPlaySettings* PlayInSettings = GetDefault<ULevelEditorPlaySettings>();
	
	// Launch multiplayer instances if necessary
	// (note that if you have 'RunUnderOneProcess' checked and do a bPlayOnLocalPcSession (standalone) - play standalone 'wins' - multiple instances will be launched for multiplayer)
	if (PlayInSettings->PlayNetMode != PIE_Standalone && (!PlayInSettings->RunUnderOneProcess || bPlayOnLocalPcSession) && !bPlayUsingLauncher)
	{
		// Launch multiple instances for pie networking here
		int32 WinX=0;
		int32 WinY=0;
		int32 PIENum = 0;
		int32 NumClients = 0;

		// Do we need to spawn a server?
		if (PlayInSettings->PlayNetMode == PIE_Client)
		{
			FString CmdLine = GenerateCmdLineForNextPieInstance(WinX, WinY, PIENum, true);
			FString URLOptions = TEXT("?Listen");
			PlayOnLocalPc(TEXT(""), URLOptions, CmdLine, PlayInSettings->ClientWindowWidth, PlayInSettings->ClientWindowHeight);

			if (!PlayInSettings->PlayNetDedicated)
			{
				// Listen server counts as a client
				NumClients++;
			}
		}

		// Spawn number of clients
		for (int32 i=NumClients; i < PlayInSettings->PlayNumberOfClients; ++i)
		{
			FString CmdLine = GenerateCmdLineForNextPieInstance(WinX, WinY, PIENum, false);
			PlayOnLocalPc(TEXT("127.0.0.1"), TEXT(""), CmdLine, PlayInSettings->ClientWindowWidth, PlayInSettings->ClientWindowHeight );
		}

		// Launch the local PIE session
		if (bPlayOnLocalPcSession)
		{
			FString CmdLine = GenerateCmdLineForNextPieInstance(WinX, WinY, PIENum, PlayInSettings->PlayNetMode == PIE_ListenServer);

			PlayOnLocalPc(
				PlayInSettings->PlayNetMode == PIE_Client ? TEXT("127.0.0.1") : TEXT(""),
				PlayInSettings->PlayNetMode == PIE_ListenServer ? TEXT("?Listen") : TEXT(""),
				CmdLine);
		}
		else if (bStartMovieCapture)
		{
			// @todo Fix for UE4.  This is a temp workaround. 
			PlayForMovieCapture();
		}
		else
		{
			PlayInEditor( GetEditorWorldContext().World(), bWantSimulateInEditor );
		}
	}
	else
	{
		// Launch standalone PIE session
		if (bPlayOnLocalPcSession)
		{
			PlayOnLocalPc();
		}
		else if (bPlayUsingLauncher)
		{
			PlayUsingLauncher();
		}
		else if (bStartMovieCapture)
		{
			// @todo Fix for UE4.  This is a temp workaround. 
			PlayForMovieCapture();
		}
		else
		{
			PlayInEditor( GetEditorWorldContext().World(), bWantSimulateInEditor );
		}
	}
}

/* Temporarily renames streaming levels for pie saving */
class FScopedRenameStreamingLevels
{
public:
	FScopedRenameStreamingLevels( UWorld* InWorld, const FString& AutosavePackagePrefix, const FString& MapnamePrefix )
		: World(InWorld)
	{
		if(InWorld->StreamingLevels.Num() > 0)
		{
			for(int32 LevelIndex=0; LevelIndex < InWorld->StreamingLevels.Num(); ++LevelIndex)
			{
				ULevelStreaming* StreamingLevel = InWorld->StreamingLevels[LevelIndex];
				if ( StreamingLevel )
				{
					const FString StreamingLevelPackageName = FString::Printf(TEXT("%s%s/%s%s"), *AutosavePackagePrefix, *FPackageName::GetLongPackagePath( StreamingLevel->PackageName.ToString() ), *MapnamePrefix, *FPackageName::GetLongPackageAssetName( StreamingLevel->PackageName.ToString() ));
					PreviousStreamingPackageNames.Add( StreamingLevel->PackageNameToLoad );
					StreamingLevel->PackageNameToLoad = FName(*StreamingLevelPackageName);
				}
			}
		}
	}

	~FScopedRenameStreamingLevels()
	{
		check(World.IsValid());
		check( PreviousStreamingPackageNames.Num() == World->StreamingLevels.Num() );
		if(World->StreamingLevels.Num() > 0)
		{
			for(int32 LevelIndex=0; LevelIndex < World->StreamingLevels.Num(); ++LevelIndex)
			{
				ULevelStreaming* StreamingLevel = World->StreamingLevels[LevelIndex];
				if ( StreamingLevel )
				{
					StreamingLevel->PackageNameToLoad = PreviousStreamingPackageNames[LevelIndex];
				}
			}
		}
	}
private:
	TWeakObjectPtr<UWorld> World;
	TArray<FName> PreviousStreamingPackageNames;
};


void UEditorEngine::SaveWorldForPlay(TArray<FString>& SavedMapNames)
{
	UWorld* World = GWorld;

	// check if PersistentLevel has any external references
	if( PackageUsingExternalObjects(World->PersistentLevel) && EAppReturnType::Yes != FMessageDialog::Open( EAppMsgType::YesNo, NSLOCTEXT("UnrealEd", "Warning_UsingExternalPackage", "This map is using externally referenced packages which won't be found when in a game and all references will be broken. Perform a map check for more details.\n\nWould you like to continue?")) )
	{
		return;
	}

	const FString PlayOnConsolePackageName = FPackageName::FilenameToLongPackageName(FPaths::Combine(*FPaths::GameSavedDir(), *PlayOnConsoleSaveDir)) + TEXT("/");

	// make a per-platform name for the map
	const FString ConsoleName = FString(TEXT("PC"));
	const FString Prefix = FString(PLAYWORLD_CONSOLE_BASE_PACKAGE_PREFIX) + ConsoleName;

	// Temporarily rename streaming levels for pie saving
	FScopedRenameStreamingLevels ScopedRenameStreamingLevels( World, PlayOnConsolePackageName, Prefix );
	
	const FString WorldPackageName = World->GetOutermost()->GetName();
	
	// spawn a play-from-here player start or a temporary player start
	AActor* PlayerStart = NULL;
	bool bCreatedPlayerStart = false;

	SpawnPlayFromHereStart( World, PlayerStart, PlayWorldLocation, PlayWorldRotation );

	if (PlayerStart != NULL)
	{
		bCreatedPlayerStart = true;
	}
	else
	{
		PlayerStart = CheckForPlayerStart();
	
		if( PlayerStart == NULL )
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.bNoCollisionFail = true;
			PlayerStart = World->SpawnActor<AActor>( APlayerStart::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo );

			bCreatedPlayerStart = true;
		}
	}
	
	// save out all open map packages
	TArray<FString> SavedWorldFileNames;
	bool bSavedWorld = SavePlayWorldPackages(World, *Prefix, /*out*/ SavedWorldFileNames);

	// Remove the player start we added if we made one
	if (bCreatedPlayerStart)
	{
		World->DestroyActor(PlayerStart);
	}

	if (bSavedWorld)
	{
		// Convert the filenames into map names
		SavedMapNames.Reserve(SavedMapNames.Num() + SavedWorldFileNames.Num());
		for (int32 Index = 0; Index < SavedWorldFileNames.Num(); ++Index)
		{
			const FString MapName = FPackageName::FilenameToLongPackageName(SavedWorldFileNames[Index]);
			SavedMapNames.Add(MapName);
		}
	}

}

// @todo gmp: temp hack for Rocket demo
void UEditorEngine::EndPlayOnLocalPc( )
{
	for (int32 i=0; i < PlayOnLocalPCSessions.Num(); ++i)
	{
		if (PlayOnLocalPCSessions[i].ProcessHandle.IsValid())
		{
			if ( FPlatformProcess::IsProcRunning(PlayOnLocalPCSessions[i].ProcessHandle) )
			{
				FPlatformProcess::TerminateProc(PlayOnLocalPCSessions[i].ProcessHandle);
			}
			PlayOnLocalPCSessions[i].ProcessHandle.Reset();
		}
	}

	PlayOnLocalPCSessions.Empty();
}

// @todo gmp: temp hack for Rocket demo
void UEditorEngine::PlayOnLocalPc(FString MapNameOverride, FString URLParms, FString CmdLineParms, int32 ResX, int32 ResY)
{
	// select map to play
	TArray<FString> SavedMapNames;
	if (MapNameOverride.IsEmpty())
	{
		FWorldContext& EditorContext = GetEditorWorldContext();
		if (EditorContext.World()->WorldComposition)
		{
			// Open world composition from original folder
			FString MapName = EditorContext.World()->GetOutermost()->GetName();
			MapName += TEXT("?worldcomposition");
			SavedMapNames.Add(MapName);
		}
		else
		{
			SaveWorldForPlay(SavedMapNames);
		}
	}

	if (SavedMapNames.Num() == 0)
	{
		return;
	}

	DisableRealtimeViewports();

	FString GameNameOrProjectFile;
	FString AdditionalParameters(TEXT(""));
	if (FPaths::IsProjectFilePathSet())
	{
		GameNameOrProjectFile = FString::Printf(TEXT("\"%s\""), *FPaths::GetProjectFilePath());

		//@todo.Rocket: Add shipping support
		bool bRunningDebug = FParse::Param(FCommandLine::Get(), TEXT("debug"));

		if (bRunningDebug)
		{
			AdditionalParameters = TEXT(" -debug");
		}
	}
	else
	{
		GameNameOrProjectFile = FApp::GetGameName();
	}

	// apply additional settings
	const ULevelEditorPlaySettings* PlayInSettings = GetDefault<ULevelEditorPlaySettings>();

	if (bPlayUsingMobilePreview)
	{
		AdditionalParameters += TEXT(" -featureleveles2 -faketouches");
	}

	if (PlayInSettings->DisableStandaloneSound)
	{
		AdditionalParameters += TEXT(" -nosound");
	}

	if (PlayInSettings->AdditionalLaunchParameters.Len() > 0)
	{
		AdditionalParameters += TEXT(" ");
		AdditionalParameters += PlayInSettings->AdditionalLaunchParameters;
	}

	FString Params = FString::Printf(TEXT("%s %s -game -PIEVIACONSOLE -ResX=%d -ResY=%d %s%s %s"),
		*GameNameOrProjectFile,
		*BuildPlayWorldURL(*SavedMapNames[0], false, URLParms),
		(ResX > 0 ? ResX : PlayInSettings->StandaloneWindowWidth),
		(ResY > 0 ? ResY : PlayInSettings->StandaloneWindowHeight),
		*FCommandLine::GetSubprocessCommandline(),
		*AdditionalParameters,
		*CmdLineParms
	);

	// launch the game process
	FString GamePath = FPlatformProcess::GenerateApplicationPath(FApp::GetName(), FApp::GetBuildConfiguration());
	FPlayOnPCInfo *NewSession = new (PlayOnLocalPCSessions) FPlayOnPCInfo();

	NewSession->ProcessHandle = FPlatformProcess::CreateProc(*GamePath, *Params, true, false, false, NULL, 0, NULL, NULL);

	if (!NewSession->ProcessHandle.IsValid())
	{
		UE_LOG(LogPlayLevel, Error, TEXT("Failed to run a copy of the game on this PC."));
	}
}

static void HandleOutputReceived(const FString& InMessage)
{
	UE_LOG(LogPlayLevel, Display, TEXT("%s"), *InMessage);
}

static void HandleCancelButtonClicked(ILauncherWorkerPtr LauncherWorker)
{
	if (LauncherWorker.IsValid())
	{
		LauncherWorker->Cancel();
	}
}

/* FMainFrameActionCallbacks callbacks
 *****************************************************************************/

class FLauncherNotificationTask
{
public:

	FLauncherNotificationTask( TWeakPtr<SNotificationItem> InNotificationItemPtr, SNotificationItem::ECompletionState InCompletionState, const FText& InText )
		: CompletionState(InCompletionState)
		, NotificationItemPtr(InNotificationItemPtr)
		, Text(InText)
	{ }

	void DoTask( ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent )
	{
		if (NotificationItemPtr.IsValid())
		{
			if (CompletionState == SNotificationItem::CS_Fail)
			{
				GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
			}
			else
			{
				GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));
			}

			TSharedPtr<SNotificationItem> NotificationItem = NotificationItemPtr.Pin();
			NotificationItem->SetText(Text);
			NotificationItem->SetCompletionState(CompletionState);
			NotificationItem->ExpireAndFadeout();
		}
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }
	ENamedThreads::Type GetDesiredThread( ) { return ENamedThreads::GameThread; }
	static const TCHAR* GetTaskName( ) { return TEXT("FLauncherNotificationTask"); }
	FORCEINLINE static TStatId GetStatId()
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLauncherNotificationTask, STATGROUP_TaskGraphTasks);
	}

private:

	SNotificationItem::ECompletionState CompletionState;
	TWeakPtr<SNotificationItem> NotificationItemPtr;
	FText Text;
};


static void HandleStageStarted(const FString& InStage, FString DeviceName, TWeakPtr<SNotificationItem> NotificationItemPtr)
{
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("StageName"), FText::FromString(InStage));
	Arguments.Add(TEXT("DeviceName"), FText::FromString(DeviceName));

	NotificationItemPtr.Pin()->SetText(FText::Format(LOCTEXT("LauncherTaskStageNotification", "{StageName} for Launch on {DeviceName}..."), Arguments));
}

static void HandleLaunchCanceled(bool* bPlayUsingLauncher, TWeakPtr<SNotificationItem> NotificationItemPtr)
{
	TGraphTask<FLauncherNotificationTask>::CreateTask().ConstructAndDispatchWhenReady(
		NotificationItemPtr,
		SNotificationItem::CS_Fail,
		LOCTEXT("LaunchtaskFailedNotification", "Launch canceled!")
	);

	*bPlayUsingLauncher = false;	
}

static void HandleLaunchCompleted(bool Succeeded, bool* bPlayUsingLauncher, TWeakPtr<SNotificationItem> NotificationItemPtr)
{
	if (Succeeded)
	{
		TGraphTask<FLauncherNotificationTask>::CreateTask().ConstructAndDispatchWhenReady(
			NotificationItemPtr,
			SNotificationItem::CS_Success,
			LOCTEXT("LauncherTaskCompleted", "Launch complete!!")
		);
	}
	else
	{
		TGraphTask<FLauncherNotificationTask>::CreateTask().ConstructAndDispatchWhenReady(
			NotificationItemPtr,
			SNotificationItem::CS_Fail,
			LOCTEXT("LauncherTaskCompleted", "Launch failed!!")
			);
	}
	*bPlayUsingLauncher = false;
}

static void HandleHyperlinkNavigate()
{
	FGlobalTabmanager::Get()->InvokeTab(FName("OutputLog"));
}

void UEditorEngine::PlayUsingLauncher()
{
	//	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("LaunchOnDeviceNotWorkingMessage", "Sorry, launching on a device is currently not supported. This feature will be available again soon.") );

	if (!PlayUsingLauncherDeviceId.IsEmpty())
	{
		ILauncherServicesModule& LauncherServicesModule = FModuleManager::LoadModuleChecked<ILauncherServicesModule>(TEXT("LauncherServices"));
		ITargetDeviceServicesModule& TargetDeviceServicesModule = FModuleManager::LoadModuleChecked<ITargetDeviceServicesModule>("TargetDeviceServices");

		// create a temporary device group and launcher profile
		ILauncherDeviceGroupRef DeviceGroup = LauncherServicesModule.CreateDeviceGroup(FGuid::NewGuid(), TEXT("PlayOnDevices"));
		DeviceGroup->AddDevice(PlayUsingLauncherDeviceId);

		// does the project have any code?
		FGameProjectGenerationModule& GameProjectModule = FModuleManager::LoadModuleChecked<FGameProjectGenerationModule>(TEXT("GameProjectGeneration"));
		bool bHasCode = GameProjectModule.Get().GetProjectCodeFileCount() > 0;

		ILauncherProfileRef LauncherProfile = LauncherServicesModule.CreateProfile(TEXT("Play On Device"));
		LauncherProfile->SetBuildGame(bHasCode && FSourceCodeNavigation::IsCompilerAvailable());
		LauncherProfile->SetCookMode(ELauncherProfileCookModes::ByTheBook);
		LauncherProfile->AddCookedPlatform(PlayUsingLauncherDeviceId.Left(PlayUsingLauncherDeviceId.Find(TEXT("@"))));
        LauncherProfile->SetForceClose(true);
        LauncherProfile->SetTimeout(60);

		// Run the same version of the cooker that the editor is compiled as
#if UE_BUILD_DEBUG
		LauncherProfile->SetCookConfiguration(EBuildConfigurations::Debug);
#elif UE_BUILD_DEBUGGAME
		LauncherProfile->SetCookConfiguration(EBuildConfigurations::DebugGame);
#elif UE_BUILD_SHIPPING
		LauncherProfile->SetCookConfiguration(EBuildConfigurations::Shipping);
#elif UE_BUILD_TEST
		LauncherProfile->SetCookConfiguration(EBuildConfigurations::Test);
#else	// UE_BUILD_DEVELOPMENT
		LauncherProfile->SetCookConfiguration(EBuildConfigurations::Development);
#endif

		LauncherProfile->SetBuildConfiguration(EBuildConfigurations::Development);
		LauncherProfile->SetDeploymentMode(ELauncherProfileDeploymentModes::CopyToDevice);
		LauncherProfile->SetDeployedDeviceGroup(DeviceGroup);
		LauncherProfile->SetHideFileServerWindow(false);
		LauncherProfile->SetLaunchMode(ELauncherProfileLaunchModes::DefaultRole);

		TArray<FString> MapNames;
		FWorldContext& EditorContext = GetEditorWorldContext();
		if (EditorContext.World()->WorldComposition)
		{
			// Open world composition from original folder
			FString MapName = EditorContext.World()->GetOutermost()->GetName();
			MapName += TEXT("?worldcomposition");

			MapNames.Add(MapName);
		}
		else
		{
			SaveWorldForPlay(MapNames);
		}

		FString InitialMapName;
		if (MapNames.Num() > 0)
		{
			InitialMapName = MapNames[0];
		}

		LauncherProfile->GetDefaultLaunchRole()->SetInitialMap(InitialMapName);

		for (const FString& MapName : MapNames)
		{
			LauncherProfile->AddCookedMap(MapName);
		}

		ILauncherPtr Launcher = LauncherServicesModule.CreateLauncher();
		ILauncherWorkerPtr LauncherWorker = Launcher->Launch(TargetDeviceServicesModule.GetDeviceProxyManager(), LauncherProfile);

		// create notification item
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Device"), FText::FromString(PlayUsingLauncherDeviceName));
		FNotificationInfo Info( FText::Format( LOCTEXT("LauncherTaskInProgressNotification", "Launching on {Device}..."), Arguments) );

		Info.Image = FEditorStyle::GetBrush(TEXT("MainFrame.CookContent"));
		Info.bFireAndForget = false;
		Info.ExpireDuration = 3.0f;
		Info.Hyperlink = FSimpleDelegate::CreateStatic(HandleHyperlinkNavigate);
		Info.ButtonDetails.Add(
			FNotificationButtonInfo(
				LOCTEXT("LauncherTaskCancel", "Cancel"),
				LOCTEXT("LauncherTaskCancelToolTip", "Cancels execution of this task."),
				FSimpleDelegate::CreateStatic(HandleCancelButtonClicked, LauncherWorker)
			)
		);

		TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);

		if (!NotificationItem.IsValid())
		{
			return;
		}

		NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);

		TWeakPtr<SNotificationItem> NotificationItemPtr(NotificationItem);
		if (LauncherWorker.IsValid() && LauncherWorker->GetStatus() != ELauncherWorkerStatus::Completed)
		{
			GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileStart_Cue.CompileStart_Cue"));
			LauncherWorker->OnOutputReceived().AddStatic(HandleOutputReceived);
			LauncherWorker->OnStageStarted().AddStatic(HandleStageStarted, PlayUsingLauncherDeviceName, NotificationItemPtr);
			LauncherWorker->OnCompleted().AddStatic(HandleLaunchCompleted, &bPlayUsingLauncher, NotificationItemPtr);
			LauncherWorker->OnCanceled().AddStatic(HandleLaunchCanceled, &bPlayUsingLauncher, NotificationItemPtr);
		}
		else
		{
			LauncherWorker.Reset();
			GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));

			NotificationItem->SetText(LOCTEXT("LauncherTaskFailedNotification", "Failed to launch task!"));
			NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
			NotificationItem->ExpireAndFadeout();
			bPlayUsingLauncher = false;
		}
	}
}

void UEditorEngine::PlayForMovieCapture()
{
	TArray<FString> SavedMapNames;
	SaveWorldForPlay(SavedMapNames);

	if (SavedMapNames.Num() == 0)
	{
		return;
	}

	// this parameter tells UE4Editor to run in game mode
	FString EditorCommandLine = SavedMapNames[0];
	EditorCommandLine += TEXT(" -game");

	// this parameter tells UGameEngine to add the auto-save dir to the paths array and repopulate the package file cache
	// this is needed in order to support streaming levels as the streaming level packages will be loaded only when needed (thus
	// their package names need to be findable by the package file caching system)
	// (we add to EditorCommandLine because the URL is ignored by WindowsTools)
	EditorCommandLine += TEXT(" -PIEVIACONSOLE");
	
	// Disable splash screen
	EditorCommandLine += TEXT(" -NoSplash");
	
	// if we want to start movie capturing right away, then append the argument for that
	if (bStartMovieCapture)
	{
		//disable movies
		EditorCommandLine += FString::Printf(TEXT(" -nomovie"));
	
		//set res options
		EditorCommandLine += FString::Printf(TEXT(" -ResX=%d"), GEditor->MatineeCaptureResolutionX);
		EditorCommandLine += FString::Printf(TEXT(" -ResY=%d"), GEditor->MatineeCaptureResolutionY);
					
		if( GUnrealEd->bNoTextureStreaming )
		{
			EditorCommandLine += TEXT(" -NoTextureStreaming");
		}

		//set fps
		EditorCommandLine += FString::Printf(TEXT(" -BENCHMARK -FPS=%d"), GEditor->MatineeCaptureFPS);
	
		if (GEditor->MatineeCaptureType == 1)
		{
			EditorCommandLine += FString::Printf(TEXT(" -MATINEESSCAPTURE=%s"), *GEngine->MatineeCaptureName);//*GEditor->MatineeNameForRecording);

			// If buffer visualization dumping is enabled, we need to tell capture process to enable it too
			static const auto CVarDumpFrames = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.BufferVisualizationDumpFrames"));

			if (CVarDumpFrames && CVarDumpFrames->GetValueOnGameThread())
			{
				EditorCommandLine += TEXT(" -MATINEEBUFFERVISUALIZATIONDUMP");
			}
		}
		else
		{
			EditorCommandLine += FString::Printf(TEXT(" -MATINEEAVICAPTURE=%s"), *GEngine->MatineeCaptureName);//*GEditor->MatineeNameForRecording);
		}
					
		EditorCommandLine += FString::Printf(TEXT(" -MATINEEPACKAGE=%s"), *GEngine->MatineePackageCaptureName);//*GEditor->MatineePackageNameForRecording);
		EditorCommandLine += FString::Printf(TEXT(" -VISIBLEPACKAGES=%s"), *GEngine->VisibleLevelsForMatineeCapture);
	
		if (GEditor->bCompressMatineeCapture == 1)
		{
			EditorCommandLine += TEXT(" -CompressCapture");
		}
	}

	FString GamePath = FPlatformProcess::GenerateApplicationPath(FApp::GetName(), FApp::GetBuildConfiguration());
	FString Params;

	if (FPaths::IsProjectFilePathSet())
	{
		Params = FString::Printf(TEXT("\"%s\" %s %s"), *FPaths::GetProjectFilePath(), *EditorCommandLine, *FCommandLine::GetSubprocessCommandline());
	}
	else
	{
		Params = FString::Printf(TEXT("%s %s %s"), FApp::GetGameName(), *EditorCommandLine, *FCommandLine::GetSubprocessCommandline());
	}

	if ( FRocketSupport::IsRocket() )
	{
		Params += TEXT(" -rocket");
	}

	FProcHandle ProcessHandle = FPlatformProcess::CreateProc(*GamePath, *Params, true, false, false, NULL, 0, NULL, NULL);

	if (ProcessHandle.IsValid())
	{
		bool bCloseEditor = false;

		GConfig->GetBool(TEXT("MatineeCreateMovieOptions"), TEXT("CloseEditor"), bCloseEditor, GEditorUserSettingsIni);

		if (bCloseEditor)
		{
			IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
			MainFrameModule.RequestCloseEditor();
		}
	}
	else
	{
		UE_LOG(LogPlayLevel, Error,  TEXT("Failed to run a copy of the game for matinee capture."));
	}
	ProcessHandle.Close();
}


void UEditorEngine::RequestEndPlayMap()
{
	if( PlayWorld )
	{
		bRequestEndPlayMapQueued = true;
	}
}

bool UEditorEngine::SavePlayWorldPackages(UWorld* InWorld, const TCHAR* Prefix, TArray<FString>& OutSavedFilenames)
{
	{
		// if this returns false, it means we should stop what we're doing and return to the editor
		bool bPromptUserToSave = true;
		bool bSaveMapPackages = false;
		bool bSaveContentPackages = true;
		if (!FEditorFileUtils::SaveDirtyPackages(bPromptUserToSave, bSaveMapPackages, bSaveContentPackages))
		{
			return false;
		}
	}

	// Update cull distance volumes before saving.
	InWorld->UpdateCullDistanceVolumes();

	// Clean up any old worlds.
	CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );

	// Save temporary copies of all levels to be used for playing in editor or using standalone PC/console
	return FEditorFileUtils::SaveWorlds(InWorld, FPaths::Combine(*FPaths::GameSavedDir(), *PlayOnConsoleSaveDir), Prefix, OutSavedFilenames);
}


FString UEditorEngine::BuildPlayWorldURL(const TCHAR* MapName, bool bSpectatorMode, FString AdditionalURLOptions)
{
	// the URL we are building up
	FString URL(MapName);

	// If we hold down control, start in spectating mode
	if (bSpectatorMode)
	{
		// Start in spectator mode
		URL += TEXT("?SpectatorOnly=1");
	}

	// Add any game-specific options set in the INI file
	URL += InEditorGameURLOptions;

	// Add any additional options that were specified for this call
	URL += AdditionalURLOptions;

	// Add any additional options that are set in the Play In Settings menu
	URL += *GetDefault<ULevelEditorPlaySettings>()->AdditionalServerGameOptions;

	return URL;
}

bool UEditorEngine::SpawnPlayFromHereStart( UWorld* World, AActor*& PlayerStart, const FVector& StartLocation, const FRotator& StartRotation )
{
	// null it out in case we don't need to spawn one, and the caller relies on us setting it
	PlayerStart = NULL;

	if( bHasPlayWorldPlacement )
	{
		// spawn the PlayerStartPIE in the given world
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel;
		PlayerStart = World->SpawnActor<AActor>(PlayFromHerePlayerStartClass, StartLocation, StartRotation, SpawnParameters);

		// make sure we were able to spawn the PlayerStartPIE there
		if(!PlayerStart)
		{
			FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Prompt_22", "Failed to create entry point. Try another location, or you may have to rebuild your level."));
			return false;
		}
		// tag the start
		ANavigationObjectBase* NavPlayerStart = Cast<ANavigationObjectBase>(PlayerStart);
		if (NavPlayerStart)
		{
			NavPlayerStart->bIsPIEPlayerStart = true;
		}
	}
	// true means we didn't need to spawn, or we succeeded
	return true;
}

/**
 * Initializes streaming levels when PIE starts up based on the players start location
 */
static void InitStreamingLevelsForPIEStartup( UGameViewportClient* GameViewportClient, UWorld* PlayWorld )
{
	check(PlayWorld);
	ULocalPlayer* Player = PlayWorld->GetFirstLocalPlayerFromController();
	if( Player )
	{
		// Create a view family for the game viewport
		FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues(
			GameViewportClient->Viewport,
			PlayWorld->Scene,
			GameViewportClient->EngineShowFlags )
			.SetRealtimeUpdate(true) );


		// Calculate a view where the player is to update the streaming from the players start location
		FVector ViewLocation;
		FRotator ViewRotation;
		Player->CalcSceneView( &ViewFamily, /*out*/ ViewLocation, /*out*/ ViewRotation, GameViewportClient->Viewport );

		// Update level streaming.
		PlayWorld->FlushLevelStreaming( &ViewFamily );
	}
}

void UEditorEngine::PlayInEditor( UWorld* InWorld, bool bInSimulateInEditor )
{
	double PIEStartTime = FPlatformTime::Seconds();

	bool bStartInSpectatorMode = false;
	
	FModifierKeysState KeysState = FSlateApplication::Get().GetModifierKeys();
	if (bInSimulateInEditor || KeysState.IsControlDown())
	{
		// if control is pressed, start in spectator mode
		bStartInSpectatorMode = true;
	}

	// Prompt the user that Matinee must be closed before PIE can occur.
	if( GEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_InterpEdit) )
	{
		const bool bContinuePIE = EAppReturnType::Yes == FMessageDialog::Open( EAppMsgType::YesNo, NSLOCTEXT("UnrealEd", "PIENeedsToCloseMatineeQ", "'Play in Editor' must close UnrealMatinee.  Continue?") );
		if ( !bContinuePIE )
		{
			return;
		}
		GEditorModeTools().DeactivateMode( FBuiltinEditorModes::EM_InterpEdit );
	}

	FBlueprintEditorUtils::FindAndSetDebuggableBlueprintInstances();

	FEditorDelegates::BeginPIE.Broadcast(bInSimulateInEditor);

	// let navigation know PIE starts so it can avoid any blueprint creation/deletion/instantiation affect editor map's navmesh changes
#if WITH_NAVIGATION_GENERATOR
	if (InWorld->GetNavigationSystem())
	{
		InWorld->GetNavigationSystem()->OnPIEStart();
	}
#endif // WITH_NAVIGATION_GENERATOR

	ULevelEditorPlaySettings const* EditorPlayInSettings = GetDefault<ULevelEditorPlaySettings>();
	check(EditorPlayInSettings);

	// Prompt the user to compile any dirty Blueprints before PIE can occur.
	bool bAnyBlueprintErrors = false;
	bool bAnyBlueprintsDirty = false;
	{
		FString DirtyBlueprints;
		FString ErrorBlueprints;

		TArray<UBlueprint*> BlueprintsToRecompile;

		double BPRegenStartTime = FPlatformTime::Seconds();
		for (TObjectIterator<UBlueprint> BlueprintIt; BlueprintIt; ++BlueprintIt)
		{
			UBlueprint* Blueprint = *BlueprintIt;

			// If the blueprint isn't fresh, try to recompile it automatically
			if( EditorPlayInSettings->AutoRecompileBlueprints )
			{
				// do not try to recompile BPs that have not changed since they last failed to compile, so don't check Blueprint->IsUpToDate()
				const bool bIsDirtyAndShouldBeRecompiled = Blueprint->IsPossiblyDirty();
				if( !FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint) 
					&& (bIsDirtyAndShouldBeRecompiled || FBlueprintEditorUtils::IsLevelScriptBlueprint(Blueprint))
					&& (Blueprint->Status != BS_Unknown) )
				{
					BlueprintsToRecompile.Add(Blueprint);
				}
				else if(BS_Error == Blueprint->Status)
				{
					bAnyBlueprintErrors = true;
					ErrorBlueprints += FString::Printf(TEXT("\n   %s"), *Blueprint->GetName());
				}
			}
			else
			{
				// Record blueprints that are not fully recompiled or had an error
				switch (Blueprint->Status)
				{
				case BS_Unknown:
					// Treating unknown as up to date for right now
					break;
				case BS_Error:
					bAnyBlueprintErrors = true;
					ErrorBlueprints += FString::Printf(TEXT("\n   %s"), *Blueprint->GetName());
					break;
				case BS_UpToDate:
				case BS_UpToDateWithWarnings:
					break;
				default:
				case BS_Dirty:
					bAnyBlueprintsDirty = true;
					DirtyBlueprints += FString::Printf(TEXT("\n   %s"), *Blueprint->GetName());
					break;
				}
			}
		}

		FMessageLog BlueprintLog("BlueprintLog");

		if( EditorPlayInSettings->AutoRecompileBlueprints )
		{
			if( BlueprintsToRecompile.Num() > 0 )
			{
				BlueprintLog.NewPage(LOCTEXT("BlueprintAutoCompilationPageLabel", "Pre-PIE auto-recompile"));

				// Recompile all necessary blueprints in a single loop, saving GC until the end
				for( auto BlueprintIt = BlueprintsToRecompile.CreateIterator(); BlueprintIt; ++BlueprintIt )
				{
					UBlueprint* Blueprint = *BlueprintIt;

					// Cache off the dirty flag for the package, so we can restore it later
					UPackage* Package = Cast<UPackage>(Blueprint->GetOutermost());
					const bool bIsPackageDirty = Package ? Package->IsDirty() : false;

					FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
					Blueprint->BroadcastChanged();

					UE_LOG(LogPlayLevel, Log, TEXT("[PIE] Compiling %s before PIE..."), *Blueprint->GetName());
					FKismetEditorUtilities::CompileBlueprint(Blueprint, false, true);
					const bool bHadError = (!Blueprint->IsUpToDate() && Blueprint->Status != BS_Unknown);
					bAnyBlueprintErrors |= bHadError;
					if (bHadError)
					{
						FFormatNamedArguments Arguments;
						Arguments.Add(TEXT("Name"), FText::FromString(Blueprint->GetName()));

						BlueprintLog.Info( FText::Format(LOCTEXT("BlueprintCompileFailed", "Blueprint {Name} failed to compile"), Arguments) );
					}

					// Restore the dirty package flag
					if (Package)
					{
						Package->SetDirtyFlag(bIsPackageDirty);
					}
				}

				CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );

				UE_LOG(LogPlayLevel, Log, TEXT("PIE:  Blueprint regeneration took %d ms (%i blueprints)"), (int32)((FPlatformTime::Seconds() - BPRegenStartTime) * 1000), BlueprintsToRecompile.Num());
			}
			else
			{
				UE_LOG(LogPlayLevel, Log, TEXT("PIE:  No blueprints needed recompiling"));
			}
		}
		else
		{
			if (bAnyBlueprintsDirty)
			{
				FFormatNamedArguments Args;
				Args.Add( TEXT("DirtyBlueprints"), FText::FromString( DirtyBlueprints ) );

				const bool bCompileDirty = EAppReturnType::Yes == FMessageDialog::Open( EAppMsgType::YesNo, FText::Format( NSLOCTEXT("PlayInEditor", "PrePIE_BlueprintsDirty", "One or more blueprints have been modified without being recompiled.  Do you want to compile them now?{DirtyBlueprints}"), Args ) );
				
				if ( bCompileDirty)
				{
					BlueprintLog.NewPage(LOCTEXT("BlueprintCompilationPageLabel", "Pre-PIE recompile"));

					// Compile all blueprints that aren't up to date
					for (TObjectIterator<UBlueprint> BlueprintIt; BlueprintIt; ++BlueprintIt)
					{
						UBlueprint* Blueprint = *BlueprintIt;
						// do not try to recompile BPs that have not changed since they last failed to compile, so don't check Blueprint->IsUpToDate()
						const bool bIsDirtyAndShouldBeRecompiled = Blueprint->IsPossiblyDirty();
						if (!FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint) && bIsDirtyAndShouldBeRecompiled)
						{
							// Cache off the dirty flag for the package, so we can restore it later
							UPackage* Package = Cast<UPackage>(Blueprint->GetOutermost());
							const bool bIsPackageDirty = Package ? Package->IsDirty() : false;

							FKismetEditorUtilities::CompileBlueprint(Blueprint);
							if (Blueprint->Status == BS_Error)
							{
								bAnyBlueprintErrors = true;
							}

							// Restore the dirty flag
							if (Package)
							{
								Package->SetDirtyFlag(bIsPackageDirty);
							}
						}
					}
				}
			}
		}

		if ( bAnyBlueprintErrors && !GIsDemoMode )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("ErrorBlueprints"), FText::FromString( ErrorBlueprints ) );

			// There was at least one blueprint with an error, make sure the user is OK with that.
			const bool bContinuePIE = EAppReturnType::Yes == FMessageDialog::Open( EAppMsgType::YesNo, FText::Format( NSLOCTEXT("PlayInEditor", "PrePIE_BlueprintErrors", "One or more blueprints has an unresolved compiler error, are you sure you want to Play in Editor?{ErrorBlueprints}"), Args ) );
			if ( !bContinuePIE )
			{
				FEditorDelegates::EndPIE.Broadcast(bInSimulateInEditor);
				if (InWorld->GetNavigationSystem())
				{
					InWorld->GetNavigationSystem()->OnPIEEnd();
				}

				return;
			}
		}
	}


	const FScopedBusyCursor BusyCursor;

	// If there's level already being played, close it. (This may change GWorld)
	if(PlayWorld)
	{
		// immediately end the playworld
		EndPlayMap();
	}

	// remember old GWorld
	EditorWorld = InWorld;

	// Clear any messages from last time
	GEngine->ClearOnScreenDebugMessages();

	// Flush all audio sources from the editor world
	if( GetAudioDevice() )
	{
		GetAudioDevice()->Flush( EditorWorld );
		GetAudioDevice()->ResetInterpolation();
	}
	EditorWorld->bAllowAudioPlayback = false;

	ULevelEditorPlaySettings* PlayInSettings = Cast<ULevelEditorPlaySettings>(ULevelEditorPlaySettings::StaticClass()->GetDefaultObject());
	int32 PIEInstance = 0;

	if (!GEditor->bAllowMultiplePIEWorlds)
	{
		PlayInSettings->RunUnderOneProcess = false;
	}

	EPlayNetMode OrigNetMode = PlayInSettings->PlayNetMode;

	if (PlayInSettings->RunUnderOneProcess)
	{
		if (!PlayInSettings->PlayNetDedicated && (PlayInSettings->PlayNumberOfClients == 1))
		{
			// Since we don't expose PlayNetMode as an option when doing RunUnderOnProcess,
			// we take 1 player and !PlayNetdedicated and being standalone.
			PlayInSettings->PlayNetMode = EPlayNetMode::PIE_Standalone;
		}
		else
		{
			// We are doing multi-player under one process so make sure the NetMode is ListenServer
			PlayInSettings->PlayNetMode = EPlayNetMode::PIE_ListenServer;
		}
	}

	// Can't allow realtime viewports whilst in PIE so disable it for ALL viewports here.
	DisableRealtimeViewports();

	if (bInSimulateInEditor || (PlayInSettings->PlayNetMode == EPlayNetMode::PIE_Standalone) || !PlayInSettings->RunUnderOneProcess)
	{
		// Only spawning 1 PIE instance under this process
		FWorldContext &PieWorldContext = CreateNewWorldContext(EWorldType::PIE);
		PieWorldContext.PIEInstance = PIEInstance++;

		CreatePlayInEditorWorld(PieWorldContext, bInSimulateInEditor, bAnyBlueprintErrors, bStartInSpectatorMode, PIEStartTime);
	}
	else
	{
		// Spawning multiple PIE instances
		if (PlayInSettings->MultipleInstancePositions.Num() == 0)
		{
			PlayInSettings->MultipleInstancePositions.SetNum(1);
		}

		PlayInSettings->MultipleInstancePositions[0] = PlayInSettings->NewWindowPosition; 

		int32 NextX = 0;
		int32 NextY = 0;
		int32 SettingsIndex = 1;
		int32 ClientNum = 0;

		PIEInstance = 1;

		// Server
		FString ServerPrefix;
		{
			PlayInSettings->PlayNetMode = EPlayNetMode::PIE_ListenServer;
			FWorldContext &PieWorldContext = CreateNewWorldContext(EWorldType::PIE);

			PieWorldContext.PIEInstance = PIEInstance++;
			PieWorldContext.RunAsDedicated = PlayInSettings->PlayNetDedicated;

			if (!PlayInSettings->PlayNetDedicated)
			{
				ClientNum++;
				GetMultipleInstancePositions(SettingsIndex++, NextX, NextY);
			}
			
			CreatePlayInEditorWorld(PieWorldContext, bInSimulateInEditor, bAnyBlueprintErrors, bStartInSpectatorMode, PIEStartTime);		
			ServerPrefix = PieWorldContext.PIEPrefix;
		}

		// Clients
		for ( ; ClientNum < PlayInSettings->PlayNumberOfClients; ++ClientNum)
		{
			PlayInSettings->PlayNetMode = EPlayNetMode::PIE_Client;
			FWorldContext &PieWorldContext = CreateNewWorldContext(EWorldType::PIE);
			PieWorldContext.PIEInstance = PIEInstance++;

			GetMultipleInstancePositions(SettingsIndex++, NextX, NextY);
			CreatePlayInEditorWorld(PieWorldContext, bInSimulateInEditor, bAnyBlueprintErrors, bStartInSpectatorMode, PIEStartTime);
			
			PieWorldContext.PIERemapPrefix = ServerPrefix;
		}

		// Restore window settings
		GetMultipleInstancePositions(0, NextX, NextY);	// restore cached settings
	}

	PlayInSettings->MultipleInstanceLastHeight = PlayInSettings->NewWindowHeight;
	PlayInSettings->MultipleInstanceLastWidth = PlayInSettings->NewWindowWidth;
	PlayInSettings->PlayNetMode = OrigNetMode;

	if (bInSimulateInEditor)
	{
		ToggleBetweenPIEandSIE();
	}
}

UWorld* UEditorEngine::CreatePlayInEditorWorld(FWorldContext &PieWorldContext, bool bInSimulateInEditor, bool bAnyBlueprintErrors, bool bStartInSpectatorMode, float PIEStartTime)
{
	//const FString Prefix = PLAYWORLD_PACKAGE_PREFIX;
	const FString WorldPackageName = EditorWorld->GetOutermost()->GetName();
	
	// Start a new PIE log page
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Package"), FText::FromString(FPackageName::GetLongPackageAssetName(WorldPackageName)));
		Arguments.Add(TEXT("TimeStamp"), FText::AsDateTime(FDateTime::Now()));

		FText PIESessionLabel = bInSimulateInEditor ?
			FText::Format(LOCTEXT("SIESessionLabel", "SIE session: {Package} ({TimeStamp})"), Arguments) : 
			FText::Format(LOCTEXT("PIESessionLabel", "PIE session: {Package} ({TimeStamp})"), Arguments);

		FMessageLog("PIE").NewPage(PIESessionLabel);
	}

	// Establish World Context for PIE World
	PieWorldContext.LastURL.Map = WorldPackageName;
	PieWorldContext.PIEPrefix = FString::Printf(TEXT("UEDPIE_%d_"), PieWorldContext.PIEInstance);

	const ULevelEditorPlaySettings* PlayInSettings = GetDefault<ULevelEditorPlaySettings>();

	// We always need to create a new PIE world unless we're using the editor world for SIE
	FString PlayWorldMapName;

	if (PlayInSettings->PlayNetMode == PIE_Client)
	{
		// We are going to connect, so just load an empty world
		PlayWorld = CreatePIEWorldFromEntry(PieWorldContext, EditorWorld, PlayWorldMapName);
	}
	else if (PlayInSettings->PlayNetMode == PIE_ListenServer && !PlayInSettings->RunUnderOneProcess)
	{
		// We *have* to save the world to disk in order to be a listen server that allows other processes to connect.
		// Otherwise, clients would not be able to load the world we are using
		PlayWorld = CreatePIEWorldBySavingToTemp(PieWorldContext, EditorWorld, PlayWorldMapName);
	}
	else
	{
		// Standard PIE path: just duplicate the EditorWorld
		PlayWorld = CreatePIEWorldByDuplication(PieWorldContext, EditorWorld, PlayWorldMapName);
	}

	if (!PlayWorld)
	{
		FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Error_FailedCreateEditorPreviewWorld", "Failed to create editor preview world."));
		// The load failed, so restore GWorld.rema
		if( GIsPlayInEditorWorld )
		{
			RestoreEditorWorld( EditorWorld );
		}

		FEditorDelegates::EndPIE.Broadcast(bInSimulateInEditor);
#if WITH_NAVIGATION_GENERATOR
		if (EditorWorld->GetNavigationSystem())
		{
			EditorWorld->GetNavigationSystem()->OnPIEEnd();
		}
#endif // WITH_NAVIGATION_GENERATOR

		return NULL;
	}
	
	PieWorldContext.SetCurrentWorld(PlayWorld);
	PieWorldContext.AddRef(PlayWorld);	// Tie this context to this UEngine::PlayWorld*

	// make sure we can clean up this world!
	PlayWorld->ClearFlags(RF_Standalone);
	PlayWorld->bKismetScriptError = bAnyBlueprintErrors;

	// If a start location is specified, spawn a temporary PlayerStartPIE actor at the start location and use it as the portal.
	AActor* PlayerStart = NULL;
	GWorld = PlayWorld;

	if ( SpawnPlayFromHereStart( PlayWorld, PlayerStart, PlayWorldLocation, PlayWorldRotation ) == false)
	{
		// go back to using the real world as GWorld
		GWorld = EditorWorld;
		EndPlayMap();

		return NULL;
	}

	SetPlayInEditorWorld( PlayWorld );

	// make a URL
	FURL URL;
	
	// If the user wants to start in spectator mode, do not use the custom play world for now
	if( UserEditedPlayWorldURL.Len() > 0 && !bStartInSpectatorMode )
	{
		// If the user edited the play world url. Verify that the map name is the same as the currently loaded map.
		URL = FURL(NULL, *UserEditedPlayWorldURL, TRAVEL_Absolute);
		if( URL.Map != PlayWorldMapName )
		{
			// Ensure the URL map name is the same as the generated play world map name.
			URL.Map = PlayWorldMapName;
		}
	}
	else
	{
		// The user did not edit the url, just build one from scratch.
		URL = FURL(NULL, *BuildPlayWorldURL( *PlayWorldMapName, bStartInSpectatorMode ), TRAVEL_Absolute);
	}

	if( !PlayWorld->SetGameMode(URL) )
	{		
		//Setting the game mode failed so bail 
		FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Error_FailedCreateEditorPreviewWorld", "Failed to create editor preview world."));
		RestoreEditorWorld( EditorWorld );

#if WITH_NAVIGATION_GENERATOR
		if (EditorWorld->GetNavigationSystem())
		{
			EditorWorld->GetNavigationSystem()->OnPIEEnd();
		}
#endif // WITH_NAVIGATION_GENERATOR

		EndPlayMap();
		return NULL;
	}

	if( GetAudioDevice() )
	{
		GetAudioDevice()->SetDefaultBaseSoundMix( PlayWorld->GetWorldSettings()->DefaultBaseSoundMix );
	}

#if PLATFORM_64BITS
	const FString PlatformBitsString( TEXT( "64" ) );
#else
	const FString PlatformBitsString( TEXT( "32" ) );
#endif
	FFormatNamedArguments Args;
	Args.Add( TEXT("GameName"), FText::FromString( FString( FApp::GetGameName() ) ) );
	Args.Add( TEXT("PlatformBits"), FText::FromString( PlatformBitsString ) );
	Args.Add( TEXT("RHIName"), FText::FromName( LegacyShaderPlatformToShaderFormat( GRHIShaderPlatform ) ) );

	if (PlayInSettings->PlayNetMode == PIE_Client)
	{
		Args.Add( TEXT("NetMode"), FText::FromString( FString::Printf(TEXT("Client %d"), PieWorldContext.PIEInstance-1) ) );
	}
	else if (PlayInSettings->PlayNetMode == PIE_ListenServer)
	{
		Args.Add( TEXT("NetMode"), FText::FromString( TEXT("Server")));
	}
	else
	{
		Args.Add( TEXT("NetMode"), FText::FromString( TEXT("Standalone")));
	}

	const FText ViewportName = FText::Format( NSLOCTEXT("UnrealEd", "PlayInEditor_RHI_F", "{GameName} Game Preview {NetMode} ({PlatformBits}-bit/{RHIName})" ), Args );

	// Make a list of all the selected actors
	TArray<UObject *> SelectedActors;
	for ( FSelectionIterator It( GetSelectedActorIterator() ); It; ++It )
	{
		AActor* Actor = static_cast<AActor*>( *It );
		if (Actor)
		{
			checkSlow( Actor->IsA(AActor::StaticClass()) );

			SelectedActors.Add( Actor );
		}
	}

	// Unselect everything
	GEditor->SelectNone( true, true, false );
	GetSelectedActors()->DeselectAll();
	GetSelectedObjects()->DeselectAll();

	// For every actor that was selected previously, make sure it's sim equivalent is selected
	for ( int32 ActorIndex = 0; ActorIndex < SelectedActors.Num(); ++ActorIndex )
	{
		AActor* Actor = Cast<AActor>( SelectedActors[ ActorIndex ] );
		if (Actor)
		{
			ActorsThatWereSelected.Add( Actor );

			AActor* SimActor = EditorUtilities::GetSimWorldCounterpartActor(Actor);
			if (SimActor && !SimActor->bHidden && bInSimulateInEditor)
			{
				SelectActor( SimActor, true, false );
			}
		}
	}

	// For play in editor, this is the viewport widget where the game is being displayed
	TSharedPtr<SViewport> PieViewportWidget;

	// Initialize the viewport client.
	UGameViewportClient* ViewportClient = NULL;
	ULocalPlayer *NewLocalPlayer = NULL;
	
	if (!PieWorldContext.RunAsDedicated)
	{
		ViewportClient = ConstructObject<UGameViewportClient>(GameViewportClientClass,this);
		ViewportClient->SetReferenceToWorldContext(PieWorldContext);
		GameViewport = ViewportClient;
		GameViewport->bIsPlayInEditorViewport = true;
		PieWorldContext.GameViewport = ViewportClient;

		FSlatePlayInEditorInfo & SlatePlayInEditorSession = SlatePlayInEditorMap.Add( PieWorldContext.ContextHandle, FSlatePlayInEditorInfo() );
		SlatePlayInEditorSession.DestinationSlateViewport = RequestedDestinationSlateViewport;	// Might be invalid depending how pie was launched. Code below handles this.
		RequestedDestinationSlateViewport = NULL;

		FString Error;
		NewLocalPlayer = ViewportClient->Init(Error);
		if(!NewLocalPlayer)
		{
			FMessageDialog::Open( EAppMsgType::Ok, FText::Format(NSLOCTEXT("UnrealEd", "Error_CouldntSpawnPlayer", "Couldn't spawn player: {0}"), FText::FromString(Error)) );
			// go back to using the real world as GWorld
			RestoreEditorWorld( EditorWorld );
			EndPlayMap();
			return NULL;
		}

		if (!bInSimulateInEditor)
		{
			SlatePlayInEditorSession.EditorPlayer = NewLocalPlayer;
		}
			
		// Note: For K2 debugging purposes this MUST be created before beginplay is called because beginplay can trigger breakpoints
		// and we need to be able to refocus the pie viewport afterwards so it must be created first in order for us to find it
		{
			// Only create a separate viewport and window if we aren't playing in a current viewport
			if( SlatePlayInEditorSession.DestinationSlateViewport.IsValid() )
			{
				TSharedPtr<ILevelViewport> LevelViewportRef = SlatePlayInEditorSession.DestinationSlateViewport.Pin();

				LevelViewportRef->StartPlayInEditorSession( ViewportClient );
			}
			else
			{		
				// Create the top level pie window and add it to Slate
				uint32 NewWindowHeight = PlayInSettings->NewWindowHeight;
				uint32 NewWindowWidth = PlayInSettings->NewWindowWidth;
				FIntPoint NewWindowPosition = PlayInSettings->NewWindowPosition;
				bool CenterNewWindow = PlayInSettings->CenterNewWindow;

				// Setup size for PIE window
				if ((NewWindowWidth <= 0) || (NewWindowHeight <= 0))
				{
					// Get desktop metrics
					FDisplayMetrics DisplayMetrics;
					FSlateApplication::Get().GetDisplayMetrics( DisplayMetrics );

					const FVector2D DisplaySize(
						DisplayMetrics.PrimaryDisplayWorkAreaRect.Right - DisplayMetrics.PrimaryDisplayWorkAreaRect.Left,
						DisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom - DisplayMetrics.PrimaryDisplayWorkAreaRect.Top
					);

					// Use a centered window at the default window size
					NewWindowPosition.X = 0;
					NewWindowPosition.Y = 0;
					NewWindowWidth = 0.75 * DisplaySize.X;
					NewWindowHeight = 0.75 * DisplaySize.Y;
					CenterNewWindow = true;
				}

				TSharedRef<SWindow> PieWindow = SNew(SWindow)
					.Title(ViewportName)
					.ScreenPosition(FVector2D( NewWindowPosition.X, NewWindowPosition.Y ))
					.ClientSize(FVector2D( NewWindowWidth, NewWindowHeight ))
					.AutoCenter(CenterNewWindow ? EAutoCenter::PreferredWorkArea : EAutoCenter::None);

				// Setup a delegate for switching to the play world on slate input events, drawing and ticking
				FOnSwitchWorldHack OnWorldSwitch = FOnSwitchWorldHack::CreateUObject( this, &UEditorEngine::OnSwitchWorldForSlatePieWindow );
				PieWindow->SetOnWorldSwitchHack( OnWorldSwitch );

				FSlateApplication::Get().AddWindow( PieWindow );

				TSharedRef<SOverlay> ViewportOverlayWidgetRef = SNew( SOverlay );
				PieViewportWidget = 
					SNew( SViewport )
						.EnableGammaCorrection( false )// Gamma correction in the game is handled in post processing in the scene renderer
						[
							ViewportOverlayWidgetRef
						];

				// Create a viewport widget for the game to render in.
				PieWindow->SetContent( PieViewportWidget.ToSharedRef() );

				// Ensure the PIE window appears does not appear behind other windows.
				PieWindow->BringToFront();

				ViewportClient->SetViewportOverlayWidget( PieWindow, ViewportOverlayWidgetRef );

				// Set up a notification when the window is closed so we can clean up PIE
				{
					struct FLocal
					{
						static void OnPIEWindowClosed( const TSharedRef< SWindow >& WindowBeingClosed, TWeakPtr< SViewport > PIEViewportWidget, int32 index )
						{
							// Save off the window position
							const FVector2D PIEWindowPos = WindowBeingClosed->GetPositionInScreen();
							const FVector2D PIEWindowClientSize = WindowBeingClosed->GetClientSizeInScreen();

							ULevelEditorPlaySettings* LevelEditorPlaySettings = ULevelEditorPlaySettings::StaticClass()->GetDefaultObject<ULevelEditorPlaySettings>();

							if (index <= 0)
							{
								LevelEditorPlaySettings->NewWindowPosition.X = FPlatformMath::Round(PIEWindowPos.X);
								LevelEditorPlaySettings->NewWindowPosition.Y = FPlatformMath::Round(PIEWindowPos.Y);
							}
							else
							{
								if (index >= LevelEditorPlaySettings->MultipleInstancePositions.Num())
								{
									LevelEditorPlaySettings->MultipleInstancePositions.SetNum(index + 1);
								}

								LevelEditorPlaySettings->MultipleInstancePositions[index] = FIntPoint(PIEWindowPos.X, PIEWindowPos.Y); 
							}

							LevelEditorPlaySettings->PostEditChange();
							LevelEditorPlaySettings->SaveConfig();

							// Route the callback
							PIEViewportWidget.Pin()->OnWindowClosed( WindowBeingClosed );
						}
					};
				
					PieWindow->SetOnWindowClosed(FOnWindowClosed::CreateStatic(&FLocal::OnPIEWindowClosed, TWeakPtr<SViewport>(PieViewportWidget), 
						PieWorldContext.PIEInstance - (PlayInSettings->PlayNetDedicated ? 1 : 0)));
				}

				// Create a new viewport that the viewport widget will use to render the game
				SlatePlayInEditorSession.SlatePlayInEditorWindowViewport = MakeShareable( new FSceneViewport( ViewportClient, PieViewportWidget ) );
				PieViewportWidget->SetViewportInterface( SlatePlayInEditorSession.SlatePlayInEditorWindowViewport.ToSharedRef() );

				SlatePlayInEditorSession.SlatePlayInEditorWindow = PieWindow;

				// Let the viewport client know what viewport is using it.  We need to set the Viewport Frame as 
				// well (which in turn sets the viewport) so that SetRes command will work.
				ViewportClient->SetViewportFrame(SlatePlayInEditorSession.SlatePlayInEditorWindowViewport.Get());
				// Mark the viewport as PIE viewport
				ViewportClient->Viewport->SetPlayInEditorViewport( ViewportClient->bIsPlayInEditorViewport );

				// Ensure the window has a valid size before calling BeginPlay
				SlatePlayInEditorSession.SlatePlayInEditorWindowViewport->ResizeFrame( PieWindow->GetSizeInScreen().X, PieWindow->GetSizeInScreen().Y, false, PieWindow->GetPositionInScreen().X, PieWindow->GetPositionInScreen().Y );
			}
		}
	}

	if ( GameViewport != NULL && GameViewport->Viewport != NULL )
	{
		// Set the game viewport that was just created as a pie viewport.
		GameViewport->Viewport->SetPlayInEditorViewport( true );
	}

	// Disable the screensaver when PIE is running.
	EnableScreenSaver( false );

	// Navigate PIE world to the Editor world origin location
	// and stream-in all relevant levels around that location
	if (PlayWorld->WorldComposition)
	{
		PlayWorld->NavigateTo(PlayWorld->GlobalOriginOffset);
	}
	
	if (PlayWorld->GetNavigationSystem() != NULL)
	{
		PlayWorld->GetNavigationSystem()->OnWorldInitDone(PieWorldContext.GamePlayers.Num() > 0 ? NavigationSystem::PIEMode : NavigationSystem::SimulationMode);
	}

	EditorWorld->TransferBlueprintDebugReferences(PlayWorld);

	PlayWorld->BeginPlay(URL);

	// This must have already been set with a call to DisableRealtimeViewports() outside of this method.
	check(!IsAnyViewportRealtime());
	
	if( NewLocalPlayer )
	{
		FString Error;
		if(!NewLocalPlayer->SpawnPlayActor(URL.ToString(1),Error,PlayWorld))
		{
			FMessageDialog::Open( EAppMsgType::Ok, FText::Format(NSLOCTEXT("UnrealEd", "Error_CouldntSpawnPlayer", "Couldn't spawn player: {0}"), FText::FromString(Error)) );
			// go back to using the real world as GWorld
			RestoreEditorWorld( EditorWorld );
			EndPlayMap();
			return NULL;
		}
	}
	
	if ( GameViewport != NULL && GameViewport->Viewport != NULL )
	{
		// Stream any levels now that need to be loaded before the game starts
		InitStreamingLevelsForPIEStartup( GameViewport, PlayWorld );
	}

	// Set up a delegate to be called in Slate when GWorld needs to change.  Slate does not have direct access to the playworld to switch itself
	FScopedConditionalWorldSwitcher::SwitchWorldForPIEDelegate = FOnSwitchWorldForPIE::CreateUObject( this, &UEditorEngine::OnSwitchWorldsForPIE );

	if( PieViewportWidget.IsValid() )
	{
		// Register the new viewport widget with Slate for viewport specific message routing.
		FSlateApplication::Get().RegisterGameViewport( PieViewportWidget.ToSharedRef() );
	}

	// go back to using the real world as GWorld
	RestoreEditorWorld( EditorWorld );

	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("MapName"), FText::FromString(PlayWorldMapName));
		Arguments.Add(TEXT("StartTime"), FPlatformTime::Seconds() - PIEStartTime);
		FMessageLog("PIE").Info( FText::Format(LOCTEXT("PIEStartTime", "Play in editor start time for {MapName} {StartTime}"), Arguments) );
	}

	// Update the details window with the actors we have just selected
	GUnrealEd->UpdateFloatingPropertyWindows();

	// Clean up any editor actors being referenced 
	GEngine->BroadcastLevelActorsChanged();

	
	// Auto connect to a local instance if necessary
	if (PlayInSettings->PlayNetMode == PIE_Client)
	{
		FURL BaseURL = PieWorldContext.LastURL;
		FString Error;
		if (Browse( PieWorldContext, FURL(&BaseURL, TEXT("127.0.0.1"), (ETravelType)TRAVEL_Absolute), Error ) == EBrowseReturnVal::Pending)
		{
			TransitionType = TT_WaitingToConnect;
		}
		else
		{
			FMessageDialog::Open( EAppMsgType::Ok, FText::Format(NSLOCTEXT("UnrealEd", "Error_CouldntLaunchPIEClient", "Couldn't Launch PIE Client: {0}"), FText::FromString(Error)) );
			EndPlayMap();
			return NULL;
		}
	}
	else if (PlayInSettings->PlayNetMode == PIE_ListenServer)
	{
		PlayWorld->Listen(URL);
	}
	
	return PlayWorld;
}

FViewport* UEditorEngine::GetActiveViewport()
{
	// Get the Level editor module and request the Active Viewport.
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().GetModuleChecked<FLevelEditorModule>( TEXT("LevelEditor") );

	TSharedPtr<ILevelViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();

	if ( ActiveLevelViewport.IsValid() )
	{
		return ActiveLevelViewport->GetActiveViewport();
	}
	
	return NULL;
}

FViewport* UEditorEngine::GetPIEViewport()
{
	// Check both cases where the PIE viewport may be, otherwise return NULL if none are found.
	if( GameViewport )
	{
		return GameViewport->Viewport;
	}
	else
	{
		for (auto It = WorldList.CreateIterator(); It; ++It)
		{
			FWorldContext &WorldContext = *It;
			if (WorldContext.WorldType == EWorldType::PIE)
			{
				FSlatePlayInEditorInfo &SlatePlayInEditorSession = SlatePlayInEditorMap.FindChecked(WorldContext.ContextHandle);
				if (SlatePlayInEditorSession.SlatePlayInEditorWindowViewport.IsValid() )
				{
					return SlatePlayInEditorSession.SlatePlayInEditorWindowViewport.Get();
				}
			}
		}
	}

	return NULL;
}

void UEditorEngine::ToggleBetweenPIEandSIE()
{
	bIsToggleBetweenPIEandSIEQueued = false;

	// The first PIE world context is the one that can toggle between PIE and SIE
	// Network PIE/SIE toggling is not really meant to be supported.
	FSlatePlayInEditorInfo * SlateInfoPtr = NULL;
	for (auto It = WorldList.CreateIterator(); It && !SlateInfoPtr; ++It)
	{
		FWorldContext &WorldContext = *It;
		if (WorldContext.WorldType == EWorldType::PIE && !WorldContext.RunAsDedicated)
		{
			SlateInfoPtr = SlatePlayInEditorMap.Find(WorldContext.ContextHandle);
			break;
		}
	}

	if (!SlateInfoPtr)
	{
		check(false);
		return;
	}

	FSlatePlayInEditorInfo & SlatePlayInEditorSession = *SlateInfoPtr;

	// This is only supported inside SLevelEditor viewports currently
	TSharedPtr<ILevelViewport> LevelViewport = SlatePlayInEditorSession.DestinationSlateViewport.Pin();
	if( ensure(LevelViewport.IsValid()) )
	{
		FLevelEditorViewportClient& EditorViewportClient = LevelViewport->GetLevelViewportClient();

		// Toggle to pie if currently simulating
		if( bIsSimulatingInEditor )
		{
			// The undo system may have a reference to a SIE object that is about to be destroyed, so clear the transactions
			ResetTransaction( NSLOCTEXT("UnrealEd", "ToggleBetweenPIEandSIE", "Toggle Between PIE and SIE") );

			// The editor viewport client wont be visible so temporarily disable it being realtime
			EditorViewportClient.SetRealtime( false, true );

			if (!SlatePlayInEditorSession.EditorPlayer.IsValid())
			{
				OnSwitchWorldsForPIE(true);

				UWorld* World = GameViewport->GetWorld();
				if (World->GetAuthGameMode())	// If there is no GameMode, we are probably the client and cannot RestartPlayer.
				{
					APlayerController* PC = World->GetFirstPlayerController();
					PC->PlayerState->bOnlySpectator = false;
					World->GetAuthGameMode()->RestartPlayer(World->GetFirstPlayerController());
				}

				OnSwitchWorldsForPIE(false);
			}

			// A game viewport already exists, tell the level viewport its in to swap to it
			LevelViewport->SwapViewportsForPlayInEditor();

			// No longer simulating
			EditorViewportClient.SetIsSimulateInEditorViewport( false );
			bIsSimulatingInEditor = false;
		}
		else
		{
			// Swap to simulate from PIE
			LevelViewport->SwapViewportsForSimulateInEditor();
	
			EditorViewportClient.SetIsSimulateInEditorViewport( true );
			bIsSimulatingInEditor = true;

			// Make sure the viewport is in real-time mode
			EditorViewportClient.SetRealtime( true );

			// The Simulate window should show stats
			EditorViewportClient.SetShowStats( true );

			if( SlatePlayInEditorSession.EditorPlayer.IsValid() )
			{
				// Move the editor camera to where the player was.  
				FVector ViewLocation;
				FRotator ViewRotation;
				SlatePlayInEditorSession.EditorPlayer.Get()->PlayerController->GetPlayerViewPoint( ViewLocation, ViewRotation );
				EditorViewportClient.SetViewLocation( ViewLocation );

				if( EditorViewportClient.IsPerspective() )
				{
					// Rotation only matters for perspective viewports not orthographic
					EditorViewportClient.SetViewRotation( ViewRotation );
				}
			}
		}
	}

	// Backup ActorsThatWereSelected as this will be cleared whilst deselecting
	TArray<TWeakObjectPtr<class AActor> > BackupOfActorsThatWereSelected(ActorsThatWereSelected);

	// Unselect everything
	GEditor->SelectNone( true, true, false );
	GetSelectedActors()->DeselectAll();
	GetSelectedObjects()->DeselectAll();

	// restore the backup
	ActorsThatWereSelected = BackupOfActorsThatWereSelected;

	// make sure each selected actors sim equivalent is selected if we're Simulating but not if we're Playing
	for ( int32 ActorIndex = 0; ActorIndex < ActorsThatWereSelected.Num(); ++ActorIndex )
	{
		TWeakObjectPtr<AActor> Actor = ActorsThatWereSelected[ ActorIndex ].Get();
		if (Actor.IsValid())
		{
			AActor* SimActor = EditorUtilities::GetSimWorldCounterpartActor(Actor.Get());
			if (SimActor && !SimActor->bHidden)
			{
				SelectActor( SimActor, bIsSimulatingInEditor, false );
			}
		}
	}
}

int32 UEditorEngine::OnSwitchWorldForSlatePieWindow( int32 WorldID )
{
	static const int32 EditorWorldID = 0;
	static const int32 PieWorldID = 1;

	int32 RestoreID = -1;
	if( WorldID == -1 && GWorld != PlayWorld && PlayWorld != NULL)
	{
		// When we have an invalid world id we always switch to the pie world in the PIE window
		const bool bSwitchToPIE = true; 
		OnSwitchWorldsForPIE( bSwitchToPIE );
		// The editor world was active restore it later
		RestoreID = EditorWorldID;
	}
	else if( WorldID == PieWorldID && GWorld != PlayWorld)
	{
		const bool bSwitchToPIE = true;
		// Want to restore the PIE world and the current world is not already the pie world
		OnSwitchWorldsForPIE( bSwitchToPIE );
	}
	else if( WorldID == EditorWorldID && GWorld != EditorWorld)
	{
		const bool bSwitchToPIE = false;
		// Want to restore the editor world and the current world is not already the editor world
		OnSwitchWorldsForPIE( bSwitchToPIE );
	}
	else
	{
		// Current world is already the same as the world being switched to (nested calls to this for example)
	}

	return RestoreID;
}

void UEditorEngine::OnSwitchWorldsForPIE( bool bSwitchToPieWorld )
{
	if( bSwitchToPieWorld )
	{
		SetPlayInEditorWorld( PlayWorld );
	}
	else
	{
		RestoreEditorWorld( EditorWorld );
	}
}

bool UEditorEngine::PackageUsingExternalObjects( ULevel* LevelToCheck, bool bAddForMapCheck )
{
	check(LevelToCheck);
	bool bFoundExternal = false;
	TArray<UObject*> ExternalObjects;
	if(PackageTools::CheckForReferencesToExternalPackages(NULL, NULL, LevelToCheck, &ExternalObjects ))
	{
		for(int32 ObjectIndex = 0; ObjectIndex < ExternalObjects.Num(); ++ObjectIndex)
		{
			// If the object in question has external references and is not pending deletion, add it to the log and tell the user about it below
			UObject* ExternalObject = ExternalObjects[ObjectIndex];

			if(!ExternalObject->IsPendingKill())
			{
				bFoundExternal = true;
				if( bAddForMapCheck ) 
				{
					FFormatNamedArguments Arguments;
					Arguments.Add(TEXT("ObjectName"), FText::FromString(ExternalObject->GetFullName()));
					FMessageLog("MapCheck").Warning()
						->AddToken(FUObjectToken::Create(ExternalObject))
						->AddToken(FTextToken::Create(FText::Format(LOCTEXT( "MapCheck_Message_UsingExternalObject", "{ObjectName} : Externally referenced"), Arguments ) ))
						->AddToken(FMapErrorToken::Create(FMapErrors::UsingExternalObject));
				}
			}
		}
	}
	return bFoundExternal;
}

UWorld* UEditorEngine::CreatePIEWorldBySavingToTemp(FWorldContext &WorldContext, UWorld* InWorld, FString &PlayWorldMapName)
{
	double StartTime = FPlatformTime::Seconds();
	UWorld* LoadedWorld = NULL;

	// We haven't saved it off yet
	TArray<FString> SavedMapNames;
	SaveWorldForPlay(SavedMapNames);

	if (SavedMapNames.Num() == 0)
	{
		UE_LOG(LogPlayLevel, Warning, TEXT("PIE: Unable to save editor world to temp file"));
		return LoadedWorld;
	}

	// Before loading the map, we need to set these flags to true so that postload will work properly
	GIsPlayInEditorWorld = true;

	UWorld::WorldTypePreLoadMap.FindOrAdd(FName(*SavedMapNames[0])) = EWorldType::PIE;

	// Load the package we saved
	UPackage* EditorLevelPackage = LoadPackage(NULL, *SavedMapNames[0], LOAD_None);
	if( EditorLevelPackage )
	{
		EditorLevelPackage->PackageFlags |= PKG_PlayInEditor;

		// Find world object and use its PersistentLevel pointer.
		LoadedWorld = UWorld::FindWorldInPackage(EditorLevelPackage);

		if (LoadedWorld)
		{
			PostCreatePIEWorld(LoadedWorld);
			UE_LOG(LogPlayLevel, Log, TEXT("PIE: Created PIE world by saving and reloading to %s (%fs)"), *LoadedWorld->GetPathName(), float(FPlatformTime::Seconds() - StartTime));
		}
		else
		{
			UE_LOG(LogPlayLevel, Warning, TEXT("PIE: Unable to find World in loaded package: %s"), *EditorLevelPackage->GetPathName());
		}
	}

	// After loading the map, reset these so that things continue as normal
	GIsPlayInEditorWorld = false;

	PlayWorldMapName = SavedMapNames[0];

	return LoadedWorld;
}

UWorld* UEditorEngine::CreatePIEWorldByDuplication(FWorldContext &WorldContext, UWorld* InWorld, FString &PlayWorldMapName)
{
	double StartTime = FPlatformTime::Seconds();
	UPackage* InPackage = Cast<UPackage>(InWorld->GetOutermost());
	UWorld* CurrentWorld = InWorld;
	UWorld* NewPIEWorld = NULL;
	
	const FString Prefix = PLAYWORLD_PACKAGE_PREFIX;
	const FString WorldPackageName = InPackage->GetName();

	// Preserve the old path keeping EditorWorld name the same
	PlayWorldMapName = FString::Printf(TEXT("%s/%s_%d_%s"), *FPackageName::GetLongPackagePath(WorldPackageName), *Prefix, WorldContext.PIEInstance, *FPackageName::GetLongPackageAssetName(WorldPackageName) );

	// Display a busy cursor while we prepare the PIE world
	const FScopedBusyCursor BusyCursor;

	// Before loading the map, we need to set these flags to true so that postload will work properly
	GIsPlayInEditorWorld = true;

	UWorld::WorldTypePreLoadMap.FindOrAdd(FName(*PlayWorldMapName)) = EWorldType::PIE;

	// Create a package for the PIE world
	UPackage* PlayWorldPackage = CastChecked<UPackage>(CreatePackage(NULL,*PlayWorldMapName));
	PlayWorldPackage->PackageFlags |= PKG_PlayInEditor;
	PlayWorldPackage->PIEInstanceID = WorldContext.PIEInstance;
	PlayWorldPackage->FileName = InPackage->FileName;

	check(GPlayInEditorID == -1 || GPlayInEditorID == WorldContext.PIEInstance);
	GPlayInEditorID = WorldContext.PIEInstance;

	{
		double SDOStart = FPlatformTime::Seconds();

		// Reset any GUID fixups with lazy pointers
		FLazyObjectPtr::ResetPIEFixups();

		// NULL GWorld before various PostLoad functions are called, this makes it easier to debug invalid GWorld accesses
		GWorld = NULL;

		// Duplicate the editor world to create the PIE world
		NewPIEWorld = CastChecked<UWorld>( StaticDuplicateObject(
			EditorWorld,			// Source root
			PlayWorldPackage,		// Destination root
			*EditorWorld->GetName(),// Name for new object
			RF_AllFlags,			// FlagMask
			NULL,					// DestClass
			SDO_DuplicateForPie		// bDuplicateForPIE
			) );

		// Fixup model components. The index buffers have been created for the components in the EditorWorld and the order
		// in which components were post-loaded matters. So don't try to guarantee a particular order here, just copy the
		// elements over.
		if ( NewPIEWorld->PersistentLevel->Model != NULL
			&& NewPIEWorld->PersistentLevel->Model == EditorWorld->PersistentLevel->Model
			&& NewPIEWorld->PersistentLevel->ModelComponents.Num() == EditorWorld->PersistentLevel->ModelComponents.Num() )
		{
			NewPIEWorld->PersistentLevel->Model->ClearLocalMaterialIndexBuffersData();
			for (int32 ComponentIndex = 0; ComponentIndex < NewPIEWorld->PersistentLevel->ModelComponents.Num(); ++ComponentIndex)
			{
				UModelComponent* SrcComponent = EditorWorld->PersistentLevel->ModelComponents[ComponentIndex];
				UModelComponent* DestComponent = NewPIEWorld->PersistentLevel->ModelComponents[ComponentIndex];
				DestComponent->CopyElementsFrom(SrcComponent);
			}
		}

		UE_LOG(LogPlayLevel, Log, TEXT("PIE: StaticDuplicateObject took: (%fs)"),  float(FPlatformTime::Seconds() - SDOStart));		
	}

	GPlayInEditorID = -1;
	check( NewPIEWorld );

	PostCreatePIEWorld(NewPIEWorld);

	// After loading the map, reset these so that things continue as normal
	GIsPlayInEditorWorld = false;
	
	UE_LOG(LogPlayLevel, Log, TEXT("PIE: Created PIE world by copying editor world from %s to %s (%fs)"), *EditorWorld->GetPathName(), *NewPIEWorld->GetPathName(), float(FPlatformTime::Seconds() - StartTime));
	return NewPIEWorld;
}

void UEditorEngine::PostCreatePIEWorld(UWorld *NewPIEWorld)
{
	double WorldInitStart = FPlatformTime::Seconds();
	
	// Init the PIE world
	NewPIEWorld->WorldType = EWorldType::PIE;
	NewPIEWorld->InitWorld();
	UE_LOG(LogPlayLevel, Log, TEXT("PIE: World Init took: (%fs)"),  float(FPlatformTime::Seconds() - WorldInitStart));

	// Tag PlayWorld Actors that also exist in EditorWorld.  At this point, no temporary/run-time actors exist in PlayWorld
	for( FActorIterator PlayActorIt(NewPIEWorld); PlayActorIt; ++PlayActorIt )
	{
		GEditor->ObjectsThatExistInEditorWorld.Set(*PlayActorIt);
	}
}

UWorld* UEditorEngine::CreatePIEWorldFromEntry(FWorldContext &WorldContext, UWorld* InWorld, FString &PlayWorldMapName)
{
	double StartTime = FPlatformTime::Seconds();

	// Create the world
	UWorld *LoadedWorld = UWorld::CreateWorld( EWorldType::PIE, false );
	check(LoadedWorld);

	// Force default GameMode class so project specific code doesn't fire off. 
	// We want this world to truly remain empty while we wait for connect!
	check(LoadedWorld->GetWorldSettings());
	LoadedWorld->GetWorldSettings()->DefaultGameMode = AGameMode::StaticClass();

	PlayWorldMapName = UGameMapsSettings::GetGameDefaultMap();
	return LoadedWorld;
}

bool UEditorEngine::WorldIsPIEInNewViewport(UWorld *InWorld)
{
	FWorldContext &WorldContext = WorldContextFromWorld(InWorld);
	if (WorldContext.WorldType == EWorldType::PIE)
	{
		FSlatePlayInEditorInfo * SlateInfoPtr = SlatePlayInEditorMap.Find(WorldContext.ContextHandle);
		if (SlateInfoPtr)
		{
			return SlateInfoPtr->SlatePlayInEditorWindow.IsValid();
		}
	}
	
	return false;
}

void UEditorEngine::SetPIEInstanceWindowSwitchDelegate(FPIEInstanceWindowSwitch InSwitchDelegate)
{
	PIEInstanceWIndowSwitchDelegate = InSwitchDelegate;
}

void UEditorEngine::FocusNextPIEWorld(UWorld *CurrentPieWorld, bool previous)
{
	// Get the current world's idx
	int32 CurrentIdx = 0;
	for (CurrentIdx = 0; CurrentPieWorld && CurrentIdx < WorldList.Num(); ++CurrentIdx)
	{
		if (WorldList[CurrentIdx].World() == CurrentPieWorld)
		{
			break;
		}
	}

	// Step through the list to find the next or previous
	int32 step = previous? -1 : 1;
	CurrentIdx += (WorldList.Num() + step);
	
	while ( CurrentPieWorld && WorldList[ CurrentIdx % WorldList.Num() ].World() != CurrentPieWorld )
	{
		FWorldContext &Context = WorldList[CurrentIdx % WorldList.Num()];
		if (Context.World() && Context.WorldType == EWorldType::PIE && Context.GameViewport != NULL)
		{
			break;
		}

		CurrentIdx += step;
	}
	
	if (WorldList[CurrentIdx % WorldList.Num()].World())
	{
		FSlatePlayInEditorInfo * SlateInfoPtr = SlatePlayInEditorMap.Find(WorldList[CurrentIdx % WorldList.Num()].ContextHandle);
		if (SlateInfoPtr && SlateInfoPtr->SlatePlayInEditorWindow.IsValid())
		{
			// Force window to front
			SlateInfoPtr->SlatePlayInEditorWindow.Pin()->BringToFront();

			// Set viewport widget to have keyboard focus
			FSlateApplication::Get().SetKeyboardFocus(SlateInfoPtr->SlatePlayInEditorWindowViewport->GetViewportWidget().Pin(), EKeyboardFocusCause::Keyboard);

			// Execute notifcation delegate incase game code has to do anything else
			PIEInstanceWIndowSwitchDelegate.ExecuteIfBound();
		}
	}
}

UGameViewportClient * UEditorEngine::GetNextPIEViewport(UGameViewportClient * CurrentViewport)
{
	// Get the current world's idx
	int32 CurrentIdx = 0;
	for (CurrentIdx = 0; CurrentViewport && CurrentIdx < WorldList.Num(); ++CurrentIdx)
	{
		if (WorldList[CurrentIdx].GameViewport == CurrentViewport)
		{
			break;
		}
	}

	// Step through the list to find the next or previous
	int32 step = 1;
	CurrentIdx += (WorldList.Num() + step);

	while ( CurrentViewport && WorldList[ CurrentIdx % WorldList.Num() ].GameViewport != CurrentViewport )
	{
		FWorldContext &Context = WorldList[CurrentIdx % WorldList.Num()];
		if (Context.GameViewport && Context.WorldType == EWorldType::PIE)
		{
			return Context.GameViewport;
		}

		CurrentIdx += step;
	}

	return NULL;
}

void UEditorEngine::RemapGamepadControllerIdForPIE(class UGameViewportClient* GameViewport, int32 &ControllerId)
{
	// Increment the controller id if we are the focused window, and RouteGamepadToSecondWindow is true (and we are running multiple clients).
	// This cause the focused window to NOT handle the input, decrement controllerID, and pass it to the next window.
	ULevelEditorPlaySettings* PlayInSettings = Cast<ULevelEditorPlaySettings>(ULevelEditorPlaySettings::StaticClass()->GetDefaultObject());
	if (PlayInSettings->RouteGamepadToSecondWindow && PlayInSettings->PlayNumberOfClients > 1 && PlayInSettings->RunUnderOneProcess && GameViewport->GetWindow().IsValid() && GameViewport->GetWindow()->HasFocusedDescendants())
	{
		ControllerId++;
	}
}

#undef LOCTEXT_NAMESPACE
