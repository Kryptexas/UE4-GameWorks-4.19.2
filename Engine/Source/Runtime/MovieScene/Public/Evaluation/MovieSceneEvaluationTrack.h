// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "UObject/Class.h"
#include "Containers/ArrayView.h"
#include "Evaluation/MovieSceneSegment.h"
#include "Evaluation/MovieScenePlayback.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
#include "Compilation/MovieSceneSegmentCompiler.h"
#include "Evaluation/MovieSceneTrackImplementation.h"
#include "MovieSceneEvaluationTree.h"
#include "MovieSceneEvaluationTrack.generated.h"

struct FMovieSceneInterrogationData;
struct FMovieSceneEvaluationTrackSegments;

class UMovieSceneTrack;

/** Enumeration to determine how a track should be evaluated */
UENUM()
enum class EEvaluationMethod : uint8
{
	/** Evaluation only ever occurs at a single time. Delta is irrelevant. Example: Basic curve eval, animation */
	Static,

	/** Evaluation from one frame to the next must consider the entire swept delta range on the track. Example: Events */
	Swept,
};

/** Custom serialized tree of section evaluation data for a track */
USTRUCT()
struct FSectionEvaluationDataTree
{
	GENERATED_BODY()

	bool Serialize(FArchive& Ar)
	{
		Ar << Tree;
		return true;
	}

	TMovieSceneEvaluationTree<FSectionEvaluationData> Tree;
};
template<> struct TStructOpsTypeTraits<FSectionEvaluationDataTree> : public TStructOpsTypeTraitsBase2<FSectionEvaluationDataTree> { enum { WithSerializer = true }; };

/** Structure that references a sorted array of segments by indirect identifiers */
USTRUCT()
struct FMovieSceneEvaluationTrackSegments
{
	GENERATED_BODY()

	/** Index operator that allows lookup by ID */
	FORCEINLINE const FMovieSceneSegment& operator[](FMovieSceneSegmentIdentifier ID) const
	{
		return SortedSegments[SegmentIdentifierToIndex[ID.GetIndex()]];
	}

	/** Index operator that allows lookup by ID */
	FORCEINLINE FMovieSceneSegment& operator[](FMovieSceneSegmentIdentifier ID)
	{
		return SortedSegments[SegmentIdentifierToIndex[ID.GetIndex()]];
	}

	/** Access the sorted index of the specified valid identifier */
	FORCEINLINE int32 GetSortedIndex(FMovieSceneSegmentIdentifier ID) const
	{
		return SegmentIdentifierToIndex[ID.GetIndex()];
	}

	/** Access the sorted array of segments */
	TArrayView<FMovieSceneSegment> GetSorted()
	{
		return SortedSegments;
	}

	/** Access the sorted array of segments */
	TArrayView<const FMovieSceneSegment> GetSorted() const
	{
		return SortedSegments;
	}

	/** Reset this container, invalidating any previously produced identifiers */
	void Reset(int32 ExpectedCapacity)
	{
		SegmentIdentifierToIndex.Reset(ExpectedCapacity);
		SortedSegments.Reset(ExpectedCapacity);
	}

	/** Add a new segment to the container */
	FMovieSceneSegmentIdentifier Add(FMovieSceneSegment&& In);

private:

	/** Array of indices into SortedSegments where each FMovieSceneSegmentIdentifier represents and index into SegmentIdentifierToIndex. Never shuffled until the container is reset. */
	UPROPERTY()
	TArray<int32> SegmentIdentifierToIndex;

	/** Array of segmented ranges contained within the track. */
	UPROPERTY()
	TArray<FMovieSceneSegment> SortedSegments;
};

/**
 * Evaluation track that is stored within an evaluation template for a sequence.
 * Contains user-defined evaluation templates, and an optional track implementation
 */
USTRUCT()
struct FMovieSceneEvaluationTrack
{
	GENERATED_BODY()

	/** Default construction (only for serialization) */
	FMovieSceneEvaluationTrack();

	/**
	 * User construction, for initialization during compilation
	 */
	MOVIESCENE_API FMovieSceneEvaluationTrack(const FGuid& InObjectBindingID);

