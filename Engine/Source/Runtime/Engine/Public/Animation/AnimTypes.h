// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimLinkableElement.h"
#include "AnimTypes.generated.h"

namespace EAnimEventTriggerOffsets
{
	enum Type
	{
		OffsetBefore,
		OffsetAfter,
		NoOffset
	};
}

ENGINE_API float GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::Type OffsetType);

/** Ticking method for AnimNotifies in AnimMontages. */
UENUM()
namespace EMontageNotifyTickType
{
	enum Type
	{
		/** Queue notifies, and trigger them at the end of the evaluation phase (faster). Not suitable for changing sections or montage position. */
		Queued,
		/** Trigger notifies as they are encountered (Slower). Suitable for changing sections or montage position. */
		BranchingPoint,
	};
}

/** Filtering method for deciding whether to trigger a notify. */
UENUM()
namespace ENotifyFilterType
{
	enum Type
	{
		/** No filtering. */
		NoFiltering,

		/** Filter By Skeletal Mesh LOD. */
		LOD,
	};
}

/**
 * Triggers an animation notify.  Each AnimNotifyEvent contains an AnimNotify object
 * which has its Notify method called and passed to the animation.
 */
USTRUCT()
struct FAnimNotifyEvent : public FAnimLinkableElement
{
	GENERATED_USTRUCT_BODY()

	/** The user requested time for this notify */
	UPROPERTY()
	float DisplayTime_DEPRECATED;

	/** An offset from the DisplayTime to the actual time we will trigger the notify, as we cannot always trigger it exactly at the time the user wants */
	UPROPERTY()
	float TriggerTimeOffset;

	/** An offset similar to TriggerTimeOffset but used for the end scrub handle of a notify state's duration */
	UPROPERTY()
	float EndTriggerTimeOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AnimNotifyEvent)
	float TriggerWeightThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AnimNotifyEvent)
	FName NotifyName;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category=AnimNotifyEvent)
	class UAnimNotify * Notify;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category=AnimNotifyEvent)
	class UAnimNotifyState * NotifyStateClass;

	UPROPERTY()
	float Duration;

	/** Linkable element to use for the end handle representing a notify state duration */
	UPROPERTY()
	FAnimLinkableElement EndLink;

	/** If TRUE, this notify has been converted from an old BranchingPoint. */
	UPROPERTY()
	bool bConvertedFromBranchingPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AnimNotifyEvent)
	TEnumAsByte<EMontageNotifyTickType::Type> MontageTickType;

	/** Defines the chance of of this notify triggering, 0 = No Chance, 1 = Always triggers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimNotifyTriggerSettings, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NotifyTriggerChance;

	/** Defines a method for filtering notifies (stopping them triggering) e.g. by looking at the meshes current LOD */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimNotifyTriggerSettings)
	TEnumAsByte<ENotifyFilterType::Type> NotifyFilterType;

	/** LOD to start filtering this notify from.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimNotifyTriggerSettings, meta = (ClampMin = "0"))
	int32 NotifyFilterLOD;

#if WITH_EDITORONLY_DATA
	/** Color of Notify in editor */
	UPROPERTY()
	FColor NotifyColor;
#endif // WITH_EDITORONLY_DATA

	/** 'Track' that the notify exists on, used for visual placement in editor and sorting priority in runtime */
	UPROPERTY()
	int32 TrackIndex;

	FAnimNotifyEvent()
		: DisplayTime_DEPRECATED(0)
		, TriggerTimeOffset(0)
		, EndTriggerTimeOffset(0)
		, TriggerWeightThreshold(ZERO_ANIMWEIGHT_THRESH)
#if WITH_EDITORONLY_DATA
		, Notify(NULL)
#endif // WITH_EDITORONLY_DATA
		, NotifyStateClass(NULL)
		, Duration(0)
		, bConvertedFromBranchingPoint(false)
		, MontageTickType(EMontageNotifyTickType::Queued)
		, NotifyTriggerChance(1.f)
		, NotifyFilterType(ENotifyFilterType::NoFiltering)
		, NotifyFilterLOD(0)
#if WITH_EDITORONLY_DATA
		, TrackIndex(0)
