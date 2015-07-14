// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "PropertyEditing.h"
#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieSceneFloatTrack.h"
#include "MovieSceneVectorTrack.h"
#include "MovieSceneBoolTrack.h"
#include "MovieSceneByteTrack.h"
#include "MovieSceneColorTrack.h"
#include "MovieSceneVisibilityTrack.h"
#include "ScopedTransaction.h"
#include "MovieSceneSection.h"
#include "ISequencerObjectBindingManager.h"
#include "ISequencerObjectChangeListener.h"
#include "ISequencerSection.h"
#include "ISectionLayoutBuilder.h"
#include "IKeyArea.h"
#include "MovieSceneFloatSection.h"
#include "MovieSceneVectorSection.h"
#include "MovieSceneBoolSection.h"
#include "MovieSceneByteSection.h"
#include "MovieSceneColorSection.h"
#include "MovieSceneVisibilitySection.h"
#include "MovieSceneToolHelpers.h"
#include "PropertyTrackEditor.h"
#include "CommonMovieSceneTools.h"
#include "PropertySection.h"

/**
 * A color section implementation
 */
class FColorPropertySection : public FPropertySection
{
public:
	FColorPropertySection( UMovieSceneSection& InSectionObject, FName SectionName )
		: FPropertySection(InSectionObject, SectionName) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override
	{
		UMovieSceneColorSection* ColorSection = Cast<UMovieSceneColorSection>(&SectionObject);

		LayoutBuilder.AddKeyArea("R", NSLOCTEXT("FColorPropertySection", "RedArea", "Red"), MakeShareable(new FFloatCurveKeyArea(&ColorSection->GetRedCurve(), ColorSection)));
		LayoutBuilder.AddKeyArea("G", NSLOCTEXT("FColorPropertySection", "GreenArea", "Green"), MakeShareable(new FFloatCurveKeyArea(&ColorSection->GetGreenCurve(), ColorSection)));
		LayoutBuilder.AddKeyArea("B", NSLOCTEXT("FColorPropertySection", "BlueArea", "Blue"), MakeShareable(new FFloatCurveKeyArea(&ColorSection->GetBlueCurve(), ColorSection)));
		LayoutBuilder.AddKeyArea("A", NSLOCTEXT("FColorPropertySection", "OpacityArea", "Opacity"), MakeShareable(new FFloatCurveKeyArea(&ColorSection->GetAlphaCurve(), ColorSection)));
	}

