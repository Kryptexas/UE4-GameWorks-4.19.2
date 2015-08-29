// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "ScopedTransaction.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "MovieScene3DPathTrack.h"
#include "MovieScene3DPathSection.h"
#include "ISequencerObjectBindingManager.h"
#include "ISequencerSection.h"
#include "ISectionLayoutBuilder.h"
#include "IKeyArea.h"
#include "MovieSceneToolHelpers.h"
#include "MovieSceneTrackEditor.h"
#include "PathTrackEditor.h"
#include "Components/SplineComponent.h"

void GetActorsWithSplineComponents(TSet<AActor*>& SelectedActors, TSet<AActor*>& UnselectedActors)
{
	if (GEditor->GetEditorWorldContext().World())
	{
		for( FActorIterator	ActorIt(GEditor->GetEditorWorldContext().World()); ActorIt; ++ActorIt )
		{
			AActor* Actor = *ActorIt;
			TArray<USplineComponent*> SplineComponents;
			Actor->GetComponents(SplineComponents);
			if (SplineComponents.Num())
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
}

/**
 * Class that draws a path section in the sequencer
 */
class F3DPathSection : public ISequencerSection
{
public:
	F3DPathSection( UMovieSceneSection& InSection, F3DPathTrackEditor* InPathTrackEditor )
		: Section( InSection )
		, PathTrackEditor(InPathTrackEditor)
	{
	}

	/** ISequencerSection interface */
	virtual UMovieSceneSection* GetSectionObject() override
	{ 
		return &Section;
	}

	virtual FText GetDisplayName() const override
	{ 
		return NSLOCTEXT("FPathSection", "DisplayName", "Path");
	}
	
	virtual FText GetSectionTitle() const override { return FText::GetEmpty(); }

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override
	{
		UMovieScene3DPathSection* PathSection = Cast<UMovieScene3DPathSection>( &Section );

		LayoutBuilder.AddKeyArea("Timing", NSLOCTEXT("FPathSection", "TimingArea", "Timing"), MakeShareable( new FFloatCurveKeyArea ( &PathSection->GetTimingCurve( ), PathSection ) ) );
	}

	virtual int32 OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const override 
	{
		// Add a box for the section
		FSlateDrawElement::MakeBox( 
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FEditorStyle::GetBrush("Sequencer.GenericSection.Background"),
			SectionClippingRect
		); 

		return LayerId;
	}
	
	virtual void BuildSectionContextMenu(FMenuBuilder& MenuBuilder) override
	{
		TSet<AActor*> SelectedActors;
		TSet<AActor*> UnselectedActors;
		GetActorsWithSplineComponents(SelectedActors, UnselectedActors);	

		if (SelectedActors.Num()+UnselectedActors.Num() == 0)
		{
			return;
		}

		if (SelectedActors.Num())
		{
			MenuBuilder.BeginSection(NAME_None, FText::FromString("Selected Splines"));

			for (TSet<AActor*>::TIterator It(SelectedActors); It; ++It)
			{
				AActor* ActorWithSplineComponent = *It;
				FFormatNamedArguments Args;
				Args.Add( TEXT("PathName"), FText::FromString( ActorWithSplineComponent->GetActorLabel() ) );

				MenuBuilder.AddMenuEntry(
					FText::Format( NSLOCTEXT("Sequencer", "SetPath", "Set {PathName}"), Args ),
					FText::Format( NSLOCTEXT("Sequencer", "SetPathTooltip", "Set path track driven by {PathName}."), Args ),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(PathTrackEditor, &F3DPathTrackEditor::SetPath, &Section, ActorWithSplineComponent))
				);
			}
			
			MenuBuilder.EndSection();
		}

		if (UnselectedActors.Num())
		{
			MenuBuilder.BeginSection(NAME_None, FText::FromString("Splines"));

			for (TSet<AActor*>::TIterator It(UnselectedActors); It; ++It)
			{
				AActor* ActorWithSplineComponent = *It;
				FFormatNamedArguments Args;
				Args.Add( TEXT("PathName"), FText::FromString( ActorWithSplineComponent->GetActorLabel() ) );

				MenuBuilder.AddMenuEntry(
					FText::Format( NSLOCTEXT("Sequencer", "SetPath", "Set {PathName}"), Args ),
					FText::Format( NSLOCTEXT("Sequencer", "SetPathTooltip", "Set path track driven by {PathName}."), Args ),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(PathTrackEditor, &F3DPathTrackEditor::SetPath, &Section, ActorWithSplineComponent))
				);
			}

			MenuBuilder.EndSection();
		}

		MenuBuilder.AddMenuSeparator();
	}

private:
	/** The section we are visualizing */
	UMovieSceneSection& Section;

	/** The path track editor */
	F3DPathTrackEditor* PathTrackEditor;
};

