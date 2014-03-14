// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"

#include "SMontageEditor.h"
#include "GraphEditor.h"
#include "GraphEditorModule.h"
#include "Editor/Kismet/Public/SKismetInspector.h"
#include "SAnimMontageScrubPanel.h"
#include "SAnimMontagePanel.h"
#include "SAnimMontageSectionsPanel.h"
#include "ScopedTransaction.h"
#include "SAnimNotifyPanel.h"
#include "SAnimCurvePanel.h"
#include "AnimPreviewInstance.h"

#define LOCTEXT_NAMESPACE "AnimSequenceEditor"

//////////////////////////////////////////////////////////////////////////
// SMontageEditor

void SMontageEditor::Construct(const FArguments& InArgs)
{
	MontageObj = InArgs._Montage;
	check(MontageObj);

	bDragging = false;
	bRebuildMontagePanel = false;

	SAnimEditorBase::Construct( SAnimEditorBase::FArguments()
		.Persona(InArgs._Persona)
		);

	PersonaPtr.Pin()->RegisterOnPostUndo(FPersona::FOnPostUndo::CreateSP( this, &SMontageEditor::PostUndo ) );	

	EditorPanels->AddSlot()
	.AutoHeight()
	.Padding(0, 10)
	[
		SAssignNew( AnimMontagePanel, SAnimMontagePanel )
		.Persona(PersonaPtr)
		.Montage(MontageObj)
		.MontageEditor(SharedThis(this))
		.WidgetWidth(S2ColumnWidget::DEFAULT_RIGHT_COLUMN_WIDTH)
		.ViewInputMin(this, &SAnimEditorBase::GetViewMinInput)
		.ViewInputMax(this, &SAnimEditorBase::GetViewMaxInput)
		.InputMin(this, &SAnimEditorBase::GetMinInput)
		.InputMax(this, &SAnimEditorBase::GetMaxInput)
		.OnSetInputViewRange(this, &SAnimEditorBase::SetInputViewRange)
	];

	EditorPanels->AddSlot()
	.AutoHeight()
	.Padding(0, 10)
	[
		SAssignNew( AnimMontageSectionsPanel, SAnimMontageSectionsPanel )
		.Montage(MontageObj)
		.MontageEditor(SharedThis(this))
	];

	EditorPanels->AddSlot()
	.AutoHeight()
	.Padding(0, 10)
	[
		SAssignNew( AnimNotifyPanel, SAnimNotifyPanel )
		.Persona(InArgs._Persona)
		.Sequence(MontageObj)
		.WidgetWidth(S2ColumnWidget::DEFAULT_RIGHT_COLUMN_WIDTH)
		.InputMin(this, &SAnimEditorBase::GetMinInput)
		.InputMax(this, &SAnimEditorBase::GetMaxInput)
		.ViewInputMin(this, &SAnimEditorBase::GetViewMinInput)
		.ViewInputMax(this, &SAnimEditorBase::GetViewMaxInput)
		.OnSetInputViewRange(this, &SAnimEditorBase::SetInputViewRange)
		.OnGetScrubValue(this, &SAnimEditorBase::GetScrubValue)
		.OnSelectionChanged(this, &SAnimEditorBase::OnSelectionChanged)
		.MarkerBars(this, &SMontageEditor::GetMarkerBarInformation)
		.OnRequestRefreshOffsets(this, &SMontageEditor::RefreshNotifyTriggerOffsets)
	];

	EditorPanels->AddSlot()
	.AutoHeight()
	.Padding(0, 10)
	[
		SAssignNew( AnimCurvePanel, SAnimCurvePanel )
		.Sequence(MontageObj)
		.WidgetWidth(S2ColumnWidget::DEFAULT_RIGHT_COLUMN_WIDTH)
		.ViewInputMin(this, &SAnimEditorBase::GetViewMinInput)
		.ViewInputMax(this, &SAnimEditorBase::GetViewMaxInput)
		.InputMin(this, &SAnimEditorBase::GetMinInput)
		.InputMax(this, &SAnimEditorBase::GetMaxInput)
		.OnSetInputViewRange(this, &SAnimEditorBase::SetInputViewRange)
		.OnGetScrubValue(this, &SAnimEditorBase::GetScrubValue)
	];

	if (MontageObj)
	{
		EnsureStartingSection();
		EnsureSlotNode();
	}

	CollapseMontage();
}

SMontageEditor::~SMontageEditor()
{
	if (PersonaPtr.IsValid())
	{
		PersonaPtr.Pin()->UnregisterOnPostUndo(this);
	}
}

