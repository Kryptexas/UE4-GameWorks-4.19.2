// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MovieSceneTrack.h"
#include "MovieSceneSection.h"
#include "MovieSceneEvalTemplate.h"
#include "MovieSceneSegmentCompilerTests.generated.h"

USTRUCT()
struct FTestMovieSceneEvalTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()

	virtual UScriptStruct& GetScriptStructImpl() const { return *StaticStruct(); }
};

UCLASS(MinimalAPI)
class UMovieSceneSegmentCompilerTestTrack : public UMovieSceneTrack
{
public:

	GENERATED_BODY()

	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override { return SectionArray; }
	virtual FMovieSceneEvalTemplatePtr CreateTemplateForSection(const UMovieSceneSection& InSection) const override;
	virtual FMovieSceneTrackSegmentBlenderPtr GetTrackSegmentBlender() const override;

	UPROPERTY()
	bool bHighPassFilter;

	UPROPERTY()
	TArray<UMovieSceneSection*> SectionArray;
};

UCLASS(MinimalAPI)
class UMovieSceneSegmentCompilerTestSection : public UMovieSceneSection
{
public:
	GENERATED_BODY()
};