	/** Copy construction/assignment */
	FMovieSceneEvaluationTrack(const FMovieSceneEvaluationTrack&) = default;
	FMovieSceneEvaluationTrack& operator=(const FMovieSceneEvaluationTrack&) = default;

	/** Move construction/assignment */
	FMovieSceneEvaluationTrack(FMovieSceneEvaluationTrack&&) = default;
	FMovieSceneEvaluationTrack& operator=(FMovieSceneEvaluationTrack&&) = default;

public:

	/**
	 * Get the object binding ID that this track belongs to
	 */
	FORCEINLINE const FGuid& GetObjectBindingID() const
	{
		return ObjectBindingID;
	}

	/**
	 * Get the segment from the given segment index
	 */
	FORCEINLINE const FMovieSceneSegment& GetSegment(FMovieSceneSegmentIdentifier ID) const
	{
		return Segments[ID];
	}

	/**
	 * Get the segment from the given segment index
	 */
	FORCEINLINE TArrayView<const FMovieSceneSegment> GetSortedSegments() const
	{
		return Segments.GetSorted();
	}

	/**
	 * Get this track's child templates
	 * NOTE that this is intended for use during the compilation phase in-editor.
	 * Beware of using this to modify templates afterwards as it will almost certainly break evaluation.
	 */
	FORCEINLINE TArrayView<FMovieSceneEvalTemplatePtr> GetChildTemplates()
	{
		return TArrayView<FMovieSceneEvalTemplatePtr>(ChildTemplates.GetData(), ChildTemplates.Num());
	}

	/**
	 * Get the template from the given template index
	 */
	FORCEINLINE const FMovieSceneEvalTemplate& GetChildTemplate(int32 TemplateIndex) const
	{
		return *ChildTemplates[TemplateIndex];
	}

	/**
	 * Check whether we have a valid child template for the specified index
	 */
	FORCEINLINE bool HasChildTemplate(int32 TemplateIndex) const
	{
		return ChildTemplates.IsValidIndex(TemplateIndex) && ChildTemplates[TemplateIndex].IsValid();
	}
	
public:

	/**
	 * Get this track's evaluation group name. Only used during compilation.
	 *
	 * @return The evaluation group
	 */
	FName GetEvaluationGroup() const
	{
		return EvaluationGroup;
	}

	/**
	 * Set this track's flush group name.
	 *
	 * @note 	When not 'None', setting an evaluation group indicates that all tracks with similar
	 *			groups and priorities should be grouped together at runtime. Named groups can
	 *			be optionally flushed immediately at runtime by calling IMovieSceneTemplateGenerator::FlushGroupImmediately
	 *			with the appropriate group.
	 *
	 * @param InEvaluationGroup 		The evaluation group to assign this track to
	 */
	void SetEvaluationGroup(FName InEvaluationGroup)
	{
		EvaluationGroup = InEvaluationGroup;
	}

public:

	/**
	 * Get the evaluation bias to apply to this track. Higher priority tracks will be evaluated first.
	 *
	 * @return The evaluation priority
	 */
	uint16 GetEvaluationPriority() const
	{
		return EvaluationPriority;
	}

	/**
	 * Get the evaluation bias to apply to this track. Higher priority tracks will be evaluated first.
	 *
	 * @param InEvaluationPriority			The new priority
	 */
	void SetEvaluationPriority(uint16 InEvaluationPriority = 1000)
	{
		EvaluationPriority = InEvaluationPriority;
	}

	/**
	 * Get the method we should use to evaluate this track
	 *
	 * @return The method to use when evaluating this track
	 */
	EEvaluationMethod GetEvaluationMethod() const
	{
		return EvaluationMethod;
	}

	/**
	 * Set the method we should use to evaluate this track
	 *
	 * @param InMethod				The method to use when evaluating this track
	 */
	void SetEvaluationMethod(EEvaluationMethod InMethod)
	{
		EvaluationMethod = InMethod;
	}

