// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "ScopedTransaction.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "MovieScene3DAttachTrack.h"
#include "MovieScene3DAttachSection.h"
#include "ISequencerSection.h"
#include "ISectionLayoutBuilder.h"
#include "IKeyArea.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieSceneToolHelpers.h"
#include "MovieSceneTrackEditor.h"
#include "ActorEditorUtils.h"
#include "GameFramework/WorldSettings.h"
#include "AttachTrackEditor.h"


void GetAttachActors(TSet<AActor*>& SelectedActors, TSet<AActor*>& UnselectedActors, TArray<UObject*> InObjects)
{
	if (GEditor->GetEditorWorldContext().World())
	{
		for( FActorIterator	ActorIt(GEditor->GetEditorWorldContext().World()); ActorIt; ++ActorIt )
		{
			AActor* Actor = *ActorIt;

			if (Actor->IsListedInSceneOutliner() &&
				!FActorEditorUtils::IsABuilderBrush(Actor) &&
				!Actor->IsA( AWorldSettings::StaticClass() ) &&
				!Actor->IsPendingKill() &&
				!InObjects.Contains(Actor))
			{			
				if (GEditor->GetSelectedActors()->IsSelected(Actor))
				{
					SelectedActors.Add(Actor);
				}
				else
				{
					UnselectedActors.Add(Actor);
				}
			}
		}
	}
	SelectedActors.Sort([](const AActor& A, const AActor& B)
	{
		return A.GetActorLabel() < B.GetActorLabel();
	});
	UnselectedActors.Sort([](const AActor& A, const AActor& B)
	{
		return A.GetActorLabel() < B.GetActorLabel();
	});
}


/**
 * Class that draws an attach section in the sequencer
 */
class F3DAttachSection
	: public ISequencerSection
{
public:

	F3DAttachSection( UMovieSceneSection& InSection, F3DAttachTrackEditor* InAttachTrackEditor )
		: Section( InSection )
		, AttachTrackEditor(InAttachTrackEditor)
	{ }

	/** ISequencerSection interface */
	virtual UMovieSceneSection* GetSectionObject() override
	{ 
		return &Section;
	}

	virtual FText GetDisplayName() const override
	{ 
		return NSLOCTEXT("FAttachSection", "DisplayName", "Attach");
	}
	
	virtual FText GetSectionTitle() const override { return FText::GetEmpty(); }

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override {}

	virtual int32 OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const override 
	{
		const ESlateDrawEffect::Type DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	
		// Add a box for the section
		FSlateDrawElement::MakeBox( 
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FEditorStyle::GetBrush("Sequencer.GenericSection.Background"),
			SectionClippingRect,
			DrawEffects
		); 

		return LayerId;
	}
	
	virtual void BuildSectionContextMenu(FMenuBuilder& MenuBuilder) override
	{
		TArray<UObject*> OutObjects;
		//@todo

		TSet<AActor*> SelectedActors;
		TSet<AActor*> UnselectedActors;
		GetAttachActors(SelectedActors, UnselectedActors, OutObjects);	
		
		if (SelectedActors.Num()+UnselectedActors.Num() == 0)
		{
			return;
		}

		if (SelectedActors.Num())
		{
			MenuBuilder.BeginSection(NAME_None, FText::FromString("Selected Actors"));

			for (TSet<AActor*>::TIterator It(SelectedActors); It; ++It)
			{
				AActor* Actor = *It;
				FFormatNamedArguments Args;
				Args.Add( TEXT("AttachName"), FText::FromString( Actor->GetActorLabel() ) );

				MenuBuilder.AddMenuEntry(
					FText::Format( NSLOCTEXT("Sequencer", "SetAttach", "Set {AttachName}"), Args ),
					FText::Format( NSLOCTEXT("Sequencer", "SetAttachTooltip", "Set attach track driven by {AttachName}."), Args ),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(AttachTrackEditor, &F3DAttachTrackEditor::SetAttach, &Section, Actor))
				);
			}
			
			MenuBuilder.EndSection();
		}

		if (UnselectedActors.Num())
		{
			MenuBuilder.BeginSection(NAME_None, FText::FromString("Actors"));

			for (TSet<AActor*>::TIterator It(UnselectedActors); It; ++It)
			{
				AActor* Actor = *It;
				FFormatNamedArguments Args;
				Args.Add( TEXT("AttachName"), FText::FromString( Actor->GetActorLabel() ) );

				MenuBuilder.AddMenuEntry(
					FText::Format( NSLOCTEXT("Sequencer", "SetAttach", "Set {AttachName}"), Args ),
					FText::Format( NSLOCTEXT("Sequencer", "SetAttachTooltip", "Set attach track driven by {AttachName}."), Args ),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(AttachTrackEditor, &F3DAttachTrackEditor::SetAttach, &Section, Actor))
				);
			}

			MenuBuilder.EndSection();
		}
	}

