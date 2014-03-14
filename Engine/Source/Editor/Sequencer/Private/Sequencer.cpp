// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "Sequencer.h"
#include "SequencerObjectSpawner.h"
#include "Toolkits/IToolkitHost.h"
#include "MovieScene.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Editor/SequencerWidgets/Public/ITimeSlider.h"
#include "Editor/EditorWidgets/Public/ITransportControl.h"
#include "Editor/EditorWidgets/Public/EditorWidgetsModule.h"
#include "Editor/LevelEditor/Public/ILevelEditor.h"
#include "Editor/LevelEditor/Public/ILevelViewport.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "SSequencer.h"
#include "ScopedTransaction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_PlayMovieScene.h"
#include "BlueprintEditorModule.h"
#include "SequencerObjectChangeListener.h"
#include "MovieSceneTrack.h"
#include "MovieSceneDirectorTrack.h"
#include "MovieSceneAudioTrack.h"
#include "MovieSceneAnimationTrack.h"
#include "MovieSceneTrackEditor.h"
#include "Toolkits/IToolkitHost.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_PlayMovieScene.h"
#include "AssetSelection.h"
#include "ScopedTransaction.h"
#include "MovieSceneShotSection.h"
#include "SubMovieSceneTrack.h"
#include "ISequencerSection.h"
#include "MovieSceneInstance.h"
#include "IKeyArea.h"
#include "SnappingUtils.h"

#define LOCTEXT_NAMESPACE "Sequencer"

DEFINE_LOG_CATEGORY_STATIC( LogSequencer, Log, All );

const FName FSequencer::SequencerMainTabId( TEXT( "Sequencer_SequencerMain" ) );
const FName FSequencer::SequencerDetailsTabId( TEXT( "Sequencer_Details" ) );

namespace SequencerDefs
{
	static const FName SequencerAppIdentifier( TEXT( "SequencerApp" ) );
}
void FSequencer::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	if( FParse::Param( FCommandLine::Get(), TEXT( "Sequencer" ) ) )
	{
		TabManager->RegisterTabSpawner( SequencerMainTabId, FOnSpawnTab::CreateSP(this, &FSequencer::SpawnTab_SequencerMain) )
			.SetDisplayName( LOCTEXT("SequencerMainTab", "Sequencer") )
			.SetGroup( MenuStructure.GetAssetEditorCategory() );
	}

	TabManager->RegisterTabSpawner( SequencerDetailsTabId, FOnSpawnTab::CreateSP(this, &FSequencer::SpawnTab_Details) )
		.SetDisplayName( LOCTEXT("SequencerDetailsTab", "Details") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
}

void FSequencer::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	if( FParse::Param( FCommandLine::Get(), TEXT( "Sequencer" ) ) )
	{
		TabManager->UnregisterTabSpawner( SequencerMainTabId );
	}
	TabManager->UnregisterTabSpawner( SequencerDetailsTabId );
	
	// @todo remove when world-centric mode is added
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.AttachSequencer(SNullWidget::NullWidget);
}

void FSequencer::InitSequencer( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit, const TArray<FOnCreateTrackEditor>& TrackEditorDelegates )
{
	if( FParse::Param( FCommandLine::Get(), TEXT( "Sequencer" ) ) )
	{
		const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout( "Standalone_Sequencer_Layout" )
			->AddArea(
			FTabManager::NewPrimaryArea()
			->Split
			(
			FTabManager::NewStack()
			->AddTab( SequencerMainTabId, ETabState::OpenedTab )
			->AddTab( SequencerDetailsTabId, ETabState::OpenedTab )
			)
			);

		const bool bCreateDefaultStandaloneMenu = true;
		const bool bCreateDefaultToolbar = false;
		FAssetEditorToolkit::InitAssetEditor( Mode, InitToolkitHost, SequencerDefs::SequencerAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu,bCreateDefaultToolbar, ObjectToEdit );

		// Create an object change listener for various systems that need to be notified when objects change
		ObjectChangeListener = MakeShareable( new FSequencerObjectChangeListener( SharedThis( this ) ) );
		// If this is a world-centric editor, then setup our puppet spawner
		if( IsWorldCentricAssetEditor() )
		{
			ObjectSpawner = MakeShareable( new FSequencerActorObjectSpawner( *this ) );
		}

		UMovieScene& RootMovieScene = *CastChecked<UMovieScene>( GetEditingObject() );
		// Focusing the initial movie scene needs to be done before the first time GetFocusedMovieSceneInstane or GetRootMovieSceneInstance is used
		RootMovieSceneInstance = MakeShareable( new FMovieSceneInstance( RootMovieScene ) );
		MovieSceneStack.Add( RootMovieSceneInstance.ToSharedRef() );

		// Make internal widgets
		SequencerWidget = SNew( SSequencer, SharedThis( this ) )
			.ViewRange( this, &FSequencer::OnGetViewRange )
			.ScrubPosition( this, &FSequencer::OnGetScrubPosition )
			.AllowAutoKey( this, &FSequencer::OnGetAllowAutoKey )
			.CleanViewEnabled( this, &FSequencer::IsUsingCleanView )
			.OnScrubPositionChanged( this, &FSequencer::OnScrubPositionChanged )
			.OnViewRangeChanged( this, &FSequencer::OnViewRangeChanged, false )
			.OnToggleAutoKey( this, &FSequencer::OnToggleAutoKey )
			.OnToggleCleanView( this, &FSequencer::OnToggleCleanView );


		// @todo remove when world-centric mode is added
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.AttachSequencer(SequencerWidget);

		// Hook into the editor's mechanism for checking whether we need live capture of PIE/SIE actor state
		GEditor->GetActorRecordingState().AddSP( this, &FSequencer::GetActorRecordingState );

		// When undo occurs, get a notification so we can make sure our view is up to date
		GEditor->RegisterForUndo(this);


		// Setup our tool's layout
		// @todo re-enable once world centric works again
		/*if( IsWorldCentricAssetEditor() )
		{
		if( !SequencerMainTab.IsValid() )
		{
		const FString TabInitializationPayload;
		SequencerMainTab = SpawnToolkitTab( SequencerMainTabId, TabInitializationPayload, EToolkitTabSpot::BelowLevelEditor );
		}

		// @todo sequencer: Ideally we should be possessing the level editor's details view instead of spawning our own
		if( !DetailsTab.IsValid() )
		{
		const FString TabInitializationPayload;		// NOTE: Payload not currently used for details
		DetailsTab = SpawnToolkitTab( SequencerDetailsTabId, TabInitializationPayload, EToolkitTabSpot::Details );
		}
		}*/


		// We need to find out when the user loads a new map, because we might need to re-create puppet actors
		// when previewing a MovieScene
		//auto& LevelEditorModule = FModuleManager::LoadModuleChecked< FLevelEditorModule >( TEXT( "LevelEditor" ) );
		LevelEditorModule.OnMapChanged().AddRaw( this, &FSequencer::OnMapChanged );


		// Start listening for important changes on this MovieScene
		// @todo sequencer: We need to correctly register for any new moviescenes that are dropped in!
		const bool bCreateIfNotFound = false;
		BindToPlayMovieSceneNode( bCreateIfNotFound );

		// Create tools and bind them to this sequencer
		for( int32 DelegateIndex = 0; DelegateIndex < TrackEditorDelegates.Num(); ++DelegateIndex )
		{
			check( TrackEditorDelegates[DelegateIndex].IsBound() );
			// Tools may exist in other modules, call a delegate that will create one for us 
			TSharedRef<FMovieSceneTrackEditor> TrackEditor = TrackEditorDelegates[DelegateIndex].Execute( SharedThis( this ) );

			// Keep track of certain editors
			if ( TrackEditor->SupportsType( UMovieSceneDirectorTrack::StaticClass() ) )
			{
				DirectorTrackEditor = TrackEditor;
			}
			else if ( TrackEditor->SupportsType( UMovieSceneAnimationTrack::StaticClass() ) )
			{
				AnimationTrackEditor = TrackEditor;
			}

			TrackEditors.Add( TrackEditor );
		}

		AttachTransportControlsToViewports();

		ZoomAnimation = FCurveSequence();
		ZoomCurve = ZoomAnimation.AddCurve(0.f, 0.35f, ECurveEaseFunction::QuadIn);
		OverlayAnimation = FCurveSequence();
		OverlayCurve = OverlayAnimation.AddCurve(0.f, 0.35f, ECurveEaseFunction::QuadIn);

		// Update initial movie scene data
		NotifyMovieSceneDataChanged();

		// NOTE: Could fill in asset editor commands here!

		BindSequencerCommands();
	}
}


