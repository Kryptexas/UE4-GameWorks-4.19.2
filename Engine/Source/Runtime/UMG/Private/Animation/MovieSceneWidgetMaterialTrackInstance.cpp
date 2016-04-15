// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieSceneWidgetMaterialTrack.h"
#include "MovieSceneWidgetMaterialTrackInstance.h"
#include "WidgetMaterialTrackUtilities.h"


FMovieSceneWidgetMaterialTrackInstance::FMovieSceneWidgetMaterialTrackInstance( UMovieSceneWidgetMaterialTrack& InMaterialTrack )
	: FMovieSceneMaterialTrackInstance( InMaterialTrack )
	, BrushPropertyName( InMaterialTrack.GetBrushPropertyName() )
{
}


UMaterialInterface* FMovieSceneWidgetMaterialTrackInstance::GetMaterialForObject( UObject* Object ) const 
{
	UWidget* Widget = Cast<UWidget>(Object);
	if ( Widget != nullptr )
	{
		FSlateBrush* Brush = WidgetMaterialTrackUtilities::GetBrush( Widget, BrushPropertyName );
		return Cast<UMaterialInterface>( Brush->GetResourceObject() );
	}
	return nullptr;
}


void FMovieSceneWidgetMaterialTrackInstance::SetMaterialForObject( UObject* Object, UMaterialInterface* Material )
{
	UWidget* Widget = Cast<UWidget>( Object );
	if ( Widget != nullptr )
	{
		FSlateBrush* Brush = WidgetMaterialTrackUtilities::GetBrush( Widget, BrushPropertyName );
		Brush->SetResourceObject( Material );
	}
}
