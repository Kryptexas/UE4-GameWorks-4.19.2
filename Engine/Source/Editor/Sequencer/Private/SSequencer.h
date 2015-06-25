// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace SequencerLayoutConstants
{
	/** The amount to indent child nodes of the layout tree */
	const float IndentAmount = 25.0f;
	/** Padding between each node */
	const float NodePadding = 3.0f;
	/** Height of each object node */
	const float ObjectNodeHeight = 20.0f;
	/** Height of each section area if there are no sections (note: section areas may be larger than this if they have children. This is the height of a section area with no children or all children hidden) */
	const float SectionAreaDefaultHeight = 15.0f;
	/** Height of each key area */
	const float KeyAreaHeight = 15.0f;
	/** Height of each category node */
	const float CategoryNodeHeight = 15.0f;
}

/**
 * The kind of breadcrumbs that sequencer uses
 */
struct FSequencerBreadcrumb
{
	enum Type
	{
		ShotType,
		MovieSceneType,
	};

	/** The type of breadcrumb this is */
	FSequencerBreadcrumb::Type BreadcrumbType;
	/** The movie scene this may point to */
	class TWeakPtr<FMovieSceneInstance> MovieSceneInstance;

	FSequencerBreadcrumb( TSharedRef<class FMovieSceneInstance> InMovieSceneInstance )
		: BreadcrumbType(FSequencerBreadcrumb::MovieSceneType)
		, MovieSceneInstance(InMovieSceneInstance) {}

	FSequencerBreadcrumb()
		: BreadcrumbType(FSequencerBreadcrumb::ShotType) {}
};

/**
 * Main sequencer UI widget
 */
class SSequencer : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam( FOnToggleBoolOption, bool )
	SLATE_BEGIN_ARGS( SSequencer )
		: _ScrubPosition( 1.0f )
	{}
		/** The current view range (seconds) */
		SLATE_ATTRIBUTE( FAnimatedRange, ViewRange )
		/** The current scrub position in (seconds) */
		SLATE_ATTRIBUTE( float, ScrubPosition )
		/** Called when the user changes the view range */
		SLATE_EVENT( FOnViewRangeChanged, OnViewRangeChanged )
		/** Called when the user has begun scrubbing */
		SLATE_EVENT( FSimpleDelegate, OnBeginScrubbing )
		/** Called when the user has finished scrubbing */
		SLATE_EVENT( FSimpleDelegate, OnEndScrubbing )
		/** Called when the user changes the scrub position */
		SLATE_EVENT( FOnScrubPositionChanged, OnScrubPositionChanged )
		/** Called to populate the add combo button in the toolbar. */
		SLATE_EVENT( FOnGetAddMenuContent, OnGetAddMenuContent )
	SLATE_END_ARGS()


	void Construct( const FArguments& InArgs, TSharedRef< class FSequencer > InSequencer );

	~SSequencer();

	virtual bool SupportsKeyboardFocus() const override { return true; }

	/** Updates the layout node tree from movie scene data */
	void UpdateLayoutTree();

	/** Causes the widget to register an empty active timer that persists until Sequencer playback stops */
	void RegisterActiveTimerForPlayback();

	/**
	 * Updates the breadcrumbs from a change in the shot filter state
	 *
	 * @param FilteringShots A list of shots that are now filtering, or none if filtering is off
	 */
	void UpdateBreadcrumbs(const TArray< TWeakObjectPtr<class UMovieSceneSection> >& FilteringShots);
	void ResetBreadcrumbs();

	/** Deletes selected nodes out of the sequencer node tree */
	void DeleteSelectedNodes();

	/** Expand or collapse selected nodes out of the sequencer node tree */
	void ToggleExpandCollapseSelectedNodes(bool bDescendants = false);
	void ExpandCollapseNode(TSharedRef<FSequencerDisplayNode> SelectedNode, bool bDescendants, bool bExpand);

	/** Whether the user is selecting. Ignore selection changes from the level when the user is selecting. */
	void SetUserIsSelecting(bool bUserIsSelectingIn) { bUserIsSelecting = bUserIsSelectingIn; }
	bool UserIsSelecting() { return bUserIsSelecting; }

	/** Called when the save button is clicked */
	FReply OnSaveMovieSceneClicked();