FSequencer::FSequencer()
	: SequencerCommandBindings( new FUICommandList )
	, TargetViewRange(0.f, 5.f)
	, LastViewRange(0.f, 5.f)
	, ScrubPosition( 0.0f )
	, bCleanViewEnabled( false )
	, PlaybackState( EMovieScenePlayerStatus::Stopped )
	, bLoopingEnabled( false )
	, bAllowAutoKey( false )
	, bPerspectiveViewportPossessionEnabled( true )
	, bNeedTreeRefresh( false )
{

}

FSequencer::~FSequencer()
{
	DetachTransportControlsFromViewports();


	// Unregister delegates
	if( FModuleManager::Get().IsModuleLoaded( TEXT( "LevelEditor" ) ) )
	{
		auto& LevelEditorModule = FModuleManager::LoadModuleChecked< FLevelEditorModule >( TEXT( "LevelEditor" ) );
		LevelEditorModule.OnMapChanged().RemoveRaw( this, &FSequencer::OnMapChanged );
	}

	if( PlayMovieSceneNode.IsValid() )
	{
		PlayMovieSceneNode->OnBindingsChanged().RemoveAll( this );
		PlayMovieSceneNode.Reset();
	}

	GEditor->GetActorRecordingState().RemoveAll( this );

	GEditor->UnregisterForUndo( this );

	// Clean up puppet objects
	if( ObjectSpawner.IsValid() )
	{
		DestroySpawnablesForAllMovieScenes();
	
		ObjectSpawner.Reset();
	}

	TrackEditors.Empty();

	DetailsView.Reset();

	/*if( SequencerMainTab.IsValid() )
	{
		// Kill the tab!
		// NOTE: It's possible that the user already closed the tab manually, but that's OK.
		SequencerMainTab->RemoveTabFromParent();
		SequencerMainTab.Reset();
	}

	if( DetailsTab.IsValid() )
	{
		// Kill the tab!
		// NOTE: It's possible that the user already closed the tab manually, but that's OK.
		DetailsTab->RemoveTabFromParent();
		DetailsTab.Reset();
	}*/

	SequencerWidget.Reset();
}


TSharedRef<SDockTab> FSequencer::SpawnTab_SequencerMain(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == SequencerMainTabId);

	return SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("Sequencer.Tabs.SequencerMain") )
		.Label( LOCTEXT("SequencerMainTitle", "Main") )
		.TabColorScale( GetTabColorScale() )
		[
			SequencerWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FSequencer::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == SequencerDetailsTabId);

	const bool bIsUpdatable = false;
	const bool bAllowFavorites = true;
	const bool bIsLockable = false;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>( "PropertyEditor" );
	const FDetailsViewArgs DetailsViewArgs( bIsUpdatable, bIsLockable, true, false, false );
	DetailsView = PropertyEditorModule.CreateDetailView( DetailsViewArgs );

	return SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("Sequencer.Tabs.Details") )	// @todo sequencer: Missing icon
		.Label( LOCTEXT("SequencerDetailsTitle", "Details") )
		.TabColorScale( GetTabColorScale() )
		[
			SNew(SBorder)
			.Padding(4)
			.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
			[
				DetailsView.ToSharedRef()
			]
		]
	;
}

FName FSequencer::GetToolkitFName() const
{
	return FName("Sequencer");
}

FText FSequencer::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Sequencer");
}


FLinearColor FSequencer::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.7, 0.0f, 0.0f, 0.5f );
}


FString FSequencer::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Sequencer ").ToString();
}


TArray< UMovieScene* > FSequencer::GetMovieScenesBeingEdited()
{
	TArray<UMovieScene* > OutMovieScenes;

	// Get the root movie scene
	OutMovieScenes.Add( RootMovieSceneInstance->GetMovieScene() );

	// Get Sub-MovieScenes
	for( auto It = MovieSceneSectionToInstanceMap.CreateConstIterator(); It; ++It )
	{
		OutMovieScenes.AddUnique( It.Value()->GetMovieScene() );
	}

	return OutMovieScenes;
}

UMovieScene* FSequencer::GetRootMovieScene() const
{
	return MovieSceneStack[0]->GetMovieScene();
}

UMovieScene* FSequencer::GetFocusedMovieScene() const
{
	// the last item is the focused movie scene
	return MovieSceneStack.Top()->GetMovieScene();
}

TSharedRef<FMovieSceneInstance> FSequencer::GetRootMovieSceneInstance() const
{
	return MovieSceneStack[0];
}


TSharedRef<FMovieSceneInstance> FSequencer::GetFocusedMovieSceneInstance() const
{
	// the last item is the focused movie scene
	return MovieSceneStack.Top();
}


void FSequencer::FocusSubMovieScene( TSharedRef<FMovieSceneInstance> SubMovieSceneInstance )
{
	// Check for infinite recursion
	check( SubMovieSceneInstance != MovieSceneStack.Top() );

	// Focus the movie scene
	MovieSceneStack.Push( SubMovieSceneInstance );

	// Reset data that is only used for the previous movie scene
	ResetPerMovieSceneData();

	// Update internal data for the new movie scene
	NotifyMovieSceneDataChanged();
}

TSharedRef<FMovieSceneInstance> FSequencer::GetInstanceForSubMovieSceneSection( UMovieSceneSection& SubMovieSceneSection ) const
{
	return MovieSceneSectionToInstanceMap.FindChecked( &SubMovieSceneSection );
}

void FSequencer::PopToMovieScene( TSharedRef<FMovieSceneInstance> SubMovieSceneInstance )
{
	if( MovieSceneStack.Num() > 1 )
	{
		// Pop until we find the movie scene to focus
		while( SubMovieSceneInstance != MovieSceneStack.Last() )
		{
			MovieSceneStack.Pop();
		}
	
		check( MovieSceneStack.Num() > 0 );

		ResetPerMovieSceneData();

		NotifyMovieSceneDataChanged();
	}
}

void FSequencer::EditDetailsForObjects( TArray< UObject* > ObjectsToEdit )
{
	// @todo re-enable when world centric is back up
	//DetailsView->SetObjects( ObjectsToEdit );
}

void FSequencer::SpawnOrDestroyPuppetObjects( TSharedRef<FMovieSceneInstance> MovieSceneInstance )
{
	ObjectSpawner->SpawnOrDestroyPuppetObjects( MovieSceneInstance );
}


void FSequencer::AddNewShot(FGuid CameraGuid)
{
	if (DirectorTrackEditor.IsValid())
	{
		DirectorTrackEditor.Pin()->AddKey(CameraGuid);
	}
}

void FSequencer::AddAnimation(FGuid ObjectGuid, class UAnimSequence* AnimSequence)
{
	if (AnimationTrackEditor.IsValid())
	{
		AnimationTrackEditor.Pin()->AddKey(ObjectGuid, AnimSequence);
	}
}

void FSequencer::RenameShot(UMovieSceneSection* ShotSection)
{
	auto ActualShotSection = CastChecked<UMovieSceneShotSection>(ShotSection);

	TSharedRef<STextEntryPopup> TextEntry = 
		SNew(STextEntryPopup)
		.Label(NSLOCTEXT("Sequencer", "RenameShotHeader", "Name").ToString())
		.DefaultText( ActualShotSection->GetTitle() )
		.OnTextCommitted(this, &FSequencer::RenameShotCommitted, ShotSection)
		.ClearKeyboardFocusOnCommit( false );
	
	NameEntryPopupWindow = FSlateApplication::Get().PushMenu(
		SequencerWidget.ToSharedRef(),
		TextEntry,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
		);

	TextEntry->FocusDefaultWidget();
}

