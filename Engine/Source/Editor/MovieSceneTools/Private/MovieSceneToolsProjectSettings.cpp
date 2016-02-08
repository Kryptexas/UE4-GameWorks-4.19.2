// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieSceneToolsProjectSettings.h"


UMovieSceneToolsProjectSettings::UMovieSceneToolsProjectSettings()
	: DefaultStartTime(0.f)
	, DefaultDuration(5.f)
	, ShotPrefix(TEXT("sht"))
	, FirstShotNumber(10)
	, ShotIncrement(10)
	, ShotNumDigits(4)
	, TakeNumDigits(3)
	, FirstTakeNumber(1)
	, TakeSeparator(TEXT("_"))
{ }
