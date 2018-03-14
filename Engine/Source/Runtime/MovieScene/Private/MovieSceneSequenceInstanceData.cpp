// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneSequenceInstanceData.h"
#include "Evaluation/MovieSceneEvalTemplateBase.h"
#include "Evaluation/MovieSceneEvalTemplateSerializer.h"

FMovieSceneSequenceInstanceDataPtr::FMovieSceneSequenceInstanceDataPtr(const FMovieSceneSequenceInstanceDataPtr& RHS)
{
	*this = RHS;
}

FMovieSceneSequenceInstanceDataPtr& FMovieSceneSequenceInstanceDataPtr::operator=(const FMovieSceneSequenceInstanceDataPtr& RHS)
{
	if (RHS.IsValid())
	{
		UScriptStruct::ICppStructOps& StructOps = *RHS->GetScriptStruct().GetCppStructOps();

		void* Allocation = Reserve(StructOps.GetSize(), StructOps.GetAlignment());
		StructOps.Construct(Allocation);
		StructOps.Copy(Allocation, &RHS.GetValue(), 1);
	}

	return *this;
}

bool FMovieSceneSequenceInstanceDataPtr::Serialize(FArchive& Ar)
{
	return SerializeInlineValue(*this, Ar);
}