#endif // WITH_EDITORONLY_DATA
	{
	}

	/** Updates trigger offset based on a combination of predicted offset and current offset */
	ENGINE_API void RefreshTriggerOffset(EAnimEventTriggerOffsets::Type PredictedOffsetType);

	/** Updates end trigger offset based on a combination of predicted offset and current offset */
	ENGINE_API void RefreshEndTriggerOffset(EAnimEventTriggerOffsets::Type PredictedOffsetType);

	/** Returns the actual trigger time for this notify. In some cases this may be different to the DisplayTime that has been set */
	ENGINE_API float GetTriggerTime() const;

	/** Returns the actual end time for a state notify. In some cases this may be different from DisplayTime + Duration */
	ENGINE_API float GetEndTriggerTime() const;

	ENGINE_API float GetDuration() const;

	ENGINE_API void SetDuration(float NewDuration);

	/** Returns true is this AnimNotify is a BranchingPoint */
	ENGINE_API bool IsBranchingPoint() const
	{
		return (MontageTickType == EMontageNotifyTickType::BranchingPoint) && GetLinkedMontage();
	}

	/** Returns true if this is blueprint derived notifies **/
	bool IsBlueprintNotify() const
	{
		return Notify != NULL || NotifyStateClass != NULL;
	}

	bool operator ==(const FAnimNotifyEvent& Other) const
	{
		return(
			(Notify && Notify == Other.Notify) || 
			(NotifyStateClass && NotifyStateClass == Other.NotifyStateClass) ||
			(!IsBlueprintNotify() && NotifyName == Other.NotifyName)
			);
	}

	/** This can be used with the Sort() function on a TArray of FAnimNotifyEvents to sort the notifies array by time, earliest first. */
	ENGINE_API bool operator <(const FAnimNotifyEvent& Other) const;

	ENGINE_API virtual void SetTime(float NewTime, EAnimLinkMethod::Type ReferenceFrame = EAnimLinkMethod::Absolute) override;
};

// Used by UAnimSequenceBase::SortNotifies() to sort its Notifies array
FORCEINLINE bool FAnimNotifyEvent::operator<(const FAnimNotifyEvent& Other) const
{
	float ATime = GetTriggerTime();
	float BTime = Other.GetTriggerTime();

	// When we've got the same time sort on the track index. Explicitly
	// using SMALL_NUMBER here incase the underlying default changes as
	// notifies can have an offset of KINDA_SMALL_NUMBER to be consider
	// distinct
	if (FMath::IsNearlyEqual(ATime, BTime, SMALL_NUMBER))
	{
		return TrackIndex < Other.TrackIndex;
	}
	else
	{
		return ATime < BTime;
	}
}

/**
 * Keyframe position data for one track.  Pos(i) occurs at Time(i).  Pos.Num() always equals Time.Num().
 */
USTRUCT()
struct FAnimNotifyTrack
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName TrackName;

	UPROPERTY()
	FLinearColor TrackColor;

	TArray<FAnimNotifyEvent*> Notifies;

	FAnimNotifyTrack()
		: TrackName(TEXT(""))
		, TrackColor(FLinearColor::White)
	{
	}

	FAnimNotifyTrack(FName InTrackName, FLinearColor InTrackColor)
		: TrackName(InTrackName)
		, TrackColor(InTrackColor)
	{
	}
};

/** 
 * Indicates whether an animation is additive, and what kind.
 */
UENUM()
enum EAdditiveAnimationType
{
	/** No additive. */
	AAT_None  UMETA(DisplayName="No additive"),
	/* Create Additive from LocalSpace Base. */
	AAT_LocalSpaceBase UMETA(DisplayName="Local Space"),
	/* Create Additive from MeshSpace Rotation Only, Translation still will be LocalSpace. */
	AAT_RotationOffsetMeshSpace UMETA(DisplayName="Mesh Space"),
	AAT_MAX,
};

UENUM()
namespace ECurveBlendOption
{
	enum Type
	{
		/** Find Max Weight of curve and use that weight. */
		MaxWeight,
		/** Normalize By Sum of Weight and use it to blend. */
		NormalizeByWeight,
		/** Blend By Weight without normalizing*/
		BlendByWeight
	};
}