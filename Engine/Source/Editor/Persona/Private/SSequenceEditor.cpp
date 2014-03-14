// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"

#include "SSequenceEditor.h"
#include "GraphEditor.h"
#include "GraphEditorModule.h"
#include "Editor/Kismet/Public/SKismetInspector.h"
#include "SAnimationScrubPanel.h"
#include "SAnimNotifyPanel.h"
#include "SAnimCurvePanel.h"

#define LOCTEXT_NAMESPACE "AnimSequenceEditor"

//////////////////////////////////////////////////////////////////////////
// SSequenceEditor

void SSequenceEditor::Construct(const FArguments& InArgs)
{
	SequenceObj = InArgs._Sequence;
	check(SequenceObj);

	SAnimEditorBase::Construct( SAnimEditorBase::FArguments()
		.Persona(InArgs._Persona)
		);

	PersonaPtr.Pin()->RegisterOnPostUndo(FPersona::FOnPostUndo::CreateSP( this, &SSequenceEditor::PostUndo ) );

	EditorPanels->AddSlot()
	.AutoHeight()
	.Padding(0, 10)
	[
		SAssignNew( AnimNotifyPanel, SAnimNotifyPanel )
		.Persona(InArgs._Persona)
		.Sequence(SequenceObj)
		.WidgetWidth(S2ColumnWidget::DEFAULT_RIGHT_COLUMN_WIDTH)
		.ViewInputMin(this, &SAnimEditorBase::GetViewMinInput)
		.ViewInputMax(this, &SAnimEditorBase::GetViewMaxInput)
		.InputMin(this, &SAnimEditorBase::GetMinInput)
		.InputMax(this, &SAnimEditorBase::GetMaxInput)
		.OnSetInputViewRange(this, &SAnimEditorBase::SetInputViewRange)
		.OnGetScrubValue(this, &SAnimEditorBase::GetScrubValue)
		.OnSelectionChanged(this, &SAnimEditorBase::OnSelectionChanged)
	];

	EditorPanels->AddSlot()
	.AutoHeight()
	.Padding(0, 10)
	[
		SAssignNew( AnimCurvePanel, SAnimCurvePanel )
		.Sequence(SequenceObj)
		.WidgetWidth(S2ColumnWidget::DEFAULT_RIGHT_COLUMN_WIDTH)
		.ViewInputMin(this, &SAnimEditorBase::GetViewMinInput)
		.ViewInputMax(this, &SAnimEditorBase::GetViewMaxInput)
		.InputMin(this, &SAnimEditorBase::GetMinInput)
		.InputMax(this, &SAnimEditorBase::GetMaxInput)
		.OnSetInputViewRange(this, &SAnimEditorBase::SetInputViewRange)
		.OnGetScrubValue(this, &SAnimEditorBase::GetScrubValue)
	];
}

SSequenceEditor::~SSequenceEditor()
{
	if (PersonaPtr.IsValid())
	{
		PersonaPtr.Pin()->UnregisterOnPostUndo(this);
	}
}

void SSequenceEditor::SetSequenceObj(UAnimSequenceBase * NewSequence)
{
	SequenceObj = NewSequence;

	if( SequenceObj )
	{
		SetInputViewRange(0, SequenceObj->SequenceLength);
	}

	AnimNotifyPanel->SetSequence(NewSequence);
	AnimCurvePanel->SetSequence(NewSequence);
	// sequence editor locks the sequence, so it doesn't get replaced by clicking 
	AnimScrubPanel->ReplaceLockedSequence(NewSequence);
}

void SSequenceEditor::PostUndo()
{
	PersonaPtr.Pin()->SetPreviewAnimationAsset(SequenceObj);
	// need to refresh panel
	if( SequenceObj )
	{
		SetInputViewRange(0, SequenceObj->SequenceLength);

		// update notify panel 
		AnimNotifyPanel->Update();
	}
}

#undef LOCTEXT_NAMESPACE