TSharedRef<class SAnimationScrubPanel> SMontageEditor::ConstructAnimScrubPanel()
{
	return SAssignNew(AnimMontageScrubPanel, SAnimMontageScrubPanel)
		.Persona(PersonaPtr)
		.LockedSequence(MontageObj)
		.ViewInputMin(this, &SMontageEditor::GetViewMinInput)
		.ViewInputMax(this, &SMontageEditor::GetViewMaxInput)
		.OnSetInputViewRange(this, &SMontageEditor::SetInputViewRange)
		.bAllowZoom(true)
		.MontageEditor(SharedThis(this));
}

void SMontageEditor::SetMontageObj(UAnimMontage * NewMontage)
{
	MontageObj = NewMontage;

	if (MontageObj)
	{
		SetInputViewRange(0, MontageObj->SequenceLength); // FIXME
	}

	AnimMontagePanel->SetMontage(NewMontage);
	AnimNotifyPanel->SetSequence(NewMontage);
	AnimCurvePanel->SetSequence(NewMontage);
	// sequence editor locks the sequence, so it doesn't get replaced by clicking 
	AnimMontageScrubPanel->ReplaceLockedSequence(NewMontage);
}

bool SMontageEditor::ValidIndexes(int32 AnimSlotIndex, int32 AnimSegmentIndex) const
{
	return (MontageObj != NULL && MontageObj->SlotAnimTracks.IsValidIndex(AnimSlotIndex) && MontageObj->SlotAnimTracks[AnimSlotIndex].AnimTrack.AnimSegments.IsValidIndex(AnimSegmentIndex) );
}

bool SMontageEditor::ValidSection(int32 SectionIndex) const
{
	return (MontageObj != NULL && MontageObj->CompositeSections.IsValidIndex(SectionIndex));
}

bool SMontageEditor::ValidBranch(int32 BranchIndex) const
{
	return (MontageObj != NULL && MontageObj->BranchingPoints.IsValidIndex(BranchIndex));
}

void SMontageEditor::RefreshNotifyTriggerOffsets()
{
	for(auto Iter = MontageObj->Notifies.CreateIterator(); Iter; ++Iter)
	{
		FAnimNotifyEvent& Notify = (*Iter);

		// Offset for the beginning of a notify
		EAnimEventTriggerOffsets::Type PredictedOffset = MontageObj->CalculateOffsetForNotify(Notify.DisplayTime);
		Notify.RefreshTriggerOffset(PredictedOffset);

		// Offset for the end of a notify state if necessary
		if(Notify.Duration > 0.0f)
		{
			PredictedOffset = MontageObj->CalculateOffsetForNotify(Notify.DisplayTime + Notify.Duration);
			Notify.RefreshEndTriggerOffset(PredictedOffset);
		}
	}

	for(auto Iter = MontageObj->BranchingPoints.CreateIterator(); Iter; ++Iter)
	{
		FBranchingPoint& BranchPoint = (*Iter);

		EAnimEventTriggerOffsets::Type PredictedOffset = MontageObj->CalculateOffsetForBranchingPoint(BranchPoint.DisplayTime);
		BranchPoint.RefreshTriggerOffset(PredictedOffset);
	}
}

bool SMontageEditor::GetSectionTime( int32 SectionIndex, float &OutTime ) const
{
	if (MontageObj != NULL && MontageObj->CompositeSections.IsValidIndex(SectionIndex))
	{
		OutTime = MontageObj->CompositeSections[SectionIndex].StartTime;
		return true;
	}

	return false;
}

TArray<FString>	SMontageEditor::GetSectionNames() const
{
	TArray<FString> Names;
	if (MontageObj != NULL)
	{
		for( int32 I=0; I < MontageObj->CompositeSections.Num(); I++)
		{
			Names.Add(MontageObj->CompositeSections[I].SectionName.ToString());
		}
	}
	return Names;
}

TArray<float> SMontageEditor::GetSectionStartTimes() const
{
	TArray<float>	Times;
	if (MontageObj != NULL)
	{
		for( int32 I=0; I < MontageObj->CompositeSections.Num(); I++)
		{
			Times.Add(MontageObj->CompositeSections[I].StartTime);
		}
	}
	return Times;
}

