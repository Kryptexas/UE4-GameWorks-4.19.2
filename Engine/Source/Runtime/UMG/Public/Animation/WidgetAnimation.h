// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieScenePlayer.h"
#include "WidgetAnimation.generated.h"

class UMovieScene;


/** A single object bound to a umg sequence */
USTRUCT()
struct FWidgetAnimationBinding
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName WidgetName;

	UPROPERTY()
	FName SlotWidgetName;

	UPROPERTY()
	FGuid AnimationGuid;

	bool operator==(const FWidgetAnimationBinding& Other) const
	{
		return WidgetName == Other.WidgetName && SlotWidgetName == Other.SlotWidgetName && AnimationGuid == Other.AnimationGuid;
	}

	friend FArchive& operator<<(FArchive& Ar, FWidgetAnimationBinding& Binding)
	{
		Ar << Binding.WidgetName;
		Ar << Binding.SlotWidgetName;
		Ar << Binding.AnimationGuid;
		return Ar;
	}
};


UCLASS(MinimalAPI)
class UWidgetAnimation : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	virtual void PostDuplicate(bool bDuplicateForPIE) override;

#if WITH_EDITOR
	static UMG_API UWidgetAnimation* GetNullAnimation();
#endif

public:
	UPROPERTY()
	UMovieScene* MovieScene;

	UPROPERTY()
	TArray<FWidgetAnimationBinding> AnimationBindings;
};
