// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/EditorWidgets/Public/ITransportControl.h"
#include "SelectedKey.h"
#include "EditorUndoClient.h"

class UMovieScene;
class IToolkitHost;
class ISequencerObjectBindingManager;
class ISequencerObjectChangeListener;

/** Parameters for initializing a Sequencer */
struct FSequencerInitParams
{
	/** The root movie scene being edited */
	UMovieScene* RootMovieScene;
	
	/** The interface for managing runtime object bindings */
	TSharedPtr<ISequencerObjectBindingManager> ObjectBindingManager;
	
	/** The Object change listener for various systems that need to be notified when objects change */
	TSharedPtr<ISequencerObjectChangeListener> ObjectChangeListener;
	
	/** The asset editor created for this (if any) */
	TSharedPtr<IToolkitHost> ToolkitHost;

	/** View parameters */
	FSequencerViewParams ViewParams;

	/** Whether or not sequencer should be edited within the level editor */
	bool bEditWithinLevelEditor;
};

/**
 * Sequencer is the editing tool for MovieScene assets
 */
class FSequencer : public ISequencer, public FGCObject, public FEditorUndoClient, public FTickableEditorObject
{ 

public:	
	static bool IsSequencerEnabled();

	/**
	 * Initializes sequencer
	 *
	 * @param	InitParams		Initialization options
	 * @param	TrackEditorDelegates	Delegates to call to create auto-key handlers for this sequencer

	 */
	void InitSequencer( const FSequencerInitParams& InitParams, const TArray<FOnCreateTrackEditor>& TrackEditorDelegates );

	/** 
	* Closes the sequencer
	*/
	void OnClose();

	/** Constructor */
	FSequencer();

	/** Destructor */
	virtual ~FSequencer();

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	// FTickableEditorObject interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FSequencer, STATGROUP_Tickables); };
	// End FTickableEditorObject interface

	/** ISequencer interface */
	virtual TSharedRef<SWidget> GetSequencerWidget() const override { return SequencerWidget.ToSharedRef(); }
	virtual UMovieScene* GetRootMovieScene() const override;
	virtual UMovieScene* GetFocusedMovieScene() const override;
	virtual void ResetToNewRootMovieScene( UMovieScene& NewRoot, TSharedRef<ISequencerObjectBindingManager> NewObjectBindingManager ) override;
	virtual TSharedRef<FMovieSceneInstance> GetRootMovieSceneInstance() const override;
	virtual TSharedRef<FMovieSceneInstance> GetFocusedMovieSceneInstance() const override;
	virtual void FocusSubMovieScene( TSharedRef<FMovieSceneInstance> SubMovieSceneInstance ) override;
	TSharedRef<FMovieSceneInstance> GetInstanceForSubMovieSceneSection( UMovieSceneSection& SubMovieSceneSection ) const override;
	virtual void AddNewShot(FGuid CameraGuid) override;
	virtual void AddAnimation(FGuid ObjectGuid, class UAnimSequence* AnimSequence) override;
	virtual bool GetAutoKeyEnabled() const override;
	virtual void SetAutoKeyEnabled(bool bAutoKeyEnabled) override;
	virtual bool IsRecordingLive() const override;
	virtual float GetCurrentLocalTime(UMovieScene& MovieScene) override;
	virtual float GetGlobalTime() override;
	virtual void SetGlobalTime(float Time) override;
	virtual void SetPerspectiveViewportPossessionEnabled(bool bEnabled) override;
	virtual FGuid GetHandleToObject(UObject* Object) override;
	virtual ISequencerObjectChangeListener& GetObjectChangeListener() override;
	virtual void NotifyMovieSceneDataChanged() override;
	virtual void UpdateRuntimeInstances() override;
	virtual void AddSubMovieScene(UMovieScene* SubMovieScene) override;
	virtual void FilterToShotSections(const TArray< TWeakObjectPtr<class UMovieSceneSection> >& ShotSections, bool bZoomToShotBounds = false) override;
	virtual void FilterToSelectedShotSections(bool bZoomToShotBounds = true) override;
	virtual bool CanKeyProperty(FCanKeyPropertyParams CanKeyPropertyParams) const override;
	virtual void KeyProperty(FKeyPropertyParams KeyPropertyParams) override;
	virtual TSharedRef<ISequencerObjectBindingManager> GetObjectBindingManager() const override;
	virtual FSequencerSelection& GetSelection() override;

