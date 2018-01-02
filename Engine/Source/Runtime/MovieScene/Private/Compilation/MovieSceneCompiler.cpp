// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCompiler.h"

#include "IMovieSceneModule.h"
#include "MovieSceneEvaluationTemplate.h"
#include "MovieSceneEvaluationTemplateInstance.h"
#include "Compilation/MovieSceneEvaluationTemplateGenerator.h"

#include "MovieScene.h"
#include "MovieSceneSequence.h"
#include "Sections/MovieSceneSubSection.h"
#include "Tracks/MovieSceneSubTrack.h"

/** Parameter structure used for gathering entities for a given time or range */
struct FGatherParameters
{
	FGatherParameters(FMovieSceneRootOverridePath& InRootPath, FMovieSceneSequenceHierarchy& InRootHierarchy, IMovieSceneSequenceTemplateStore& InTemplateStore, TRange<float> InCompileRange)
		: RootPath(InRootPath)
		, RootHierarchy(InRootHierarchy)
		, TemplateStore(InTemplateStore)
		, RootCompileRange(InCompileRange)
		, RootClampRange(TRange<float>::All())
		, LocalCompileRange(RootCompileRange)
		, LocalClampRange(RootClampRange)
		, Flags(ESectionEvaluationFlags::None)
		, HierarchicalBias(0)
	{}

	FGatherParameters CreateForSubData(const FMovieSceneSubSequenceData& SubData) const
	{
		FGatherParameters SubParams = *this;

		SubParams.RootToSequenceTransform   = SubData.RootToSequenceTransform;
		SubParams.HierarchicalBias          = SubData.HierarchicalBias;

		SubParams.LocalCompileRange         = SubParams.RootCompileRange * SubData.RootToSequenceTransform;
		SubParams.LocalClampRange           = SubParams.RootClampRange   * SubData.RootToSequenceTransform;

		return SubParams;
	}

	void SetClampRange(TRange<float> InNewRootClampRange)
	{
		RootClampRange  = InNewRootClampRange;
		LocalClampRange = InNewRootClampRange * RootToSequenceTransform;
	}

	/** Path from root to current sequence */
	FMovieSceneRootOverridePath& RootPath;
	/** Hierarchy for the root sequence template */
	FMovieSceneSequenceHierarchy& RootHierarchy;
	/** Store from which to retrieve templates */
	IMovieSceneSequenceTemplateStore& TemplateStore;

	/** The range that is being compiled in the root's time-space */
	TRange<float> RootCompileRange;
	/** A range to clamp compilation to in the root's time-space */
	TRange<float> RootClampRange;

	/** The range that is being compiled in the current sequence's time-space */
	TRange<float> LocalCompileRange;
	/** A range to clamp compilation to in the current sequence's time-space */
	TRange<float> LocalClampRange;

	/** Evaluation flags for the current sequence */
	ESectionEvaluationFlags Flags;

	/** Transform from the root time-space to the current sequence's time-space */
	FMovieSceneSequenceTransform RootToSequenceTransform;

	/** Current accumulated hierarchical bias */
	int32 HierarchicalBias;
};

struct FCompileOnTheFlyData
{
	/** Primary sort - group */
	uint16 GroupEvaluationPriority;
	/** Secondary sort - Hierarchical bias */
	int32 HierarchicalBias;
	/** Tertiary sort - Eval priority */
	int32 EvaluationPriority;

	/** Whether the track requires initialization or not */
	bool bRequiresInit;
	/** Cached ptr to the evaluation track */
	const FMovieSceneEvaluationTrack* Track;
	/** Cached segment ptr within the above track */
	FMovieSceneEvaluationFieldSegmentPtr Segment;
};

/** Gathered data for a given time or range */
struct FMovieSceneGatheredCompilerData
{
	/** Intersection of any empty space that overlaps the currently evaluating time range */
	TRange<float> EmptyOverlappingRange;
	/** Tree of tracks to evaluate */
	TMovieSceneEvaluationTree<FCompileOnTheFlyData> Tracks;
	/** Tree of active sequences */
	TMovieSceneEvaluationTree<FMovieSceneSequenceID> Sequences;
};

