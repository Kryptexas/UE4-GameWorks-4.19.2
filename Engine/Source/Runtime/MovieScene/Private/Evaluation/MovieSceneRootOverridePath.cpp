// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneRootOverridePath.h"
#include "MovieSceneSequenceHierarchy.h"

void FMovieSceneRootOverridePath::Reset()
{
	ReverseOverrideRootPath.Reset();
}

void FMovieSceneRootOverridePath::Set(FMovieSceneSequenceID OverrideRootID, const FMovieSceneSequenceHierarchy& RootHierarchy)
{
	ReverseOverrideRootPath.Reset();

	FMovieSceneSequenceID CurrentSequenceID = OverrideRootID;

	while (CurrentSequenceID != MovieSceneSequenceID::Root)
	{
		const FMovieSceneSequenceHierarchyNode* CurrentNode = RootHierarchy.FindNode(CurrentSequenceID);
		const FMovieSceneSubSequenceData* OuterSubData = RootHierarchy.FindSubData(CurrentSequenceID);
		if (!ensureAlwaysMsgf(CurrentNode && OuterSubData, TEXT("Malformed sequence hierarchy")))
		{
			return;
		}

		ReverseOverrideRootPath.Add(OuterSubData->DeterministicSequenceID);
		CurrentSequenceID = CurrentNode->ParentID;
	}
}