	/** @return The current view range */
	FAnimatedRange GetViewRange() const;

	bool IsPerspectiveViewportPosessionEnabled() const { return bPerspectiveViewportPossessionEnabled; }

	/**
	 * Pops the current focused movie scene from the stack.  The parent of this movie scene will be come the focused one
	 */
	void PopToMovieScene( TSharedRef<FMovieSceneInstance> SubMovieSceneInstance );

	/**
	 * Spawn (or destroy) puppet objects as needed to match the spawnables array in the MovieScene we're editing
	 *
	 * @param MovieSceneInstance	The movie scene instance to spawn or destroy puppet objects for
	 */
	void SpawnOrDestroyPuppetObjects( TSharedRef<FMovieSceneInstance> MovieSceneInstance );

	/** 
	 * Deletes the passed in section
	 */
	void DeleteSection(class UMovieSceneSection* Section);

	/**
	 * Deletes the currently selected in keys
	 */
	void DeleteSelectedKeys();

	/**
	 * @return Movie scene tools used by the sequencer
	 */
	const TArray< TSharedPtr<FMovieSceneTrackEditor> >& GetTrackEditors() const { return TrackEditors; }

	/**
	 * Attempts to add a new spawnable to the MovieScene for the specified asset or class
	 *
	 * @param	Object	The asset or class object to add a spawnable for
	 *
	 * @return	The spawnable guid for the spawnable, or an invalid Guid if we were not able to create a spawnable
	 */
	virtual FGuid AddSpawnableForAssetOrClass( UObject* Object, UObject* CounterpartGamePreviewObject ) ;

	/**
	 * Call when an asset is dropped into the sequencer. Will proprogate this
	 * to all tracks, and potentially consume this asset
	 * so it won't be added as a spawnable
	 *
	 * @param DroppedAsset		The asset that is dropped in
	 * @param TargetObjectGuid	Object to be targeted on dropping
	 * @return					If true, this asset should be consumed
	 */
	virtual bool OnHandleAssetDropped( UObject* DroppedAsset, const FGuid& TargetObjectGuid );
	
	/**
	 * Called to delete all moviescene data from a node
	 *
	 * @param NodeToBeDeleted	Node with data that should be deleted
	 */
	virtual void OnRequestNodeDeleted( TSharedRef<const FSequencerDisplayNode>& NodeToBeDeleted );

	/**
	 * Copies properties from one actor to another.  Only properties that are different are copied.  This is used
	 * to propagate changed properties from a living actor in the scene to properties of a blueprint CDO actor
	 *
	 * @param	PuppetActor	The source actor (the puppet actor in the scene we're copying properties from)
	 * @param	TargetActor	The destination actor (data-only blueprint actor CDO)
	 */
	virtual void CopyActorProperties( AActor* PuppetActor, AActor* TargetActor ) const;

	/**
	 * Zooms to the edges of all currently selected sections
	 */
	void ZoomToSelectedSections();

	/**
	 * Gets all shots that are filtering currently
	 */
	const TArray< TWeakObjectPtr<UMovieSceneSection> >& GetFilteringShotSections() const;

	/**
	 * Checks to see if an object is unfilterable
	 */
	bool IsObjectUnfilterable(const FGuid& ObjectGuid) const;

	/**
	 * Declares an object unfilterable
	 */
	void AddUnfilterableObject(const FGuid& ObjectGuid);

	/**
	 * Checks to see if shot filtering is on
	 */
	bool IsShotFilteringOn() const;

	/**
	 * Gets the overlay fading animation curve lerp
	 */
	float GetOverlayFadeCurve() const;

	/**
	 * Checks if a section is visible, given current shot filtering
	 */
	bool IsSectionVisible(UMovieSceneSection* Section) const;

	/** Gets the command bindings for the sequencer */
	TSharedPtr<FUICommandList> GetCommandBindings() { return SequencerCommandBindings; }

	/**
	 * Builds up the context menu for object binding nodes in the outliner
	 * 
	 * @param MenuBuilder	The context menu builder to add things to
	 * @param ObjectBinding	The object binding of the selected node
	 * @param ObjectClass	The class of the selected object
	 */
	void BuildObjectBindingContextMenu(class FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const class UClass* ObjectClass);

