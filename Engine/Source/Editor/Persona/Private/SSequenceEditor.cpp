// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"

#include "SSequenceEditor.h"
#include "GraphEditor.h"
#include "GraphEditorModule.h"
#include "Editor/Kismet/Public/SKismetInspector.h"
#include "SAnimationScrubPanel.h"
#include "SAnimNotifyPanel.h"
#include "SAnimCurvePanel.h"
#include "SAnimTrackCurvePanel.h"

#define LOCTEXT_NAMESPACE "AnimSequenceEditor"

//////////////////////////////////////////////////////////////////////////
// SSequenceEditor

void SSequenceEditor::Construct(const FArguments& InArgs, TSharedRef<class IPersonaPreviewScene> InPreviewScene, TSharedRef<class IEditableSkeleton> InEditableSkeleton, FSimpleMulticastDelegate& OnPostUndo, FSimpleMulticastDelegate& OnCurvesChanged)
{
	SequenceObj = InArgs._Sequence;
	check(SequenceObj);
	PreviewScenePtr = InPreviewScene;

	SAnimEditorBase::Construct( SAnimEditorBase::FArguments()
		.OnObjectsSelected(InArgs._OnObjectsSelected), 
		InPreviewScene );

	OnPostUndo.Add(FPersona::FOnPostUndo::CreateSP( this, &SSequenceEditor::PostUndo ) );
	OnCurvesChanged.Add(FPersona::FOnTrackCurvesChanged::CreateSP( this, &SSequenceEditor::HandleCurvesChanged) );
	
	EditorPanels->AddSlot()
	.AutoHeight()
	.Padding(0, 10)
	[
		SAssignNew( AnimNotifyPanel, SAnimNotifyPanel, OnPostUndo)
		.Sequence(SequenceObj)
		.WidgetWidth(S2ColumnWidget::DEFAULT_RIGHT_COLUMN_WIDTH)
		.ViewInputMin(this, &SAnimEditorBase::GetViewMinInput)
		.ViewInputMax(this, &SAnimEditorBase::GetViewMaxInput)
		.InputMin(this, &SAnimEditorBase::GetMinInput)
		.InputMax(this, &SAnimEditorBase::GetMaxInput)
		.OnSetInputViewRange(this, &SAnimEditorBase::SetInputViewRange)
		.OnGetScrubValue(this, &SAnimEditorBase::GetScrubValue)
		.OnSelectionChanged(this, &SAnimEditorBase::OnSelectionChanged)
		.OnAnimNotifiesChanged(InArgs._OnAnimNotifiesChanged)
		.OnInvokeTab(InArgs._OnInvokeTab)
	];

	EditorPanels->AddSlot()
	.AutoHeight()
	.Padding(0, 10)
	[
		SAssignNew( AnimCurvePanel, SAnimCurvePanel, InEditableSkeleton )
		.Sequence(SequenceObj)
		.WidgetWidth(S2ColumnWidget::DEFAULT_RIGHT_COLUMN_WIDTH)
		.ViewInputMin(this, &SAnimEditorBase::GetViewMinInput)
		.ViewInputMax(this, &SAnimEditorBase::GetViewMaxInput)
		.InputMin(this, &SAnimEditorBase::GetMinInput)
		.InputMax(this, &SAnimEditorBase::GetMaxInput)
		.OnSetInputViewRange(this, &SAnimEditorBase::SetInputViewRange)
		.OnGetScrubValue(this, &SAnimEditorBase::GetScrubValue)
	];

	UAnimSequence * AnimSeq = Cast<UAnimSequence>(SequenceObj);
	if (AnimSeq)
	{
		EditorPanels->AddSlot()
		.AutoHeight()
		.Padding(0, 10)
		[
			SAssignNew(AnimTrackCurvePanel, SAnimTrackCurvePanel)
			.Sequence(AnimSeq)
			.WidgetWidth(S2ColumnWidget::DEFAULT_RIGHT_COLUMN_WIDTH)
			.ViewInputMin(this, &SAnimEditorBase::GetViewMinInput)
			.ViewInputMax(this, &SAnimEditorBase::GetViewMaxInput)
			.InputMin(this, &SAnimEditorBase::GetMinInput)
			.InputMax(this, &SAnimEditorBase::GetMaxInput)
			.OnSetInputViewRange(this, &SAnimEditorBase::SetInputViewRange)
			.OnGetScrubValue(this, &SAnimEditorBase::GetScrubValue)
		];
	}
}

void SSequenceEditor::PostUndo()
{
	GetPreviewScene()->SetPreviewAnimationAsset(SequenceObj);

	if( SequenceObj )
	{
		SetInputViewRange(0, SequenceObj->SequenceLength);

		AnimNotifyPanel->Update();
		AnimCurvePanel->UpdatePanel();
		if (AnimTrackCurvePanel.IsValid())
		{
			AnimTrackCurvePanel->UpdatePanel();
		}
	}
}

void SSequenceEditor::HandleCurvesChanged()
{
	if (AnimTrackCurvePanel.IsValid())
	{
		AnimTrackCurvePanel->UpdatePanel();
	}
}

#undef LOCTEXT_NAMESPACE