TArray<FTrackMarkerBar> SMontageEditor::GetMarkerBarInformation() const
{
	TArray<FTrackMarkerBar> MarkerBars;
	if (MontageObj != NULL)
	{
		for( int32 I=0; I < MontageObj->CompositeSections.Num(); I++)
		{
			FTrackMarkerBar Bar;
			Bar.Time = MontageObj->CompositeSections[I].StartTime;
			Bar.DrawColour = FLinearColor(0.f,1.f,0.f);
			MarkerBars.Add(Bar);
		}
		for( int32 I = 0; I < MontageObj->BranchingPoints.Num(); ++I )
		{
			FTrackMarkerBar Bar;
			Bar.Time = MontageObj->BranchingPoints[I].DisplayTime;
			Bar.DrawColour = FLinearColor(0.8f,0.f,0.8f);
			MarkerBars.Add(Bar);
		}
	}
	return MarkerBars;
}

TArray<float> SMontageEditor::GetAnimSegmentStartTimes() const
{
	TArray<float>	Times;
	if (MontageObj != NULL)
	{
		for ( int32 i=0; i < MontageObj->SlotAnimTracks.Num(); i++)
		{
			for (int32 j=0; j < MontageObj->SlotAnimTracks[i].AnimTrack.AnimSegments.Num(); j++)
			{
				Times.Add( MontageObj->SlotAnimTracks[i].AnimTrack.AnimSegments[j].StartPos );
			}
		}
	}
	return Times;
}

void SMontageEditor::OnEditSectionTime( int32 SectionIndex, float NewTime)
{
	if (MontageObj != NULL && MontageObj->CompositeSections.IsValidIndex(SectionIndex))
	{
		if(!bDragging)
		{
			//If this is the first drag event
			const FScopedTransaction Transaction( LOCTEXT("EditSection", "Edit Section Start Time") );
			MontageObj->Modify();
		}
		bDragging = true;

		MontageObj->CompositeSections[SectionIndex].StartTime = NewTime;
	}
}
void SMontageEditor::OnEditSectionTimeFinish( int32 SectionIndex )
{
	bDragging = false;

	if(MontageObj!=NULL)
	{
		SortSections();
		RefreshNotifyTriggerOffsets();
		MontageObj->MarkPackageDirty();
		AnimMontageSectionsPanel->Update();
	}
}

void SMontageEditor::PreAnimUpdate()
{
	MontageObj->Modify();
}

void SMontageEditor::PostAnimUpdate()
{
	MontageObj->MarkPackageDirty();
	SortAndUpdateMontage();
}

void SMontageEditor::RebuildMontagePanel()
{
	SortAndUpdateMontage();
	AnimMontageSectionsPanel->Update();
	bRebuildMontagePanel = false;
}

void SMontageEditor::OnMontageChange(class UObject *EditorAnimBaseObj, bool Rebuild)
{
	bDragging = false;

	if(MontageObj!=NULL)
	{
		if(Rebuild)
		{
			bRebuildMontagePanel = true;
		} 
		else
		{
			CollapseMontage();
		}
		
		MontageObj->MarkPackageDirty();
	}
}

void SMontageEditor::InitDetailsViewEditorObject(UEditorAnimBaseObj* EdObj)
{
	EdObj->InitFromAnim(MontageObj, FOnAnimObjectChange::CreateSP( SharedThis(this), &SMontageEditor::OnMontageChange ));
}

/** This will remove empty spaces in the montage's anim segment but not resort. e.g. - all cached indexes remains valid. UI IS NOT REBUILT after this */
void SMontageEditor::CollapseMontage()
{
	if (MontageObj==NULL)
	{
		return;
	}

	for (int32 i=0; i < MontageObj->SlotAnimTracks.Num(); i++)
	{
		MontageObj->SlotAnimTracks[i].AnimTrack.CollapseAnimSegments();
	}

	RecalculateSequenceLength();
}

/** This will sort all components of the montage and update (recreate) the UI */
void SMontageEditor::SortAndUpdateMontage()
{
	if (MontageObj==NULL)
	{
		return;
	}
	
	SortAnimSegments();

	RecalculateSequenceLength();

	SortSections();

	SortBranchPoints();

	RefreshNotifyTriggerOffsets();

	// Update view (this will recreate everything)
	AnimMontagePanel->Update();
	AnimMontageSectionsPanel->Update();

	// Restart the preview instance of the montage
	RestartPreview();
}

