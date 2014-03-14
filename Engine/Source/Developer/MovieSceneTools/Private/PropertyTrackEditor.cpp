// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "PropertyEditing.h"
#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieSceneFloatTrack.h"
#include "MovieSceneVectorTrack.h"
#include "MovieSceneBoolTrack.h"
#include "MovieSceneColorTrack.h"
#include "ScopedTransaction.h"
#include "MovieSceneSection.h"
#include "ISequencerObjectChangeListener.h"
#include "ISequencerSection.h"
#include "ISectionLayoutBuilder.h"
#include "IKeyArea.h"
#include "MovieSceneFloatSection.h"
#include "MovieSceneVectorSection.h"
#include "MovieSceneBoolSection.h"
#include "MovieSceneColorSection.h"
#include "MovieSceneToolHelpers.h"
#include "PropertyTrackEditor.h"
#include "CommonMovieSceneTools.h"

/**
 * A generic implementation for displaying simple property sections
 */
class FPropertySection : public ISequencerSection
{
public:
	FPropertySection( UMovieSceneSection& InSectionObject, FName SectionName )
		: SectionObject( InSectionObject )
	{
		DisplayName = EngineUtils::SanitizeDisplayName( SectionName.ToString(), false );
	}

	/** ISequencerSection interface */
	virtual UMovieSceneSection* GetSectionObject() OVERRIDE { return &SectionObject; }

	virtual FString GetDisplayName() const OVERRIDE
	{
		return DisplayName;
	}
	
	virtual FString GetSectionTitle() const OVERRIDE { return FString(); }

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const OVERRIDE {}

	virtual int32 OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const OVERRIDE 
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
	
protected:
	/** Display name of the section */
	FString DisplayName;
	/** The section we are visualizing */
	UMovieSceneSection& SectionObject;
};

/**
 * A color section implementation
 */
class FColorPropertySection : public FPropertySection
{
public:
	FColorPropertySection( UMovieSceneSection& InSectionObject, FName SectionName )
		: FPropertySection(InSectionObject, SectionName) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const OVERRIDE
	{
		UMovieSceneColorSection* ColorSection = Cast<UMovieSceneColorSection>(&SectionObject);

		LayoutBuilder.AddKeyArea("R", TEXT("Red"), MakeShareable(new FFloatCurveKeyArea(ColorSection->GetRedCurve())));
		LayoutBuilder.AddKeyArea("G", TEXT("Green"), MakeShareable(new FFloatCurveKeyArea(ColorSection->GetGreenCurve())));
		LayoutBuilder.AddKeyArea("B", TEXT("Blue"), MakeShareable(new FFloatCurveKeyArea(ColorSection->GetBlueCurve())));
		LayoutBuilder.AddKeyArea("A", TEXT("Alpha"), MakeShareable(new FFloatCurveKeyArea(ColorSection->GetAlphaCurve())));
	}

	virtual int32 OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const OVERRIDE 
	{
		const UMovieSceneColorSection* ColorSection = Cast<const UMovieSceneColorSection>(&SectionObject);

		float StartTime = ColorSection->GetStartTime();
		float EndTime = ColorSection->GetEndTime();
		float SectionDuration = EndTime - StartTime;
		
		if (!FMath::IsNearlyZero(SectionDuration))
		{
			// If we are showing a background pattern and the colors is transparent, draw a checker pattern
			const FSlateBrush* CheckerBrush = FEditorStyle::GetBrush( "Checker" );
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), CheckerBrush, SectionClippingRect);
	
			TArray<FSlateGradientStop> GradientStops;

			TArray< TKeyValuePair<float, FLinearColor> > ColorKeys;
			ConsolidateColorCurves(ColorKeys, ColorSection);

			for (int32 i = 0; i < ColorKeys.Num(); ++i)
			{
				float Time = ColorKeys[i].Key;
				FLinearColor Color = ColorKeys[i].Value;
				float TimeFraction = (Time - StartTime) / SectionDuration;

				GradientStops.Add(FSlateGradientStop(FVector2D(TimeFraction * AllottedGeometry.Size.X, 0),
					Color ));
			}

			if (GradientStops.Num() > 0)
			{
				FSlateDrawElement::MakeGradient(
					OutDrawElements,
					LayerId + 1,
					AllottedGeometry.ToPaintGeometry(),
					GradientStops,
					Orient_Vertical,
					SectionClippingRect
				);
			}
		}

		return LayerId + 1;
	}

