// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "Persona.h"
#include "GraphEditor.h"
#include "SNodePanel.h"
#include "STrack.h"
#include "SCurveEditor.h"
#include "SAnimTrackPanel.h"

DECLARE_DELEGATE( FOnMontageLengthChange )
DECLARE_DELEGATE( FOnMontagePropertyChanged )


//////////////////////////////////////////////////////////////////////////
// SAnimMontagePanel
//	This is the main montage editing widget that is responsible for setting up
//	a set of generic widgets (STrack and STrackNodes) for editing an anim montage.
//
//	SAnimMontagePanel will usually not edit the montage directly but just setup the 
//  callbacks to SMontageEditor.
//
//////////////////////////////////////////////////////////////////////////

class SAnimMontagePanel : public SAnimTrackPanel
{
public:
	SLATE_BEGIN_ARGS( SAnimMontagePanel )
		: _Montage()
		, _Persona()
		, _CurrentPosition()
		, _ViewInputMin()
		, _ViewInputMax()
		, _InputMin()
		, _InputMax()
		, _OnSetInputViewRange()
		, _OnGetScrubValue()
		, _MontageEditor()
		, _OnMontageChange()
	{}

	SLATE_ARGUMENT( class UAnimMontage*, Montage)
	SLATE_ARGUMENT( TWeakPtr<FPersona>, Persona)
	SLATE_ARGUMENT( TWeakPtr<class SMontageEditor>, MontageEditor)
	SLATE_ARGUMENT( float, WidgetWidth )
	SLATE_ATTRIBUTE( float, CurrentPosition )
	SLATE_ATTRIBUTE( float, ViewInputMin )
	SLATE_ATTRIBUTE( float, ViewInputMax )
	SLATE_ATTRIBUTE( float, InputMin )
	SLATE_ATTRIBUTE( float, InputMax )
	SLATE_EVENT( FOnSetInputViewRange, OnSetInputViewRange )
	SLATE_EVENT( FOnGetScrubValue, OnGetScrubValue )
	SLATE_EVENT( FOnAnimObjectChange, OnMontageChange )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetMontage(class UAnimMontage * InMontage);

	// SWidget interface
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;

	void Update();

	// Functions used as delegates that build the context menus for the montage tracks
	void SummonTrackContextMenu( FMenuBuilder& MenuBuilder, float DataPosX, int32 SectionIndex, int32 AnimSlotIndex );
	void SummonSectionContextMenu( FMenuBuilder& MenuBuilder, int32 SectionIndex );
	void SummonBranchNodeContextMenu( FMenuBuilder& MenuBuilder, int32 BranchIndex );

	void OnNewSlotClicked();
	void OnNewSectionClicked(float DataPosX);
	void OnNewBranchClicked(float DataPosX);

	void OnBranchPointClicked( int32 BranchPointIndex );

	void ShowSegmentInDetailsView(int32 AnimSegmentIndex, int32 AnimSlotIndex);
	void ShowBranchPointInDetailsView(int32 BranchPointIndex);
	void ShowSectionInDetailsView(int32 SectionIndex);

	void OnSlotNodeNameChangeCommit( const FText& NewText, ETextCommit::Type CommitInfo, int32 SlodeNodeIndex );

	void ClearSelected();

private:
	TWeakPtr<FPersona> Persona;
	TWeakPtr<SMontageEditor>	MontageEditor;
	TSharedPtr<SBorder> PanelArea;
	class UAnimMontage* Montage;
	TAttribute<float> CurrentPosition;

	TArray<TSharedPtr<class SMontageEdTrack>>	TrackList;

	FString LastContextHeading;

	STrackNodeSelectionSet SelectionSet;
	
	void CreateNewSlot(const FText& NewSlotName, ETextCommit::Type CommitInfo);
	void CreateNewSection(const FText& NewSectionName, ETextCommit::Type CommitInfo, float StartTime);
	void CreateNewBranch(const FText& NewBranchName, ETextCommit::Type CommitInfo, float StartTime);
	void RenameBranchPoint(const FText &NewName, ETextCommit::Type CommitInfo, int32 BranchPointIndex);
};