// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneSequence.h"
#include "Evaluation/MovieSceneEvaluationCustomVersion.h"
#include "Evaluation/MovieSceneSequenceTemplateStore.h"
#include "MovieScene.h"
#include "EditorObjectVersion.h"
#include "Tracks/MovieSceneSubTrack.h"
#include "Sections/MovieSceneSubSection.h"
#include "Compilation/MovieSceneCompiler.h"

UMovieSceneSequence::UMovieSceneSequence(const FObjectInitializer& Init)
	: Super(Init)
{
	bParentContextsAreSignificant = false;
}

#if WITH_EDITORONLY_DATA
void UMovieSceneSequence::PostDuplicate(bool bDuplicateForPIE)
{
	if (bDuplicateForPIE)
	{
		FMovieSceneSequencePrecompiledTemplateStore Store;
		FMovieSceneCompiler::Compile(*this, Store);
	}

	Super::PostDuplicate(bDuplicateForPIE);
}
#endif // WITH_EDITORONLY_DATA

void UMovieSceneSequence::PostLoad()
{
#if WITH_EDITORONLY_DATA
	// Wipe compiled data on editor load to ensure we don't try and iteratively compile previously saved content. In a cooked game, this will contain our up-to-date compiled template.
	PrecompiledEvaluationTemplate = FMovieSceneEvaluationTemplate();
#endif

	Super::PostLoad();
}

void UMovieSceneSequence::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FMovieSceneEvaluationCustomVersion::GUID);
	Ar.UsingCustomVersion(FEditorObjectVersion::GUID);

#if WITH_EDITORONLY_DATA
	if (Ar.IsCooking() && !HasAnyFlags(RF_ClassDefaultObject))
	{
		FMovieSceneSequencePrecompiledTemplateStore Store;
		FMovieSceneCompiler::Compile(*this, Store);
	}
	else if (Ar.IsSaving())
	{
		// Don't save template data unless we're cooking
		PrecompiledEvaluationTemplate = FMovieSceneEvaluationTemplate();
	}
#endif
	
	Super::Serialize(Ar);
}

FGuid UMovieSceneSequence::FindPossessableObjectId(UObject& Object, UObject* Context) const
{
	UMovieScene* MovieScene = GetMovieScene();
	if (!MovieScene)
	{
		return FGuid();
	}

	// Search all possessables
	for (int32 Index = 0; Index < MovieScene->GetPossessableCount(); ++Index)
	{
		FGuid ThisGuid = MovieScene->GetPossessable(Index).GetGuid();
		if (LocateBoundObjects(ThisGuid, Context).Contains(&Object))
		{
			return ThisGuid;
		}
	}
	return FGuid();
}
