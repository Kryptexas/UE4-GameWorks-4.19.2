// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieSceneSubSection.h"
#include "MovieSceneSequence.h"
#include "MovieSceneSubTrack.h"
#include "SubTrackEditor.h"


namespace SubTrackEditorConstants
{
	const float TrackHeight = 50.0f;
}


#define LOCTEXT_NAMESPACE "FSubTrackEditor"


/**
 * A generic implementation for displaying simple property sections.
 */
class FSubSection
	: public ISequencerSection
{
public:

	FSubSection(TSharedPtr<ISequencer> InSequencer, UMovieSceneSection& InSection, FName SectionName)
		: DisplayName(NSLOCTEXT("FSubSection", "DisplayName", "Scenes"))
		, SectionObject(*CastChecked<UMovieSceneSubSection>(&InSection))
		, Sequencer(InSequencer)
	{
		SequenceInstance = InSequencer->GetSequenceInstanceForSection(InSection);
	}

public:

	// ISequencerSection interface

	virtual void GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder) const override
	{
		// do nothing
	}

	virtual FText GetDisplayName() const override
	{
		return GetSectionTitle();
	}

	virtual float GetSectionHeight() const override
	{
		return SubTrackEditorConstants::TrackHeight;
	}

	virtual UMovieSceneSection* GetSectionObject() override
	{
		return &SectionObject;
	}

	virtual FText GetSectionTitle() const override
	{
		return FText::FromString(SectionObject.GetSequence()->GetName());
	}

	virtual int32 OnPaintSection(const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled) const override 
	{
		const UMovieScene* MovieScene = SectionObject.GetSequence()->GetMovieScene();
		int32 NumTracks = MovieScene->GetPossessableCount() + MovieScene->GetSpawnableCount() + MovieScene->GetMasterTracks().Num();
		const ESlateDrawEffect::Type DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

		// add a box for the section
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FEditorStyle::GetBrush("Sequencer.GenericSection.Background"),
			SectionClippingRect,
			DrawEffects,
			FColor(220, 120, 120)
		);

		// add number of tracks within the section's sequence
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToOffsetPaintGeometry(FVector2D(5.0f, 32.0f)),
			FText::Format(LOCTEXT("NumTracksFormat", "Number of tracks: {0}"), FText::AsNumber(NumTracks)),
			FEditorStyle::GetFontStyle("NormalFont"),
			SectionClippingRect,
			DrawEffects,
			FLinearColor::White
		);

		return LayerId;
	}

	virtual FReply OnSectionDoubleClicked(const FGeometry& SectionGeometry, const FPointerEvent& MouseEvent) override
	{
		if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			Sequencer.Pin()->FocusSequenceInstance(SequenceInstance.Pin().ToSharedRef());
		}

		return FReply::Handled();
	}

private:

	/** Display name of the section */
	FText DisplayName;

	/** The section we are visualizing */
	UMovieSceneSubSection& SectionObject;

	/** The instance that this section is part of */
	TWeakPtr<FMovieSceneSequenceInstance> SequenceInstance;

	/** Sequencer interface */
	TWeakPtr<ISequencer> Sequencer;
};


#undef LOCTEXT_NAMESPACE


#define LOCTEXT_NAMESPACE "FSubTrackEditor"


/* FSubTrackEditor structors
 *****************************************************************************/

FSubTrackEditor::FSubTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FMovieSceneTrackEditor(InSequencer) 
{ }


/* ISequencerTrackEditor interface
 *****************************************************************************/

void FSubTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	UMovieSceneSequence* RootMovieSceneSequence = GetSequencer()->GetRootMovieSceneSequence();

	if ((RootMovieSceneSequence == nullptr) || (RootMovieSceneSequence->GetClass()->GetName() != TEXT("LevelSequence")))
	{
		return;
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddSubTrack", "Sub Track"),
		LOCTEXT("AddSubTooltip", "Adds a new track that can contain other sequences."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "Sequencer.Tracks.Sub"),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FSubTrackEditor::HandleAddSubTrackMenuEntryExecute)
		)
	);
}


TSharedRef<ISequencerTrackEditor> FSubTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FSubTrackEditor(InSequencer));
}


TSharedRef<ISequencerSection> FSubTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track)
{
	return MakeShareable(new FSubSection(GetSequencer(), SectionObject, Track.GetTrackName()));
}


bool FSubTrackEditor::HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid)
{
	UMovieSceneSequence* Sequence = Cast<UMovieSceneSequence>(Asset);

	if (Sequence == nullptr)
	{
		return false;
	}

	if (CanAddSubSequence(*Sequence))
	{
		AnimatablePropertyChanged(UMovieSceneSubTrack::StaticClass(), FOnKeyProperty::CreateRaw(this, &FSubTrackEditor::HandleSequenceAdded, Sequence));
	}

	return true;
}


bool FSubTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	// We support sub movie scenes
	return Type == UMovieSceneSubTrack::StaticClass();
}


/* FSubTrackEditor callbacks
 *****************************************************************************/

bool FSubTrackEditor::CanAddSubSequence(const UMovieSceneSequence& Sequence) const
{
	// prevent adding ourselves and ensure we have a valid movie scene
	UMovieSceneSequence* FocusedSequence = GetSequencer()->GetFocusedMovieSceneSequence();

	if ((FocusedSequence == nullptr) || (FocusedSequence == &Sequence) || (FocusedSequence->GetMovieScene() == nullptr))
	{
		return false;
	}

	// ensure that the other sequence has a valid movie scene
	UMovieScene* SequenceMovieScene = Sequence.GetMovieScene();

	if (SequenceMovieScene == nullptr)
	{
		return false;
	}

	// make sure we are not contained in the other sequence (circular dependency)
	// @todo sequencer: this check is not sufficient (does not prevent circular dependencies of 2+ levels)
	UMovieSceneSubTrack* SequenceSubTrack = SequenceMovieScene->FindMasterTrack<UMovieSceneSubTrack>();

	if (SequenceSubTrack == nullptr)
	{
		return true;
	}

	return !SequenceSubTrack->ContainsSequence(*FocusedSequence, true);
}


/* FSubTrackEditor callbacks
 *****************************************************************************/

void FSubTrackEditor::HandleAddSubTrackMenuEntryExecute()
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

	if (FocusedMovieScene == nullptr)
	{
		return;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("Sequencer", "AddSubTrack_Transaction", "Add Sub Track"));
	FocusedMovieScene->Modify();

	UMovieSceneTrack* NewTrack = FocusedMovieScene->AddMasterTrack<UMovieSceneSubTrack>();
	ensure(NewTrack);

	GetSequencer()->NotifyMovieSceneDataChanged();
}


void FSubTrackEditor::HandleSequenceAdded(float KeyTime, class UMovieSceneSequence* Sequence)
{
	auto SubTrack = FindOrAddMasterTrack<UMovieSceneSubTrack>();
	SubTrack->AddSequence(*Sequence, KeyTime);
}


#undef LOCTEXT_NAMESPACE
