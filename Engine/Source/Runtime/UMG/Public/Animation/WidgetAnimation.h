// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieScenePlayer.h"
#include "WidgetAnimationBinding.h"
#include "WidgetAnimation.generated.h"


class UMovieScene;


UCLASS(BlueprintType, MinimalAPI)
class UWidgetAnimation
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

#if WITH_EDITOR
	/**
	 * Get a placeholder animation.
	 *
	 * @return Placeholder animation.
	 */
	static UMG_API UWidgetAnimation* GetNullAnimation();
#endif

	/**
	 * Get the start time of this animation.
	 *
	 * @return Start time in seconds.
	 * @see GetEndTime
	 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	UMG_API float GetStartTime() const;

	/**
	 * Get the end time of this animation.
	 *
	 * @return End time in seconds.
	 * @see GetStartTime
	 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	UMG_API float GetEndTime() const;

public:

	UPROPERTY()
	UMovieScene* MovieScene;

	UPROPERTY()
	TArray<FWidgetAnimationBinding> AnimationBindings;
};