	virtual int32 OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const override 
	{
		const UMovieSceneColorSection* ColorSection = Cast<const UMovieSceneColorSection>(&SectionObject);

		float StartTime = ColorSection->GetStartTime();
		float EndTime = ColorSection->GetEndTime();
		float SectionDuration = EndTime - StartTime;

		if (!FMath::IsNearlyZero(SectionDuration))
		{
			LayerId = FPropertySection::OnPaintSection( AllottedGeometry, SectionClippingRect, OutDrawElements, LayerId, bParentEnabled );

			FVector2D GradientSize = FVector2D(AllottedGeometry.Size.X, (AllottedGeometry.Size.Y / 4)-3.0f);

			FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry(FVector2D(0,0), GradientSize );

			// If we are showing a background pattern and the colors is transparent, draw a checker pattern
			const FSlateBrush* CheckerBrush = FEditorStyle::GetBrush( "Checker" );
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, PaintGeometry, CheckerBrush, SectionClippingRect);
	
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
					PaintGeometry,
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

				if (InsertKeyIndex == INDEX_NONE && bShouldAddKey)
				{
					InsertKeyIndex = TimesWithKeys.Num();
				}

				if (bShouldAddKey)
				{
					TimesWithKeys.Insert(KeyTime, InsertKeyIndex);
				}
			}
		}

		// @todo Sequencer Optimize - This another O(n^2) loop, since Eval is O(n)!
		for (int32 i = 0; i < TimesWithKeys.Num(); ++i)
		{
			OutColorKeys.Add( TKeyValuePair<float, FLinearColor>(TimesWithKeys[i], Section->Eval(TimesWithKeys[i], FLinearColor(0,0,0,0) ) ) );
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

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override
	{
		UMovieSceneFloatSection* FloatSection = Cast<UMovieSceneFloatSection>( &SectionObject );
		LayoutBuilder.SetSectionAsKeyArea( MakeShareable( new FFloatCurveKeyArea( &FloatSection->GetFloatCurve(), FloatSection ) ) );
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

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override
	{
		UMovieSceneVectorSection* VectorSection = Cast<UMovieSceneVectorSection>( &SectionObject );
		int32 ChannelsUsed = VectorSection->GetChannelsUsed();
		check(ChannelsUsed >= 2 && ChannelsUsed <= 4);

		LayoutBuilder.AddKeyArea("Vector.X", NSLOCTEXT("FVectorPropertySection", "XArea", "X"), MakeShareable(new FFloatCurveKeyArea(&VectorSection->GetCurve(0), VectorSection)));
		LayoutBuilder.AddKeyArea("Vector.Y", NSLOCTEXT("FVectorPropertySection", "YArea", "Y"), MakeShareable(new FFloatCurveKeyArea(&VectorSection->GetCurve(1), VectorSection)));
		if (ChannelsUsed >= 3)
		{
			LayoutBuilder.AddKeyArea("Vector.Z", NSLOCTEXT("FVectorPropertySection", "ZArea", "Z"), MakeShareable(new FFloatCurveKeyArea(&VectorSection->GetCurve(2), VectorSection)));
		}
		if (ChannelsUsed >= 4)
		{
			LayoutBuilder.AddKeyArea("Vector.W", NSLOCTEXT("FVectorPropertySection", "WArea", "W"), MakeShareable(new FFloatCurveKeyArea(&VectorSection->GetCurve(3), VectorSection)));
		}
	}
};

/**
 * An implementation of bool property sections
 */
class FBoolPropertySection : public FPropertySection
{
public:
	FBoolPropertySection( UMovieSceneSection& InSectionObject, FName SectionName, ISequencer* InSequencer )
		: FPropertySection(InSectionObject, SectionName)
	, Sequencer(InSequencer) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override
	{
		UMovieSceneBoolSection* BoolSection = Cast<UMovieSceneBoolSection>( &SectionObject );
		LayoutBuilder.SetSectionAsKeyArea( MakeShareable( new FBoolKeyArea( BoolSection->GetCurve(), BoolSection ) ) );
	}

	virtual int32 OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const override 
	{
		FTimeToPixel TimeToPixelConverter = SectionObject.IsInfinite() ? 
			FTimeToPixel( AllottedGeometry, Sequencer->GetViewRange() ) : 
			FTimeToPixel( AllottedGeometry, TRange<float>( SectionObject.GetStartTime(), SectionObject.GetEndTime() ) );

		// custom drawing for bool curves
		UMovieSceneBoolSection* BoolSection = Cast<UMovieSceneBoolSection>( &SectionObject );

		TArray<float> SectionSwitchTimes;

		if (SectionObject.IsInfinite() && Sequencer->GetViewRange().GetLowerBoundValue() < SectionObject.GetStartTime())
		{
			SectionSwitchTimes.Add(Sequencer->GetViewRange().GetLowerBoundValue());
		}
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

		if (SectionObject.IsInfinite() && Sequencer->GetViewRange().GetUpperBoundValue() > SectionObject.GetEndTime())
		{
			SectionSwitchTimes.Add(Sequencer->GetViewRange().GetUpperBoundValue());
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
private:
	ISequencer* Sequencer;
};

/**
* An implementation of byte property sections
*/
class FBytePropertySection : public FPropertySection
{
public:
	FBytePropertySection(UMovieSceneSection& InSectionObject, FName InSectionName, UEnum* InEnum)
		: FPropertySection(InSectionObject, InSectionName)
		, Enum(InEnum) {}

	virtual void GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder) const override
	{
		UMovieSceneByteSection* ByteSection = Cast<UMovieSceneByteSection>(&SectionObject);
		if (Enum != nullptr)
		{
			LayoutBuilder.SetSectionAsKeyArea(MakeShareable(new FEnumKeyArea(ByteSection->GetCurve(), ByteSection, Enum)));
		}
		else
		{
			LayoutBuilder.SetSectionAsKeyArea(MakeShareable(new FIntegralKeyArea(ByteSection->GetCurve(), ByteSection)));
		}
	}

private:
	UEnum* Enum;
};

FPropertyTrackEditor::FPropertyTrackEditor( TSharedRef<ISequencer> InSequencer )
	: FMovieSceneTrackEditor( InSequencer ) 
{
	// Get the object change listener for the sequencer and register a delegates for when properties change that we care about
	ISequencerObjectChangeListener& ObjectChangeListener = InSequencer->GetObjectChangeListener();
	ObjectChangeListener.GetOnAnimatablePropertyChanged( NAME_FloatProperty ).AddRaw( this, &FPropertyTrackEditor::OnAnimatedPropertyChanged<float, UMovieSceneFloatTrack> );
	ObjectChangeListener.GetOnAnimatablePropertyChanged( NAME_BoolProperty ).AddRaw( this, &FPropertyTrackEditor::OnAnimatedBoolPropertyChanged<bool, UMovieSceneBoolTrack> );
	ObjectChangeListener.GetOnAnimatablePropertyChanged( NAME_ByteProperty ).AddRaw( this, &FPropertyTrackEditor::OnAnimatedPropertyChanged<uint8, UMovieSceneByteTrack> );
	ObjectChangeListener.GetOnAnimatablePropertyChanged( NAME_Vector ).AddRaw( this, &FPropertyTrackEditor::OnAnimatedVectorPropertyChanged );
	ObjectChangeListener.GetOnAnimatablePropertyChanged( NAME_Vector4 ).AddRaw( this, &FPropertyTrackEditor::OnAnimatedVectorPropertyChanged );
	ObjectChangeListener.GetOnAnimatablePropertyChanged( NAME_Vector2D ).AddRaw( this, &FPropertyTrackEditor::OnAnimatedVectorPropertyChanged );
	ObjectChangeListener.GetOnAnimatablePropertyChanged( NAME_Color ).AddRaw( this, &FPropertyTrackEditor::OnAnimatedColorPropertyChanged );
	ObjectChangeListener.GetOnAnimatablePropertyChanged( NAME_LinearColor ).AddRaw( this, &FPropertyTrackEditor::OnAnimatedColorPropertyChanged );
	ObjectChangeListener.GetOnAnimatablePropertyChanged( "SlateColor").AddRaw( this, &FPropertyTrackEditor::OnAnimatedColorPropertyChanged );

}

FPropertyTrackEditor::~FPropertyTrackEditor()
{
	TSharedPtr<ISequencer> Sequencer = GetSequencer();
	if( Sequencer.IsValid() )
	{
		ISequencerObjectChangeListener& ObjectChangeListener = Sequencer->GetObjectChangeListener();
		ObjectChangeListener.GetOnAnimatablePropertyChanged(NAME_FloatProperty).RemoveAll(this);
		ObjectChangeListener.GetOnAnimatablePropertyChanged(NAME_BoolProperty).RemoveAll(this);
		ObjectChangeListener.GetOnAnimatablePropertyChanged(NAME_ByteProperty).RemoveAll(this);
		ObjectChangeListener.GetOnAnimatablePropertyChanged(NAME_Vector).RemoveAll(this);
		ObjectChangeListener.GetOnAnimatablePropertyChanged(NAME_Vector4).RemoveAll(this);
		ObjectChangeListener.GetOnAnimatablePropertyChanged(NAME_Vector2D).RemoveAll(this);
		ObjectChangeListener.GetOnAnimatablePropertyChanged(NAME_Color).RemoveAll(this);
		ObjectChangeListener.GetOnAnimatablePropertyChanged(NAME_LinearColor).RemoveAll(this);
		ObjectChangeListener.GetOnAnimatablePropertyChanged("SlateColor").RemoveAll(this);
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
		Type == UMovieSceneByteTrack::StaticClass() ||
		Type == UMovieSceneVectorTrack::StaticClass() ||
		Type == UMovieSceneColorTrack::StaticClass() ||
		Type == UMovieSceneVisibilityTrack::StaticClass();
}

TSharedRef<ISequencerSection> FPropertyTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track )
{
	check( SupportsType( SectionObject.GetOuter()->GetClass() ) );

	UClass* SectionClass = SectionObject.GetOuter()->GetClass();

	TSharedRef<ISequencerSection> NewSection = TSharedRef<ISequencerSection>(
		SectionClass == UMovieSceneColorTrack::StaticClass() ? new FColorPropertySection( SectionObject, Track->GetTrackName() ) :
		SectionClass == UMovieSceneBoolTrack::StaticClass() ? new FBoolPropertySection( SectionObject, Track->GetTrackName(), GetSequencer().Get() ) :
		SectionClass == UMovieSceneByteTrack::StaticClass() ? new FBytePropertySection(SectionObject, Track->GetTrackName(), Cast<UMovieSceneByteTrack>(SectionObject.GetOuter())->GetEnum()) :
		SectionClass == UMovieSceneVectorTrack::StaticClass() ? new FVectorPropertySection( SectionObject, Track->GetTrackName() ) :
		SectionClass == UMovieSceneFloatTrack::StaticClass() ? new FFloatPropertySection( SectionObject, Track->GetTrackName() ) :
		SectionClass == UMovieSceneVisibilityTrack::StaticClass() ? new FBoolPropertySection( SectionObject, Track->GetTrackName(), GetSequencer().Get() ) :
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

UEnum* GetEnumForByteTrack(TSharedPtr<ISequencer> Sequencer, const FGuid& OwnerObjectHandle, FName PropertyName, UMovieSceneByteTrack* ByteTrack)
{
	TArray<UObject*> RuntimeObjects;
	Sequencer->GetObjectBindingManager()->GetRuntimeObjects(Sequencer->GetFocusedMovieSceneInstance(), OwnerObjectHandle, RuntimeObjects);

	TSet<UEnum*> PropertyEnums;
	for (UObject* RuntimeObject : RuntimeObjects)
	{
		UProperty* Property = RuntimeObject->GetClass()->FindPropertyByName(PropertyName);
		if (Property != nullptr)
		{
			UByteProperty* ByteProperty = Cast<UByteProperty>(Property);
			if (ByteProperty != nullptr && ByteProperty->Enum != nullptr)
			{
				PropertyEnums.Add(ByteProperty->Enum);
			}
		}
	}

	UEnum* TrackEnum;
	if (PropertyEnums.Num() == 1)
	{
		TrackEnum = PropertyEnums.Array()[0];
	}
	else
	{
		TrackEnum = nullptr;
	}
	return TrackEnum;
}

UMovieSceneTrack* FPropertyTrackEditor::AddTrack(UMovieScene* FocusedMovieScene, const FGuid& ObjectHandle, TSubclassOf<class UMovieSceneTrack> TrackClass, FName UniqueTypeName)
{
	UMovieSceneTrack* NewTrack = FMovieSceneTrackEditor::AddTrack(FocusedMovieScene, ObjectHandle, TrackClass, UniqueTypeName);
	if (NewTrack->GetClass() == UMovieSceneByteTrack::StaticClass())
	{
		UMovieSceneByteTrack* ByteTrack = Cast<UMovieSceneByteTrack>(NewTrack);
		UEnum* TrackEnum = GetEnumForByteTrack(GetSequencer(), ObjectHandle, UniqueTypeName, ByteTrack);
		if (TrackEnum != nullptr)
		{
			ByteTrack->SetEnum(TrackEnum);
		}
	}
	return NewTrack;
}

template <typename Type, typename TrackType>
void FPropertyTrackEditor::OnAnimatedPropertyChanged(const FPropertyChangedParams& PropertyChangedParams )
{
	// Get the value from the property 
	Type Value = *PropertyChangedParams.GetPropertyValue<Type>();

	AnimatablePropertyChanged(TrackType::StaticClass(), PropertyChangedParams.bRequireAutoKey,
		FOnKeyProperty::CreateRaw(this, &FPropertyTrackEditor::OnKeyProperty<Type, TrackType>, PropertyChangedParams, Value) );
}

template <typename Type, typename TrackType>
void FPropertyTrackEditor::OnAnimatedBoolPropertyChanged(const FPropertyChangedParams& PropertyChangedParams)
{
	// Get the value from the property 
	const UBoolProperty* BoolProperty = Cast<const UBoolProperty>(PropertyChangedParams.PropertyPath.Last());
	if (BoolProperty)
	{
		bool BoolValue = BoolProperty->GetPropertyValue(BoolProperty->ContainerPtrToValuePtr<void>(static_cast<const void*>(PropertyChangedParams.ObjectsThatChanged.Last())));

		AnimatablePropertyChanged(TrackType::StaticClass(), PropertyChangedParams.bRequireAutoKey,
			FOnKeyProperty::CreateRaw(this, &FPropertyTrackEditor::OnKeyProperty<Type, TrackType>, PropertyChangedParams, BoolValue));
	}
}

void FPropertyTrackEditor::OnAnimatedVectorPropertyChanged(const FPropertyChangedParams& PropertyChangedParams)
{
	bool bIsVector2D = false, bIsVector = false, bIsVector4 = false;
	const UStructProperty* StructProp = Cast<const UStructProperty>(PropertyChangedParams.PropertyPath.Last());
	if (StructProp && StructProp->Struct)
	{
		FName StructName = StructProp->Struct->GetFName();

		bIsVector2D = StructName == NAME_Vector2D;
		bIsVector = StructName == NAME_Vector;
		bIsVector4 = StructName == NAME_Vector4;
	}
	if (!bIsVector2D && !bIsVector && !bIsVector4) 
	{
		return;
	}

	// Get the vector value from the property
	if (bIsVector2D)
	{
		FVectorKey<FVector2D> NewKey;
		NewKey.CurveName = PropertyChangedParams.StructPropertyNameToKey;
		NewKey.bAddKeyEvenIfUnchanged = !PropertyChangedParams.bRequireAutoKey;
		NewKey.Value = *PropertyChangedParams.GetPropertyValue<FVector2D>();

		FOnKeyProperty OnKeyPropertyDelegate = FOnKeyProperty::CreateRaw( this, &FPropertyTrackEditor::OnKeyProperty<FVectorKey<FVector2D>, UMovieSceneVectorTrack>, PropertyChangedParams, NewKey );
		AnimatablePropertyChanged(UMovieSceneVectorTrack::StaticClass(), PropertyChangedParams.bRequireAutoKey, OnKeyPropertyDelegate);
	}
	else if (bIsVector)
	{
		FVectorKey<FVector> NewKey;
		NewKey.CurveName = PropertyChangedParams.StructPropertyNameToKey;
		NewKey.bAddKeyEvenIfUnchanged = !PropertyChangedParams.bRequireAutoKey;
		NewKey.Value = *PropertyChangedParams.GetPropertyValue<FVector>();

		FOnKeyProperty OnKeyPropertyDelegate = FOnKeyProperty::CreateRaw(this, &FPropertyTrackEditor::OnKeyProperty<FVectorKey<FVector>, UMovieSceneVectorTrack>, PropertyChangedParams, NewKey );
		AnimatablePropertyChanged(UMovieSceneVectorTrack::StaticClass(), PropertyChangedParams.bRequireAutoKey, OnKeyPropertyDelegate);
	}
	else if (bIsVector4)
	{
		FVectorKey<FVector4> NewKey;
		NewKey.CurveName = PropertyChangedParams.StructPropertyNameToKey;
		NewKey.bAddKeyEvenIfUnchanged = !PropertyChangedParams.bRequireAutoKey;
		NewKey.Value = *PropertyChangedParams.GetPropertyValue<FVector4>();

		FOnKeyProperty OnKeyPropertyDelegate = FOnKeyProperty::CreateRaw(this, &FPropertyTrackEditor::OnKeyProperty<FVectorKey<FVector4>, UMovieSceneVectorTrack>, PropertyChangedParams, NewKey );
		AnimatablePropertyChanged(UMovieSceneVectorTrack::StaticClass(), PropertyChangedParams.bRequireAutoKey, OnKeyPropertyDelegate );
	}
}

void FPropertyTrackEditor::OnAnimatedColorPropertyChanged(const FPropertyChangedParams& PropertyChangedParams )
{
	bool bIsFColor = false, bIsFLinearColor = false, bIsSlateColor = false;
	const UStructProperty* StructProp = Cast<const UStructProperty>(PropertyChangedParams.PropertyPath.Last());
	if (StructProp && StructProp->Struct)
	{
		FName StructName = StructProp->Struct->GetFName();

		bIsFColor = StructName == NAME_Color;
		bIsFLinearColor = StructName == NAME_LinearColor;
		bIsSlateColor = StructName == FName("SlateColor");
	}

	if (!bIsFColor && !bIsFLinearColor && !bIsSlateColor) 
	{
		return;
	}

	FName PropertyName = PropertyChangedParams.PropertyPath.Last()->GetFName();

	FLinearColor ColorValue;
	if (bIsFColor)
	{
		ColorValue = PropertyChangedParams.GetPropertyValue<FColor>()->ReinterpretAsLinear();
	}
	else
	{
		ColorValue = *PropertyChangedParams.GetPropertyValue<FLinearColor>();
	}

	if( StructProp->HasMetaData("HideAlphaChannel") )
	{
		ColorValue.A = 1;
	}


	FColorKey Key;
	Key.Value = ColorValue;
	Key.CurveName = PropertyChangedParams.StructPropertyNameToKey;
	Key.bAddKeyEvenIfUnchanged = !PropertyChangedParams.bRequireAutoKey;
	Key.bIsSlateColor = bIsSlateColor;

	AnimatablePropertyChanged(UMovieSceneColorTrack::StaticClass(), PropertyChangedParams.bRequireAutoKey, FOnKeyProperty::CreateRaw(this, &FPropertyTrackEditor::OnKeyProperty<FColorKey, UMovieSceneColorTrack>, PropertyChangedParams, Key) );
}


template <typename Type, typename TrackType>
void FPropertyTrackEditor::OnKeyProperty( float KeyTime, FPropertyChangedParams PropertyChangedParams, Type Value )
{
	for( UObject* Object : PropertyChangedParams.ObjectsThatChanged )
	{
		FGuid ObjectHandle = FindOrCreateHandleToObject( Object );
		if (ObjectHandle.IsValid())
		{
			FName PropertyName = PropertyChangedParams.PropertyPath.Last()->GetFName();

			// Look for a customized track class for this property on the meta data
			const FString& MetaSequencerTrackClass = PropertyChangedParams.PropertyPath.Last()->GetMetaData(TEXT("SequencerTrackClass"));			
			TSubclassOf<UMovieSceneTrack> SequencerTrackClass = TrackType::StaticClass();
			if (!MetaSequencerTrackClass.IsEmpty())
			{
				UClass* MetaClass = FindObject<UClass>(ANY_PACKAGE, *MetaSequencerTrackClass);
				if (!MetaClass)
				{
					MetaClass = LoadObject<UClass>(nullptr, *MetaSequencerTrackClass);
				}
				if (MetaClass != NULL)
					SequencerTrackClass = MetaClass;
			}

			UMovieSceneTrack* Track = GetTrackForObject( ObjectHandle, SequencerTrackClass, PropertyName );
			if( ensure( Track ) )
			{
				TrackType* TypedTrack = CastChecked<TrackType>(Track);

				TypedTrack->SetPropertyNameAndPath( PropertyName, PropertyChangedParams.GetPropertyPathString() );
				// Find or add a new section at the auto-key time and changing the property same property
				// AddKeyToSection is not actually a virtual, it's redefined in each class with a different type
				bool bSuccessfulAdd = TypedTrack->AddKeyToSection( KeyTime, Value );
				if (bSuccessfulAdd)
				{
					TypedTrack->SetAsShowable();
				}
			}
		}
	}
}