F3DPathTrackEditor::F3DPathTrackEditor( TSharedRef<ISequencer> InSequencer )
	: FMovieSceneTrackEditor( InSequencer ) 
{
}

F3DPathTrackEditor::~F3DPathTrackEditor()
{
}

TSharedRef<FMovieSceneTrackEditor> F3DPathTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new F3DPathTrackEditor( InSequencer ) );
}

bool F3DPathTrackEditor::SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const
{
	// We support animatable transforms
	return Type == UMovieScene3DPathTrack::StaticClass();
}

TSharedRef<ISequencerSection> F3DPathTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track )
{
	check( SupportsType( SectionObject.GetOuter()->GetClass() ) );

	TSharedRef<ISequencerSection> NewSection( new F3DPathSection( SectionObject, this ) );

	return NewSection;
}

void F3DPathTrackEditor::AddKey( const FGuid& ObjectGuid, UObject* AdditionalAsset )
{
	AddPath(ObjectGuid, AdditionalAsset);
}

void F3DPathTrackEditor::BuildObjectBindingContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass)
{
	// build a menu with all the actors that have spline components, add the selected actors first
	if (ObjectClass->IsChildOf(AActor::StaticClass()))
	{
		TSet<AActor*> SelectedActors;
		TSet<AActor*> UnselectedActors;
		GetActorsWithSplineComponents(SelectedActors, UnselectedActors);

		if (SelectedActors.Num()+UnselectedActors.Num() > 0)
		{
			MenuBuilder.AddSubMenu(
				NSLOCTEXT("Sequencer", "AddPath", "Add Path"), NSLOCTEXT("Sequencer", "AddPathTooltip", "Adds a path track."),
				FNewMenuDelegate::CreateRaw(this, &F3DPathTrackEditor::AddPathSubMenu, ObjectBinding));
		}
		else
		{
			AActor* ActorWithSplineComponent = nullptr;
			FFormatNamedArguments Args;
			MenuBuilder.AddMenuEntry(
				FText::Format( NSLOCTEXT("Sequencer", "AddPath", "Add Path"), Args),
				FText::Format( NSLOCTEXT("Sequencer", "AddPathTooltip", "Adds a path track."), Args ),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &F3DPathTrackEditor::AddPath, ObjectBinding, (UObject*)ActorWithSplineComponent))
			);
		}
	}
}