private:
	void ConsolidateColorCurves(TArray< TKeyValuePair<float, FLinearColor> >& OutColorKeys, const UMovieSceneColorSection* Section) const
	{
		// @todo Sequencer Optimize - This could all get cached, instead of recalculating everything every OnPaint

		const FRichCurve* Curves[4] = {
			&Section->GetRedCurve(),
			&Section->GetGreenCurve(),
			&Section->GetBlueCurve(),
			&Section->GetAlphaCurve()
		};

		// @todo Sequencer Optimize - This is a O(n^2) loop!
		// Our times are floats, which means we can't use a map and
		// do a quick lookup to see if the keys already exist
		// because the keys are ordered, we could take advantage of that, however
		TArray<float> TimesWithKeys;
		for (int32 i = 0; i < 4; ++i)
		{
			const FRichCurve* Curve = Curves[i];
			for (auto It(Curve->GetKeyIterator()); It; ++It)
			{
				float KeyTime = It->Time;

				bool bShouldAddKey = true;

				int32 InsertKeyIndex = INDEX_NONE;
				for (int32 k = 0; k < TimesWithKeys.Num(); ++k)
				{
					if (FMath::IsNearlyEqual(TimesWithKeys[k], KeyTime))
					{
						bShouldAddKey = false;
						break;
					}
					else if (TimesWithKeys[k] > KeyTime)
					{
						InsertKeyIndex = k;
						break;
					}
				}

				if (InsertKeyIndex == INDEX_NONE && bShouldAddKey) {InsertKeyIndex = TimesWithKeys.Num();}

				if (bShouldAddKey)
				{
					TimesWithKeys.Insert(KeyTime, InsertKeyIndex);
				}
			}
		}

		// @todo Sequencer Optimize - This another O(n^2) loop, since Eval is O(n)!
		for (int32 i = 0; i < TimesWithKeys.Num(); ++i)
		{
			OutColorKeys.Add(TKeyValuePair<float, FLinearColor>(TimesWithKeys[i], Section->Eval(TimesWithKeys[i])));
		}
	}
};

/**
 * An implementation of float property sections
 */
class FFloatPropertySection : public FPropertySection
{
public:
	FFloatPropertySection( UMovieSceneSection& InSectionObject, FName SectionName )
		: FPropertySection(InSectionObject, SectionName) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const OVERRIDE
	{
		UMovieSceneFloatSection* FloatSection = Cast<UMovieSceneFloatSection>( &SectionObject );
		LayoutBuilder.SetSectionAsKeyArea( MakeShareable( new FFloatCurveKeyArea( FloatSection->GetFloatCurve() ) ) );
	}
};

/**
 * An implementation of vector property sections
 */
class FVectorPropertySection : public FPropertySection
{
public:
	FVectorPropertySection( UMovieSceneSection& InSectionObject, FName SectionName )
		: FPropertySection(InSectionObject, SectionName) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const OVERRIDE
	{
		UMovieSceneVectorSection* VectorSection = Cast<UMovieSceneVectorSection>( &SectionObject );
		int32 ChannelsUsed = VectorSection->GetChannelsUsed();
		check(ChannelsUsed >= 2 && ChannelsUsed <= 4);

		LayoutBuilder.AddKeyArea("Vector.X", TEXT("X"), MakeShareable(new FFloatCurveKeyArea(VectorSection->GetCurve(0))));
		LayoutBuilder.AddKeyArea("Vector.Y", TEXT("Y"), MakeShareable(new FFloatCurveKeyArea(VectorSection->GetCurve(1))));
		if (ChannelsUsed >= 3)
		{LayoutBuilder.AddKeyArea("Vector.Z", TEXT("Z"), MakeShareable(new FFloatCurveKeyArea(VectorSection->GetCurve(2))));}
		if (ChannelsUsed >= 4)
		{LayoutBuilder.AddKeyArea("Vector.W", TEXT("W"), MakeShareable(new FFloatCurveKeyArea(VectorSection->GetCurve(3))));}
	}
};

/**
 * An implementation of bool property sections
 */
class FBoolPropertySection : public FPropertySection
{
public:
	FBoolPropertySection( UMovieSceneSection& InSectionObject, FName SectionName )
		: FPropertySection(InSectionObject, SectionName) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const OVERRIDE
	{
		UMovieSceneBoolSection* BoolSection = Cast<UMovieSceneBoolSection>( &SectionObject );
		LayoutBuilder.SetSectionAsKeyArea( MakeShareable( new FIntegralKeyArea( BoolSection->GetCurve() ) ) );
	}

