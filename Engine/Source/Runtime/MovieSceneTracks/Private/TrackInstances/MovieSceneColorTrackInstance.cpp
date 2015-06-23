// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneColorTrackInstance.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieSceneColorTrack.h"
#include "SlateCore.h"


FMovieSceneColorTrackInstance::FMovieSceneColorTrackInstance( UMovieSceneColorTrack& InColorTrack )
{
	ColorTrack = &InColorTrack;

	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( ColorTrack->GetPropertyName(), ColorTrack->GetPropertyPath() ) );
}

void FMovieSceneColorTrackInstance::SaveState(const TArray<UObject*>& RuntimeObjects)
{
	for( UObject* Object : RuntimeObjects )
	{
		if( ColorTrack->IsSlateColor() )
		{
			FSlateColor ColorValue = PropertyBindings->GetCurrentValue<FSlateColor>(Object);
			InitSlateColorMap.Add(Object, ColorValue);
		}
		else
		{
			FLinearColor ColorValue = PropertyBindings->GetCurrentValue<FLinearColor>(Object);
			InitLinearColorMap.Add(Object, ColorValue);
		}
	}
}

void FMovieSceneColorTrackInstance::RestoreState(const TArray<UObject*>& RuntimeObjects)
{
	for( UObject* Object : RuntimeObjects )
	{
		if( ColorTrack->IsSlateColor() )
		{
			FSlateColor *ColorValue = InitSlateColorMap.Find(Object);
			if (ColorValue != NULL)
			{
				PropertyBindings->CallFunction(Object, ColorValue);
			}
		}
		else
		{
			FLinearColor *ColorValue = InitLinearColorMap.Find(Object);
			if (ColorValue != NULL)
			{
				PropertyBindings->CallFunction(Object, ColorValue);
			}
		}
	}

	PropertyBindings->UpdateBindings( RuntimeObjects );
}

void FMovieSceneColorTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	for(UObject* Object : RuntimeObjects)
	{
		if( ColorTrack->IsSlateColor() )
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
			FLinearColor ColorValue = PropertyBindings->GetCurrentValue<FLinearColor>(Object);
			if(ColorTrack->Eval(Position, LastPosition, ColorValue))
			{
				PropertyBindings->CallFunction(Object, &ColorValue);
			}
		}
	}
}

void FMovieSceneColorTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	PropertyBindings->UpdateBindings( RuntimeObjects );
}