private:

	/** The section we are visualizing */
	UMovieSceneSection& Section;

	/** The attach track editor */
	F3DAttachTrackEditor* AttachTrackEditor;
};


F3DAttachTrackEditor::F3DAttachTrackEditor( TSharedRef<ISequencer> InSequencer )
	: FMovieSceneTrackEditor( InSequencer ) 
{
}


F3DAttachTrackEditor::~F3DAttachTrackEditor()
{
}


TSharedRef<ISequencerTrackEditor> F3DAttachTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new F3DAttachTrackEditor( InSequencer ) );
}


bool F3DAttachTrackEditor::SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const
{
	// We support animatable transforms
	return Type == UMovieScene3DAttachTrack::StaticClass();
}


TSharedRef<ISequencerSection> F3DAttachTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track )
{
	check( SupportsType( SectionObject.GetOuter()->GetClass() ) );

	return MakeShareable( new F3DAttachSection( SectionObject, this ) );
}


void F3DAttachTrackEditor::AddKey( const FGuid& ObjectGuid, UObject* AdditionalAsset )
{
	AddAttach(ObjectGuid, AdditionalAsset);
}


void F3DAttachTrackEditor::BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass)
{
	// build a menu with all the possible actors, add the selected actors first
	if (ObjectClass->IsChildOf(AActor::StaticClass()))
	{
		TArray<UObject*> OutObjects;
		GetSequencer()->GetRuntimeObjects( GetSequencer()->GetFocusedMovieSceneSequenceInstance(), ObjectBinding, OutObjects);

		TSet<AActor*> SelectedActors;
		TSet<AActor*> UnselectedActors;
		GetAttachActors(SelectedActors, UnselectedActors, OutObjects);

		if (SelectedActors.Num()+UnselectedActors.Num() > 0)
		{
			MenuBuilder.AddSubMenu(
				NSLOCTEXT("Sequencer", "AddAttach", "Attach"), NSLOCTEXT("Sequencer", "AddAttachTooltip", "Adds an attach track."),
				FNewMenuDelegate::CreateRaw(this, &F3DAttachTrackEditor::AddAttachSubMenu, ObjectBinding));
		}
		else
		{
			AActor* Actor = nullptr;
			FFormatNamedArguments Args;
			MenuBuilder.AddMenuEntry(
				FText::Format( NSLOCTEXT("Sequencer", "AddAttach", "Attach"), Args),
				FText::Format( NSLOCTEXT("Sequencer", "AddAttachTooltip", "Adds an attach track."), Args ),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &F3DAttachTrackEditor::AddAttach, ObjectBinding, (UObject*)Actor),
						  FCanExecuteAction::CreateLambda( []{ return false; } ))
			);
		}
	}
}


