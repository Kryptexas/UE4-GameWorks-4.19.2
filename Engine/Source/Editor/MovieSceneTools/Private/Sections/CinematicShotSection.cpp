// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "ISectionLayoutBuilder.h"
#include "Runtime/MovieSceneTracks/Public/Sections/MovieSceneCinematicShotSection.h"
#include "CinematicShotTrackEditor.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "SInlineEditableTextBlock.h"
#include "CinematicShotTrackEDitor.h"
#include "MovieSceneToolHelpers.h"

#define LOCTEXT_NAMESPACE "FCinematicShotSection"


/* FCinematicShotSection structors
 *****************************************************************************/

FCinematicShotSection::FCinematicShotSection(TSharedPtr<ISequencer> InSequencer, TSharedPtr<FTrackEditorThumbnailPool> InThumbnailPool, UMovieSceneSection& InSection, TSharedPtr<FCinematicShotTrackEditor> InCinematicShotTrackEditor)
	: FThumbnailSection(InSequencer, InThumbnailPool, InSection)
	, SectionObject(*CastChecked<UMovieSceneCinematicShotSection>(&InSection))
	, Sequencer(InSequencer)
	, CinematicShotTrackEditor(InCinematicShotTrackEditor)
{
	if (InSequencer->HasSequenceInstanceForSection(InSection))
	{
		SequenceInstance = InSequencer->GetSequenceInstanceForSection(InSection);
	}
}


FCinematicShotSection::~FCinematicShotSection()
{
}

int32 FCinematicShotSection::OnPaintSection(const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled) const
{
	if (SequenceInstance.IsValid())
	{
		return FThumbnailSection::OnPaintSection(AllottedGeometry, SectionClippingRect, OutDrawElements, LayerId, bParentEnabled);
	}
	else
	{
		const ESlateDrawEffect::Type DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

		FSlateDrawElement::MakeBox( 
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FEditorStyle::GetBrush("Sequencer.GenericSection.Background"),
			SectionClippingRect,
			DrawEffects,
			FLinearColor::Black
		); 

		return LayerId + 2;
	}
}

void FCinematicShotSection::BuildSectionContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding)
{
	MenuBuilder.BeginSection(NAME_None, LOCTEXT("ShotMenuText", "Shot"));
	{
		if (SequenceInstance.IsValid())
		{
			MenuBuilder.AddSubMenu(
				LOCTEXT("TakesMenu", "Takes"),
				LOCTEXT("TakesMenuTooltip", "Shot takes"),
				FNewMenuDelegate::CreateLambda([=](FMenuBuilder& InMenuBuilder){ AddTakesMenu(InMenuBuilder); }));

			MenuBuilder.AddMenuEntry(
				LOCTEXT("NewTake", "New Take"),
				FText::Format(LOCTEXT("NewTakeTooltip", "Create a new take for {0}"), SectionObject.GetShotDisplayName()),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(CinematicShotTrackEditor.Pin().ToSharedRef(), &FCinematicShotTrackEditor::NewTake, &SectionObject))
			);
		}

		MenuBuilder.AddMenuEntry(
			LOCTEXT("InsertNewShot", "Insert Shot"),
			FText::Format(LOCTEXT("InsertNewShotTooltip", "Insert a new shot after {0}"), SectionObject.GetShotDisplayName()),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(CinematicShotTrackEditor.Pin().ToSharedRef(), &FCinematicShotTrackEditor::InsertShot, &SectionObject))
		);

		if (SequenceInstance.IsValid())
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("DuplicateShot", "Duplicate Shot"),
				FText::Format(LOCTEXT("DuplicateShotTooltip", "Duplicate {0} to create a new shot"), SectionObject.GetShotDisplayName()),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(CinematicShotTrackEditor.Pin().ToSharedRef(), &FCinematicShotTrackEditor::DuplicateShot, &SectionObject))
			);

			/*
			//@todo
			MenuBuilder.AddMenuEntry(
				LOCTEXT("RenameShot", "Rename Shot"),
				FText::Format(LOCTEXT("RenameShotTooltip", "Rename {0}"), SectionObject.GetShotDisplayName()),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(CinematicShotTrackEditor.Pin().ToSharedRef(), &FCinematicShotTrackEditor::RenameShot, &SectionObject))
			);
			*/
		}
	}
	MenuBuilder.EndSection();
}

void FCinematicShotSection::AddTakesMenu(FMenuBuilder& MenuBuilder)
{
	TArray<uint32> TakeNumbers;
	MovieSceneToolHelpers::GatherTakes(&SectionObject, TakeNumbers);

	for (auto TakeNumber : TakeNumbers)
	{
		MenuBuilder.AddMenuEntry(
			FText::Format(LOCTEXT("TakeNumber", "Take {0}"), FText::AsNumber(TakeNumber)),
			FText::Format(LOCTEXT("TakeNumberTooltip", "Switch to take {0}"), FText::AsNumber(TakeNumber)),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(CinematicShotTrackEditor.Pin().ToSharedRef(), &FCinematicShotTrackEditor::SwitchTake, &SectionObject, TakeNumber))
		);
	}
}

FText FCinematicShotSection::GetDisplayName() const
{
	return NSLOCTEXT("FCinematicShotSection", "", "Shot");
}

/* FCinematicShotSection callbacks
 *****************************************************************************/

ACameraActor* FCinematicShotSection::GetCameraObject() const
{
	return CinematicShotTrackEditor.IsValid() ? CinematicShotTrackEditor.Pin()->GetCinematicShotCamera().Get() : nullptr;
}

FText FCinematicShotSection::HandleThumbnailTextBlockText() const
{
	return SectionObject.GetShotDisplayName();
}


void FCinematicShotSection::HandleThumbnailTextBlockTextCommitted(const FText& NewShotName, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter && !HandleThumbnailTextBlockText().EqualTo(NewShotName))
	{
		SectionObject.Modify();

		const FScopedTransaction Transaction(LOCTEXT("SetShotName", "Set Shot Name"));
	
		SectionObject.SetShotDisplayName(NewShotName);
	}
}

FReply FCinematicShotSection::OnSectionDoubleClicked(const FGeometry& SectionGeometry, const FPointerEvent& MouseEvent)
{
	if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		if (SequenceInstance.IsValid())
		{
			Sequencer.Pin()->FocusSequenceInstance(SequenceInstance.Pin().ToSharedRef());
		}
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
