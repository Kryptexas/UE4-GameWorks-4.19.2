// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "Persona.h"
#include "GraphEditor.h"
#include "SNodePanel.h"
#include "SAnimTrackPanel.h"
#include "SCurveEditor.h"

DECLARE_DELEGATE_OneParam( FOnSelectionChanged, const FGraphPanelSelectionSet& )
DECLARE_DELEGATE( FOnUpdatePanel )
DECLARE_DELEGATE_RetVal( float, FOnGetScrubValue )

//////////////////////////////////////////////////////////////////////////
// SAnimCurvePanel

class SAnimCurvePanel: public SAnimTrackPanel
{
public:
	SLATE_BEGIN_ARGS( SAnimCurvePanel )
		: _Sequence()
		, _ViewInputMin()
		, _ViewInputMax()
		, _InputMin()
		, _InputMax()
		, _OnSetInputViewRange()
		, _OnGetScrubValue()
	{}

	/**
	 * AnimSequenceBase to be used for this panel
	 */
	SLATE_ARGUMENT( class UAnimSequenceBase*, Sequence)
	/**
	 * Right side of widget width (outside of curve)
	 */
	SLATE_ARGUMENT( float, WidgetWidth )
	/**
	 * Viewable Range control variables
	 */
	SLATE_ATTRIBUTE( float, ViewInputMin )
	SLATE_ATTRIBUTE( float, ViewInputMax )
	SLATE_ATTRIBUTE( float, InputMin )
	SLATE_ATTRIBUTE( float, InputMax )
	SLATE_EVENT( FOnSetInputViewRange, OnSetInputViewRange )
	/**
	 * Get current value
	 */
	SLATE_EVENT( FOnGetScrubValue, OnGetScrubValue )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	/**
	 * Set New Sequence
	 */
	void SetSequence(class UAnimSequenceBase *	InSequence);

	/**
	 * Create New Curve Track
	 */
	FReply CreateNewTrack();

	/**
	 * Add Track
	 */
	void AddTrack(const FText & CurveNameToAdd, ETextCommit::Type CommitInfo);
	
	/**
	 * Delete Track
	 *
	 */
	FReply DeleteTrack(const FString & CurveNameToDelete);

	/**
	 * Build and display curve track context menu for all tracks.
	 *
	 */
	FReply OnContextMenu();

	/**
	 * Toggle all the curve modes of all the tracks
	 */
	void ToggleAllCurveModes(ESlateCheckBoxState::Type NewState, EAnimCurveFlags ModeToSet);

	/**
	 * Are all the curve modes of all the tracks set to the mode
	 */
	ESlateCheckBoxState::Type AreAllCurvesOfMode(EAnimCurveFlags ModeToTest) const;

	/**
	 * Visibility of the set-all-tracks button
	 */
	EVisibility IsSetAllTracksButtonVisible() const;

private:
	TSharedPtr<SSplitter> PanelSlot;
	class UAnimSequenceBase* Sequence;
	TAttribute<float> CurrentPosition;
	FOnGetScrubValue OnGetScrubValue;
	TMap<FString, TWeakPtr<class SCurveEdTrack> > Tracks;

	/**
	 * This is to control visibility of the curves, so you can edit or not
	 * Get Widget that shows all curve list and edit
	 */
	TSharedRef<SWidget>		GenerateCurveList();
	/**
	 * Returns true if this curve is editable
	 */
	ESlateCheckBoxState::Type	IsCurveEditable(FName CurveName) const;
	/**
	 * Toggle curve visibility
	 */
	void		ToggleEditability(ESlateCheckBoxState::Type NewType, FName CurveName);
	/**
	 * Refresh Panel
	 */
	FReply		RefreshPanel();
	/**
	 *	Show All Curves
	 */
	FReply		ShowAll(bool bShow);

private:
	/**
	 * Update Panel 
	 * @todo this has to be more efficient. Right now it does refresh whole things if it has to
	 */
	void UpdatePanel(bool bClearAll=false);
};