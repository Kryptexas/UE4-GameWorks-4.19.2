// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "Sections/MovieSceneSkeletalAnimationSection.h"
#include "Compilation/MovieSceneCompilerRules.h"
#include "Evaluation/MovieSceneEvaluationTrack.h"
#include "Evaluation/MovieSceneSkeletalAnimationTemplate.h"
#include "Compilation/IMovieSceneTemplateGenerator.h"

#define LOCTEXT_NAMESPACE "MovieSceneSkeletalAnimationTrack"


/* UMovieSceneSkeletalAnimationTrack structors
 *****************************************************************************/

UMovieSceneSkeletalAnimationTrack::UMovieSceneSkeletalAnimationTrack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	TrackTint = FColor(124, 15, 124, 65);
#endif

	EvalOptions.bEvaluateNearestSection = EvalOptions.bCanEvaluateNearestSection = true;
}


/* UMovieSceneSkeletalAnimationTrack interface
 *****************************************************************************/

void UMovieSceneSkeletalAnimationTrack::AddNewAnimation(float KeyTime, UAnimSequenceBase* AnimSequence)
{
	UMovieSceneSkeletalAnimationSection* NewSection = Cast<UMovieSceneSkeletalAnimationSection>(CreateNewSection());
	{
		NewSection->InitialPlacement(AnimationSections, KeyTime, KeyTime + AnimSequence->SequenceLength, SupportsMultipleRows());
		NewSection->Params.Animation = AnimSequence;
	}

	AddSection(*NewSection);
}


TArray<UMovieSceneSection*> UMovieSceneSkeletalAnimationTrack::GetAnimSectionsAtTime(float Time)
{
	TArray<UMovieSceneSection*> Sections;
	for (auto Section : AnimationSections)
	{
		if (Section->IsTimeWithinSection(Time))
		{
			Sections.Add(Section);
		}
	}

	return Sections;
}


/* UMovieSceneTrack interface
 *****************************************************************************/


const TArray<UMovieSceneSection*>& UMovieSceneSkeletalAnimationTrack::GetAllSections() const
{
	return AnimationSections;
}


bool UMovieSceneSkeletalAnimationTrack::SupportsMultipleRows() const
{
	return true;
}


UMovieSceneSection* UMovieSceneSkeletalAnimationTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSkeletalAnimationSection>(this);
}


void UMovieSceneSkeletalAnimationTrack::RemoveAllAnimationData()
{
	AnimationSections.Empty();
}


bool UMovieSceneSkeletalAnimationTrack::HasSection(const UMovieSceneSection& Section) const
{
	return AnimationSections.Contains(&Section);
}


void UMovieSceneSkeletalAnimationTrack::AddSection(UMovieSceneSection& Section)
{
	AnimationSections.Add(&Section);
}


void UMovieSceneSkeletalAnimationTrack::RemoveSection(UMovieSceneSection& Section)
{
	AnimationSections.Remove(&Section);
}


bool UMovieSceneSkeletalAnimationTrack::IsEmpty() const
{
	return AnimationSections.Num() == 0;
}


TRange<float> UMovieSceneSkeletalAnimationTrack::GetSectionBoundaries() const
{
	TArray<TRange<float>> Bounds;

	for (auto Section : AnimationSections)
	{
		Bounds.Add(Section->GetRange());
	}

	return TRange<float>::Hull(Bounds);
}

#if WITH_EDITORONLY_DATA

FText UMovieSceneSkeletalAnimationTrack::GetDefaultDisplayName() const
{
	return LOCTEXT("TrackName", "Animation");
}

#endif

TInlineValue<FMovieSceneSegmentCompilerRules> UMovieSceneSkeletalAnimationTrack::GetRowCompilerRules() const
{
	// Apply an upper bound exclusive blend
	struct FSkeletalAnimationRowCompilerRules : FMovieSceneSegmentCompilerRules
	{
		FSkeletalAnimationRowCompilerRules(TInlineValue<FMovieSceneSegmentCompilerRules>&& InParentCompilerRules) { ParentCompilerRules = MoveTemp(InParentCompilerRules); }

#if PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS
		FSkeletalAnimationRowCompilerRules(FSkeletalAnimationRowCompilerRules&&) = default;
		FSkeletalAnimationRowCompilerRules& operator=(FSkeletalAnimationRowCompilerRules&&) = default;
#else
		FSkeletalAnimationRowCompilerRules(FSkeletalAnimationRowCompilerRules&& RHS)
			: FMovieSceneSegmentCompilerRules(MoveTemp(RHS))
			, ParentCompilerRules(MoveTemp(RHS.ParentCompilerRules))
		{}
		FSkeletalAnimationRowCompilerRules& operator=(FSkeletalAnimationRowCompilerRules&& RHS)
		{
			FMovieSceneSegmentCompilerRules::operator=(MoveTemp(RHS));
			ParentCompilerRules = MoveTemp(RHS.ParentCompilerRules);
			return *this;
		}
#endif
		virtual void BlendSegment(FMovieSceneSegment& Segment, const TArrayView<const FMovieSceneSectionData>& SourceData) const
		{
			// Make the skeletal animation sections upper bound exclusive so that when you place animation 
			// sections back to back, they don't blend at the transition times. This might be an option that 
			// is exposed to the user in the future.

			MovieSceneSegmentCompiler::BlendSegmentUpperBoundExclusive(Segment, SourceData);

			ParentCompilerRules.GetValue().BlendSegment(Segment, SourceData);
		}

		TInlineValue<FMovieSceneSegmentCompilerRules> ParentCompilerRules;
	};
	return FSkeletalAnimationRowCompilerRules(Super::GetRowCompilerRules());
}

void UMovieSceneSkeletalAnimationTrack::PostCompile(FMovieSceneEvaluationTrack& OutTrack, const FMovieSceneTrackCompilerArgs& Args) const
{
	FMovieSceneSharedDataId UniqueId = FMovieSceneSkeletalAnimationSharedTrack::GetSharedDataKey().UniqueId;

	FMovieSceneEvaluationTrack ActuatorTemplate(Args.ObjectBindingId);
	ActuatorTemplate.DefineAsSingleTemplate(FMovieSceneSkeletalAnimationSharedTrack());
	ActuatorTemplate.SetEvaluationPriority(ActuatorTemplate.GetEvaluationPriority() - 1);
	Args.Generator.AddSharedTrack(MoveTemp(ActuatorTemplate), UniqueId, *this);
}

#undef LOCTEXT_NAMESPACE
