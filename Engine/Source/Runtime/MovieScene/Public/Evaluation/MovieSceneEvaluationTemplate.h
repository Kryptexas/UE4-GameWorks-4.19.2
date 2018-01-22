// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include "UObject/WeakObjectPtr.h"
#include "Misc/Guid.h"
#include "UObject/Class.h"
#include "MovieSceneTrack.h"
#include "Evaluation/MovieSceneTrackIdentifier.h"
#include "Evaluation/MovieSceneEvaluationField.h"
#include "Containers/ArrayView.h"
#include "Evaluation/MovieSceneEvaluationTrack.h"
#include "Evaluation/MovieSceneEvaluationTree.h"
#include "Evaluation/MovieSceneSequenceHierarchy.h"
#include "MovieSceneEvaluationTemplate.generated.h"

class UMovieSceneSequence;
class UMovieSceneSubSection;



USTRUCT()
struct FMovieSceneTemplateGenerationLedger
{
	GENERATED_BODY()

public:

	/**
	 * Lookup a track identifier by its originating signature
	 */
	MOVIESCENE_API FMovieSceneTrackIdentifier FindTrack(const FGuid& InSignature) const;

	/**
	 * Add a new track for the specified signature. Signature must not have already been used
	 */
	MOVIESCENE_API void AddTrack(const FGuid& InSignature, FMovieSceneTrackIdentifier Identifier);

	/**
	 * Check whether we already have a sub section for the specified signature
	 */
	bool ContainsSubSection(const FGuid& InSignature)
	{
		return SubSectionRanges.Contains(InSignature);
	}

public:

	UPROPERTY()
	FMovieSceneTrackIdentifier LastTrackIdentifier;

	/** Map of track signature to array of track identifiers that it created */
	UPROPERTY()
	TMap<FGuid, FMovieSceneTrackIdentifier> TrackSignatureToTrackIdentifier;

	/** Map of sub section ranges that exist in a template */
	UPROPERTY()
	TMap<FGuid, FFloatRange> SubSectionRanges;
};
template<> struct TStructOpsTypeTraits<FMovieSceneTemplateGenerationLedger> : public TStructOpsTypeTraitsBase2<FMovieSceneTemplateGenerationLedger> { enum { WithCopy = true }; };

/** Custom serialized track field data that allows efficient lookup of each track contained within this template for a given time */
USTRUCT()
struct FMovieSceneTrackFieldData
{
	GENERATED_BODY()

	bool Serialize(FArchive& Ar)
	{
		Ar << Field;
		return true;
	}

	TMovieSceneEvaluationTree<FMovieSceneTrackIdentifier> Field;
};
template<> struct TStructOpsTypeTraits<FMovieSceneTrackFieldData> : public TStructOpsTypeTraitsBase2<FMovieSceneTrackFieldData> { enum { WithSerializer = true }; };

/** Data that represents a single sub-section */
USTRUCT()
struct FMovieSceneSubSectionData
{
	GENERATED_BODY()

	FMovieSceneSubSectionData() {}

	FMovieSceneSubSectionData(UMovieSceneSubSection& InSubSection, const FGuid& InObjectBindingId, ESectionEvaluationFlags InFlags);

	friend FArchive& operator<<(FArchive& Ar, FMovieSceneSubSectionData& In)
	{
		return Ar << In.Section << In.ObjectBindingId << In.Flags;
	}

	/** The sub section itself */
	UPROPERTY()
	TWeakObjectPtr<UMovieSceneSubSection> Section;

	/** The object binding that the sub section belongs to (usually zero) */
	UPROPERTY()
	FGuid ObjectBindingId;

	/** Evaluation flags for the section */
	UPROPERTY()
	ESectionEvaluationFlags Flags;
};

/** Custom serialized track field data that allows efficient lookup of each sub section contained within this template for a given time */
USTRUCT()
struct FMovieSceneSubSectionFieldData
{
	GENERATED_BODY()

	bool Serialize(FArchive& Ar)
	{
		Ar << Field;
		return true;
	}

	TMovieSceneEvaluationTree<FMovieSceneSubSectionData> Field;
};
template<> struct TStructOpsTypeTraits<FMovieSceneSubSectionFieldData> : public TStructOpsTypeTraitsBase2<FMovieSceneSubSectionFieldData> { enum { WithSerializer = true }; };


/**
 * Template that is used for efficient runtime evaluation of a movie scene sequence. Potentially serialized into the asset.
 */
USTRUCT()
struct FMovieSceneEvaluationTemplate
{
	GENERATED_BODY()

	/**
	 * Default construction
	 */
	FMovieSceneEvaluationTemplate()
	{
	}

public:

	/**
	 * Attempt to locate a track with the specified identifier
	 */
	FMovieSceneEvaluationTrack* FindTrack(FMovieSceneTrackIdentifier Identifier)
	{
		// Fast, most common path
		if (FMovieSceneEvaluationTrack* Track = Tracks.Find(Identifier))
		{
			return Track;
		}

		return StaleTracks.Find(Identifier);
	}

