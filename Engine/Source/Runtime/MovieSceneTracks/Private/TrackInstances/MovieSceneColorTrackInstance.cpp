// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneColorTrackInstance.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieSceneColorTrack.h"
#include "SlateCore.h"


FMovieSceneColorTrackInstance::FMovieSceneColorTrackInstance( UMovieSceneColorTrack& InColorTrack )
	: ColorTrack( &InColorTrack )
	, ColorType( EColorType::Linear )
{
	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( ColorTrack->GetPropertyName(), ColorTrack->GetPropertyPath() ) );
}

void FMovieSceneColorTrackInstance::SaveState(const TArray<UObject*>& RuntimeObjects)
{
	for( UObject* Object : RuntimeObjects )
	{
		if( ColorType == EColorType::Slate )
		{
			FSlateColor ColorValue = PropertyBindings->GetCurrentValue<FSlateColor>(Object);
			InitSlateColorMap.Add(Object, ColorValue);
		}
		else if( ColorType == EColorType::Linear )
		{
			UProperty* Property = PropertyBindings->GetProperty(Object);
			FLinearColor ColorValue = PropertyBindings->GetCurrentValue<FLinearColor>(Object);
			InitLinearColorMap.Add(Object, ColorValue);
		}
		else if( ColorType == EColorType::RegularColor )
		{
			UProperty* Property = PropertyBindings->GetProperty(Object);
			FColor ColorValue = PropertyBindings->GetCurrentValue<FColor>(Object);
			FLinearColor LinearColorValue = ColorValue.ReinterpretAsLinear();
			InitLinearColorMap.Add(Object, ColorValue);
		}
		else
		{
			checkf(0, TEXT("Invalid Color Type"));
		}
	}
}

void FMovieSceneColorTrackInstance::RestoreState(const TArray<UObject*>& RuntimeObjects)
{
	for( UObject* Object : RuntimeObjects )
	{
		if (!IsValid(Object))
		{
			continue;
		}

		if( ColorType == EColorType::Slate )
		{
			FSlateColor* ColorValue = InitSlateColorMap.Find(Object);
			if (ColorValue != NULL)
			{
				PropertyBindings->CallFunction(Object, ColorValue);
			}
		}
		else if( ColorType == EColorType::Linear || ColorType == EColorType::RegularColor )
		{
			// todo 
			FLinearColor* ColorValue = InitLinearColorMap.Find(Object);
			if (ColorValue != NULL)
			{
				PropertyBindings->CallFunction(Object, ColorValue);
			}
		}
		else
		{
			checkf(0, TEXT("Invalid Color Type"));
		}
	}

	PropertyBindings->UpdateBindings( RuntimeObjects );
}

void FMovieSceneColorTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	for(UObject* Object : RuntimeObjects)
	{
		if( ColorType == EColorType::Slate )
		{
			FSlateColor ColorValue = PropertyBindings->GetCurrentValue<FSlateColor>(Object);
			FLinearColor LinearColor = ColorValue.GetSpecifiedColor();
			if(ColorTrack->Eval(Position, LastPosition, LinearColor))
			{
				FSlateColor NewColor(LinearColor);
				PropertyBindings->CallFunction(Object, &NewColor);
			}
		}
		else
		{
			FLinearColor ColorValue = ColorType == EColorType::Linear ? PropertyBindings->GetCurrentValue<FLinearColor>(Object) : PropertyBindings->GetCurrentValue<FColor>(Object).ReinterpretAsLinear();
			if(ColorTrack->Eval(Position, LastPosition, ColorValue))
			{
				PropertyBindings->CallFunction(Object, &ColorValue);
			}
		}
	}
}

void FMovieSceneColorTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	if( RuntimeObjects.Num() > 0 )
	{
		PropertyBindings->UpdateBindings( RuntimeObjects );

		// Cache off what type of color this is. Just examine the first object since the property should be the same
		UProperty* ColorProp = PropertyBindings->GetProperty( RuntimeObjects[0] );

		const UStructProperty* StructProp = Cast<const UStructProperty>(ColorProp);
		if (StructProp && StructProp->Struct)
		{
			FName StructName = StructProp->Struct->GetFName();

			static const FName SlateColor("SlateColor");

			if( StructName == NAME_Color )
			{
				ColorType = EColorType::RegularColor;
			}
			else if( StructName == SlateColor )
			{
				ColorType = EColorType::Slate;
			}
			else
			{
				ColorType = EColorType::Linear;
			}
		}
	}

}