	virtual int32 OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const OVERRIDE 
	{
		FTimeToPixel TimeToPixelConverter( AllottedGeometry, TRange<float>( SectionObject.GetStartTime(), SectionObject.GetEndTime() ) );

		// custom drawing for bool curves
		UMovieSceneBoolSection* BoolSection = Cast<UMovieSceneBoolSection>( &SectionObject );

		TArray<float> SectionSwitchTimes;
		SectionSwitchTimes.Add(SectionObject.GetStartTime());
		FIntegralCurve& BoolCurve = BoolSection->GetCurve();
		for (TArray<FIntegralKey>::TConstIterator It(BoolCurve.GetKeyIterator()); It; ++It)
		{
			float Time = It->Time;
			if (Time > SectionObject.GetStartTime() && Time < SectionObject.GetEndTime())
			{
				SectionSwitchTimes.Add(Time);
			}
		}
		SectionSwitchTimes.Add(SectionObject.GetEndTime());

		for (int32 i = 0; i < SectionSwitchTimes.Num() - 1; ++i)
		{
			float FirstTime = SectionSwitchTimes[i];
			float NextTime = SectionSwitchTimes[i+1];
			float FirstTimeInPixels = TimeToPixelConverter.TimeToPixel(FirstTime);
			float NextTimeInPixels = TimeToPixelConverter.TimeToPixel(NextTime);
			if (BoolSection->Eval(FirstTime))
			{
				FSlateDrawElement::MakeBox( 
					OutDrawElements,
					LayerId,
					AllottedGeometry.ToPaintGeometry(FVector2D(FirstTimeInPixels, 0), FVector2D(NextTimeInPixels - FirstTimeInPixels, AllottedGeometry.Size.Y)),
					FEditorStyle::GetBrush("Sequencer.GenericSection.Background"),
					SectionClippingRect,
					ESlateDrawEffect::None,
					FLinearColor(0.f, 1.f, 0.f, 1.f)
				);
			}
			else
			{
				FSlateDrawElement::MakeBox( 
					OutDrawElements,
					LayerId,
					AllottedGeometry.ToPaintGeometry(FVector2D(FirstTimeInPixels, 0), FVector2D(NextTimeInPixels - FirstTimeInPixels, AllottedGeometry.Size.Y)),
					FEditorStyle::GetBrush("Sequencer.GenericSection.Background"),
					SectionClippingRect,
					ESlateDrawEffect::None,
					FLinearColor(1.f, 0.f, 0.f, 1.f)
				);
			}
		}

		return LayerId+1;
	}
};

FPropertyTrackEditor::FPropertyTrackEditor( TSharedRef<ISequencer> InSequencer )
	: FMovieSceneTrackEditor( InSequencer ) 
{
	// Get the object change listener for the sequencer and register a delegates for when properties change that we care about
	ISequencerObjectChangeListener& ObjectChangeListener = InSequencer->GetObjectChangeListener();
	ObjectChangeListener.GetOnAnimatablePropertyChanged( UFloatProperty::StaticClass() ).AddRaw( this, &FPropertyTrackEditor::OnAnimatedPropertyChanged<float, UMovieSceneFloatTrack> );
	ObjectChangeListener.GetOnAnimatablePropertyChanged( UBoolProperty::StaticClass() ).AddRaw( this, &FPropertyTrackEditor::OnAnimatedPropertyChanged<bool, UMovieSceneBoolTrack> );
	ObjectChangeListener.GetOnAnimatablePropertyChanged( UStructProperty::StaticClass() ).AddRaw( this, &FPropertyTrackEditor::OnAnimatedVectorPropertyChanged );
	ObjectChangeListener.GetOnAnimatablePropertyChanged( UStructProperty::StaticClass() ).AddRaw( this, &FPropertyTrackEditor::OnAnimatedColorPropertyChanged );
}

FPropertyTrackEditor::~FPropertyTrackEditor()
{
	TSharedPtr<ISequencer> Sequencer = GetSequencer();
	if( Sequencer.IsValid() )
	{
		ISequencerObjectChangeListener& ObjectChangeListener = Sequencer->GetObjectChangeListener();
		ObjectChangeListener.GetOnAnimatablePropertyChanged( UFloatProperty::StaticClass() ).RemoveAll( this );
		ObjectChangeListener.GetOnAnimatablePropertyChanged( UBoolProperty::StaticClass() ).RemoveAll( this );
		ObjectChangeListener.GetOnAnimatablePropertyChanged( UStructProperty::StaticClass() ).RemoveAll( this );
	}
}



TSharedRef<FMovieSceneTrackEditor> FPropertyTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new FPropertyTrackEditor( InSequencer ) );
}

