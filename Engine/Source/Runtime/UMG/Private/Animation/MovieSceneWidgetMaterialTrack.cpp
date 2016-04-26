// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieSceneWidgetMaterialTrack.h"
#include "MovieSceneWidgetMaterialTrackInstance.h"


UMovieSceneWidgetMaterialTrack::UMovieSceneWidgetMaterialTrack( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
}


TSharedPtr<IMovieSceneTrackInstance> UMovieSceneWidgetMaterialTrack::CreateInstance()
{
	return MakeShareable(new FMovieSceneWidgetMaterialTrackInstance(*this));
}


#if WITH_EDITORONLY_DATA
FText UMovieSceneWidgetMaterialTrack::GetDefaultDisplayName() const
{
	return FText::Format(NSLOCTEXT("UMGAnimation", "MaterialTrackFormat", "{0} Material"), FText::FromName(BrushPropertyName));
}
#endif