void F3DAttachTrackEditor::AddAttachSubMenu(FMenuBuilder& MenuBuilder, FGuid ObjectBinding)
{
	TArray<UObject*> OutObjects;
	GetSequencer()->GetRuntimeObjects( GetSequencer()->GetFocusedMovieSceneSequenceInstance(), ObjectBinding, OutObjects);

	TSet<AActor*> SelectedActors;
	TSet<AActor*> UnselectedActors;
	GetAttachActors(SelectedActors, UnselectedActors, OutObjects);	

	if (SelectedActors.Num()+UnselectedActors.Num() == 0)
	{
		return;
	}

	if (SelectedActors.Num())
	{
		MenuBuilder.BeginSection(NAME_None, FText::FromString("Selected Actors"));

		for (TSet<AActor*>::TIterator It(SelectedActors); It; ++It)
		{
			AActor* Actor = *It;
			FFormatNamedArguments Args;
			Args.Add( TEXT("AttachName"), FText::FromString( Actor->GetActorLabel() ) );

			MenuBuilder.AddMenuEntry(
				FText::Format( NSLOCTEXT("Sequencer", "AddAttach", "{AttachName}"), Args ),
				FText::Format( NSLOCTEXT("Sequencer", "AddAttachTooltip", "Add attach track driven by {AttachName}."), Args ),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &F3DAttachTrackEditor::AddAttach, ObjectBinding, (UObject*)Actor))
			);
		}
			
		MenuBuilder.EndSection();
	}

	if (UnselectedActors.Num())
	{
		MenuBuilder.BeginSection(NAME_None, FText::FromString("Actors"));

		for (TSet<AActor*>::TIterator It(UnselectedActors); It; ++It)
		{
			AActor* Actor = *It;
			FFormatNamedArguments Args;
			Args.Add( TEXT("AttachName"), FText::FromString( Actor->GetActorLabel() ) );

			MenuBuilder.AddMenuEntry(
				FText::Format( NSLOCTEXT("Sequencer", "AddAttach", "{AttachName}"), Args ),
				FText::Format( NSLOCTEXT("Sequencer", "AddAttachTooltip", "Add attach track driven by {AttachName}."), Args ),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &F3DAttachTrackEditor::AddAttach, ObjectBinding, (UObject*)Actor))
			);
		}

		MenuBuilder.EndSection();
	}
}


void F3DAttachTrackEditor::AddAttach(FGuid ObjectGuid, UObject* AdditionalAsset)
{
	TArray<UObject*> OutObjects;
	GetSequencer()->GetRuntimeObjects( GetSequencer()->GetFocusedMovieSceneSequenceInstance(), ObjectGuid, OutObjects);

	AnimatablePropertyChanged( UMovieScene3DAttachTrack::StaticClass(),
		FOnKeyProperty::CreateRaw( this, &F3DAttachTrackEditor::AddKeyInternal, OutObjects, AdditionalAsset) );
}


void F3DAttachTrackEditor::SetAttach(UMovieSceneSection* Section, AActor* Actor)
{
	const FScopedTransaction Transaction(NSLOCTEXT("Sequencer", "UndoSetAttach", "Set Attach"));

	UMovieScene3DAttachSection* AttachSection = (UMovieScene3DAttachSection*)(Section);
	FGuid ActorId = FindOrCreateHandleToObject(Actor);

	if (ActorId.IsValid())
	{
		AttachSection->SetConstraintId(ActorId);
	}
}


void F3DAttachTrackEditor::AddKeyInternal( float KeyTime, const TArray<UObject*> Objects, UObject* AdditionalAsset)
{
	AActor* Actor = nullptr;
	
	if (AdditionalAsset != nullptr)
	{
		Actor = Cast<AActor>(AdditionalAsset);
	}

	FGuid ActorId;

	if (Actor != nullptr)
	{
		ActorId = FindOrCreateHandleToObject(Actor);
	}

	if (!ActorId.IsValid())
	{
		return;
	}

	for( int32 ObjectIndex = 0; ObjectIndex < Objects.Num(); ++ObjectIndex )
	{
		UObject* Object = Objects[ObjectIndex];

		FGuid ObjectHandle = FindOrCreateHandleToObject( Object );
		if (ObjectHandle.IsValid())
		{
			MovieSceneHelpers::SetRuntimeObjectMobility(Object);

			UMovieSceneTrack* Track = GetTrackForObject( ObjectHandle, UMovieScene3DAttachTrack::StaticClass(), FName("Attach"));

			if (ensure(Track))
			{
				// Clamp to next attach section's start time or the end of the current sequencer view range
				float AttachEndTime = GetSequencer()->GetViewRange().GetUpperBoundValue();
	
				for (int32 AttachSectionIndex = 0; AttachSectionIndex < Track->GetAllSections().Num(); ++AttachSectionIndex)
				{
					float StartTime = Track->GetAllSections()[AttachSectionIndex]->GetStartTime();
					float EndTime = Track->GetAllSections()[AttachSectionIndex]->GetEndTime();
					if (KeyTime < StartTime)
					{
						if (AttachEndTime > StartTime)
						{
							AttachEndTime = StartTime;
						}
					}
				}

				Cast<UMovieScene3DAttachTrack>(Track)->AddConstraint( KeyTime, AttachEndTime, ActorId );
			}
		}
	}
}