	/**
	 * Builds up the edit buttons for object binding nodes in the outliner
	 * 
	 * @param EditBox	    The edit box to add things to
	 * @param ObjectBinding	The object binding of the selected node
	 * @param ObjectClass	The class of the selected object
	 */
	void BuildObjectBindingEditButtons(TSharedPtr<SHorizontalBox> EditBox, const FGuid& ObjectBinding, const class UClass* ObjectClass);

	/** IMovieScenePlayer interface */
	virtual void GetRuntimeObjects( TSharedRef<FMovieSceneInstance> MovieSceneInstance, const FGuid& ObjectHandle, TArray< UObject* >& OutObjects ) const override;
	virtual void UpdateCameraCut(UObject* ObjectToViewThrough, bool bNewCameraCut) const override;
	virtual EMovieScenePlayerStatus::Type GetPlaybackStatus() const override;
	virtual void AddOrUpdateMovieSceneInstance( class UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneInstance> InstanceToAdd ) override;
	virtual void RemoveMovieSceneInstance( class UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneInstance> InstanceToRemove ) override;

	virtual void SpawnActorsForMovie( TSharedRef<FMovieSceneInstance> MovieSceneInstance );

	virtual TArray< UMovieScene* > GetMovieScenesBeingEdited();

	/** Called by LevelEditor when the map changes */
	void OnMapChanged(class UWorld* NewWorld, EMapChangeType::Type MapChangeType);

	/** Called when an actor is dropped into Sequencer */
	void OnActorsDropped( const TArray<TWeakObjectPtr<AActor> >& Actors );

	/** Functions to push on to the transport controls we use */
	FReply OnPlay(bool bTogglePlay=true);
	FReply OnRecord();
	FReply OnStepForward();
	FReply OnStepBackward();
	FReply OnStepToEnd();
	FReply OnStepToBeginning();
	FReply OnToggleLooping();
	bool IsLooping() const;
	EPlaybackMode::Type GetPlaybackMode() const;

	/** @return The toolkit that this sequencer is hosted in (if any) */
	TSharedPtr<IToolkitHost> GetToolkitHost() const { return ToolkitHost.Pin(); }

	/** @return Whether or not this sequencer is used in the level editor */
	bool IsLevelEditorSequencer() const { return bIsEditingWithinLevelEditor; }

	/** Called to save the current movie scene */
	void SaveCurrentMovieScene();
protected:
	/**
	 * Reset data about a movie scene when pushing or popping a movie scene
	 */
	void ResetPerMovieSceneData();


	/**
	 * Destroys spawnables for all movie scenes in the stack
	 */
	void DestroySpawnablesForAllMovieScenes();

	/** Sets the actor CDO such that it is placed in front of the active perspective viewport camera, if we have one */
	static void PlaceActorInFrontOfCamera( AActor* ActorCDO );


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

	/** @return The current scrub position */
	virtual float OnGetScrubPosition() const { return ScrubPosition; }

	/** @return Whether or not we currently allow autokeying */
	virtual bool OnGetAllowAutoKey() const { return bAllowAutoKey; }

	/**
	 * Called when the view range is changed by the user
	 *
	 * @param	NewViewRange The new view range
	 */
	void OnViewRangeChanged( TRange<float> NewViewRange, EViewRangeInterpolation Interpolation = EViewRangeInterpolation::Animated );

	/**
	 * Calculates the amount of encroachment the specified time has into the autoscroll region, if any
	 */
	TOptional<float> CalculateAutoscrollEncroachment(float NewTime) const;

	/**
	 * Called when the scrub position is changed by the user
	 * This will stop any playback from happening
	 *
	 * @param NewScrubPosition	The new scrub position
	 */
	void OnScrubPositionChanged( float NewScrubPosition, bool bScrubbing );

	/** Called when the user has begun scrubbing */
	void OnBeginScrubbing();

	/** Called when the user has finished scrubbing */
	void OnEndScrubbing();

	/**
	 * Called when auto-key is toggled by a user
	 *
	 * @param bInAllowAutoKey	The new auto key state
	 */
	void OnToggleAutoKey();

	/** Called to toggle auto-scroll on and off */
	void OnToggleAutoScroll();
	bool GetAutoScrollEnabled() const { return bAutoScrollEnabled; }

	/** Called via UEditorEngine::GetActorRecordingStateEvent to check to see whether we need to record actor state */
	void GetActorRecordingState( bool& bIsRecording /* In+Out */ ) const;
	
	/** Called when a user executes the delete command to delete sections or keys */
	void DeleteSelectedItems();
	