IMovieSceneModule& GetMovieSceneModule()
{
	static TWeakPtr<IMovieSceneModule> WeakMovieSceneModule;

	TSharedPtr<IMovieSceneModule> Shared = WeakMovieSceneModule.Pin();
	if (!Shared.IsValid())
	{
		WeakMovieSceneModule = IMovieSceneModule::Get().GetWeakPtr();
		Shared = WeakMovieSceneModule.Pin();
	}
	check(Shared.IsValid());

	return *Shared;
}

void FMovieSceneCompiler::Compile(UMovieSceneSequence& InCompileSequence, IMovieSceneSequenceTemplateStore& InTemplateStore)
{
	FMovieSceneEvaluationTemplate& CompileTemplate = InTemplateStore.AccessTemplate(InCompileSequence);

	// Pass down a mutable path to the gather functions
	FMovieSceneRootOverridePath RootPath;

	// Gather everything that happens, recursively
	FMovieSceneGatheredCompilerData GatherData;
	FGatherParameters GatherParams(RootPath, CompileTemplate.Hierarchy, InTemplateStore, TRange<float>::All());
	GatherCompileOnTheFlyData(InCompileSequence, GatherParams, GatherData);

	// Wipe the current evaluation field for the template
	CompileTemplate.EvaluationField = FMovieSceneEvaluationField();

	TArray<FCompileOnTheFlyData> CompileData;

	for (FMovieSceneEvaluationTreeRangeIterator It(GatherData.Tracks); It; ++It)
	{
		CompileData.Reset();

		for (const FCompileOnTheFlyData& TrackData : GatherData.Tracks.GetAllData(It.Node()))
		{
			CompileData.Add(TrackData);
		}

		// Sort the compilation data based on (in order):
		//  1. Group
		//  2. Hierarchical bias
		//  3. Evaluation priority
		CompileData.Sort(SortPredicate);

		// Compose the final result for the compiled range
		FCompiledGroupResult Result(It.Range());

		// Generate the evaluation group by gathering initialization and evaluation ptrs for each unique group
		PopulateEvaluationGroup(Result, CompileData);

		// Compute meta data for this segment
		TMovieSceneEvaluationTreeDataIterator<FMovieSceneSequenceID> SubSequences = GatherData.Sequences.GetAllData(GatherData.Sequences.IterateFromLowerBound(It.Range().GetLowerBound()).Node());
		PopulateMetaData(Result, CompileTemplate.Hierarchy, CompileData, SubSequences);

		CompileTemplate.EvaluationField.Add(Result.Range, MoveTemp(Result.Group), MoveTemp(Result.MetaData));
	}
}