void F3DPathTrackEditor::AddPathSubMenu(FMenuBuilder& MenuBuilder, FGuid ObjectBinding)
{
	TSet<AActor*> SelectedActors;
	TSet<AActor*> UnselectedActors;
	GetActorsWithSplineComponents(SelectedActors, UnselectedActors);	

	if (SelectedActors.Num()+UnselectedActors.Num() == 0)
	{
		return;
	}

	if (SelectedActors.Num())
	{
		MenuBuilder.BeginSection(NAME_None, FText::FromString("Selected Splines"));

		for (TSet<AActor*>::TIterator It(SelectedActors); It; ++It)
		{
			AActor* ActorWithSplineComponent = *It;
			FFormatNamedArguments Args;
			Args.Add( TEXT("PathName"), FText::FromString( ActorWithSplineComponent->GetActorLabel() ) );

			MenuBuilder.AddMenuEntry(
				FText::Format( NSLOCTEXT("Sequencer", "AddPath", "Add {PathName}"), Args ),
				FText::Format( NSLOCTEXT("Sequencer", "AddPathTooltip", "Add path track driven by {PathName}."), Args ),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &F3DPathTrackEditor::AddPath, ObjectBinding, (UObject*)ActorWithSplineComponent))
			);
		}
			
		MenuBuilder.EndSection();
	}

	if (UnselectedActors.Num())
	{
		MenuBuilder.BeginSection(NAME_None, FText::FromString("Splines"));

		for (TSet<AActor*>::TIterator It(UnselectedActors); It; ++It)
		{
			AActor* ActorWithSplineComponent = *It;
			FFormatNamedArguments Args;
			Args.Add( TEXT("PathName"), FText::FromString( ActorWithSplineComponent->GetActorLabel() ) );

			MenuBuilder.AddMenuEntry(
				FText::Format( NSLOCTEXT("Sequencer", "AddPath", "Add {PathName}"), Args ),
				FText::Format( NSLOCTEXT("Sequencer", "AddPathTooltip", "Add path track driven by {PathName}."), Args ),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &F3DPathTrackEditor::AddPath, ObjectBinding, (UObject*)ActorWithSplineComponent))
			);
		}

		MenuBuilder.EndSection();
	}
}

void F3DPathTrackEditor::AddPath(FGuid ObjectGuid, UObject* AdditionalAsset)
{
	TArray<UObject*> OutObjects;
	GetSequencer()->GetRuntimeObjects( GetSequencer()->GetFocusedMovieSceneInstance(), ObjectGuid, OutObjects);

	AnimatablePropertyChanged( UMovieScene3DPathTrack::StaticClass(), false,
		FOnKeyProperty::CreateRaw( this, &F3DPathTrackEditor::AddKeyInternal, OutObjects, AdditionalAsset) );
}

void F3DPathTrackEditor::SetPath(UMovieSceneSection* Section, AActor* ActorWithSplineComponent)
{
	const FScopedTransaction Transaction(NSLOCTEXT("Sequencer", "UndoSetPath", "Set Path"));

	UMovieScene3DPathSection* PathSection = (UMovieScene3DPathSection*)(Section);
	FGuid SplineId = FindOrCreateHandleToObject(ActorWithSplineComponent);

	if (SplineId.IsValid())
	{
		PathSection->SetPathId(SplineId);
	}
}

void F3DPathTrackEditor::AddKeyInternal( float KeyTime, const TArray<UObject*> Objects, UObject* AdditionalAsset)
{
	AActor* ActorWithSplineComponent = nullptr;
	
	if (AdditionalAsset != nullptr)
	{
		ActorWithSplineComponent = Cast<AActor>(AdditionalAsset);
	}

	FGuid SplineId;

	if (ActorWithSplineComponent != nullptr)
	{
		SplineId = FindOrCreateHandleToObject(ActorWithSplineComponent);
	}

	if (SplineId.IsValid())
	{
		for( int32 ObjectIndex = 0; ObjectIndex < Objects.Num(); ++ObjectIndex )
		{
			UObject* Object = Objects[ObjectIndex];

			FGuid ObjectHandle = FindOrCreateHandleToObject( Object );
			if (ObjectHandle.IsValid())
			{
				UMovieSceneTrack* Track = GetTrackForObject( ObjectHandle, UMovieScene3DPathTrack::StaticClass(), FName("Path"));

				if (ensure(Track))
				{
					// Clamp to next path section's start time or the end of the current sequencer view range
					float PathEndTime = GetSequencer()->GetViewRange().GetUpperBoundValue();
	
					for (int32 PathSectionIndex = 0; PathSectionIndex < Track->GetAllSections().Num(); ++PathSectionIndex)
					{
						float StartTime = Track->GetAllSections()[PathSectionIndex]->GetStartTime();
						float EndTime = Track->GetAllSections()[PathSectionIndex]->GetEndTime();
						if (KeyTime < StartTime)
						{
							if (PathEndTime > StartTime)
							{
								PathEndTime = StartTime;
							}
						}
					}

					Cast<UMovieScene3DPathTrack>(Track)->AddPath( KeyTime, PathEndTime, SplineId );
				}
			}
		}
	}
}
