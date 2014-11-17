// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "MarginTrackEditor.h"
#include "Developer/MovieSceneTools/Public/PropertySection.h"
#include "Developer/MovieSceneTools/Public/MovieSceneToolHelpers.h"
#include "Runtime/UMG/Public/Animation/MovieSceneMarginSection.h"
#include "Runtime/UMG/Public/Animation/MovieSceneMarginTrack.h"
#include "Editor/Sequencer/Public/ISectionLayoutBuilder.h"
#include "Editor/Sequencer/Public/ISequencerObjectChangeListener.h"
#include "Editor/PropertyEditor/Public/PropertyHandle.h"

class FMarginPropertySection : public FPropertySection
{
public:
	FMarginPropertySection( UMovieSceneSection& InSectionObject, FName SectionName )
		: FPropertySection(InSectionObject, SectionName) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override
	{
		UMovieSceneMarginSection* MarginSection = Cast<UMovieSceneMarginSection>(&SectionObject);

		LayoutBuilder.AddKeyArea("Left", NSLOCTEXT("FMarginPropertySection", "MarginLeft", "Left"), MakeShareable(new FFloatCurveKeyArea(MarginSection->GetLeftCurve())));
		LayoutBuilder.AddKeyArea("Top", NSLOCTEXT("FMarginPropertySection", "MarginTop", "Top"), MakeShareable(new FFloatCurveKeyArea(MarginSection->GetTopCurve())));
		LayoutBuilder.AddKeyArea("Right", NSLOCTEXT("FMarginPropertySection", "MarginRight", "Right"), MakeShareable(new FFloatCurveKeyArea(MarginSection->GetRightCurve())));
		LayoutBuilder.AddKeyArea("Bottom", NSLOCTEXT("FMarginPropertySection", "MarginBottom", "Bottom"), MakeShareable(new FFloatCurveKeyArea(MarginSection->GetBottomCurve())));
	}
};

FMarginTrackEditor::FMarginTrackEditor( TSharedRef<ISequencer> InSequencer )
	: FMovieSceneTrackEditor( InSequencer ) 
{
	// Get the object change listener for the sequencer and register a delegates for when properties change that we care about
	ISequencerObjectChangeListener& ObjectChangeListener = InSequencer->GetObjectChangeListener();
	ObjectChangeListener.GetOnAnimatablePropertyChanged( "Margin" ).AddRaw( this, &FMarginTrackEditor::OnMarginChanged );
}

FMarginTrackEditor::~FMarginTrackEditor()
{
	TSharedPtr<ISequencer> Sequencer = GetSequencer();
	if( Sequencer.IsValid() )
	{
		ISequencerObjectChangeListener& ObjectChangeListener = Sequencer->GetObjectChangeListener();
		ObjectChangeListener.GetOnAnimatablePropertyChanged( "Margin" ).RemoveAll( this );
	}
}



TSharedRef<FMovieSceneTrackEditor> FMarginTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new FMarginTrackEditor( InSequencer ) );
}

bool FMarginTrackEditor::SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const
{
	return Type == UMovieSceneMarginTrack::StaticClass();
}

TSharedRef<ISequencerSection> FMarginTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track )
{
	check( SupportsType( SectionObject.GetOuter()->GetClass() ) );

	UClass* SectionClass = SectionObject.GetOuter()->GetClass();

	TSharedRef<ISequencerSection> NewSection = MakeShareable( new FMarginPropertySection( SectionObject, Track->GetTrackName() ) );

	return NewSection;
}
/*

void FMarginTrackEditor::AddKey(const FGuid& ObjectGuid, UObject* AdditionalAsset)
{
	ISequencerObjectChangeListener& ObjectChangeListener = GetSequencer()->GetObjectChangeListener();

	TArray<UObject*> OutObjects;
	GetSequencer()->GetRuntimeObjects( GetSequencer()->GetFocusedMovieSceneInstance(), ObjectGuid, OutObjects);
	for (int32 i = 0; i < OutObjects.Num(); ++i)
	{
		ObjectChangeListener.TriggerAllPropertiesChanged(OutObjects[i]);
	}
}*/

void FMarginTrackEditor::OnMarginChanged( const TArray<UObject*>& InObjectsThatChanged, const IPropertyHandle& PropertyValue, bool bRequireAutoKey )
{
	FName PropertyName = PropertyValue.GetProperty()->GetFName();

	AnimatablePropertyChanged
	(
		UMovieSceneMarginTrack::StaticClass(), 
		bRequireAutoKey,
		FOnKeyProperty::CreateRaw(this, &FMarginTrackEditor::OnKeyMargin, &InObjectsThatChanged, &PropertyValue, PropertyName ) 
	);
}


void FMarginTrackEditor::OnKeyMargin( float KeyTime, const TArray<UObject*>* InObjectsThatChanged, const IPropertyHandle* PropertyValue, FName PropertyName )
{
	TArray<const void*> MarginValues;
	PropertyValue->AccessRawData( MarginValues );

	for( int32 ObjectIndex = 0; ObjectIndex < InObjectsThatChanged->Num(); ++ObjectIndex )
	{
		UObject* Object = (*InObjectsThatChanged)[ObjectIndex];
		FMargin MarginValue = *(const FMargin*)MarginValues[ObjectIndex];

		FGuid ObjectHandle = FindOrCreateHandleToObject( Object );
		if (ObjectHandle.IsValid())
		{
			UMovieSceneTrack* Track = GetTrackForObject( ObjectHandle, UMovieSceneMarginTrack::StaticClass(), PropertyName );
			if( ensure( Track ) )
			{
				UMovieSceneMarginTrack* MarginTrack = CastChecked<UMovieSceneMarginTrack>(Track);
				MarginTrack->SetPropertyName( PropertyName );
				// Find or add a new section at the auto-key time and changing the property same property
				// AddKeyToSection is not actually a virtual, it's redefined in each class with a different type
				bool bSuccessfulAdd = MarginTrack->AddKeyToSection( KeyTime, MarginValue );
				if (bSuccessfulAdd)
				{
					MarginTrack->SetAsShowable();
				}
			}
		}
	}
}