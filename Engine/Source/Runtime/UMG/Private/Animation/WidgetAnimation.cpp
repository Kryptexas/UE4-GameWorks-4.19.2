// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieScene.h"
#include "WidgetAnimation.h"


/* UWidgetAnimation structors
 *****************************************************************************/

UWidgetAnimation::UWidgetAnimation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{ }


/* UWidgetAnimation interface
 *****************************************************************************/

#if WITH_EDITOR

UWidgetAnimation* UWidgetAnimation::GetNullAnimation()
{
	static UWidgetAnimation* NullAnimation = nullptr;

	if (!NullAnimation)
	{
		NullAnimation = NewObject<UWidgetAnimation>(GetTransientPackage(), NAME_None, RF_RootSet);
		NullAnimation->MovieScene = NewObject<UMovieScene>(NullAnimation, FName("No Animation"), RF_RootSet);
	}

	return NullAnimation;
}

#endif


float UWidgetAnimation::GetStartTime() const
{
	return MovieScene->GetTimeRange().GetLowerBoundValue();
}


float UWidgetAnimation::GetEndTime() const
{
	return MovieScene->GetTimeRange().GetUpperBoundValue();
}
