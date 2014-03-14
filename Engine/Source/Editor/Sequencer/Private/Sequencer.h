// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISequencerInternals.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"	// For EMapChangeType::Type
#include "Editor/EditorWidgets/Public/ITransportControl.h"

class UMovieScene;


/**
 * Sequencer is the editing tool for MovieScene assets
 */
class FSequencer :
	public ISequencerInternals, FGCObject, public FEditorUndoClient
{ 

public:

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

	/**
	 * Edits the specified object
	 *
	 * @param	Mode							Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost					When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	ObjectToEdit					The object to edit
	 * @param	TrackEditorDelegates		Delegates to call to create auto-key handlers for this sequencer
	 */
	void InitSequencer( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit, const TArray<FOnCreateTrackEditor>& TrackEditorDelegates );

	/** Constructor */
	FSequencer();

	/** Destructor */
	virtual ~FSequencer();

	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;

	/** ISequencerInternals interface */
	virtual TArray< UMovieScene* > GetMovieScenesBeingEdited() OVERRIDE;
	virtual UMovieScene* GetRootMovieScene() const OVERRIDE;
	virtual UMovieScene* GetFocusedMovieScene() const OVERRIDE;
	virtual TSharedRef<FMovieSceneInstance> GetRootMovieSceneInstance() const OVERRIDE;
	virtual TSharedRef<FMovieSceneInstance> GetFocusedMovieSceneInstance() const OVERRIDE;
	virtual void FocusSubMovieScene( TSharedRef<FMovieSceneInstance> SubMovieSceneInstance ) OVERRIDE;
	TSharedRef<FMovieSceneInstance> GetInstanceForSubMovieSceneSection( UMovieSceneSection& SubMovieSceneSection ) const OVERRIDE;
	virtual void PopToMovieScene( TSharedRef<FMovieSceneInstance> SubMovieSceneInstance ) OVERRIDE;
	virtual void EditDetailsForObjects( TArray< UObject* > ObjectsToEdit ) OVERRIDE;
	virtual void SpawnOrDestroyPuppetObjects( TSharedRef<FMovieSceneInstance> MovieSceneInstance ) OVERRIDE;
	virtual void AddNewShot(FGuid CameraGuid) OVERRIDE;
	virtual void AddAnimation( FGuid ObjectGuid, class UAnimSequence* AnimSequence ) OVERRIDE;
	virtual void RenameShot(class UMovieSceneSection* ShotSection) OVERRIDE;
	virtual void DeleteSection(class UMovieSceneSection* Section) OVERRIDE;
	virtual void DeleteKeys(TArray<FSelectedKey> KeysToDelete) OVERRIDE;
	virtual class UK2Node_PlayMovieScene* BindToPlayMovieSceneNode( const bool bCreateIfNotFound ) OVERRIDE;
	virtual bool IsAutoKeyEnabled() const OVERRIDE;
	virtual bool IsRecordingLive() const OVERRIDE;
	virtual float GetCurrentLocalTime( UMovieScene& MovieScene ) OVERRIDE;
	virtual float GetGlobalTime() OVERRIDE;
	virtual void SetGlobalTime(float Time) OVERRIDE;
	virtual void SetPerspectiveViewportPossessionEnabled(bool bEnabled) OVERRIDE;
	virtual FGuid GetHandleToObject( UObject* Object ) OVERRIDE;
	virtual const TArray< TSharedPtr<FMovieSceneTrackEditor> >& GetTrackEditors() const OVERRIDE { return TrackEditors; }
	virtual ISequencerObjectChangeListener& GetObjectChangeListener() const OVERRIDE;
	virtual void NotifyMovieSceneDataChanged() OVERRIDE;
	virtual void Tick( const float InDeltaTime ) OVERRIDE;
	virtual FGuid AddSpawnableForAssetOrClass( UObject* Object, UObject* CounterpartGamePreviewObject ) OVERRIDE;
	virtual void AddSubMovieScene( UMovieScene* SubMovieScene ) OVERRIDE;
	virtual bool OnHandleAssetDropped(UObject* DroppedAsset, const FGuid& TargetObjectGuid) OVERRIDE;
	virtual void OnRequestNodeDeleted( TSharedRef<const FSequencerDisplayNode>& NodeToBeDeleted ) OVERRIDE;
	virtual void CopyActorProperties( AActor* PuppetActor, AActor* TargetActor ) const OVERRIDE;
	virtual TArray< TWeakObjectPtr<UMovieSceneSection> > GetSelectedSections() const OVERRIDE;
	virtual void SelectSection(UMovieSceneSection* Section) OVERRIDE;
	virtual bool IsSectionSelected( UMovieSceneSection* Section) const OVERRIDE;
	virtual void ClearSectionSelection() OVERRIDE;
	virtual void ZoomToSelectedSections() OVERRIDE;
	virtual void SelectKey( const FSelectedKey& Key ) OVERRIDE;
	virtual void ClearKeySelection() OVERRIDE;
	virtual bool IsKeySelected( const FSelectedKey& Key ) const OVERRIDE;
	virtual TSet<FSelectedKey>& GetSelectedKeys() OVERRIDE;
	virtual void FilterToShotSections( const TArray< TWeakObjectPtr<class UMovieSceneSection> >& ShotSections, bool bZoomToShotBounds ) OVERRIDE;
	virtual void FilterToSelectedShotSections(bool bZoomToShotBounds = true) OVERRIDE;
	virtual TArray< TWeakObjectPtr<UMovieSceneSection> > GetFilteringShotSections() const OVERRIDE;
	virtual bool IsObjectUnfilterable(const FGuid& ObjectGuid) const OVERRIDE;
	virtual void AddUnfilterableObject(const FGuid& ObjectGuid) OVERRIDE;
	virtual bool IsShotFilteringOn() const OVERRIDE;
	virtual bool IsUsingCleanView() const OVERRIDE;
	virtual float GetOverlayFadeCurve() const OVERRIDE;
	virtual bool IsSectionVisible(UMovieSceneSection* Section) const OVERRIDE;
	virtual FUICommandList& GetCommandBindings() OVERRIDE {return *SequencerCommandBindings;}
	virtual void BuildObjectBindingContextMenu(class FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const class UClass* ObjectClass) OVERRIDE;


	/** IMovieScenePlayer interface */
	virtual void GetRuntimeObjects( TSharedRef<FMovieSceneInstance> MovieSceneInstance, const FGuid& ObjectHandle, TArray< UObject* >& OutObjects ) const;
	virtual void UpdateViewports(AActor* ActorToViewThrough) const OVERRIDE;
	virtual EMovieScenePlayerStatus::Type GetPlaybackStatus() const OVERRIDE;
	virtual void AddMovieSceneInstance( class UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneInstance> InstanceToAdd ) OVERRIDE;
	virtual void RemoveMovieSceneInstance( class UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneInstance> InstanceToRemove ) OVERRIDE;

	virtual void SpawnActorsForMovie( TSharedRef<FMovieSceneInstance> MovieSceneInstance );

	/** FGCObject interface */
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	// @todo remove when world-centric mode is added
	TSharedRef<class SSequencer> GetMainSequencer();

private:
	TSharedRef<SDockTab> SpawnTab_SequencerMain(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);	
protected:

	/** Generates and attaches transport control widgets to the main level editor viewports */
	void AttachTransportControlsToViewports();
	/** Purges all transport control widgets from the main level editor viewports */
	void DetachTransportControlsFromViewports();
	
	/**
	 * Reset data about a movie scene when pushing or popping a movie scene
	 */
	void ResetPerMovieSceneData();

	/** Updates runtime instances when a movie scene changes */
	void UpdateRuntimeInstances();

	/**
	 * Destroys spawnables for all movie scenes in the stack
	 */
	void DestroySpawnablesForAllMovieScenes();

	/** Gets the visibility of a transport control based on the viewport it's attached to */
	EVisibility GetTransportControlVisibility(TSharedPtr<ILevelViewport> LevelViewport) const;

	/** Sets the actor CDO such that it is placed in front of the active perspective viewport camera, if we have one */
	static void PlaceActorInFrontOfCamera( AActor* ActorCDO );

	/** Functions to push on to the transport controls we use */
	FReply OnPlay();
	FReply OnRecord();
	FReply OnStepForward();
	FReply OnStepBackward();
	FReply OnStepToEnd();
	FReply OnStepToBeginning();
	FReply OnToggleLooping();
	bool IsLooping() const;
	EPlaybackMode::Type GetPlaybackMode() const;

	/**
	 * Gets the far time boundaries of the currently edited movie scene
	 * If the scene has shots, it only takes the shot section boundaries
	 * Otherwise, it finds the furthest boundaries of all sections
	 */
	TRange<float> GetTimeBounds() const;
	
	/**
	 * Gets the time boundaries of the currently filtering shot sections.
	 * If there are no shot filters, an empty range is returned.
	 */
	TRange<float> GetFilteringShotsTimeBounds() const;


	/**
	 * Given a MovieScene to search for, attempts to find a LevelScript node that is playing the
	 * supplied MovieScene.  Only valid to call when in world-centric mode.
	 *
	 * @param	MovieScene	The MovieScene to search for
	 *
	 * @return	The LevelScript node we found, or NULL for no matches
	 */
	class UK2Node_PlayMovieScene* FindPlayMovieSceneNodeInLevelScript( const class UMovieScene* MovieScene );


	/**
	 * Creates a new "PlayMovieScene" node in the level script for the current level and associates it with the
	 * supplied MovieScene.  Only valid to call when in world-centric mode.
	 *
	 * @param	MovieScene	The MovieScene to associate with the newly-created node
	 *
	 * @return	The LevelScript node we created, or NULL on failure (does not assert)
	 */
	class UK2Node_PlayMovieScene* CreateNewPlayMovieSceneNode( UMovieScene* MovieScene );

	/** Called by LevelEditor when the map changes */
	void OnMapChanged( class UWorld* NewWorld, EMapChangeType::Type MapChangeType );

	/** Called by UK2Node_PlayMovieScene when the bindings for that node are changed through Kismet */
	void OnPlayMovieSceneBindingsChanged();

	/** @return The current view range */
	virtual TRange<float> OnGetViewRange() const;

	/** @return The current scrub position */
	virtual float OnGetScrubPosition() const { return ScrubPosition; }

	/** @return Whether or not we currently allow autokeying */
	virtual bool OnGetAllowAutoKey() const { return bAllowAutoKey; }

	/**
	 * Called when the view range is changed by the user
	 *
	 * @param	NewViewRange The new view range
	 */
	void OnViewRangeChanged( TRange<float> NewViewRange, bool bSmoothZoom );

	/**
	 * Called when the scrub position is changed by the user
	 * This will stop any playback from happening
	 *
	 * @param NewScrubPosition	The new scrub position
	 */
	void OnScrubPositionChanged( float NewScrubPosition, bool bScrubbing );

	/**
	 * Called when auto-key is toggled by a user
	 *
	 * @param bInAllowAutoKey	The new auto key state
	 */
	void OnToggleAutoKey( bool bInAllowAutoKey );
	
	/**
	 * Called when auto-key is toggled by a user
	 *
	 * @param bInAllowAutoKey	The new auto key state
	 */
	void OnToggleCleanView( bool bInCleanViewEnabled );

	/** Called via UEditorEngine::GetActorRecordingStateEvent to check to see whether we need to record actor state */
	void GetActorRecordingState( bool& bIsRecording /* In+Out */ ) const;
	
	/* Called when committing a rename shot text entry popup */
	void RenameShotCommitted(const FText& RenameText, ETextCommit::Type CommitInfo, UMovieSceneSection* Section);

	/** Called when a user executes the delete command to delete sections or keys */
	void DeleteSelectedItems();
	
	/** Manually sets a key for the selected objects at the current time */
	void SetKey();

	/** Generates command bindings for UI commands */
	void BindSequencerCommands();

	// Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) OVERRIDE;
	virtual void PostRedo(bool bSuccess) OVERRIDE { PostUndo(bSuccess); }
	// End of FEditorUndoClient

private:
	/** Command list for seququencer commands */
	TSharedRef<FUICommandList> SequencerCommandBindings;

	/** A map of all the transport controls to viewports that this sequencer has made */
	TMap< TSharedPtr<class ILevelViewport>, TSharedPtr<class SWidget> > TransportControls;

	/** List of tools we own */
	TArray< TSharedPtr<FMovieSceneTrackEditor> > TrackEditors;

	/** The editors we keep track of for special behaviors */
	TWeakPtr<FMovieSceneTrackEditor> DirectorTrackEditor;
	TWeakPtr<FMovieSceneTrackEditor> AnimationTrackEditor;

	/** Details view */
	TSharedPtr< class IDetailsView > DetailsView;

	/** Object spawner */
	TSharedPtr< class ISequencerObjectSpawner > ObjectSpawner;

	/** Listener for object changes being made while this sequencer is open*/
	TSharedPtr< class FSequencerObjectChangeListener > ObjectChangeListener; 

	/** The runtime instance for the root movie scene */
	TSharedPtr< class FMovieSceneInstance > RootMovieSceneInstance;

	TMap< TWeakObjectPtr<UMovieSceneSection>, TSharedRef<FMovieSceneInstance> > MovieSceneSectionToInstanceMap;

	/** Main sequencer widget */
	TSharedPtr< class SSequencer > SequencerWidget;
	
	/** Reference to owner of the current popup */
	TWeakPtr<class SWindow> NameEntryPopupWindow;

	/** 'PlayMovieScene' node in level script that we're currently associating with the active MovieScene.  This node contains the bindings
	    information that we need to be able to preview possessed actors in the level */
	TWeakObjectPtr< UK2Node_PlayMovieScene > PlayMovieSceneNode;

	/** A list of all shots that are acting as filters */
	TArray< TWeakObjectPtr<class UMovieSceneSection> > FilteringShots;

	/** A list of sections that are considered unfilterable */
	TArray< TWeakObjectPtr<class UMovieSceneSection> > UnfilterableSections;

	/** A list of object guids that will be visible, regardless of shot filters */
	TArray<FGuid> UnfilterableObjects;

	/** Selected non-shot sections */
	TArray< TWeakObjectPtr<class UMovieSceneSection> > SelectedSections;

	/** Set of selected keys */
	TSet< FSelectedKey > SelectedKeys;

	/** Stack of movie scenes.  The first element is always the root movie scene.  The last element is the focused movie scene */
	TArray< TSharedRef<FMovieSceneInstance> > MovieSceneStack;

	/** The time range target to be viewed */
	TRange<float> TargetViewRange;
	/** The last time range that was viewed */
	TRange<float> LastViewRange;
	/** Zoom smoothing curves */
	FCurveSequence ZoomAnimation;
	FCurveHandle ZoomCurve;
	/** Overlay fading curves */
	FCurveSequence OverlayAnimation;
	FCurveHandle OverlayCurve;

	/** The current scrub position */
	// @todo sequencer: Should use FTimespan or "double" for Time Cursor Position! (cascades)
	float ScrubPosition;
	
	/** Whether the clean sequencer view is enabled */
	bool bCleanViewEnabled;

	/** Whether we are playing, recording, etc. */
	EMovieScenePlayerStatus::Type PlaybackState;

	/** Whether looping while playing is enabled for this sequencer */
	bool bLoopingEnabled;

	/** Whether or not we are allowing autokey */
	bool bAllowAutoKey;

	/** Whether or not we allow perspective viewports to be hijacked by this sequencer */
	bool bPerspectiveViewportPossessionEnabled;


	/**	The tab ids for all the tabs used */
	static const FName SequencerMainTabId;
	static const FName SequencerDetailsTabId;

	/** Stores a dirty bit for whether the sequencer tree (and other UI bits) may need to be refreshed.  We
	    do this simply to avoid refreshing the UI more than once per frame. (e.g. during live recording where
		the MovieScene data can change many times per frame.) */
	bool bNeedTreeRefresh;
};
