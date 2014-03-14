// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "Persona.h"
#include "GraphEditor.h"
#include "SNodePanel.h"
#include "SCurveEditor.h"
#include "SAnimTrackPanel.h"
#include "SAnimEditorBase.h"

DECLARE_DELEGATE_OneParam( FOnSelectionChanged, const FGraphPanelSelectionSet& )
DECLARE_DELEGATE( FOnUpdatePanel )
DECLARE_DELEGATE_RetVal( float, FOnGetScrubValue )
DECLARE_DELEGATE( FRefreshOffsetsRequest )
DECLARE_DELEGATE( FDeleteNotify )

class SAnimNotifyNode;
class SAnimNotifyTrack;
class FNotifyDragDropOp;

//////////////////////////////////////////////////////////////////////////
// SAnimNotifyPanel

// @todo anim : register when it's opened for the animsequence
// broadcast when animsequence changed, so that we refresh for multiple window
class SAnimNotifyPanel: public SAnimTrackPanel
{
public:
	SLATE_BEGIN_ARGS( SAnimNotifyPanel )
		: _Persona()
		, _Sequence()
		, _CurrentPosition()
		, _ViewInputMin()
		, _ViewInputMax()
		, _InputMin()
		, _InputMax()
		, _OnSetInputViewRange()
		, _OnSelectionChanged()
		, _OnGetScrubValue()
		, _OnRequestRefreshOffsets()
	{}

	SLATE_ARGUMENT( TSharedPtr<FPersona>,	Persona )
	SLATE_ARGUMENT( class UAnimSequenceBase*, Sequence)
	SLATE_ARGUMENT( float, WidgetWidth )
	SLATE_ATTRIBUTE( float, CurrentPosition )
	SLATE_ATTRIBUTE( float, ViewInputMin )
	SLATE_ATTRIBUTE( float, ViewInputMax )
	SLATE_ATTRIBUTE( float, InputMin )
	SLATE_ATTRIBUTE( float, InputMax )
	SLATE_ATTRIBUTE( TArray<FTrackMarkerBar>, MarkerBars )
	SLATE_EVENT( FOnSetInputViewRange, OnSetInputViewRange )
	SLATE_EVENT( FOnSelectionChanged, OnSelectionChanged )
	SLATE_EVENT( FOnGetScrubValue, OnGetScrubValue )
	SLATE_EVENT( FRefreshOffsetsRequest, OnRequestRefreshOffsets)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SAnimNotifyPanel();

	void SetSequence(class UAnimSequenceBase *	InSequence);

	FReply InsertTrack(int32 TrackIndexToInsert);
	FReply DeleteTrack(int32 TrackIndexToDelete);
	bool CanDeleteTrack(int32 TrackIndexToDelete);
	void DeleteNotify(FAnimNotifyEvent* Notify);
	void Update();
	TWeakPtr<FPersona> GetPersona() const { return PersonaPtr; }

	/** Returns the position of the notify node currently being dragged. Returns -1 if no node is being dragged */
	float CalculateDraggedNodePos() const;

	/**Handler for when a notify node drag has been initiated */
	FReply OnNotifyNodeDragStarted(TSharedRef<SAnimNotifyNode> NotifyNode, const FVector2D& ScreenCursorPos, const FVector2D& ScreenNodePosition, const bool bDragOnMarker);

	virtual float GetSequenceLength() const OVERRIDE {return Sequence->SequenceLength;}

private:
	TSharedPtr<SBorder> PanelArea;
	class UAnimSequenceBase* Sequence;
	float WidgetWidth;
	TAttribute<float> CurrentPosition;
	FOnSelectionChanged OnSelectionChanged;
	FOnGetScrubValue OnGetScrubValue;

	/** Delegate to request a refresh of the offsets calculated for notifies */
	FRefreshOffsetsRequest OnRequestRefreshOffsets;

	/** Store the position of a currently dragged node for display across tracks */
	float CurrentDragXPosition;

	/** Cached list of anim tracks for notify node drag drop */
	TArray<TSharedPtr<SAnimNotifyTrack>> NotifyAnimTracks;

	// this just refresh notify tracks - UI purpose only
	// do not call this from here. This gets called by asset. 
	void RefreshNotifyTracks();
	void PostUndo();

	/** Handler for generic editor delete command */
	void OnGenericDelete();

	/** We support keyboard focus to detect when we should process key commands like delete */
	virtual bool SupportsKeyboardFocus() const OVERRIDE
	{
		return true;
	}

	/** Persona reference **/
	TWeakPtr<FPersona> PersonaPtr;

	/** Attribute for accessing any section/branching point positions we have to draw */
	TAttribute<TArray<FTrackMarkerBar>>	MarkerBars;
};