void FSequencer::DeleteSection(class UMovieSceneSection* Section)
{
	UMovieScene* MovieScene = GetFocusedMovieScene();
	
	bool bAnythingRemoved = false;

	UMovieSceneTrack* Track = CastChecked<UMovieSceneTrack>( Section->GetOuter() );

	// If this check fails then the section is outered to a type that doesnt know about the section
	checkSlow( Track->HasSection(Section) );
	
	Track->SetFlags( RF_Transactional );
	Track->Modify();

	Track->RemoveSection(Section);

	bAnythingRemoved = true;
	
	if( bAnythingRemoved )
	{
		// @todo Sequencer - Remove this
		NotifyMovieSceneDataChanged();
	}
}

void FSequencer::DeleteKeys(TArray<FSelectedKey> KeysToDelete)
{
	for (int32 i = 0; i < KeysToDelete.Num(); ++i)
	{
		const FSelectedKey& SelectedKey = KeysToDelete[i];

		if (SelectedKey.IsValid())
		{
			SelectedKey.KeyArea->DeleteKey(SelectedKey.KeyHandle.GetValue());
		}
	}
}

void FSequencer::RenameShotCommitted(const FText& RenameText, ETextCommit::Type CommitInfo, UMovieSceneSection* Section)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		auto ShotSection = CastChecked<UMovieSceneShotSection>(Section);
		ShotSection->SetTitle(RenameText);
	}

	if (NameEntryPopupWindow.IsValid())
	{
		NameEntryPopupWindow.Pin()->RequestDestroyWindow();
	}
}



void FSequencer::OnMapChanged( class UWorld* NewWorld, EMapChangeType::Type MapChangeType )
{
	// @todo Sequencer Sub-MovieScenes Needs more investigation of what to spawn
	// Destroy our puppets because the world is going away.  We probably don't have to do this (the actors will
	// be naturally destroyed with the level, but we might as well.)
	if( ObjectSpawner.IsValid() )
	{
		const bool bDestroyAll = true;
		DestroySpawnablesForAllMovieScenes();
	}

	// @todo sequencer: We should only wipe/respawn puppets that are affected by the world that is being changed! (multi-UWorld support)
	if( MapChangeType == EMapChangeType::LoadMap || MapChangeType == EMapChangeType::NewMap )
	{
		SpawnOrDestroyPuppetObjects( GetFocusedMovieSceneInstance() );
	}
}

void FSequencer::NotifyMovieSceneDataChanged()
{
	PlaybackState = EMovieScenePlayerStatus::Stopped;
	bNeedTreeRefresh = true;
}

void FSequencer::OnPlayMovieSceneBindingsChanged()
{
	// Something may have changed with the objects bound to the PlayMovieScene node, which may require us to change
	// which actors are possessed, and thus create new puppets for different actors.
	// @todo Sequencer Sub-MovieScenes Spawn other movie scene puppets?
	SpawnOrDestroyPuppetObjects( GetFocusedMovieSceneInstance() );
}

TRange<float> FSequencer::OnGetViewRange() const
{
	return TRange<float>(FMath::Lerp(LastViewRange.GetLowerBoundValue(), TargetViewRange.GetLowerBoundValue(), ZoomCurve.GetLerp()),
		FMath::Lerp(LastViewRange.GetUpperBoundValue(), TargetViewRange.GetUpperBoundValue(), ZoomCurve.GetLerp()));
}

UK2Node_PlayMovieScene* FSequencer::BindToPlayMovieSceneNode( const bool bCreateIfNotFound )
{
	auto* MovieScene = GetFocusedMovieScene();

	// Update level script
	UK2Node_PlayMovieScene* FoundPlayMovieSceneNode = FindPlayMovieSceneNodeInLevelScript( MovieScene );
	if( FoundPlayMovieSceneNode == NULL )
	{
		if( bCreateIfNotFound )
		{
			// Couldn't find an existing node that uses this MovieScene, so we'll create one now.
			FoundPlayMovieSceneNode = CreateNewPlayMovieSceneNode( MovieScene );
			if( FoundPlayMovieSceneNode != NULL )
			{
				// Let the user know that we plopped down a new PlayMovieScene event in their level script graph
				{
					FNotificationInfo Info( LOCTEXT("AddedPlayMovieSceneEventToLevelScriptGraph", "A new event for this MovieScene was added to this level's script") );
					Info.bFireAndForget = true;
					Info.bUseThrobber = false;
					Info.bUseSuccessFailIcons = false;
					Info.bUseLargeFont = false;
					Info.ExpireDuration = 5.0f;		// Stay visible for awhile so the user has time to click "Show Graph" if they want to

					// @todo sequencer: Needs better artwork (see DefaultStyle.cpp)
					Info.Image = FEditorStyle::GetBrush( TEXT( "Sequencer.NotificationImage_AddedPlayMovieSceneEvent" ) );

					struct Local
					{
						/**
							* Called by our notification's hyperlink to open the Level Script editor and navigate to our newly-created PlayMovieScene node
							*
							* @param	Sequencer			The Sequencer we're using
							* @param	PlayMovieSceneNode	The node that we need to jump to
							*/
						static void NavigateToPlayMovieSceneNode( ISequencerInternals* Sequencer, UK2Node_PlayMovieScene* PlayMovieSceneNode )
						{
							check( Sequencer != NULL );
							check( PlayMovieSceneNode != NULL );

							IBlueprintEditor* BlueprintEditor = NULL;

							auto* LSB = CastChecked< ULevelScriptBlueprint >( PlayMovieSceneNode->GetBlueprint() );

							// @todo sequencer: Support using world-centric editing here?  (just need to set EToolkitMode::WorldCentric)
							if( FAssetEditorManager::Get().OpenEditorForAsset( LSB, EToolkitMode::Standalone, Sequencer->GetToolkitHost() ) )
							{
								const bool bFocusIfOpen = true;
								BlueprintEditor = static_cast< IBlueprintEditor* >( FAssetEditorManager::Get().FindEditorForAsset( LSB, bFocusIfOpen ) );
							}

							if( BlueprintEditor != NULL )
							{
								const bool bRequestRename = false;
								BlueprintEditor->JumpToHyperlink( PlayMovieSceneNode, false );
							}
							else
							{
								UE_LOG( LogSequencer, Warning, TEXT( "Unable to open Blueprint Editor to edit newly-created PlayMovieScene event" ) );
							}
						}
					};

					ISequencerInternals* SequencerInternals = this;
					Info.Hyperlink = FSimpleDelegate::CreateStatic( &Local::NavigateToPlayMovieSceneNode, SequencerInternals, FoundPlayMovieSceneNode );
					Info.HyperlinkText = LOCTEXT("AddedPlayMovieSceneEventToLevelScriptGraph_Hyperlink", "Show Graph");

					TSharedPtr< SNotificationItem > NewNotification = FSlateNotificationManager::Get().AddNotification(Info);
				}
			}
		}
	}

	if( PlayMovieSceneNode.Get() != FoundPlayMovieSceneNode )
	{
		if( PlayMovieSceneNode.IsValid() )
		{
			// Unhook from old node
			PlayMovieSceneNode.Get()->OnBindingsChanged().RemoveAll( this );
		}
		
		PlayMovieSceneNode = FoundPlayMovieSceneNode;

		if( PlayMovieSceneNode.IsValid() )
		{
			// Bind to the new node
			PlayMovieSceneNode.Get()->OnBindingsChanged().AddSP( this, &FSequencer::OnPlayMovieSceneBindingsChanged );
		}
	}

	return PlayMovieSceneNode.Get();
}

bool FSequencer::IsAutoKeyEnabled() const 
{
	return bAllowAutoKey;
}

bool FSequencer::IsRecordingLive() const 
{
	return PlaybackState == EMovieScenePlayerStatus::Recording && GIsPlayInEditorWorld;
}

float FSequencer::GetCurrentLocalTime( UMovieScene& MovieScene )
{
	//@todo Sequencer - Nested movie scenes:  Figure out the parent of the passed in movie scene and 
	// calculate local time
	return ScrubPosition;
}

float FSequencer::GetGlobalTime()
{
	return ScrubPosition;
}

void FSequencer::SetGlobalTime( float NewTime )
{
	float LastTime = ScrubPosition;

	// Update the position
	ScrubPosition = NewTime;

	RootMovieSceneInstance->Update( ScrubPosition, LastTime, *this );
}

void FSequencer::SetPerspectiveViewportPossessionEnabled(bool bEnabled)
{
	bPerspectiveViewportPossessionEnabled = bEnabled;
}


