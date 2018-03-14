// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Animation/MovieSceneWidgetMaterialTemplate.h"
#include "Animation/WidgetMaterialTrackUtilities.h"
#include "Animation/MovieSceneWidgetMaterialTrack.h"
#include "Components/Widget.h"
#include "MovieSceneEvaluation.h"

// Container to ensure unique IDs per property path
TMovieSceneAnimTypeIDContainer<TArray<FName>> BrushPropertyIDs;

struct FWidgetMaterialAccessor : FDefaultMaterialAccessor
{
	FWidgetMaterialAccessor(const TArray<FName>& InBrushPropertyNamePath)
		: AnimTypeID(BrushPropertyIDs.GetAnimTypeID(InBrushPropertyNamePath))
		, BrushPropertyNamePath(InBrushPropertyNamePath)
	{}

	FMovieSceneAnimTypeID GetAnimTypeID() const
	{
		return AnimTypeID;
	}

	UMaterialInterface* GetMaterialForObject(UObject& Object)
	{
		if (UWidget* Widget = Cast<UWidget>(&Object))
		{
			FWidgetMaterialHandle Handle = WidgetMaterialTrackUtilities::GetMaterialHandle( Widget, BrushPropertyNamePath );
			if (Handle.IsValid())
			{
				return Handle.GetMaterial();
			}
		}
		return nullptr;
	}

	void SetMaterialForObject(UObject& Object, UMaterialInterface& Material)
	{
		if (UWidget* Widget = Cast<UWidget>(&Object))
		{
			FWidgetMaterialHandle Handle = WidgetMaterialTrackUtilities::GetMaterialHandle(Widget, BrushPropertyNamePath);
			Handle.SetMaterial(&Material);
		}
	}

	FMovieSceneAnimTypeID AnimTypeID;
	TArray<FName> BrushPropertyNamePath;
};

FMovieSceneWidgetMaterialSectionTemplate::FMovieSceneWidgetMaterialSectionTemplate(const UMovieSceneParameterSection& Section, const UMovieSceneWidgetMaterialTrack& Track)
	: FMovieSceneParameterSectionTemplate(Section)
	, BrushPropertyNamePath(Track.GetBrushPropertyNamePath())
{
}

void FMovieSceneWidgetMaterialSectionTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	TMaterialTrackExecutionToken<FWidgetMaterialAccessor> ExecutionToken(BrushPropertyNamePath);

	EvaluateCurves(Context, ExecutionToken.Values);

	ExecutionTokens.Add(MoveTemp(ExecutionToken));
}
