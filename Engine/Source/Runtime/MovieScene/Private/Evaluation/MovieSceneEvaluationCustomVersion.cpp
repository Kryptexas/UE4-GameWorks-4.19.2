// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieScenePrivatePCH.h"
#include "MovieSceneEvaluationCustomVersion.h"

// Register the custom version with core
FCustomVersionRegistration GRegisterMovieSceneEvaluationCustomVersion(FMovieSceneEvaluationCustomVersion::GUID, FMovieSceneEvaluationCustomVersion::LatestVersion, TEXT("MovieSceneEvaluation"));