private:
	/** Empty active timer to ensure Slate ticks during Sequencer playback */
	EActiveTimerReturnType EnsureSlateTickDuringPlayback(double InCurrentTime, float InDeltaTime);	

	/** Makes the toolbar. */
	TSharedRef<SWidget> MakeToolBar();

	/** Makes the add menu for the toolbar. */
	TSharedRef<SWidget> MakeAddMenu();

	/** Makes the snapping menu for the toolbar. */
	TSharedRef<SWidget> MakeSnapMenu();

	/** Makes the curve editor menu for the toolbar. */
	TSharedRef<SWidget> MakeCurveEditorMenu();

	/** Makes and configures a set of the standard UE transport controls. */
	TSharedRef<SWidget> MakeTransportControls();

	/**
	* @return The value of the current time snap interval.
	*/
	float OnGetTimeSnapInterval() const;

	/**
	* Called when the time snap interval changes.
	*
	* @param InInterval	The new time snap interval
	*/
	void OnTimeSnapIntervalChanged(float InInterval);

	/**
	* @return The value of the current value snap interval.
	*/
	float OnGetValueSnapInterval() const;

	/**
	* Called when the value snap interval changes.
	*
	* @param InInterval	The new value snap interval
	*/
	void OnValueSnapIntervalChanged( float InInterval );

	/**
	 * Called when the outliner search terms change                                                              
	 */
	void OnOutlinerSearchChanged( const FText& Filter );

	/**
	 * @return The fill percentage of the animation outliner
	 */
	float GetColumnFillCoefficient(int32 ColumnIndex) const { return ColumnFillCoefficients[ColumnIndex]; }

	/** Get the amount of space that the outliner spacer should fill */
	float GetOutlinerSpacerFill() const;

	/** Get the visibility of the curve area */
	EVisibility GetCurveEditorVisibility() const;

	/** Get the visibility of the track area */
	EVisibility GetTrackAreaVisibility() const;

	/** SWidget interface */
	/** @todo Sequencer Basic drag and drop support.  Doesn't belong here most likely */
	virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) override;
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override;

	/**
	 * Called when one or more assets are dropped into the widget
	 *
	 * @param	DragDropOp	Information about the asset(s) that were dropped
	 */
	void OnAssetsDropped( const class FAssetDragDropOp& DragDropOp );
	
	/**
	 * Called when one or more classes are dropped into the widget
	 *
	 * @param	DragDropOp	Information about the class(es) that were dropped
	 */
	void OnClassesDropped( const class FClassDragDropOp& DragDropOp );
	
	/**
	 * Called when one or more unloaded classes are dropped into the widget
	 *
	 * @param	DragDropOp	Information about the unloaded class(es) that were dropped
	 */
	void OnUnloadedClassesDropped( const class FUnloadedClassDragDropOp& DragDropOp );
	
	/**
	 * Called when one or more actors are dropped into the widget
	 *
	 * @param	DragDropOp	Information about the actor(s) that was dropped
	 */
	void OnActorsDropped( class FActorDragDropGraphEdOp& DragDropOp ); 
	
	/**
	* Delegate used when actor selection changes in the level
	*
	*/	
	void OnActorSelectionChanged( UObject* obj );

	/** Called when a breadcrumb is clicked on in the sequencer */
	void OnCrumbClicked(const FSequencerBreadcrumb& Item);

	/** Gets the root movie scene name */
	FText GetRootMovieSceneName() const;

	/** Gets the title of the passed in shot section */
	FText GetShotSectionTitle(UMovieSceneSection* ShotSection) const;

	/** Gets whether or not the breadcrumb trail should be visible. */
	EVisibility GetBreadcrumbTrailVisibility() const;

	/** Gets whether or not the curve editor toolbar should be visibe. */
	EVisibility GetCurveEditorToolBarVisibility() const;

	/** Called when a column fill percentage is changed by a splitter slot. */
	void OnColumnFillCoefficientChanged(float FillCoefficient, int32 ColumnIndex);

	/** Called when the curve editor is shown or hidden */
	void OnCurveEditorVisibilityChanged();

private:

	/** Section area widget */
	TSharedPtr<class SSequencerTrackArea> TrackArea;
	/** Outliner widget */
	TSharedPtr<class SSequencerTrackOutliner> TrackOutliner;
	/** The curve editor. */
	TSharedPtr<class SSequencerCurveEditor> CurveEditor;
	/** Sequencer node tree for movie scene data */
	TSharedPtr<class FSequencerNodeTree> SequencerNodeTree;
	/** The breadcrumb trail widget for this sequencer */
	TSharedPtr< class SBreadcrumbTrail<FSequencerBreadcrumb> > BreadcrumbTrail;
	/** The main sequencer interface */
	TWeakPtr<FSequencer> Sequencer;
	/** The fill coefficients of each column in the grid. */
	float ColumnFillCoefficients[2];
	/** Whether the active timer is currently registered */
	bool bIsActiveTimerRegistered;
	/** Whether the user is selecting. Ignore selection changes from the level when the user is selecting. */
	bool bUserIsSelecting;

	FOnGetAddMenuContent OnGetAddMenuContent;
};