	/**
	 * Attempt to locate a track with the specified identifier
	 */
	const FMovieSceneEvaluationTrack* FindTrack(FMovieSceneTrackIdentifier Identifier) const
	{
		// Fast, most common path
		if (const FMovieSceneEvaluationTrack* Track = Tracks.Find(Identifier))
		{
			return Track;
		}

		return StaleTracks.Find(Identifier);
	}

	/**
	 * Find a track within this template that relates to the specified signature
	 */
	FMovieSceneEvaluationTrack* FindTrack(const FGuid& InSignature)
	{
		return FindTrack(TemplateLedger.FindTrack(InSignature));
	}

	/**
	 * Find a track within this template that relates to the specified signature
	 */
	const FMovieSceneEvaluationTrack* FindTrack(const FGuid& InSignature) const
	{
		return FindTrack(TemplateLedger.FindTrack(InSignature));
	}

	/**
	 * Test whether the specified track identifier relates to a stale track
	 */
	bool IsTrackStale(FMovieSceneTrackIdentifier Identifier) const
	{
		return StaleTracks.Contains(Identifier);
	}

	/**
	 * Add a new sub section
	 */
	MOVIESCENE_API void AddSubSectionRange(UMovieSceneSubSection& InSubSection, const FGuid& InObjectBindingId, const TRange<float>& InRange, ESectionEvaluationFlags InFlags);

	/**
	 * Add a new track for the specified identifier
	 */
	MOVIESCENE_API FMovieSceneTrackIdentifier AddTrack(const FGuid& InSignature, FMovieSceneEvaluationTrack&& InTrack);

	/**
	 * Define the structural lookup for the specified track identifier, optionally invalidating any overlapping areas in the evaluation field
	 *
	 * @param TrackIdentifier 				The identifier for the track
	 * @param bInvalidateEvaluationField 	Whether to invalidate any overlapping sections of the cached evaluation field
	 */
	MOVIESCENE_API void DefineTrackStructure(FMovieSceneTrackIdentifier TrackIdentifier, bool bInvalidateEvaluationField);

	/**
	 * Remove any tracks that correspond to the specified signature
	 */
	MOVIESCENE_API void RemoveTrack(const FGuid& InSignature);

	/**
	 * Iterate this template's tracks.
	 */
	MOVIESCENE_API const TMap<FMovieSceneTrackIdentifier, FMovieSceneEvaluationTrack>& GetTracks() const;

	/**
	 * Iterate this template's tracks (non-const).
	 * NOTE that this is intended for use during the compilation phase in-editor. 
	 * Beware of using this to modify tracks afterwards as it will almost certainly break evaluation.
	 */
	MOVIESCENE_API TMap<FMovieSceneTrackIdentifier, FMovieSceneEvaluationTrack>& GetTracks();

	/**
	 * Called after this template has been serialized in some way
	 */
	MOVIESCENE_API void PostSerialize(const FArchive& Ar);

	/**
	 * Purge any stale tracks we may have
	 */
	void PurgeStaleTracks()
	{
		StaleTracks.Reset();
	}

public:

	/**
	 * Get this template's generation ledger
	 */
	const FMovieSceneTemplateGenerationLedger& GetLedger() const
	{
		return TemplateLedger;
	}

	/**
	 * Remove any data within this template that does not reside in the specified set of signatures
	 */
	void RemoveStaleData(const TSet<FGuid>& ActiveSignatures);

	/**
	 * Reset this template's field data and sub section cache (keeps tracks alive)
	 */
	void ResetFieldData();

	/**
	 * Access this template's track field
	 */
	const TMovieSceneEvaluationTree<FMovieSceneTrackIdentifier>& GetTrackField() const;

	/**
	 * Access this template's sub section field
	 */
	const TMovieSceneEvaluationTree<FMovieSceneSubSectionData>& GetSubSectionField() const;

private:

	/** Map of evaluation tracks from identifier to track */
	UPROPERTY()
	TMap<FMovieSceneTrackIdentifier, FMovieSceneEvaluationTrack> Tracks;

	/** Transient map of stale tracks. */
	TMap<FMovieSceneTrackIdentifier, FMovieSceneEvaluationTrack> StaleTracks;

public:

	/** Evaluation field for efficient runtime evaluation */
	UPROPERTY()
	FMovieSceneEvaluationField EvaluationField;

	/** Map of all sequences found in this template (recursively) */
	UPROPERTY()
	FMovieSceneSequenceHierarchy Hierarchy;

	UPROPERTY()
	FGuid SequenceSignature;

private:

	UPROPERTY()
	FMovieSceneTemplateGenerationLedger TemplateLedger;

	UPROPERTY()
	FMovieSceneTrackFieldData TrackFieldData;

	UPROPERTY()
	FMovieSceneSubSectionFieldData SubSectionFieldData;

};
template<> struct TStructOpsTypeTraits<FMovieSceneEvaluationTemplate> : public TStructOpsTypeTraitsBase2<FMovieSceneEvaluationTemplate> { enum { WithPostSerialize = true }; };