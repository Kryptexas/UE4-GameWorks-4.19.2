// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieScenePlayer.h"
#include "WidgetAnimationBinding.h"
#include "WidgetAnimation.generated.h"


class UMovieScene;
class UWidgetTree;


UCLASS(BlueprintType, MinimalAPI)
class UWidgetAnimation
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

#if WITH_EDITOR
	static UMG_API UWidgetAnimation* GetNullAnimation();
#endif

	UFUNCTION(BlueprintCallable, Category="Animation")
	UMG_API float GetStartTime() const;

	UFUNCTION(BlueprintCallable, Category="Animation")
	UMG_API float GetEndTime() const;

public:

	UPROPERTY()
	UMovieScene* MovieScene;

	UPROPERTY()
	TArray<FWidgetAnimationBinding> AnimationBindings;
};
