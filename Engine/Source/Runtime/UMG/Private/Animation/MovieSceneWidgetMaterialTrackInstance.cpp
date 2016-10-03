// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieSceneWidgetMaterialTrack.h"
#include "MovieSceneWidgetMaterialTrackInstance.h"
#include "WidgetMaterialTrackUtilities.h"


FMovieSceneWidgetMaterialTrackInstance::FMovieSceneWidgetMaterialTrackInstance( UMovieSceneWidgetMaterialTrack& InMaterialTrack )
	: FMovieSceneMaterialTrackInstance( InMaterialTrack )
	, BrushPropertyNamePath( InMaterialTrack.GetBrushPropertyNamePath() )
{
}


UMaterialInterface* FMovieSceneWidgetMaterialTrackInstance::GetMaterialForObject( UObject* Object ) const 
{
	UWidget* Widget = Cast<UWidget>(Object);
	if ( Widget != nullptr )
	{
		FWidgetMaterialHandle Handle = WidgetMaterialTrackUtilities::GetMaterialHandle( Widget, BrushPropertyNamePath );
		if (Handle.IsValid() )
		{
			return Handle.GetMaterial();
		}
	}
	return nullptr;
}


void FMovieSceneWidgetMaterialTrackInstance::SetMaterialForObject( UObject* Object, UMaterialInterface* Material )
{
	UWidget* Widget = Cast<UWidget>( Object );
	if ( Widget != nullptr )
	{
		FWidgetMaterialHandle Handle = WidgetMaterialTrackUtilities::GetMaterialHandle(Widget, BrushPropertyNamePath);
		Handle.SetMaterial(Material);
	}
}