bool FPropertyTrackEditor::SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const
{
	// We support animatable floats and bools and colors
	return Type == UMovieSceneFloatTrack::StaticClass() ||
		Type == UMovieSceneBoolTrack::StaticClass() ||
		Type == UMovieSceneVectorTrack::StaticClass() ||
		Type == UMovieSceneColorTrack::StaticClass();
}

TSharedRef<ISequencerSection> FPropertyTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track )
{
	check( SupportsType( SectionObject.GetOuter()->GetClass() ) );

	UClass* SectionClass = SectionObject.GetOuter()->GetClass();

	TSharedRef<ISequencerSection> NewSection = TSharedRef<ISequencerSection>(
		SectionClass == UMovieSceneColorTrack::StaticClass() ? new FColorPropertySection( SectionObject, Track->GetTrackName() ) :
		SectionClass == UMovieSceneBoolTrack::StaticClass() ? new FBoolPropertySection( SectionObject, Track->GetTrackName() ) :
		SectionClass == UMovieSceneVectorTrack::StaticClass() ? new FVectorPropertySection( SectionObject, Track->GetTrackName() ) :
		SectionClass == UMovieSceneFloatTrack::StaticClass() ? new FFloatPropertySection( SectionObject, Track->GetTrackName() ) :
		new FPropertySection( SectionObject, Track->GetTrackName() )
		);

	return NewSection;
}

void FPropertyTrackEditor::AddKey(const FGuid& ObjectGuid, UObject* AdditionalAsset)
{
	ISequencerObjectChangeListener& ObjectChangeListener = GetSequencer()->GetObjectChangeListener();

	TArray<UObject*> OutObjects;
	GetSequencer()->GetRuntimeObjects( GetSequencer()->GetFocusedMovieSceneInstance(), ObjectGuid, OutObjects);
	for (int32 i = 0; i < OutObjects.Num(); ++i)
	{
		ObjectChangeListener.TriggerAllPropertiesChanged(OutObjects[i]);
	}
}

template <typename Type, typename TrackType>
void FPropertyTrackEditor::OnAnimatedPropertyChanged( const TArray<UObject*>& InObjectsThatChanged, const IPropertyHandle& PropertyValue, bool bRequireAutoKey )
{
	FName PropertyName = PropertyValue.GetProperty()->GetFName();

	// Get the value from the property 
	Type Value;
	FPropertyAccess::Result Result = PropertyValue.GetValue( Value );
	check( Result == FPropertyAccess::Success );
	
	AnimatablePropertyChanged(TrackType::StaticClass(), bRequireAutoKey,
		FOnKeyProperty::CreateRaw(this, &FPropertyTrackEditor::OnKeyProperty<Type, TrackType>, PropertyName, InObjectsThatChanged, (const Type*)&Value, bRequireAutoKey));
}

void FPropertyTrackEditor::OnAnimatedVectorPropertyChanged( const TArray<UObject*>& InObjectsThatChanged, const IPropertyHandle& PropertyValue, bool bRequireAutoKey )
{
	bool bIsVector2D = false, bIsVector = false, bIsVector4 = false;
	const UStructProperty* StructProp = Cast<const UStructProperty>(PropertyValue.GetProperty());
	if (StructProp && StructProp->Struct)
	{
		FName StructName = StructProp->Struct->GetFName();

		bIsVector2D = StructName == NAME_Vector2D;
		bIsVector = StructName == NAME_Vector;
		bIsVector4 = StructName == NAME_Vector4;
	}
	if (!bIsVector2D && !bIsVector && !bIsVector4) {return;}


	FName PropertyName = PropertyValue.GetProperty()->GetFName();

	// Get the vector value from the property
	FPropertyAccess::Result Result = FPropertyAccess::Fail;
	if (bIsVector2D)
	{
		FVector2D Value;
		Result = PropertyValue.GetValue( Value );
		check( Result == FPropertyAccess::Success );
		AnimatablePropertyChanged(UMovieSceneVectorTrack::StaticClass(), bRequireAutoKey,
			FOnKeyProperty::CreateRaw(this, &FPropertyTrackEditor::OnKeyProperty<FVector2D, UMovieSceneVectorTrack>, PropertyName, InObjectsThatChanged, (const FVector2D*)&Value, bRequireAutoKey));
	}
	else if (bIsVector)
	{
		FVector Value;
		Result = PropertyValue.GetValue( Value );
		check( Result == FPropertyAccess::Success );
		AnimatablePropertyChanged(UMovieSceneVectorTrack::StaticClass(), bRequireAutoKey,
			FOnKeyProperty::CreateRaw(this, &FPropertyTrackEditor::OnKeyProperty<FVector, UMovieSceneVectorTrack>, PropertyName, InObjectsThatChanged, (const FVector*)&Value, bRequireAutoKey));
	}
	else if (bIsVector4)
	{
		FVector4 Value;
		Result = PropertyValue.GetValue( Value );
		check( Result == FPropertyAccess::Success );
		AnimatablePropertyChanged(UMovieSceneVectorTrack::StaticClass(), bRequireAutoKey,
			FOnKeyProperty::CreateRaw(this, &FPropertyTrackEditor::OnKeyProperty<FVector4, UMovieSceneVectorTrack>, PropertyName, InObjectsThatChanged, (const FVector4*)&Value, bRequireAutoKey));
	}
	else {check(0);}
}