TOptional<FCompiledGroupResult> FMovieSceneCompiler::CompileTime(float InGlobalTime, UMovieSceneSequence& InCompileSequence, IMovieSceneSequenceTemplateStore& InTemplateStore)
{
	FMovieSceneEvaluationTemplate& CompileTemplate = InTemplateStore.AccessTemplate(InCompileSequence);

	TRange<float> CompileRange = TRange<float>::Inclusive(InGlobalTime, InGlobalTime);

	// Pass down a mutable path to the gather functions
	FMovieSceneRootOverridePath RootPath;

	// Gather everything that happens at this time, recursively
	FMovieSceneGatheredCompilerData GatherData;
	FGatherParameters GatherParams(RootPath, CompileTemplate.Hierarchy, InTemplateStore, CompileRange);
	GatherCompileOnTheFlyData(InCompileSequence, GatherParams, GatherData);

	FMovieSceneEvaluationTreeRangeIterator NodeAtTime = GatherData.Tracks.IterateFromTime(InGlobalTime);
	if (ensure(NodeAtTime))
	{
		TRange<float> CompiledRange = TRange<float>::Intersection(
			TRange<float>::Intersection(
				GatherData.EmptyOverlappingRange,
				GatherData.Sequences.IterateFromLowerBound(CompileRange.GetLowerBound()).Range()
			),
			NodeAtTime.Range());

		// Always include the global time in the compiled range - this is necessary when floating point rounding
		// introduced during sub sequence transformations shift the range to just outside the current time
		CompiledRange = TRange<float>::Hull(CompiledRange, CompileRange);

		TArray<FCompileOnTheFlyData> SortedCompileData;
		for (const FCompileOnTheFlyData& TrackData : GatherData.Tracks.GetAllData(NodeAtTime.Node()))
		{
			SortedCompileData.Add(TrackData);
		}

		// Sort the compilation data based on (in order):
		//  1. Group
		//  2. Hierarchical bias
		//  3. Evaluation priority
		SortedCompileData.Sort(SortPredicate);

		// Compose the final result for the compiled range
		FCompiledGroupResult Result(CompiledRange);

		// Generate the evaluation group by gathering initialization and evaluation ptrs for each unique group
		PopulateEvaluationGroup(Result, SortedCompileData);

		// Compute meta data for this segment
		TMovieSceneEvaluationTreeDataIterator<FMovieSceneSequenceID> SubSequences = GatherData.Sequences.GetAllData(GatherData.Sequences.IterateFromLowerBound(CompileRange.GetLowerBound()).Node());
		PopulateMetaData(Result, CompileTemplate.Hierarchy, SortedCompileData, SubSequences);

		return Result;
	}

	return TOptional<FCompiledGroupResult>();
}

void FMovieSceneCompiler::CompileHierarchy(const UMovieSceneSequence& InRootSequence, FMovieSceneSequenceHierarchy& OutHierarchy, FMovieSceneSequenceID RootSequenceID, int32 MaxDepth)
{
	FMovieSceneRootOverridePath Path;
	Path.Set(RootSequenceID, OutHierarchy);

	CompileHierarchy(InRootSequence, OutHierarchy, Path, MaxDepth);
}

void FMovieSceneCompiler::CompileHierarchy(const UMovieSceneSequence& InSequence, FMovieSceneSequenceHierarchy& OutHierarchy, FMovieSceneRootOverridePath& Path, int32 MaxDepth)
{
	UMovieScene* MovieScene = InSequence.GetMovieScene();
	if (!MovieScene)
	{
		return;
	}

	FMovieSceneSequenceID ParentID = Path.Remap(MovieSceneSequenceID::Root);

	// Remove all existing children
	if (FMovieSceneSequenceHierarchyNode* ExistingNode = OutHierarchy.FindNode(ParentID))
	{
		OutHierarchy.Remove(ExistingNode->Children);
	}

	auto ProcessSection = [&](const UMovieSceneSection* Section, const FGuid& InObjectBindingId)
	{
		const UMovieSceneSubSection* SubSection = Cast<UMovieSceneSubSection>(Section);
		const UMovieSceneSequence* SubSequence = SubSection ? SubSection->GetSequence() : nullptr;

		if (SubSequence)
		{
			FMovieSceneSequenceID DeterministicID = SubSection->GetSequenceID();

			GetOrCreateSubSequenceData(Path.Remap(DeterministicID), ParentID, *SubSection, InObjectBindingId, OutHierarchy);

			int32 NewMaxDepth = MaxDepth == -1 ? -1 : MaxDepth - 1;
			if (NewMaxDepth == -1 || NewMaxDepth > 1)
			{
				Path.Push(DeterministicID);

				CompileHierarchy(*SubSequence, OutHierarchy, Path, NewMaxDepth);

				Path.Pop();
			}
		}
	};

	for (const UMovieSceneTrack* Track : MovieScene->GetMasterTracks())
	{
		for (const UMovieSceneSection* Section : Track->GetAllSections())
		{
			ProcessSection(Section, FGuid());
		}
	}

	for (const FMovieSceneBinding& ObjectBinding : MovieScene->GetBindings())
	{
		for (const UMovieSceneTrack* Track : ObjectBinding.GetTracks())
		{
			for (const UMovieSceneSection* Section : Track->GetAllSections())
			{
				ProcessSection(Section, ObjectBinding.GetObjectGuid());
			}
		}
	}
}