float SMontageEditor::CalculateSequenceLengthOfEditorObject() const
{
	float NewSequenceLength = 0.f;
	for (auto Iter = MontageObj->SlotAnimTracks.CreateIterator(); Iter; ++Iter)
	{
		FSlotAnimationTrack& SlotAnimTrack = (*Iter);
		if ( SlotAnimTrack.AnimTrack.AnimSegments.Num() > 0 )
		{
			NewSequenceLength = FMath::Max(NewSequenceLength, SlotAnimTrack.AnimTrack.GetLength());
		}
	}
	return NewSequenceLength;
}

/** Sort Segments by starting time */
void SMontageEditor::SortAnimSegments()
{
	for (int32 I=0; I < MontageObj->SlotAnimTracks.Num(); I++)
	{
		MontageObj->SlotAnimTracks[I].AnimTrack.SortAnimSegments();
	}
}

/** Sort BranchPoints by time */
void SMontageEditor::SortBranchPoints()
{
	struct FCompareBranchPoints
	{
		bool operator()( const FBranchingPoint &A, const FBranchingPoint &B ) const
		{
			return A.GetTriggerTime() < B.GetTriggerTime();
		}
	};
	if (MontageObj != NULL)
	{
		MontageObj->BranchingPoints.Sort(FCompareBranchPoints());
	}
}

/** Sort Composite Sections by Start TIme */
void SMontageEditor::SortSections()
{
	struct FCompareSections
	{
		bool operator()( const FCompositeSection &A, const FCompositeSection &B ) const
		{
			return A.StartTime < B.StartTime;
		}
	};
	if (MontageObj != NULL)
	{
		MontageObj->CompositeSections.Sort(FCompareSections());
	}

	EnsureStartingSection();
}

/** Ensure there is at least one section in the montage and that the first section starts at T=0.f */
void SMontageEditor::EnsureStartingSection()
{
	if(MontageObj->CompositeSections.Num() <= 0)
	{
		FCompositeSection NewSection;
		NewSection.StartTime = 0.f;
		NewSection.SectionName = FName(TEXT("Default"));
		MontageObj->CompositeSections.Add(NewSection);
		MontageObj->MarkPackageDirty();
	}

	check(MontageObj->CompositeSections.Num() > 0);
	if(MontageObj->CompositeSections[0].StartTime > 0.f)
	{
		MontageObj->CompositeSections[0].StartTime = 0.f;
		MontageObj->MarkPackageDirty();
	}
}

/** Esnure there is at least one slot node track */
void SMontageEditor::EnsureSlotNode()
{
	if (MontageObj && MontageObj->SlotAnimTracks.Num()==0)
	{
		AddNewMontageSlot(TEXT(""));
		MontageObj->MarkPackageDirty();
	}
}

/** Make sure all Sections, Branches, and Notifies are clamped to NewEndTime (called before NewEndTime is set to SequenceLength) */
bool SMontageEditor::ClampToEndTime(float NewEndTime)
{
	bool bClampingNeeded = SAnimEditorBase::ClampToEndTime(NewEndTime);
	if(bClampingNeeded)
	{
		float ratio = NewEndTime / MontageObj->SequenceLength;

		for(int32 i=0; i < MontageObj->CompositeSections.Num(); i++)
		{
			if(MontageObj->CompositeSections[i].StartTime > NewEndTime)
			{
				MontageObj->CompositeSections[i].StartTime *= ratio;
			}
		}

		for(int32 i=0; i < MontageObj->BranchingPoints.Num(); i++)
		{
			FBranchingPoint& BranchPoint = MontageObj->BranchingPoints[i];
			if(BranchPoint.DisplayTime > NewEndTime)
			{
				BranchPoint.DisplayTime *= ratio;
				BranchPoint.TriggerTimeOffset = GetTriggerTimeOffsetForType(MontageObj->CalculateOffsetForBranchingPoint(BranchPoint.DisplayTime));
			}
		}

		for(int32 i=0; i < MontageObj->Notifies.Num(); i++)
		{
			FAnimNotifyEvent& Notify = MontageObj->Notifies[i];

			if(Notify.DisplayTime >= NewEndTime)
			{
				Notify.DisplayTime *= ratio;
				Notify.TriggerTimeOffset = GetTriggerTimeOffsetForType(MontageObj->CalculateOffsetForNotify(Notify.DisplayTime));
			}
		}
	}

	return bClampingNeeded;
}

void SMontageEditor::AddNewSection(float StartTime, FString SectionName)
{
	if(MontageObj!=NULL)
	{
		const FScopedTransaction Transaction( LOCTEXT("AddNewSection", "Add New Section") );
		MontageObj->Modify();

		if (MontageObj->AddAnimCompositeSection(FName(*SectionName), StartTime) != INDEX_NONE)
		{
			RebuildMontagePanel();
		}
	}
}