void FPropertyTrackEditor::OnAnimatedColorPropertyChanged( const TArray<UObject*>& InObjectsThatChanged, const IPropertyHandle& PropertyValue, bool bRequireAutoKey )
{
	bool bIsFColor = false, bIsFLinearColor = false;
	const UStructProperty* StructProp = Cast<const UStructProperty>(PropertyValue.GetProperty());
	if (StructProp && StructProp->Struct)
	{
		FName StructName = StructProp->Struct->GetFName();

		bIsFColor = StructName == NAME_Color;
		bIsFLinearColor = StructName == NAME_LinearColor;
	}
	if (!bIsFColor && !bIsFLinearColor) {return;}

	check(bIsFColor ^ bIsFLinearColor);


	FName PropertyName = PropertyValue.GetProperty()->GetFName();
					
	const UClass* PropertyClass = PropertyValue.GetPropertyClass();

	TSharedPtr<IPropertyHandle> ChildProperties[4] = {
		PropertyValue.GetChildHandle("R"),
		PropertyValue.GetChildHandle("G"),
		PropertyValue.GetChildHandle("B"),
		PropertyValue.GetChildHandle("A")
	};

	int32 FColorChannels[4];
	float FLinearColorChannels[4];
	for (int32 i = 0; i < 4; ++i)
	{
		FPropertyAccess::Result Result = bIsFColor ?
			ChildProperties[i]->GetValue(FColorChannels[i]) :
			ChildProperties[i]->GetValue(FLinearColorChannels[i]);
		check( Result == FPropertyAccess::Success );
	}

	FLinearColor ColorValue;
	if (bIsFColor)
	{
		ColorValue = FColor(FColorChannels[0], FColorChannels[1], FColorChannels[2], FColorChannels[3]).ReinterpretAsLinear();
	}
	else
	{
		ColorValue = FLinearColor(FLinearColorChannels[0], FLinearColorChannels[1], FLinearColorChannels[2], FLinearColorChannels[3]);
	}
	
	AnimatablePropertyChanged(UMovieSceneColorTrack::StaticClass(), bRequireAutoKey,
		FOnKeyProperty::CreateRaw(this, &FPropertyTrackEditor::OnKeyProperty<FLinearColor, UMovieSceneColorTrack>, PropertyName, InObjectsThatChanged, (const FLinearColor*)&ColorValue, bRequireAutoKey));
}


template <typename Type, typename TrackType>
void FPropertyTrackEditor::OnKeyProperty( float KeyTime, FName PropertyName, const TArray<UObject*> InObjectsThatChanged, const Type* Value, bool bAutoKeying )
{
	for( int32 ObjectIndex = 0; ObjectIndex < InObjectsThatChanged.Num(); ++ObjectIndex )
	{
		UObject* Object = InObjectsThatChanged[ObjectIndex];

		if( Object )
		{
			bool bTrackExists = TrackForObjectExists(Object, TrackType::StaticClass(), PropertyName);

			UMovieSceneTrack* Track = GetTrackForObject( Object, TrackType::StaticClass(), PropertyName );
			if( ensure( Track ) )
			{
				TrackType* TypedTrack = CastChecked<TrackType>(Track);
				TypedTrack->SetPropertyName( PropertyName );
				// Find or add a new section at the auto-key time and changing the property same property
				// AddKeyToSection is not actually a virtual, it's redefined in each class with a different type
				bool bSuccessfulAdd = TypedTrack->AddKeyToSection( KeyTime, *Value );
				if (bSuccessfulAdd && (bAutoKeying || bTrackExists))
				{
					TypedTrack->SetAsShowable();
				}
			}
		}
	}
}