void FMovieSceneCompiler::GatherCompileOnTheFlyData(UMovieSceneSequence& InSequence, const FGatherParameters& Params, FMovieSceneGatheredCompilerData& OutData)
{
	// Regenerate the track structure if it's out of date
	FMovieSceneEvaluationTemplate& Template = Params.TemplateStore.AccessTemplate(InSequence);
	if (Template.SequenceSignature != InSequence.GetSignature())
	{
		FMovieSceneEvaluationTemplateGenerator(InSequence, Template).Generate();
	}

	// Iterate tracks within this template
	for (TTuple<FMovieSceneTrackIdentifier, FMovieSceneEvaluationTrack>& Pair : Template.GetTracks())
	{
		const bool bTrackMatchesFlags = ( Params.Flags == ESectionEvaluationFlags::None )
			|| ( EnumHasAnyFlags(Params.Flags, ESectionEvaluationFlags::PreRoll)  && Pair.Value.ShouldEvaluateInPreroll()  )
			|| ( EnumHasAnyFlags(Params.Flags, ESectionEvaluationFlags::PostRoll) && Pair.Value.ShouldEvaluateInPostroll() );

		if (bTrackMatchesFlags)
		{
			GatherCompileDataForTrack(Pair.Value, Pair.Key, Params, OutData);
		}
	}

	TRange<float> CompileClampIntersection = TRange<float>::Intersection(Params.LocalCompileRange, Params.LocalClampRange);

	// Iterate sub section ranges that overlap with the compile range
	FGatherParameters SubSectionGatherParams = Params;

	const TMovieSceneEvaluationTree<FMovieSceneSubSectionData>& SubSectionField = Template.GetSubSectionField();

	// Start iterating the field from the lower bound of the compile range
	FMovieSceneEvaluationTreeRangeIterator SubSectionIt(SubSectionField.IterateFromLowerBound(CompileClampIntersection.GetLowerBound()));

	// Intersect the unique range in the tree with the current overlapping empty range to constrict the resulting compile range in the case where this is a gap between sub sections
	OutData.EmptyOverlappingRange = TRange<float>::Intersection(OutData.EmptyOverlappingRange, SubSectionIt.Range() * Params.RootToSequenceTransform.Inverse());

	for ( ; SubSectionIt && SubSectionIt.Range().Overlaps(CompileClampIntersection); ++SubSectionIt)
	{
		TRange<float> ThisSegmentRange = SubSectionIt.Range() * Params.RootToSequenceTransform.Inverse();
		if (ThisSegmentRange.IsEmpty())
		{
			continue;
		}

		SubSectionGatherParams.SetClampRange(ThisSegmentRange);

		// Iterate all sub sections in the current range
		for (const FMovieSceneSubSectionData& SubSectionData : SubSectionField.GetAllData(SubSectionIt.Node()))
		{
			UMovieSceneSubSection* SubSection = SubSectionData.Section.Get();
			if (!SubSection)
			{
				continue;
			}

			UMovieSceneSubTrack* SubTrack = SubSection->GetTypedOuter<UMovieSceneSubTrack>();

			const bool bTrackMatchesFlags = ( Params.Flags == ESectionEvaluationFlags::None )
				|| ( EnumHasAnyFlags(Params.Flags, ESectionEvaluationFlags::PreRoll)  && SubTrack && SubTrack->EvalOptions.bEvaluateInPreroll  )
				|| ( EnumHasAnyFlags(Params.Flags, ESectionEvaluationFlags::PostRoll) && SubTrack && SubTrack->EvalOptions.bEvaluateInPostroll );

			if (bTrackMatchesFlags && SubSection)
			{
				SubSectionGatherParams.Flags = SubSectionData.Flags;

				GatherCompileDataForSubSection(*SubSection, SubSectionData.ObjectBindingId, SubSectionGatherParams, OutData);
			}
		}
	}
}