FGuid FSequencer::GetHandleToObject( UObject* Object )
{
	TSharedRef<FMovieSceneInstance> FocusedMovieSceneInstance = GetFocusedMovieSceneInstance();
	UMovieScene* FocusedMovieScene = FocusedMovieSceneInstance->GetMovieScene();

	// First check to see if this is a puppet object.  If so, we'll get a handle to the movie scene's spawnable
	// entry for that puppet.
	FGuid ObjectGuid = ObjectSpawner->FindSpawnableGuidForPuppetObject( Object );
	if( ObjectGuid.IsValid() )
	{
		// Found a puppet's spawnable entry!		
	}
	else
	{
		// Is this a game preview object?
		const bool bIsGamePreviewObject = !!( Object->GetOutermost()->PackageFlags & PKG_PlayInEditor );
		if( bIsGamePreviewObject )
		{
			// OK, so someone is asking for a handle to an object from a game preview session, probably because
			// they want to capture keys during live simulation.

			// Object is in the editor world, so go ahead and possess it if we can

			// Check to see if we already have a puppet that was generated from a recording of this game preview object
			// @todo sequencer livecapture: We could support recalling counterpart by full name instead of weak pointer, to allow "overdubbing" of previously recorded actors, when the new actors in the current play session have the same path name
			// @todo sequencer livecapture: Ideally we could capture from editor-world actors that are "puppeteered" as well (real time)
			FMovieSceneSpawnable* FoundSpawnable = FocusedMovieScene->FindSpawnableForCounterpart( Object );
			if( FoundSpawnable != NULL )
			{
				ObjectGuid = FoundSpawnable->GetGuid();
			}
			else
			{
				// No existing object was found, so we'll need to create a new spawnable (and puppet) for this
				// game preview object right now.
				UObject* ActorTemplate = Object;	// Pass in the actor itself as the template.  AddSpawnableForAssetOrClass() will use this actor's class!
				UObject* CounterpartGamePreviewObject = Object;
				ObjectGuid = AddSpawnableForAssetOrClass( Object, CounterpartGamePreviewObject );

				// Update puppet actors
				SpawnOrDestroyPuppetObjects( FocusedMovieSceneInstance );
			}
		}
		else
		{
			// Object is in the editor world, so go ahead and possess it if we can
			FocusedMovieScene = GetFocusedMovieScene();

			// Make sure we're bound to the level script node which contains data about possessables.  But don't bother creating
			// a node if we don't have one, at least not yet.
			{
				const bool bCreateIfNotFound = false;
				BindToPlayMovieSceneNode( bCreateIfNotFound );
			}
			
			if( PlayMovieSceneNode.IsValid() )
			{
				ObjectGuid = PlayMovieSceneNode->FindGuidForObject( Object );
			}

			// The object was not found, bind it now by adding a new possessable for it
			if( !ObjectGuid.IsValid() )
			{
				// Add a new possessable for this object.  Create a new PlayMovieScene node now, if we don't already have one.
				const bool bCreateIfNotFound = true;
				BindToPlayMovieSceneNode( bCreateIfNotFound );

				const FString& PossessableName = Object->GetName();
				const FGuid PossessableGuid = FocusedMovieScene->AddPossessable( PossessableName, Object->GetClass() );
				
				if (IsShotFilteringOn()) {AddUnfilterableObject(PossessableGuid);}

				// Bind the object to the handle
				TArray< UObject* > Objects;
				Objects.Add( Object );
				PlayMovieSceneNode->BindPossessableToObjects( PossessableGuid, Objects );

				// A possessable was created so we need to respawn its puppet
				SpawnOrDestroyPuppetObjects( FocusedMovieSceneInstance );

				ObjectGuid = PossessableGuid;
			}
		}
	}

	return ObjectGuid;
}

ISequencerObjectChangeListener& FSequencer::GetObjectChangeListener() const 
{ 
	return *ObjectChangeListener;
}


void FSequencer::SpawnActorsForMovie( TSharedRef<FMovieSceneInstance> MovieSceneInstance )
{
	if( ObjectSpawner.IsValid() )
	{
		ObjectSpawner->SpawnOrDestroyPuppetObjects( MovieSceneInstance );
	}
}

void FSequencer::GetRuntimeObjects( TSharedRef<FMovieSceneInstance> MovieSceneInstance, const FGuid& ObjectHandle, TArray< UObject*>& OutObjects ) const
{
	if( ObjectSpawner.IsValid() )
	{	
		// First, try to find spawnable puppet objects for the specified Guid
		UObject* FoundPuppetObject = ObjectSpawner->FindPuppetObjectForSpawnableGuid( MovieSceneInstance, ObjectHandle );
		if( FoundPuppetObject != NULL )
		{
			// Found a puppet object!
			OutObjects.Reset();
			OutObjects.Add( FoundPuppetObject );
		}
		else
		{
			// No puppets were found for spawnables, so now we'll check for possessed actors
			if( PlayMovieSceneNode != NULL )
			{
				// @todo Sequencer SubMovieScene: Figure out how to instance possessables
				OutObjects = PlayMovieSceneNode->FindBoundObjects( ObjectHandle );
			}
		}
	}
}

EMovieScenePlayerStatus::Type FSequencer::GetPlaybackStatus() const
{
	return PlaybackState;
}

void FSequencer::AddMovieSceneInstance( UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneInstance> InstanceToAdd )
{
	MovieSceneSectionToInstanceMap.Add( &MovieSceneSection, InstanceToAdd );

	SpawnOrDestroyPuppetObjects( InstanceToAdd );
}

void FSequencer::RemoveMovieSceneInstance( UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneInstance> InstanceToRemove )
{
	const bool bDestroyAll = true;
	ObjectSpawner->SpawnOrDestroyPuppetObjects( InstanceToRemove, bDestroyAll );

	MovieSceneSectionToInstanceMap.Remove( &MovieSceneSection );
}

void FSequencer::Tick(const float InDeltaTime)
{
	if( bNeedTreeRefresh )
	{
		// @todo - Sequencer Will be called too often
		UpdateRuntimeInstances();

		SequencerWidget->UpdateLayoutTree();
		bNeedTreeRefresh = false;
	}

	float NewTime = GetGlobalTime() + InDeltaTime;
	if (PlaybackState == EMovieScenePlayerStatus::Playing ||
		PlaybackState == EMovieScenePlayerStatus::Recording)
	{
		TRange<float> TimeBounds = GetTimeBounds();
		if (!TimeBounds.IsEmpty())
		{
			if (NewTime > TimeBounds.GetUpperBoundValue())
			{
				if (bLoopingEnabled)
				{
					NewTime -= TimeBounds.Size<float>();
				}
				else
				{
					NewTime = TimeBounds.GetUpperBoundValue();
					PlaybackState = EMovieScenePlayerStatus::Stopped;
				}
			}
			
			if (NewTime < TimeBounds.GetLowerBoundValue())
			{
				NewTime = TimeBounds.GetLowerBoundValue();
			}

			SetGlobalTime(NewTime);
		}
		else
		{
			// no bounds at all, stop playing
			PlaybackState = EMovieScenePlayerStatus::Stopped;
		}
	}
}


void FSequencer::UpdateViewports(AActor* ActorToViewThrough) const
{
	if (!bPerspectiveViewportPossessionEnabled) {return;}

	for (int32 i = 0; i < GEditor->LevelViewportClients.Num(); ++i)
	{
		FLevelEditorViewportClient* LevelVC = GEditor->LevelViewportClients[i];
		if (LevelVC && LevelVC->IsPerspective() && LevelVC->AllowMatineePreview())
		{
			if (ActorToViewThrough)
			{
				LevelVC->SetViewLocation( ActorToViewThrough->GetActorLocation() );
				LevelVC->SetViewRotation( ActorToViewThrough->GetActorRotation() );
			}
			else
			{
				LevelVC->ViewFOV = LevelVC->FOVAngle;
			}

			ACameraActor* Camera = Cast<ACameraActor>(ActorToViewThrough);
			
			// If viewing through a camera - PP settings of camera.
			LevelVC->SetPostprocessCameraActor(Camera);

			if (Camera)
			{
				LevelVC->ViewFOV = LevelVC->FOVAngle = Camera->CameraComponent->FieldOfView;
				// If the Camera's aspect ratio is zero, put a more reasonable default here - this at least stops it from crashing
				// nb. the AspectRatio will be reported as a Map Check Warning
				if( Camera->CameraComponent->AspectRatio == 0 )
				{
					LevelVC->AspectRatio = 1.7f;
				}
				else
				{
					LevelVC->AspectRatio = Camera->CameraComponent->AspectRatio;
				}
			}
		}
	}
}

