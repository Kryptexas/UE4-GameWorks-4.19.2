// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintEditor.h"


//////////////////////////////////////////////////////////////////////////
// SCurveAssetWidget
class SCurveAssetWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SCurveAssetWidget ){}
	SLATE_ATTRIBUTE( FString, VisibleText )
		SLATE_EVENT( FSimpleDelegate, OnCreateCurve )
		SLATE_EVENT( FOnTextChanged, OnTextChanged )
	SLATE_END_ARGS()


	void Construct(const FArguments& InArgs);

private:

	/**	Reference to editable text */
	TAttribute< FString > VisibleText;

	/**	Callback function pointer to execute curve asset creation command */
	FSimpleDelegate OnCreateCurve;

	/** Callback function pointer to notify editable text changes */
	FOnTextChanged OnTextChanged;

	/** Widget that we want to be focused when the popup is shown  */
	TSharedPtr<SWidget> WidgetWithDefaultFocus;

	/** Function to execute curve asset creation command */
	FReply OnCreateButtonClicked( );


	/** Function to notify editable text changes */
	void OnEditableTextChanged( const FText& NewString );
};


//////////////////////////////////////////////////////////////////////////
// FTimelineEdTrack

/** Represents a single track on the timeline */
class FTimelineEdTrack
{
public:
	/** Enum to indicate whether this is an event track, a float interp track or a vector interp track */
	enum ETrackType
	{
		TT_Event,
		TT_FloatInterp,
		TT_VectorInterp,
		TT_LinearColorInterp,
	};

public:
	/** The type of track this is */
	ETrackType TrackType;
	/** The index of this track within its type's array */
	int32 TrackIndex;
	/** Trigger when a rename is requested on the track */
	FSimpleDelegate OnRenameRequest;
public:
	static TSharedRef<FTimelineEdTrack> Make(ETrackType InType, int32 InIndex)
	{
		return MakeShareable(new FTimelineEdTrack(InType, InIndex));
	}

private:
	/** Hidden constructor, always use Make above */
	FTimelineEdTrack(ETrackType InType, int32 InIndex)
		: TrackType(InType)
		, TrackIndex(InIndex)
	{}

	/** Hidden constructor, always use Make above */
	FTimelineEdTrack() {}
};

//////////////////////////////////////////////////////////////////////////
// STimelineEdTrack

/** Widget for drawing a single track */
class STimelineEdTrack : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( STimelineEdTrack ){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FTimelineEdTrack> InTrack, TSharedPtr<class STimelineEditor> InTimelineEd);
private:
	/** Pointer to the underlying track information */
	TSharedPtr<FTimelineEdTrack> Track;

	/** Pointer back to the timeline editor widget */
	TWeakPtr<class STimelineEditor> TimelineEdPtr;

	/** Pointer to track widget for drawing keys */
	TSharedPtr<class SCurveEditor> TrackWidget;

	/**	Pointer to a window which prompts user to save internal curve as an external asset */
	TSharedPtr<SWindow> AssetCreationWindow;

	/**	File path to be used to save the external curve asset */
	FString SaveCurveAssetPath;

	/**	Pointer to the curve */
	UCurveBase* CurveBasePtr;

	/** String to display external curve name in the text box*/
	FString ExternalCurveName;

	/** String to display external curve path as tooltip*/
	FString ExternalCurvePath;

	/**	Get curve asset path used to save the asset*/
	FString GetSaveCurvePath() const;

	/** Set curve asset path used to save the asset*/
	void SetSaveCurvePath(const FText& NewStr);

	/**Function to destroy popup window*/
	void OnCloseCreateCurveWindow();

	/** Callback function to confirm button press in order to create curve asset */
	void OnCreateButtonClicked();

	/** Function to create curve asset inside the user specified package*/
	UCurveBase* CreateCurveAsset();

	/** Callback function to initiate the external curve asset creation process*/
	void OnCreateExternalCurve();

	/** Function to create dialog which prompts to create external curve*/
	TSharedRef<SWidget> CreateCurveAssetWidget();

	/** Initialize default asset path*/
	void InitCurveAssetPath();

	/** Get the current external curve name*/
	FText GetExternalCurveName( ) const;

	/** Get the current external curve path*/
	FString GetExternalCurvePath( ) const;

	/** Function to replace internal curve with an external curve*/
	void SwitchToExternalCurve(UCurveBase* AssetCurvePtr);

	/** Function to update track with external curve reference*/
	void UseExternalCurve( UObject* AssetObj );

	/** Function to convert external curve to an internal curve*/
	void UseInternalCurve( );

	/** Callback function to replace internal curve with an external curve*/
	FReply OnClickUse();

	/** Callback function to replace external curve with an internal curve*/
	FReply OnClickClear();

	/** Callback function to locate external curve asset in content browser */
	FReply OnClickBrowse();

	/**Function to reset external curve info*/
	void ResetExternalCurveInfo( );

	/** Function to copy data from one curve to another*/
	static void CopyCurveData( const FRichCurve* SrcCurve, FRichCurve* DestCurve );

public:
	/** Inline block for changing name of track */
	TSharedPtr<SInlineEditableTextBlock> InlineNameBlock;
};

//////////////////////////////////////////////////////////////////////////
// STimelineEditor

/** Type used for list widget of tracks */
typedef SListView< TSharedPtr<FTimelineEdTrack> > STimelineEdTrackListType;