void FMovieSceneCompiler::GatherCompileDataForSubSection(const UMovieSceneSubSection& SubSection, const FGuid& InObjectBindingId, const FGatherParameters& Params, FMovieSceneGatheredCompilerData& OutData)
{
	UMovieSceneSequence* SubSequence = SubSection.GetSequence();
	if (!SubSequence)
	{
		return;
	}

	FMovieSceneSequenceID UnAccumulatedSequenceID = SubSection.GetSequenceID();

	// Hash this source ID with the outer sequence ID to make it unique
	FMovieSceneSequenceID ParentSequenceID = Params.RootPath.Remap(MovieSceneSequenceID::Root);
	FMovieSceneSequenceID InnerSequenceID  = Params.RootPath.Remap(UnAccumulatedSequenceID);

	// Add the active sequence ID to each range. We add each range individually since this range may inform the final compiled range
	OutData.Sequences.Add(Params.RootClampRange, InnerSequenceID);

	// Add this sub sequence ID to the root path
	Params.RootPath.Push(UnAccumulatedSequenceID);

	// Find/add sub data in the root template
	TOptional<FGatherParameters> SubParams;

	{
		const FMovieSceneSubSequenceData* CompilationSubData = GetOrCreateSubSequenceData(InnerSequenceID, ParentSequenceID, SubSection, InObjectBindingId, Params.RootHierarchy);
		check(CompilationSubData);

		SubParams = Params.CreateForSubData(*CompilationSubData);
		// Any code after this point may reallocate the root hierarchy, so CompilationSubData cannot be used
	}

	GatherCompileOnTheFlyData(*SubSequence, SubParams.GetValue(), OutData);

	// Pop the path off the root path
	Params.RootPath.Pop();
}

const FMovieSceneSubSequenceData* FMovieSceneCompiler::GetOrCreateSubSequenceData(FMovieSceneSequenceID InnerSequenceID, FMovieSceneSequenceID ParentSequenceID, const UMovieSceneSubSection& SubSection, const FGuid& InObjectBindingId, FMovieSceneSequenceHierarchy& InOutHierarchy)
{
	// Find/add sub data in the root template
	const FMovieSceneSubSequenceData* SubData = InOutHierarchy.FindSubData(InnerSequenceID);
	if (SubData)
	{
		return SubData;
	}
	
	FSubSequenceInstanceDataParams InstanceParams{ InnerSequenceID, FMovieSceneEvaluationOperand(ParentSequenceID, InObjectBindingId) };
	FMovieSceneSubSequenceData NewSubData = SubSection.GenerateSubSequenceData(InstanceParams);

	// Intersect this inner sequence's valid play range with the parent's if possible
	const FMovieSceneSubSequenceData* ParentSubData = ParentSequenceID != MovieSceneSequenceID::Root ? InOutHierarchy.FindSubData(ParentSequenceID) : nullptr;
	if (ParentSubData)
	{
		TRange<float> ParentPlayRangeChildSpace = ParentSubData->PlayRange * NewSubData.RootToSequenceTransform;
		NewSubData.PlayRange = TRange<float>::Intersection(ParentPlayRangeChildSpace, NewSubData.PlayRange);

		// Accumulate parent transform
		NewSubData.RootToSequenceTransform = NewSubData.RootToSequenceTransform * ParentSubData->RootToSequenceTransform;

		// Accumulate parent hierarchical bias
		NewSubData.HierarchicalBias += ParentSubData->HierarchicalBias;
	}

	// Add the sub data to the root hierarchy
	InOutHierarchy.Add(NewSubData, InnerSequenceID, ParentSequenceID);

	return InOutHierarchy.FindSubData(InnerSequenceID);
}