void SMontageEditor::RemoveSection(int32 SectionIndex)
{
	if(ValidSection(SectionIndex))
	{
		const FScopedTransaction Transaction( LOCTEXT("DeleteSection", "Delete Section") );
		MontageObj->Modify();

		MontageObj->CompositeSections.RemoveAt(SectionIndex);
		EnsureStartingSection();
		MontageObj->MarkPackageDirty();
		AnimMontageSectionsPanel->Update();
		RestartPreview();
	}
}

FString	SMontageEditor::GetSectionName(int32 SectionIndex) const
{
	if (ValidSection(SectionIndex))
	{
		return MontageObj->GetSectionName(SectionIndex).ToString();
	}
	return FString();
}

float SMontageEditor::GetBranchPointStartPos(int32 BranchPointIndex) const
{
	if (ValidBranch(BranchPointIndex))
	{
		return MontageObj->BranchingPoints[BranchPointIndex].DisplayTime;
	}
	return 0.f;
}

FString	SMontageEditor::GetBranchPointName(int32 BranchPointIndex) const
{
	if (ValidBranch(BranchPointIndex))
	{
		return MontageObj->BranchingPoints[BranchPointIndex].EventName.ToString();
	}
	return FString();
}

void SMontageEditor::SetBranchPointStartPos(float NewStartPos, int32 BranchPointIndex)
{
	if (ValidBranch(BranchPointIndex))
	{
		const FScopedTransaction Transaction( LOCTEXT("SetBranchPoint", "Set Branch Point Start Time") );
		MontageObj->Modify();

		FBranchingPoint& BranchPoint = MontageObj->BranchingPoints[BranchPointIndex];
		BranchPoint.DisplayTime = NewStartPos;
		BranchPoint.RefreshTriggerOffset(MontageObj->CalculateOffsetForBranchingPoint(BranchPoint.DisplayTime));
	}
}

void SMontageEditor::RemoveBranchPoint(int32 BranchPointIndex)
{
	if(ValidBranch(BranchPointIndex))
	{
		const FScopedTransaction Transaction( LOCTEXT("RemoveBranchPoint", "Remove Branch Point") );
		MontageObj->Modify();

		MontageObj->BranchingPoints.RemoveAt(BranchPointIndex);
		MontageObj->MarkPackageDirty();
		SortAndUpdateMontage();
	}
}
void SMontageEditor::AddBranchPoint(float StartTime, FString EventName)
{
	if(MontageObj!=NULL)
	{
		const FScopedTransaction Transaction( LOCTEXT("AddBranchPoint", "Add Branch Point") );
		MontageObj->Modify();

		FBranchingPoint NewBranch;
		NewBranch.DisplayTime=StartTime;
		NewBranch.TriggerTimeOffset = GetTriggerTimeOffsetForType(MontageObj->CalculateOffsetForBranchingPoint(NewBranch.DisplayTime));
		NewBranch.EventName=FName(*EventName);
		MontageObj->BranchingPoints.Add(NewBranch);
		MontageObj->MarkPackageDirty();
		SortAndUpdateMontage();
	}
}

void SMontageEditor::RenameBranchPoint(int32 BranchIndex, FString NewEventName)
{
	if(ValidBranch(BranchIndex))
	{
		const FScopedTransaction Transaction( LOCTEXT("RenameBranchPoint", "Rename Branch Point") );
		MontageObj->Modify();

		MontageObj->BranchingPoints[BranchIndex].EventName = FName( *NewEventName );
		MontageObj->MarkPackageDirty();
	}
}

void SMontageEditor::RenameSlotNode(int32 SlotIndex, FString NewSlotName)
{
	if(MontageObj->SlotAnimTracks.IsValidIndex(SlotIndex))
	{
		FName NewName(*NewSlotName);
		if(MontageObj->SlotAnimTracks[SlotIndex].SlotName != NewName)
		{
			const FScopedTransaction Transaction( LOCTEXT("RenameSlot", "Rename Slot") );
			MontageObj->Modify();

			MontageObj->SlotAnimTracks[SlotIndex].SlotName = NewName;
			MontageObj->MarkPackageDirty();
		}
	}
}

