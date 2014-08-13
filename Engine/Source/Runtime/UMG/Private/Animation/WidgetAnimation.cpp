// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieScene.h"
#include "WidgetAnimation.h"

UWidgetAnimation::UWidgetAnimation(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

#if WITH_EDITOR

UWidgetAnimation* UWidgetAnimation::GetNullAnimation()
{
	static UWidgetAnimation* NullAnimation = nullptr;
	if( !NullAnimation )
	{
		NullAnimation = ConstructObject<UWidgetAnimation>( UWidgetAnimation::StaticClass(), GetTransientPackage(), NAME_None, RF_RootSet );
		NullAnimation->MovieScene = ConstructObject<UMovieScene>( UMovieScene::StaticClass(), NullAnimation, FName("No Animation"), RF_RootSet );
	}

	return NullAnimation;
}

#endif