void FMovieSceneCompiler::GatherCompileDataForTrack(FMovieSceneEvaluationTrack& Track, FMovieSceneTrackIdentifier TrackID, const FGatherParameters& Params, FMovieSceneGatheredCompilerData& OutData)
{
	auto RequiresInit = [&Track](FSectionEvaluationData EvalData)
	{
		return Track.HasChildTemplate(EvalData.ImplIndex) && Track.GetChildTemplate(EvalData.ImplIndex).RequiresInitialization();
	};

	FMovieSceneSequenceTransform SequenceToRootTransform = Params.RootToSequenceTransform.Inverse();
	FMovieSceneSequenceID CurrentSequenceID              = Params.RootPath.Remap(MovieSceneSequenceID::Root);

	TArray<FMovieSceneSegmentIdentifier> SegmentIDs      = Track.GetSegmentsInRange(Params.LocalCompileRange);
	if (SegmentIDs.Num() == 0)
	{
		// No segment at this time, so just report the time range of the empty space.
		TRange<float> EmptyTrackSpace = Track.GetUniqueRangeFromLowerBound(Params.LocalCompileRange.GetLowerBound());
		TRange<float> ClampedEmptyTrackSpaceRoot = TRange<float>::Intersection(Params.LocalClampRange, EmptyTrackSpace) * SequenceToRootTransform;

		OutData.EmptyOverlappingRange = TRange<float>::Intersection(ClampedEmptyTrackSpaceRoot, OutData.EmptyOverlappingRange);
	}
	else for (FMovieSceneSegmentIdentifier SegmentID : SegmentIDs)
	{
		const FMovieSceneSegment& ThisSegment = Track.GetSegment(SegmentID);

		FCompileOnTheFlyData Data;
		Data.Segment = FMovieSceneEvaluationFieldSegmentPtr(CurrentSequenceID, TrackID, SegmentID);
		Data.GroupEvaluationPriority = GetMovieSceneModule().GetEvaluationGroupParameters(Track.GetEvaluationGroup()).EvaluationPriority;
		Data.HierarchicalBias = Params.HierarchicalBias;
		Data.EvaluationPriority = Track.GetEvaluationPriority();
		Data.Track = &Track;
		Data.bRequiresInit = ThisSegment.Impls.ContainsByPredicate(RequiresInit);

		TRange<float> IntersectionRange = TRange<float>::Intersection(Params.LocalClampRange, ThisSegment.Range) * SequenceToRootTransform;
		if (!IntersectionRange.IsEmpty())
		{
			OutData.Tracks.Add(IntersectionRange, Data);
		}
	}
}

void FMovieSceneCompiler::PopulateMetaData(FCompiledGroupResult& OutResult, const FMovieSceneSequenceHierarchy& RootHierarchy, const TArray<FCompileOnTheFlyData>& SortedCompileData, TMovieSceneEvaluationTreeDataIterator<FMovieSceneSequenceID> SubSequences)
{
	OutResult.MetaData.Reset();

	// Add all the init tracks first
	uint32 SortOrder = 0;
	for (const FCompileOnTheFlyData& CompileData : SortedCompileData)
	{
		if (!CompileData.bRequiresInit)
		{
			continue;
		}

		FMovieSceneEvaluationFieldSegmentPtr SegmentPtr = CompileData.Segment;

		// Add the track key
		FMovieSceneEvaluationKey TrackKey(SegmentPtr.SequenceID, SegmentPtr.TrackIdentifier);
		OutResult.MetaData.ActiveEntities.Add(FMovieSceneOrderedEvaluationKey{ TrackKey, SortOrder++ });

		for (FSectionEvaluationData EvalData : CompileData.Track->GetSegment(SegmentPtr.SegmentID).Impls)
		{
			FMovieSceneEvaluationKey SectionKey = TrackKey.AsSection(EvalData.ImplIndex);
			OutResult.MetaData.ActiveEntities.Add(FMovieSceneOrderedEvaluationKey{ SectionKey, SortOrder++ });
		}
	}

	// Then all the eval tracks
	for (const FCompileOnTheFlyData& CompileData : SortedCompileData)
	{
		if (CompileData.bRequiresInit)
		{
			continue;
		}

		FMovieSceneEvaluationFieldSegmentPtr SegmentPtr = CompileData.Segment;

		// Add the track key
		FMovieSceneEvaluationKey TrackKey(SegmentPtr.SequenceID, SegmentPtr.TrackIdentifier);
		OutResult.MetaData.ActiveEntities.Add(FMovieSceneOrderedEvaluationKey{ TrackKey, SortOrder++ });

		for (FSectionEvaluationData EvalData : CompileData.Track->GetSegment(SegmentPtr.SegmentID).Impls)
		{
			FMovieSceneEvaluationKey SectionKey = TrackKey.AsSection(EvalData.ImplIndex);
			OutResult.MetaData.ActiveEntities.Add(FMovieSceneOrderedEvaluationKey{ SectionKey, SortOrder++ });
		}
	}

	Algo::SortBy(OutResult.MetaData.ActiveEntities, &FMovieSceneOrderedEvaluationKey::Key);

	{
		OutResult.MetaData.ActiveSequences.Reset();
		OutResult.MetaData.ActiveSequences.Add(MovieSceneSequenceID::Root);

		for (FMovieSceneSequenceID SequenceID : SubSequences)
		{
			const FMovieSceneSubSequenceData* SubData = RootHierarchy.FindSubData(SequenceID);
			check(SubData);

			UMovieSceneSequence* Sequence = SubData->GetSequence();

			OutResult.MetaData.ActiveSequences.Add(SequenceID);
			OutResult.MetaData.SubSequenceSignatures.Add(SequenceID, Sequence ? Sequence->GetSignature() : FGuid());
		}

		OutResult.MetaData.ActiveSequences.Sort();
	}
}