void FSequencer::AddReferencedObjects( FReferenceCollector& Collector )
{
	for( int32 MovieSceneIndex = 0; MovieSceneIndex < MovieSceneStack.Num(); ++MovieSceneIndex )
	{
		UMovieScene* Scene = MovieSceneStack[MovieSceneIndex]->GetMovieScene();
		Collector.AddReferencedObject( Scene );
	}
}

void FSequencer::AttachTransportControlsToViewports()
{
	FLevelEditorModule* Module = FModuleManager::Get().LoadModulePtr<FLevelEditorModule>("LevelEditor");
	if (Module)
	{
		TSharedPtr<ILevelEditor> LevelEditor = Module->GetFirstLevelEditor();
		const TArray< TSharedPtr<ILevelViewport> >& LevelViewports = LevelEditor->GetViewports();
		
		FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::Get().LoadModuleChecked<FEditorWidgetsModule>( "EditorWidgets" );

		FTransportControlArgs TransportControlArgs;
		TransportControlArgs.OnForwardPlay.BindSP(this, &FSequencer::OnPlay);
		TransportControlArgs.OnRecord.BindSP(this, &FSequencer::OnRecord);
		TransportControlArgs.OnForwardStep.BindSP(this, &FSequencer::OnStepForward);
		TransportControlArgs.OnBackwardStep.BindSP(this, &FSequencer::OnStepBackward);
		TransportControlArgs.OnForwardEnd.BindSP(this, &FSequencer::OnStepToEnd);
		TransportControlArgs.OnBackwardEnd.BindSP(this, &FSequencer::OnStepToBeginning);
		TransportControlArgs.OnToggleLooping.BindSP(this, &FSequencer::OnToggleLooping);
		TransportControlArgs.OnGetLooping.BindSP(this, &FSequencer::IsLooping);
		TransportControlArgs.OnGetPlaybackMode.BindSP(this, &FSequencer::GetPlaybackMode);

		for (int32 i = 0; i < LevelViewports.Num(); ++i)
		{
			const TSharedPtr<ILevelViewport>& LevelViewport = LevelViewports[i];
			
			TSharedPtr<SWidget> TransportControl =
				SNew(SHorizontalBox)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Bottom)
				.Padding(4.f)
				[
					SNew(SBorder)
					.Padding(4.f)
					.Cursor( EMouseCursor::Default )
					.BorderImage( FEditorStyle::GetBrush( "FilledBorder" ) )
					.Visibility(this, &FSequencer::GetTransportControlVisibility, LevelViewport)
					.Content()
					[
						EditorWidgetsModule.CreateTransportControl(TransportControlArgs)
					]
				];

			LevelViewport->AddOverlayWidget(TransportControl.ToSharedRef());

			TransportControls.Add(LevelViewport, TransportControl);
		}
	}
}


void FSequencer::DetachTransportControlsFromViewports()
{
	FLevelEditorModule* Module = FModuleManager::Get().LoadModulePtr<FLevelEditorModule>("LevelEditor");
	if (Module)
	{
		TSharedPtr<ILevelEditor> LevelEditor = Module->GetFirstLevelEditor();
		if (LevelEditor.IsValid())
		{
			const TArray< TSharedPtr<ILevelViewport> >& LevelViewports = LevelEditor->GetViewports();
		
			for (int32 i = 0; i < LevelViewports.Num(); ++i)
			{
				const TSharedPtr<ILevelViewport>& LevelViewport = LevelViewports[i];

				TSharedPtr<SWidget>* TransportControl = TransportControls.Find(LevelViewport);
				if (TransportControl && TransportControl->IsValid())
				{
					LevelViewport->RemoveOverlayWidget(TransportControl->ToSharedRef());
				}
			}
		}
	}
}

void FSequencer::ResetPerMovieSceneData()
{
	FilterToShotSections( TArray< TWeakObjectPtr<UMovieSceneSection> >(), false );

	//@todo Sequencer - We may want to preserve selections when moving between movie scenes
	SelectedSections.Empty();
	SelectedKeys.Empty();

	// @todo run through all tracks for new movie scene changes
	//  needed for audio track decompression
}

void FSequencer::UpdateRuntimeInstances()
{
	// Spawn runtime objects for the root movie scene
	SpawnOrDestroyPuppetObjects( RootMovieSceneInstance.ToSharedRef() );

	// Refresh the current root instance
	RootMovieSceneInstance->RefreshInstance( *this );
}

void FSequencer::DestroySpawnablesForAllMovieScenes()
{
	ObjectSpawner->DestroyAllPuppetObjects();
}