	/**
	 * Define how this track evaluates in pre and postroll
	 *
	 * @param bInEvaluateInPreroll	Whether this track should evaluate in preroll
	 * @param bInEvaluateInPostroll	Whether this track should evaluate in postroll
	 */
	void SetPreAndPostrollConditions(bool bInEvaluateInPreroll, bool bInEvaluateInPostroll)
	{
		bEvaluateInPreroll = bInEvaluateInPreroll;
		bEvaluateInPostroll = bInEvaluateInPostroll;
	}

	/**
	 * @return Whether this track should evaluate in preroll
	 */
	bool ShouldEvaluateInPreroll() const
	{
		return bEvaluateInPreroll;
	}

	/**
	 * @return Whether this track should evaluate in postroll
	 */
	bool ShouldEvaluateInPostroll() const
	{
		return bEvaluateInPostroll;
	}

public:

	/**
	 * Called to initialize the specified segment index
	 * 
	 * @param SegmentID				The segment we are evaluating
	 * @param Operand				Operand that relates to the thing we will animate
	 * @param Context				Current sequence context
	 * @param PersistentData		Persistent data store
	 * @param Player				The player that is responsible for playing back this template
	 */
	void Initialize(FMovieSceneSegmentIdentifier SegmentID, const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const;

	/**
	 * Called to evaluate the specified segment index
	 * 
	 * @param SegmentID				The segment we are evaluating
	 * @param Operand				Operand that relates to the thing we will animate
	 * @param Context				Current sequence context
	 * @param PersistentData		Persistent data store
	 * @param ExecutionTokens		Token stack on which to add tokens that will be executed later
	 */
	void Evaluate(FMovieSceneSegmentIdentifier SegmentID, const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const;

	/**
	 * Default implementation of initialization of child templates for the specified segment
	 * 
	 * @param SegmentID				The segment we are evaluating
	 * @param Operand				Operand that relates to the thing we will animate
	 * @param Context				Current sequence context
	 * @param PersistentData		Persistent data store
	 * @param Player				The player that is responsible for playing back this template
	 */
	MOVIESCENE_API void DefaultInitialize(FMovieSceneSegmentIdentifier SegmentID, const FMovieSceneEvaluationOperand& Operand, FMovieSceneContext Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const;

	/**
	 * Default implementation of evaluation of child templates for the specified segment
	 * 
	 * @param SegmentID				The segment we are evaluating
	 * @param Operand				Operand that relates to the thing we will animate
	 * @param Context				Current sequence context
	 * @param PersistentData		Persistent data store
	 * @param ExecutionTokens		Token stack on which to add tokens that will be executed later
	 */
	MOVIESCENE_API void DefaultEvaluate(FMovieSceneSegmentIdentifier SegmentID, const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const;

	/**
	 * Interrogate this template for its output. Should not have any side effects.
	 *
	 * @param Context				Evaluation context specifying the current evaluation time, sub sequence transform and other relevant information.
	 * @param Container				Container to populate with the desired output from this track
	 * @param BindingOverride		Optional binding to specify the object that is being animated by this track
	 */
	MOVIESCENE_API  void Interrogate(const FMovieSceneContext& Context, FMovieSceneInterrogationData& Container, UObject* BindingOverride = nullptr);

private:

	/**
	 * Implementation function for static evaluation
	 */
	void EvaluateStatic(FMovieSceneSegmentIdentifier SegmentID, const FMovieSceneEvaluationOperand& Operand, FMovieSceneContext Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const;

	/**
	 * Implementation function for swept evaluation
	 */
	void EvaluateSwept(FMovieSceneSegmentIdentifier SegmentID, const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const;

public:

	/**
	 * Assign a single eval template to this track, spanning the entire sequence
	 * 
	 * @param InTemplate		The template to insert
	 */
	MOVIESCENE_API void DefineAsSingleTemplate(FMovieSceneEvalTemplatePtr&& InTemplate);

	/**
	 * Add an evaluation template to this track with the given track index
	 * 
	 * @param InTemplate		The template to insert
	 * @return The index of the new template
	 */
	MOVIESCENE_API int32 AddChildTemplate(FMovieSceneEvalTemplatePtr&& InTemplate);

	/**
	 * Find the first segment that overlaps the specified range
	 *
	 * @param InLocalRange 			The range in this track's space to overlap
	 * @return A (potentially invalid) segment identifier for the first segment whose range overlaps the specified range
	 */
	FMovieSceneSegmentIdentifier FindFirstSegment(TRange<float> InLocalRange);

	/**
	 * Find or compile a segment for the specified time
	 *
	 * @param InTime 				The time to lookup or compile for
	 * @return A (potentially invalid) segment identifier for the segment whose range overlaps the specified time
	 */
	FMovieSceneSegmentIdentifier GetSegmentFromTime(float InTime);

	/**
	 * Find or compile a segment for the specified iterator
	 *
	 * @param Iterator 				An iterator that should be used to find or compile time ranges. The iterator's current range is used to lookup segments
	 * @return A (potentially invalid) segment identifier for the segment whose range overlaps the specified iterator's time range
	 */
	FMovieSceneSegmentIdentifier GetSegmentFromIterator(FMovieSceneEvaluationTreeRangeIterator Iterator);

	/**
	 * Find or compile all segments that overlap the specified range
	 *
	 * @param InLocalRange 			The range in this track's space to overlap
	 * @return An (potentially empty) array of segment identifiers that overlap the specified range
	 */
	TArray<FMovieSceneSegmentIdentifier> GetSegmentsInRange(TRange<float> InLocalRange);

	/**
	 * Get the smallest range of unique FSectionEvaluationData combinations that overlaps the specified lower bound
	 *
	 * @param InLowerBound 			The lower bound from which to start looking for a time range
	 * @return The smallest time range of unique evaluation data entries that encompasses the specified lower bound
	 */
	TRange<float> GetUniqueRangeFromLowerBound(TRangeBound<float> InLowerBound) const;

	/**
	 * Set the source track from which this track originates
	 */
	void SetSourceTrack(const UMovieSceneTrack* InSourceTrack)
	{
		SourceTrack = InSourceTrack;
	}

	/**
	 * Get the source track from which this track originates
	 */
	const UMovieSceneTrack* GetSourceTrack() const
	{
		return SourceTrack;
	}

	/**
	 * Add section evaluation data to the evaluation tree for this track
	 *
	 * @param Range 		The range that the specified section data should apply to
	 * @param EvalData 		The actual evaluation data
	 */
	void AddTreeData(TRange<float> Range, FSectionEvaluationData EvalData)
	{
		EvaluationTree.Tree.Add(Range, EvalData);
	}

	/**
	 * Add section evaluation data to the evaluation tree for this track, ensuring no duplciates
	 *
	 * @param Range 		The range that the specified section data should apply to
	 * @param EvalData 		The actual evaluation data
	 */
	void AddUniqueTreeData(TRange<float> Range, FSectionEvaluationData EvalData)
	{
		EvaluationTree.Tree.AddUnique(Range, EvalData);
	}

	/**
	 * Iterate all this track's unique time ranges
	 */
	FMovieSceneEvaluationTreeRangeIterator Iterate() const
	{
		return FMovieSceneEvaluationTreeRangeIterator(EvaluationTree.Tree);
	}

	/**
	 * Iterate all this track's unique time ranges
	 */
	TMovieSceneEvaluationTreeDataIterator<FSectionEvaluationData> GetData(FMovieSceneEvaluationTreeNodeHandle TreeNodeHandle) const
	{
		return EvaluationTree.Tree.GetAllData(TreeNodeHandle);
	}

	/**
	 * Assign a track implementation template to this track
	 * @note Track implementations are evaluated once per frame before any segments.
	 *
	 * @param InImplementation	The track implementation to use
	 */
	template<typename T>
	typename TEnableIf<TPointerIsConvertibleFromTo<T, FMovieSceneTrackImplementation>::Value>::Type 
		SetTrackImplementation(T&& InImpl)
	{
		TrackTemplate = FMovieSceneTrackImplementationPtr(MoveTemp(InImpl));
		TrackTemplate->SetupOverrides();
	}

	/**
	 * Setup overrides for any contained templates
	 */
	MOVIESCENE_API void SetupOverrides();

	/**
	 * Post serialize function
	 */
	MOVIESCENE_API void PostSerialize(const FArchive& Ar);

private:

	/**
	 * Validate the segment array and remove any invalid ptrs
	 */
	void ValidateSegments();

	/**
	 * Compile a segment for the specified iterator node
	 *
	 * @param iterator 			An iterator that should be used to compile segments.
	 * @return A segment identifier for the newly created segment (or invalid if compilation was unsuccessful)
	 */
	FMovieSceneSegmentIdentifier CompileSegment(FMovieSceneEvaluationTreeRangeIterator Iterator);

public:

	/**
	 * Called before this track is evaluated for the first time, or since OnEndEvaluation has been called
	 *
	 * @param PersistentData		Persistent data proxy that may contain data pertaining to this entity
	 * @param Player				The player that is responsible for playing back this template
	 */
	FORCEINLINE void OnBeginEvaluation(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
	{
		if (!TrackTemplate.IsValid())
		{
			return;
		}
		TrackTemplate->OnBeginEvaluation(PersistentData, Player);
	}

	/**
	 * Called after this track is no longer being evaluated
	 *
	 * @param PersistentData		Persistent data proxy that may contain data pertaining to this entity
	 * @param Player				The player that is responsible for playing back this template
	 */
	void OnEndEvaluation(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
	{
		if (!TrackTemplate.IsValid())
		{
			return;
		}
		TrackTemplate->OnEndEvaluation(PersistentData, Player);
	}

private:

	/** ID of the possessable or spawnable within the UMovieScene this track belongs to, if any. Zero guid where this relates to a master track. */
	UPROPERTY()
	FGuid ObjectBindingID;

	/** Evaluation priority. Highest is evaluated first */
	UPROPERTY()
	uint16 EvaluationPriority;

	/** Evaluation method - static or swept */
	UPROPERTY()
	EEvaluationMethod EvaluationMethod;

	/** Array of segmented ranges contained within the track. */
	UPROPERTY()
	FMovieSceneEvaluationTrackSegments Segments;

	/** The movie scene track that created this evaluation track. */
	UPROPERTY()
	const UMovieSceneTrack* SourceTrack;

	/** Evaluation tree specifying what happens at any given time. */
	UPROPERTY()
	FSectionEvaluationDataTree EvaluationTree;

	/** Domain-specific evaluation templates (normally 1 per section) */
	UPROPERTY()
	TArray<FMovieSceneEvalTemplatePtr> ChildTemplates;

	/** Domain-specific track implementation override. */
	UPROPERTY()
	FMovieSceneTrackImplementationPtr TrackTemplate;

	/** Flush group that determines whether this track belongs to a group of tracks */
	UPROPERTY()
	FName EvaluationGroup;

	/** Whether this track is evaluated in preroll */
	UPROPERTY()
	uint32 bEvaluateInPreroll : 1;

	/** Whether this track is evaluated in postroll */
	UPROPERTY()
	uint32 bEvaluateInPostroll : 1;
};

template<> struct TStructOpsTypeTraits<FMovieSceneEvaluationTrack> : public TStructOpsTypeTraitsBase2<FMovieSceneEvaluationTrack> { enum { WithPostSerialize = true, WithCopy = false }; };

#if WITH_DEV_AUTOMATION_TESTS

struct FScopedOverrideTrackSegmentBlender
{
	MOVIESCENE_API FScopedOverrideTrackSegmentBlender(FMovieSceneTrackSegmentBlenderPtr&& InTrackSegmentBlender);
	MOVIESCENE_API ~FScopedOverrideTrackSegmentBlender();

	FScopedOverrideTrackSegmentBlender(const FScopedOverrideTrackSegmentBlender&) = delete;
	FScopedOverrideTrackSegmentBlender& operator=(const FScopedOverrideTrackSegmentBlender&) = delete;
};

#endif