bool FMovieSceneCompiler::SortPredicate(const FCompileOnTheFlyData& A, const FCompileOnTheFlyData& B)
{
	if (A.GroupEvaluationPriority != B.GroupEvaluationPriority)
	{
		return A.GroupEvaluationPriority > B.GroupEvaluationPriority;
	}
	else if (A.HierarchicalBias != B.HierarchicalBias)
	{
		return A.HierarchicalBias < B.HierarchicalBias;
	}
	else
	{
		return A.EvaluationPriority > B.EvaluationPriority;
	}
}

void FMovieSceneCompiler::AddPtrsToGroup(FMovieSceneEvaluationGroup& Group, TArray<FMovieSceneEvaluationFieldSegmentPtr>& InitPtrs, TArray<FMovieSceneEvaluationFieldSegmentPtr>& EvalPtrs)
{
	if (!InitPtrs.Num() && !EvalPtrs.Num())
	{
		return;
	}

	FMovieSceneEvaluationGroupLUTIndex Index;

	Index.LUTOffset = Group.SegmentPtrLUT.Num();
	Index.NumInitPtrs = InitPtrs.Num();
	Index.NumEvalPtrs = EvalPtrs.Num();

	Group.LUTIndices.Add(Index);
	Group.SegmentPtrLUT.Append(InitPtrs);
	Group.SegmentPtrLUT.Append(EvalPtrs);

	InitPtrs.Reset();
	EvalPtrs.Reset();
}

void FMovieSceneCompiler::PopulateEvaluationGroup(FCompiledGroupResult& OutResult, const TArray<FCompileOnTheFlyData>& SortedCompileData)
{
	TArray<FMovieSceneEvaluationFieldSegmentPtr> EvalPtrs;
	TArray<FMovieSceneEvaluationFieldSegmentPtr> InitPtrs;

	// Now iterate the tracks and insert indices for initialization and evaluation
	FName CurrentEvaluationGroup, LastEvaluationGroup;

	for (const FCompileOnTheFlyData& Data : SortedCompileData)
	{
		// If we're now in a different flush group, add the ptrs to the group
		{
			CurrentEvaluationGroup = Data.Track->GetEvaluationGroup();
			if (CurrentEvaluationGroup != LastEvaluationGroup)
			{
				AddPtrsToGroup(OutResult.Group, InitPtrs, EvalPtrs);
			}
			LastEvaluationGroup = CurrentEvaluationGroup;
		}

		// If this track requires initialization, add it to the init array
		if (Data.bRequiresInit)
		{
			InitPtrs.Add(Data.Segment);
		}

		// All tracks require evaluation implicitly
		EvalPtrs.Add(Data.Segment);
	}
	AddPtrsToGroup(OutResult.Group, InitPtrs, EvalPtrs);
}