/** Overall timeline editing widget */
class STimelineEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( STimelineEditor ){}
	SLATE_END_ARGS()

private:
	/** List widget used for showing tracks */
	TSharedPtr<STimelineEdTrackListType> TrackListView;

	/** Underlying array of tracks, used by TrackListView */
	TArray< TSharedPtr<FTimelineEdTrack> > TrackList;

	/** Pointer back to owning Kismet 2 tool */
	TWeakPtr<FBlueprintEditor> Kismet2Ptr;

	/** Text box for editing length of timeline */
	TSharedPtr<SEditableTextBox> TimelineLengthEdit;

	/** If we want the timeline to loop */
	TSharedPtr<SCheckBox> LoopCheckBox;

	/** If we want the timeline to replicate */
	TSharedPtr<SCheckBox> ReplicatedCheckBox;

	/** If we want the timeline to auto-play */
	TSharedPtr<SCheckBox> PlayCheckBox;

	/** If we want the timeline to play to the full specified length, or just to the last keyframe of its curves */
	TSharedPtr<SCheckBox> UseLastKeyframeCheckBox;

	/** Pointer to the timeline object we are editing */
	UTimelineTemplate* TimelineObj;

	/** Minimum input shown for tracks */
	float ViewMinInput;

	/** Maximum input shown for tracks */
	float ViewMaxInput;

	/** The default name of the last track created, used to identify which track needs to be renamed. */
	FName NewTrackPendingRename;

	/** The commandlist for the Timeline editor. */
	TSharedPtr< FUICommandList > CommandList;

	/** The current desired size of the timeline */
	FVector2D TimelineDesiredSize;

	/** The nominal desired height of a single timeline track at 1.0x height */
	float NominalTimelineDesiredHeight;
public:
	/** Get the timeline object that we are currently editing */
	UTimelineTemplate* GetTimeline();

	/** Called when the timeline changes to get the editor to refresh its state */
	void OnTimelineChanged();

	void Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InKismet2, UTimelineTemplate* InTimelineObj);

	float GetViewMinInput() const;
	float GetViewMaxInput() const;

	/** Return length of timeline */
	float GetTimelineLength() const;

	void SetInputViewRange(float InViewMinInput, float InViewMaxInput);
	/** When user attempts to commit the name of a track*/
	bool OnVerifyTrackNameCommit(const FText& TrackName, FText& OutErrorMessage, FTTTrackBase* TrackBase, STimelineEdTrack* Track );
	/** When user commits the name of a track*/
	void OnTrackNameCommitted(const FText& Name, ETextCommit::Type CommitInfo, FTTTrackBase* TrackBase, STimelineEdTrack* Track );

	/**Create curve object based on curve type*/
	UCurveBase* CreateNewCurve( FTimelineEdTrack::ETrackType Type );

	/** Gets the desired size for timelines */
	FVector2D GetTimelineDesiredSize() const;

	// SWidget interface
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	// End of SWidget interface
private:
	/** Used by list view to create a track widget from the track item struct */
	TSharedRef<ITableRow> MakeTrackWidget( TSharedPtr<FTimelineEdTrack> Track, const TSharedRef<STableViewBase>& OwnerTable );
	/** Add a new track to the timeline */
	FReply CreateNewTrack( FTimelineEdTrack::ETrackType Type );

	/** Checks if the user can delete the selected tracks */
	bool CanDeleteSelectedTracks() const;
	/** Deletes the currently selected tracks */
	void OnDeleteSelectedTracks();

	/** Get the name of the timeline we are editing */
	FString GetTimelineName() const;

	/** Get state of autoplay box*/
	ESlateCheckBoxState::Type IsAutoPlayChecked() const;
	/** Handle play box being changed */
	void OnAutoPlayChanged(ESlateCheckBoxState::Type NewType);

	/** Get state of loop box */
	ESlateCheckBoxState::Type IsLoopChecked() const;
	/** Handle loop box being changed */
	void OnLoopChanged(ESlateCheckBoxState::Type NewType);

	/** Get state of replicated box */
	ESlateCheckBoxState::Type IsReplicatedChecked() const;
	/** Handle loop box being changed */
	void OnReplicatedChanged(ESlateCheckBoxState::Type NewType);

	/** Get state of the use last keyframe checkbox */
	ESlateCheckBoxState::Type IsUseLastKeyframeChecked() const;
	/** Handle toggling between use last keyframe and use length */
	void OnUseLastKeyframeChanged(ESlateCheckBoxState::Type NewType);

	/** Get current length of timeline as string */
	FText GetLengthString() const;
	/** Handle length string being changed */
	void OnLengthStringChanged(const FText& NewString, ETextCommit::Type CommitInfo);

	/** Function to check whether a curve asset is selected in content browser in order to enable "Add Curve Asset" button */
	bool IsCurveAssetSelected() const;

	/** Create new track from curve asset */
	FReply CreateNewTrackFromAsset();

	/** Callback when a track item is scrolled into view */
	void OnItemScrolledIntoView( TSharedPtr<FTimelineEdTrack> InTrackNode, const TSharedPtr<ITableRow>& InWidget );

	/** Checks if you can rename the selected track */
	bool CanRenameSelectedTrack() const;
	/** Informs the selected track that the user wants to rename it */
	void OnRequestTrackRename() const;

	/** Creates the right click context menu for the track list */
	TSharedPtr< SWidget > MakeContextMenu() const;

	void SetSizeScaleValue(float NewValue);
	float GetSizeScaleValue() const;
};