EVisibility FSequencer::GetTransportControlVisibility(TSharedPtr<ILevelViewport> LevelViewport) const
{
	FLevelEditorViewportClient& ViewportClient = LevelViewport->GetLevelViewportClient();
	return (ViewportClient.IsPerspective() && ViewportClient.AllowMatineePreview()) ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply FSequencer::OnPlay()
{
	if( PlaybackState == EMovieScenePlayerStatus::Playing ||
		PlaybackState == EMovieScenePlayerStatus::Recording )
	{
		PlaybackState = EMovieScenePlayerStatus::Stopped;
		// Update on stop (cleans up things like sounds that are playing)
		RootMovieSceneInstance->Update( ScrubPosition, ScrubPosition, *this );
	}
	else
	{
		TRange<float> TimeBounds = GetTimeBounds();
		if (!TimeBounds.IsEmpty())
		{
			float CurrentTime = GetGlobalTime();
			if (CurrentTime < TimeBounds.GetLowerBoundValue() || CurrentTime >= TimeBounds.GetUpperBoundValue())
			{
				SetGlobalTime(TimeBounds.GetLowerBoundValue());
			}
			PlaybackState = EMovieScenePlayerStatus::Playing;
		}
	}

	return FReply::Handled();
}

FReply FSequencer::OnRecord()
{
	if (PlaybackState != EMovieScenePlayerStatus::Recording)
	{
		PlaybackState = EMovieScenePlayerStatus::Recording;

		// @todo sequencer livecapture: Ideally we would support fixed timestep capture from simulation
		//			Basically we need to run the PIE world at a fixed time step, capturing key frames every frame. 
		//			The editor world would still be ticked at the normal throttle-real-time rate
	}
	else
	{
		PlaybackState = EMovieScenePlayerStatus::Stopped;
	}

	return FReply::Handled();
}

FReply FSequencer::OnStepForward()
{
	PlaybackState = EMovieScenePlayerStatus::Stopped;
	// @todo Sequencer Add proper step sizes
	SetGlobalTime(ScrubPosition + 0.1f);
	return FReply::Handled();
}

FReply FSequencer::OnStepBackward()
{
	PlaybackState = EMovieScenePlayerStatus::Stopped;
	// @todo Sequencer Add proper step sizes
	SetGlobalTime(ScrubPosition - 0.1f);
	return FReply::Handled();
}

FReply FSequencer::OnStepToEnd()
{
	PlaybackState = EMovieScenePlayerStatus::Stopped;
	TRange<float> TimeBounds = GetTimeBounds();
	if (!TimeBounds.IsEmpty())
	{
		SetGlobalTime(TimeBounds.GetUpperBoundValue());
	}
	return FReply::Handled();
}

FReply FSequencer::OnStepToBeginning()
{
	PlaybackState = EMovieScenePlayerStatus::Stopped;
	TRange<float> TimeBounds = GetTimeBounds();
	if (!TimeBounds.IsEmpty())
	{
		SetGlobalTime(TimeBounds.GetLowerBoundValue());
	}
	return FReply::Handled();
}

FReply FSequencer::OnToggleLooping()
{
	bLoopingEnabled = !bLoopingEnabled;
	return FReply::Handled();
}

bool FSequencer::IsLooping() const
{
	return bLoopingEnabled;
}

EPlaybackMode::Type FSequencer::GetPlaybackMode() const
{
	return PlaybackState == EMovieScenePlayerStatus::Playing ? EPlaybackMode::PlayingForward :
		PlaybackState == EMovieScenePlayerStatus::Recording ? EPlaybackMode::Recording :
		EPlaybackMode::Stopped;
}


TRange<float> FSequencer::GetTimeBounds() const
{
	// When recording, we never want to constrain the time bound range.  You might not even have any sections or keys yet
	// but we need to be able to move the time cursor during playback so you can capture data in real-time
	if( PlaybackState == EMovieScenePlayerStatus::Recording )
	{
		return TRange<float>( -100000.0f, 100000.0f );
	}

	const UMovieScene* MovieScene = GetFocusedMovieScene();
	const UMovieSceneTrack* AnimatableShot = MovieScene->FindMasterTrack( UMovieSceneDirectorTrack::StaticClass() );
	if (AnimatableShot)
	{
		// try getting filtered shot boundaries
		TRange<float> Bounds = GetFilteringShotsTimeBounds();
		if (!Bounds.IsEmpty()) {return Bounds;}

		// try getting the bounds of all shots
		Bounds = AnimatableShot->GetSectionBoundaries();
		if (!Bounds.IsEmpty()) {return Bounds;}
	}
	
	return MovieScene->GetTimeRange();
}

TRange<float> FSequencer::GetFilteringShotsTimeBounds() const
{
	if (IsShotFilteringOn())
	{
		TArray< TRange<float> > Bounds;
		for (int32 i = 0; i < FilteringShots.Num(); ++i)
		{
			Bounds.Add(FilteringShots[i]->GetRange());
		}

		return TRange<float>::Hull(Bounds);
	}

	return TRange<float>::Empty();
}


UK2Node_PlayMovieScene* FSequencer::FindPlayMovieSceneNodeInLevelScript( const UMovieScene* MovieScene )
{
	// Grab the world object for this editor
	check( MovieScene != NULL );
	check( IsWorldCentricAssetEditor() );
	UWorld* World = GetToolkitHost()->GetWorld();
	check( World != NULL );

	// Search all levels in the specified world
	for( TArray<ULevel*>::TConstIterator LevelIter( World->GetLevels().CreateConstIterator() ); LevelIter; ++LevelIter )
	{
		auto* Level = *LevelIter;
		if( Level != NULL )
		{
			// We don't want to create a level script if one doesn't exist yet.  We just want to grab the one
			// that we already have, if one exists.
			const bool bDontCreate = true;
			ULevelScriptBlueprint* LSB = Level->GetLevelScriptBlueprint( bDontCreate );
			if( LSB != NULL )
			{
				TArray<UK2Node_PlayMovieScene*> EventNodes;
				FBlueprintEditorUtils::GetAllNodesOfClass( LSB, EventNodes );
				for( auto FoundNodeIter( EventNodes.CreateIterator() ); FoundNodeIter; ++FoundNodeIter )
				{
					UK2Node_PlayMovieScene* FoundNode = *FoundNodeIter;
					if( FoundNode->GetMovieScene() == MovieScene )
					{
						return FoundNode;
					}
				}
			}
		}
	}

	return NULL;
}


UK2Node_PlayMovieScene* FSequencer::CreateNewPlayMovieSceneNode( UMovieScene* MovieScene )
{
	// Grab the world object for this editor
	check( MovieScene != NULL );
	check( IsWorldCentricAssetEditor() );
	UWorld* World = GetToolkitHost()->GetWorld();
	check( World != NULL );

	ULevel* Level = World->GetCurrentLevel();
	check( Level != NULL );

	// Here, we'll create a level script if one does not yet exist.
	const bool bDontCreate = false;
	ULevelScriptBlueprint* LSB = Level->GetLevelScriptBlueprint( bDontCreate );
	if( LSB != NULL )
	{
		UEdGraph* TargetGraph = NULL;
		if( LSB->UbergraphPages.Num() > 0 )
		{
			TargetGraph = LSB->UbergraphPages[0]; // Just use the first graph
		}

		if( ensure( TargetGraph != NULL ) )
		{
			// Figure out a decent place to stick the node
			const FVector2D NewNodePos = TargetGraph->GetGoodPlaceForNewNode();

			// Create a new node
			UK2Node_PlayMovieScene* TemplateNode = NewObject<UK2Node_PlayMovieScene>();
			TemplateNode->SetMovieScene( MovieScene );
			return FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_PlayMovieScene>(TargetGraph, TemplateNode, NewNodePos);;
		}
	}

	return NULL;
}

void FSequencer::OnViewRangeChanged( TRange<float> NewViewRange, bool bSmoothZoom )
{
	if (!NewViewRange.IsEmpty())
	{
		if (bSmoothZoom)
		{
			if (!ZoomAnimation.IsPlaying())
			{
				LastViewRange = TargetViewRange;

				ZoomAnimation.Play();
			}
			TargetViewRange = NewViewRange;
		}
		else
		{
			TargetViewRange = LastViewRange = NewViewRange;
			ZoomAnimation.JumpToEnd();
		}
	}
}

void FSequencer::OnScrubPositionChanged( float NewScrubPosition, bool bScrubbing )
{
	if (bScrubbing)
	{
		PlaybackState =
			PlaybackState == EMovieScenePlayerStatus::BeginningScrubbing || PlaybackState == EMovieScenePlayerStatus::Scrubbing ?
			EMovieScenePlayerStatus::Scrubbing : EMovieScenePlayerStatus::BeginningScrubbing;
	}
	else
	{
		PlaybackState = EMovieScenePlayerStatus::Stopped;
	}

	SetGlobalTime( NewScrubPosition );
}

void FSequencer::OnToggleAutoKey( bool bInAllowAutoKey )
{
	bAllowAutoKey = bInAllowAutoKey;
}

void FSequencer::OnToggleCleanView( bool bInCleanViewEnabled )
{
	bCleanViewEnabled = bInCleanViewEnabled;
}

// @todo remove when world-centric mode is added
TSharedRef<class SSequencer> FSequencer::GetMainSequencer()
{
	return SequencerWidget.ToSharedRef();
}


FGuid FSequencer::AddSpawnableForAssetOrClass( UObject* Object, UObject* CounterpartGamePreviewObject )
{
	// Grab the MovieScene that is currently focused.  We'll add our Blueprint as an inner of the
	// MovieScene asset.
	UMovieScene* OwnerMovieScene = GetFocusedMovieScene();

	// @todo sequencer: Undo doesn't seem to be working at all
	const FScopedTransaction Transaction( LOCTEXT("UndoAddingObject", "Add Object to MovieScene") );

	// Use the class as the spawnable's name if this is an actor class, otherwise just use the object name (asset)
	const bool bIsActorClass = Object->IsA( AActor::StaticClass() ) && !Object->HasAnyFlags( RF_ArchetypeObject );
	const FName AssetName = bIsActorClass ? Object->GetClass()->GetFName() : Object->GetFName();

	// Inner objects don't need a name (it will be auto-generated by the UObject system), but we want one in this case
	// because the class of any actors that are created from this Blueprint may end up being user-facing.
	const FName BlueprintName = MakeUniqueObjectName( OwnerMovieScene, UBlueprint::StaticClass(), AssetName );

	// Use the asset name as the initial spawnable name
	const FString& NewSpawnableName = AssetName.ToString();		// @todo sequencer: Need UI to allow user to rename these slots

	// Create our new blueprint!
	UBlueprint* NewBlueprint = NULL;
	{
		// @todo sequencer: Add support for forcing specific factories for an asset
		UActorFactory* FactoryToUse = NULL;
		if( bIsActorClass )
		{
			// Placing an actor class directly::
			FactoryToUse = GEditor->FindActorFactoryForActorClass( Object->GetClass() );
		}
		else
		{
			// Placing an asset
			FactoryToUse = FActorFactoryAssetProxy::GetFactoryForAssetObject( Object );
		}

		if( FactoryToUse != NULL )
		{
			// Create the blueprint
			NewBlueprint = FactoryToUse->CreateBlueprint( Object, OwnerMovieScene, BlueprintName );
		}
		else if( bIsActorClass )
		{
			// We don't have a factory, but we can still try to create a blueprint for this actor class
			NewBlueprint = FKismetEditorUtilities::CreateBlueprint( Object->GetClass(), OwnerMovieScene, BlueprintName, EBlueprintType::BPTYPE_Normal, UBlueprint::StaticClass() );
		}
	}

	FGuid NewSpawnableGuid;
	if( ensure( NewBlueprint != NULL ) )
	{
		if( NewBlueprint->GeneratedClass != NULL && FBlueprintEditorUtils::IsActorBased( NewBlueprint ) )
		{
			AActor* ActorCDO = CastChecked< AActor >( NewBlueprint->GeneratedClass->ClassDefaultObject );

			// If we have a counterpart object, then copy the properties from that object back into our blueprint's CDO
			// @todo sequencer livecapture: This isn't really good enough to handle complex actors.  The dynamically-spawned actor could have components that
			// were created in its construction script or via straight-up C++ code.  Instead what we should probably do is duplicate the PIE actor and generate
			// our CDO from that duplicate.  It could get pretty complex though.
			if( CounterpartGamePreviewObject != NULL )
			{
				AActor* CounterpartGamePreviewActor = CastChecked< AActor >( CounterpartGamePreviewObject );
				CopyActorProperties( CounterpartGamePreviewActor, ActorCDO );
			}
			else
			{
				// Place the new spawnable in front of the camera (unless we were automatically created from a PIE actor)
				PlaceActorInFrontOfCamera( ActorCDO );
			}
		}

		NewSpawnableGuid = OwnerMovieScene->AddSpawnable( NewSpawnableName, NewBlueprint, CounterpartGamePreviewObject );

		if (IsShotFilteringOn()) {AddUnfilterableObject(NewSpawnableGuid);}
	}
	else
	{
		// Factory could not produce a blueprint or some other error occurred.  Not expected to ever happen.
	}

	return NewSpawnableGuid;
}

void FSequencer::AddSubMovieScene( UMovieScene* SubMovieScene )
{
	// @todo Sequencer - sub-moviescenes This should be moved to the sub-moviescene editor

	// Grab the MovieScene that is currently focused.  THis is the movie scene that will contain the sub-moviescene
	UMovieScene* OwnerMovieScene = GetFocusedMovieScene();

	// @todo sequencer: Undo doesn't seem to be working at all
	const FScopedTransaction Transaction( LOCTEXT("UndoAddingObject", "Add Object to MovieScene") );

	OwnerMovieScene->Modify();

	UMovieSceneTrack* Type = OwnerMovieScene->FindMasterTrack( USubMovieSceneTrack::StaticClass() ) ;
	if( !Type )
	{
		Type = OwnerMovieScene->AddMasterTrack( USubMovieSceneTrack::StaticClass() );
	}

	USubMovieSceneTrack* SubMovieSceneType = CastChecked<USubMovieSceneTrack>( Type );

	SubMovieSceneType->AddMovieSceneSection( SubMovieScene, ScrubPosition );
}

bool FSequencer::OnHandleAssetDropped(UObject* DroppedAsset, const FGuid& TargetObjectGuid)
{
	bool bWasConsumed = false;
	for (int32 i = 0; i < TrackEditors.Num(); ++i)
	{
		bool bWasHandled = TrackEditors[i]->HandleAssetAdded(DroppedAsset, TargetObjectGuid);
		if (bWasHandled)
		{
			// @todo Sequencer - This will crash if multiple editors try to handle a single asset
			// Should we allow this? How should it consume then?
			check(!bWasConsumed);
			bWasConsumed = true;
		}
	}
	return bWasConsumed;
}

void FSequencer::OnRequestNodeDeleted( TSharedRef<const FSequencerDisplayNode>& NodeToBeDeleted )
{
	bool bAnySpawnablesRemoved = false;
	bool bAnythingRemoved = false;
	
	TSharedRef<FMovieSceneInstance> MovieSceneInstance = GetFocusedMovieSceneInstance();
	UMovieScene* OwnerMovieScene = MovieSceneInstance->GetMovieScene();

	// Only object nodes or section areas can be deleted
	if( NodeToBeDeleted->GetType() == ESequencerNode::Object  )
	{
		OwnerMovieScene->SetFlags( RF_Transactional );
		const FGuid& BindingToRemove = StaticCastSharedRef<const FObjectBindingNode>( NodeToBeDeleted )->GetObjectBinding();

		// Try to remove as a spawnable first
		bool bRemoved = OwnerMovieScene->RemoveSpawnable( BindingToRemove );
		if( bRemoved )
		{
			bAnySpawnablesRemoved = true;
		}

		if( !bRemoved )
		{
			// The guid should be associated with a possessable if it wasnt a spawnable
			bRemoved = OwnerMovieScene->RemovePossessable( BindingToRemove );
			// If this check fails the guid was not associated with a spawnable or possessable so there was an invalid guid being stored on a node
			check( bRemoved );
		}

		bAnythingRemoved = true;
	}
	else if( NodeToBeDeleted->GetType() == ESequencerNode::Track  )
	{
		TSharedRef<const FTrackNode> SectionAreaNode = StaticCastSharedRef<const FTrackNode>( NodeToBeDeleted );
		UMovieSceneTrack* Track = SectionAreaNode->GetTrack();

		UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

		FocusedMovieScene->SetFlags( RF_Transactional );
		
		if( FocusedMovieScene->IsAMasterTrack( Track ) )
		{
			FocusedMovieScene->RemoveMasterTrack( Track );
		}
		else
		{
			FocusedMovieScene->RemoveTrack( Track );
		}
		
		bAnythingRemoved = true;
	}

	
	if( bAnythingRemoved )
	{
		if( bAnySpawnablesRemoved )
		{
			// @todo Sequencer Sub-MovieScenes needs to destroy objects for all movie scenes that had this node
			SpawnOrDestroyPuppetObjects( MovieSceneInstance );
		}

		NotifyMovieSceneDataChanged();
	}
}

void FSequencer::PlaceActorInFrontOfCamera( AActor* ActorCDO )
{
	// Place the actor in front of the active perspective camera if we have one
	if( GCurrentLevelEditingViewportClient != NULL && GCurrentLevelEditingViewportClient->IsPerspective() )
	{
		// Don't allow this when the active viewport is showing a simulation/PIE level
		const bool bIsViewportShowingPIEWorld = ( GCurrentLevelEditingViewportClient->GetWorld()->GetOutermost()->PackageFlags & PKG_PlayInEditor ) != 0;
		if( !bIsViewportShowingPIEWorld )
		{
			// @todo sequencer actors: Ideally we could use the actor's collision to figure out how far to push out
			// the object (like when placing in viewports), but we can't really do that because we're only dealing with a CDO
			const float DistanceFromCamera = 50.0f;

			// Find a place to put the object
			// @todo sequencer cleanup: This code should be reconciled with the GEditor->MoveActorInFrontOfCamera() stuff
			const FVector& CameraLocation = GCurrentLevelEditingViewportClient->GetViewLocation();
			const FRotator& CameraRotation = GCurrentLevelEditingViewportClient->GetViewRotation();
			const FVector CameraDirection = CameraRotation.Vector();

			FVector NewLocation = CameraLocation + CameraDirection * ( DistanceFromCamera + GetDefault<ULevelEditorViewportSettings>()->BackgroundDropDistance );
			FSnappingUtils::SnapPointToGrid( NewLocation, FVector::ZeroVector );

			ActorCDO->SetActorRelativeLocation( NewLocation );
			ActorCDO->SetActorRelativeRotation( CameraRotation );
		}
	}
}


void FSequencer::CopyActorProperties( AActor* PuppetActor, AActor* TargetActor ) const
{
	// @todo sequencer: How do we make this undoable with the original action that caused the change in the first place? 
	//       -> Ideally we are still within the transaction scope while performing this change

	// @todo sequencer: If we decide to propagate changes BACK to an actor after changing the CDO, we may need to
	// do additional work to make sure transient state is refreshed (e.g. call PostEditMove() is transform is changed)

	// @todo sequencer: Pass option here to call PostEditChange, if needed
	const int32 CopiedPropertyCount = EditorUtilities::CopyActorProperties( PuppetActor, TargetActor );
}


void FSequencer::GetActorRecordingState( bool& bIsRecording /* In+Out */ ) const
{
	if( IsRecordingLive() )
	{
		bIsRecording = true;
	}
}

void FSequencer::PostUndo(bool bSuccess)
{
	NotifyMovieSceneDataChanged();
}

TArray< TWeakObjectPtr<UMovieSceneSection> > FSequencer::GetSelectedSections() const
{
	return SelectedSections;
}

void FSequencer::SelectSection(UMovieSceneSection* Section)
{
	if (!Section->IsA<UMovieSceneShotSection>())
	{
		// if we select something, consider it unfilterable until we change shot filters
		UnfilterableSections.AddUnique(TWeakObjectPtr<UMovieSceneSection>(Section));
	}

	SelectedSections.Add(TWeakObjectPtr<UMovieSceneSection>(Section));
}

bool FSequencer::IsSectionSelected(UMovieSceneSection* Section) const
{
	return SelectedSections.Contains(TWeakObjectPtr<UMovieSceneSection>(Section));
}

void FSequencer::ClearSectionSelection()
{
	SelectedSections.Empty();
}

void FSequencer::ZoomToSelectedSections()
{
	TArray< TRange<float> > Bounds;
	for (int32 i = 0; i < SelectedSections.Num(); ++i)
	{
		Bounds.Add(SelectedSections[i]->GetRange());
	}
	TRange<float> BoundsHull = TRange<float>::Hull(Bounds);

	if (BoundsHull.IsEmpty())
	{
		BoundsHull = GetTimeBounds();
	}

	if (!BoundsHull.IsEmpty())
	{
		OnViewRangeChanged(BoundsHull, true);
	}
}


void FSequencer::SelectKey( const FSelectedKey& Key )
{
	SelectedKeys.Add( Key );
}

void FSequencer::ClearKeySelection()
{
	SelectedKeys.Empty();
}

bool FSequencer::IsKeySelected( const FSelectedKey& Key ) const
{
	return SelectedKeys.Contains( Key );
}

TSet<FSelectedKey>& FSequencer::GetSelectedKeys()
{
	return SelectedKeys; 
}

void FSequencer::FilterToShotSections(const TArray< TWeakObjectPtr<class UMovieSceneSection> >& ShotSections, bool bZoomToShotBounds )
{
	TArray< TWeakObjectPtr<UMovieSceneSection> > ActualShotSections;
	for (int32 i = 0; i < ShotSections.Num(); ++i)
	{
		if (ShotSections[i]->IsA<UMovieSceneShotSection>())
		{
			ActualShotSections.Add(ShotSections[i]);
		}
	}

	bool bWasPreviouslyFiltering = IsShotFilteringOn();
	bool bIsNowFiltering = ActualShotSections.Num() > 0;

	FilteringShots.Empty();
	UnfilterableSections.Empty();
	UnfilterableObjects.Empty();

	if (bIsNowFiltering)
	{
		for ( int32 ShotSectionIndex = 0; ShotSectionIndex < ActualShotSections.Num(); ++ShotSectionIndex )
		{
			FilteringShots.Add( ActualShotSections[ShotSectionIndex] );
		}

		// populate unfilterable shots - shots that started not filtered
		TArray< TWeakObjectPtr<class UMovieSceneSection> > TempUnfilterableSections;
		const TArray<UMovieSceneSection*>& AllSections = GetFocusedMovieScene()->GetAllSections();
		for (int32 SectionIndex = 0; SectionIndex < AllSections.Num(); ++SectionIndex)
		{
			UMovieSceneSection* Section = AllSections[SectionIndex];
			if (!Section->IsA<UMovieSceneShotSection>() && IsSectionVisible(Section))
			{
				TempUnfilterableSections.Add(Section);
			}
		}
		// wait until after we've collected them all before applying in order to
		// prevent wastefully searching through UnfilterableSections in IsSectionVisible
		UnfilterableSections = TempUnfilterableSections;
	}

	if (!bWasPreviouslyFiltering && bIsNowFiltering)
	{
		OverlayAnimation.Play();
	}
	else if (bWasPreviouslyFiltering && !bIsNowFiltering) 
	{
		OverlayAnimation.PlayReverse();
	}

	if( bZoomToShotBounds )
	{
		// zoom in
		OnViewRangeChanged(GetTimeBounds(), true);
	}

	SequencerWidget->UpdateBreadcrumbs(ActualShotSections);
}

void FSequencer::FilterToSelectedShotSections(bool bZoomToShotBounds)
{
	TArray< TWeakObjectPtr<UMovieSceneSection> > SelectedShotSections;
	for (int32 i = 0; i < SelectedSections.Num(); ++i)
	{
		if (SelectedSections[i]->IsA<UMovieSceneShotSection>())
		{
			SelectedShotSections.Add(SelectedSections[i]);
		}
	}
	FilterToShotSections(SelectedShotSections, bZoomToShotBounds);
}

TArray< TWeakObjectPtr<UMovieSceneSection> > FSequencer::GetFilteringShotSections() const
{
	return FilteringShots;
}

bool FSequencer::IsObjectUnfilterable(const FGuid& ObjectGuid) const
{
	return UnfilterableObjects.Find(ObjectGuid) != INDEX_NONE;
}

void FSequencer::AddUnfilterableObject(const FGuid& ObjectGuid)
{
	UnfilterableObjects.AddUnique(ObjectGuid);
}

bool FSequencer::IsShotFilteringOn() const
{
	return FilteringShots.Num() > 0;
}

bool FSequencer::IsUsingCleanView() const
{
	return bCleanViewEnabled;
}

float FSequencer::GetOverlayFadeCurve() const
{
	return OverlayCurve.GetLerp();
}

bool FSequencer::IsSectionVisible(UMovieSceneSection* Section) const
{
	// if no shots are being filtered, don't filter at all
	bool bShowAll = !IsShotFilteringOn();

	bool bIsAShotSection = Section->IsA<UMovieSceneShotSection>();

	bool bIsUnfilterable = UnfilterableSections.Find(Section) != INDEX_NONE;

	// do not draw if not within the filtering shot sections
	TRange<float> SectionRange = Section->GetRange();
	bool bIsVisible = bShowAll || bIsAShotSection || bIsUnfilterable;
	for (int32 i = 0; i < FilteringShots.Num() && !bIsVisible; ++i)
	{
		TRange<float> ShotRange = FilteringShots[i]->GetRange();
		bIsVisible |= SectionRange.Overlaps(ShotRange);
	}

	return bIsVisible;
}

void FSequencer::DeleteSelectedItems()
{
	DeleteKeys(SelectedKeys.Array());

	for (int32 i = 0; i < SelectedSections.Num(); ++i)
	{
		DeleteSection(SelectedSections[i].Get());
	}

	SequencerWidget->DeleteSelectedNodes();
}

void FSequencer::SetKey()
{
	USelection* Selection = GEditor->GetSelectedActors();
	TArray<UObject*> SelectedActors;
	Selection->GetSelectedObjects( AActor::StaticClass(), SelectedActors );
	for (TArray<UObject*>::TIterator It(SelectedActors); It; ++It)
	{
		// @todo Handle case of actors which aren't in sequencer yet

		FGuid ObjectGuid = GetHandleToObject(*It);
		for (int i = 0; i < TrackEditors.Num(); ++i)
		{
			// @todo Handle this director track business better
			if (TrackEditors[i] != DirectorTrackEditor.Pin())
			{
				TrackEditors[i]->AddKey(ObjectGuid);
			}
		}
	}
}


void FSequencer::BindSequencerCommands()
{
	const FSequencerCommands& Commands = FSequencerCommands::Get();

	SequencerCommandBindings->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP( this, &FSequencer::DeleteSelectedItems ) );

	SequencerCommandBindings->MapAction(
		Commands.SetKey,
		FExecuteAction::CreateSP( this, &FSequencer::SetKey ) );
}

void FSequencer::BuildObjectBindingContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass)
{
	for (int32 i = 0; i < TrackEditors.Num(); ++i)
	{
		TrackEditors[i]->BuildObjectBindingContextMenu(MenuBuilder, ObjectBinding, ObjectClass);
	}
}

#undef LOCTEXT_NAMESPACE