	/** Transport controls */
	void TogglePlay();
	void PlayForward();
	void Rewind();
	void StepForward();
	void StepBackward();

	/** Expand or collapse selected nodes */
	void ToggleExpandCollapseNodes();

	/** Expand or collapse selected nodes and descendants*/
	void ToggleExpandCollapseNodesAndDescendants();

	/** Manually sets a key for the selected objects at the current time */
	void SetKey();

	/** Generates command bindings for UI commands */
	void BindSequencerCommands();

	// Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
	// End of FEditorUndoClient

	void OnSectionSelectionChanged();

	/** Called before the world is going to be saved. The sequencer puts everything back to its initial state. */
	void OnPreSaveWorld(uint32 SaveFlags, class UWorld* World);

	/** Called after the world has been saved. The sequencer updates to the animated state. */
	void OnPostSaveWorld(uint32 SaveFlags, class UWorld* World, bool bSuccess);

	/** Updates a viewport client from camera cut data */
	void UpdatePreviewLevelViewportClientFromCameraCut( FLevelEditorViewportClient& InViewportClient, UObject* InCameraObject, bool bNewCameraCut ) const;

private:
	TMap< TWeakObjectPtr<UMovieSceneSection>, TSharedRef<FMovieSceneInstance> > MovieSceneSectionToInstanceMap;

	/** Command list for sequencer commands */
	TSharedRef<FUICommandList> SequencerCommandBindings;

	/** List of tools we own */
	TArray< TSharedPtr<FMovieSceneTrackEditor> > TrackEditors;

	/** The editors we keep track of for special behaviors */
	TWeakPtr<FMovieSceneTrackEditor> ShotTrackEditor;
	TWeakPtr<FMovieSceneTrackEditor> AnimationTrackEditor;

	/** Manager for handling runtime object bindings */
	TSharedPtr< ISequencerObjectBindingManager > ObjectBindingManager;

	/** Listener for object changes being made while this sequencer is open*/
	TSharedPtr< class ISequencerObjectChangeListener > ObjectChangeListener;

	/** The runtime instance for the root movie scene */
	TSharedPtr< class FMovieSceneInstance > RootMovieSceneInstance;

	/** Main sequencer widget */
	TSharedPtr< class SSequencer > SequencerWidget;
	
	/** The asset editor that created this Sequencer if any */
	TWeakPtr<IToolkitHost> ToolkitHost;

	/** A list of all shots that are acting as filters */
	TArray< TWeakObjectPtr<class UMovieSceneSection> > FilteringShots;

	/** A list of sections that are considered unfilterable */
	TArray< TWeakObjectPtr<class UMovieSceneSection> > UnfilterableSections;

	/** A list of object guids that will be visible, regardless of shot filters */
	TArray<FGuid> UnfilterableObjects;

	/** Stack of movie scenes.  The first element is always the root movie scene.  The last element is the focused movie scene */
	TArray< TSharedRef<FMovieSceneInstance> > MovieSceneStack;

	/** The time range target to be viewed */
	TRange<float> TargetViewRange;
	/** The last time range that was viewed */
	TRange<float> LastViewRange;
	/** The amount of autoscroll pan offset that is currently being applied */
	TOptional<float> AutoscrollOffset;
	/** Whether or not we are allowing autoscroll */
	bool bAutoScrollEnabled;

	/** Zoom smoothing curves */
	FCurveSequence ZoomAnimation;
	FCurveHandle ZoomCurve;
	/** Overlay fading curves */
	FCurveSequence OverlayAnimation;
	FCurveHandle OverlayCurve;

	/** Whether we are playing, recording, etc. */
	EMovieScenePlayerStatus::Type PlaybackState;

	/** The current scrub position */
	// @todo sequencer: Should use FTimespan or "double" for Time Cursor Position! (cascades)
	float ScrubPosition;

	/** Whether looping while playing is enabled for this sequencer */
	bool bLoopingEnabled;

	/** Whether or not we are allowing autokey */
	bool bAllowAutoKey;

	bool bPerspectiveViewportPossessionEnabled;

	/** True if this sequencer is being edited within the level editor */
	bool bIsEditingWithinLevelEditor;

	/** Stores a dirty bit for whether the sequencer tree (and other UI bits) may need to be refreshed.  We
	    do this simply to avoid refreshing the UI more than once per frame. (e.g. during live recording where
		the MovieScene data can change many times per frame.) */
	bool bNeedTreeRefresh;

	FSequencerSelection Selection;
};