void SMontageEditor::AddNewMontageSlot( FString NewSlotName )
{
	if(MontageObj != NULL)
	{
		const FScopedTransaction Transaction( LOCTEXT("AddSlot", "Add Slot") );
		MontageObj->Modify();

		FSlotAnimationTrack NewTrack;
		NewTrack.SlotName = FName(*NewSlotName);
		MontageObj->SlotAnimTracks.Add( NewTrack );
		MontageObj->MarkPackageDirty();

		AnimMontagePanel->Update();
	}
}

FText SMontageEditor::GetMontageSlotName(int32 SlotIndex) const
{
	if(MontageObj->SlotAnimTracks.IsValidIndex(SlotIndex) && MontageObj->SlotAnimTracks[SlotIndex].SlotName != NAME_None)
	{
		return FText::FromName( MontageObj->SlotAnimTracks[SlotIndex].SlotName );
	}	
	return FText::GetEmpty();
}

void SMontageEditor::RemoveMontageSlot(int32 AnimSlotIndex)
{
	if(MontageObj != NULL && MontageObj->SlotAnimTracks.IsValidIndex(AnimSlotIndex))
	{
		const FScopedTransaction Transaction( LOCTEXT("RemoveSlot", "Remove Slot") );
		MontageObj->Modify();

		MontageObj->SlotAnimTracks.RemoveAt(AnimSlotIndex);
		MontageObj->MarkPackageDirty();
		AnimMontagePanel->Update();
	}
}

void SMontageEditor::ShowSectionInDetailsView(int32 SectionIndex)
{
	UEditorCompositeSection *Obj = Cast<UEditorCompositeSection>(ShowInDetailsView(UEditorCompositeSection::StaticClass()));
	if(Obj != NULL)
	{
		Obj->InitSection(SectionIndex);
	}
	RestartPreviewFromSection(SectionIndex);
}

void SMontageEditor::RestartPreview()
{
	UAnimPreviewInstance * Preview = Cast<UAnimPreviewInstance>(PersonaPtr.Pin()->GetPreviewMeshComponent() ? PersonaPtr.Pin()->GetPreviewMeshComponent()->PreviewInstance:NULL);
	if (Preview)
	{
		Preview->MontagePreview_PreviewNormal();
	}
}

void SMontageEditor::RestartPreviewFromSection(int32 FromSectionIdx)
{
	UAnimPreviewInstance * Preview = Cast<UAnimPreviewInstance>(PersonaPtr.Pin()->GetPreviewMeshComponent() ? PersonaPtr.Pin()->GetPreviewMeshComponent()->PreviewInstance:NULL);
	if(Preview)
	{
		Preview->MontagePreview_PreviewNormal(FromSectionIdx);
	}
}

void SMontageEditor::RestartPreviewPlayAllSections()
{
	UAnimPreviewInstance * Preview = Cast<UAnimPreviewInstance>(PersonaPtr.Pin()->GetPreviewMeshComponent() ? PersonaPtr.Pin()->GetPreviewMeshComponent()->PreviewInstance:NULL);
	if(Preview)
	{
		Preview->MontagePreview_PreviewAllSections();
	}
}

void SMontageEditor::MakeDefaultSequentialSections()
{
	check(MontageObj!=NULL);
	SortSections();
	for(int32 SectionIdx=0; SectionIdx < MontageObj->CompositeSections.Num(); SectionIdx++)
	{
		MontageObj->CompositeSections[SectionIdx].NextSectionName = MontageObj->CompositeSections.IsValidIndex(SectionIdx+1) ? MontageObj->CompositeSections[SectionIdx+1].SectionName : NAME_None;
	}
	RestartPreview();
}

void SMontageEditor::ClearSquenceOrdering()
{
	check(MontageObj!=NULL);
	SortSections();
	for(int32 SectionIdx=0; SectionIdx < MontageObj->CompositeSections.Num(); SectionIdx++)
	{
		MontageObj->CompositeSections[SectionIdx].NextSectionName = NAME_None;
	}
	RestartPreview();
}

void SMontageEditor::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	// we should not update any property related within PostEditChange, 
	// so this is defered to Tick, when it needs to rebuild, just mark it and this will update in the next tick
	if (bRebuildMontagePanel)
	{
		RebuildMontagePanel();
	}

	SWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void SMontageEditor::PostUndo()
{
	// when undo or redo happens, we still have to recalculate length, so we can't rely on sequence length changes or not
	if (MontageObj->SequenceLength)
	{
		MontageObj->SequenceLength = 0.f;
	}

	RebuildMontagePanel(); //Rebuild here, undoing adds can cause slate to crash later on if we don't
}
#undef LOCTEXT_NAMESPACE

