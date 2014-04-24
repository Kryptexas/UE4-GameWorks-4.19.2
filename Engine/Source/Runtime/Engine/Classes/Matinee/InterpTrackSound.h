// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InterpTrackSound.generated.h"

/**
 *	A track that plays sounds on the groups Actor.
 */


/** Information for one sound in the track. */
USTRUCT()
struct FSoundTrackKey
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	float Volume;

	UPROPERTY()
	float Pitch;

	UPROPERTY(EditAnywhere, Category=SoundTrackKey)
	class USoundBase* Sound;



		FSoundTrackKey()
		: Time(0)
		, Volume(1.f)
		, Pitch(1.f)
		, Sound(NULL)
		{
		}
	
};

UCLASS(HeaderGroup=Interpolation, MinimalAPI, meta=( DisplayName = "Sound Track" ) )
class UInterpTrackSound : public UInterpTrackVectorBase
{
	GENERATED_UCLASS_BODY()

	/** Array of sounds to play at specific times. */
	UPROPERTY()
	TArray<struct FSoundTrackKey> Sounds;

	/** if set, sound plays only when playing the matinee in reverse instead of when the matinee plays forward */
	UPROPERTY(EditAnywhere, Category=InterpTrackSound)
	uint32 bPlayOnReverse:1;

	/** If true, sounds on this track will not be forced to finish when the matinee sequence finishes. */
	UPROPERTY(EditAnywhere, Category=InterpTrackSound)
	uint32 bContinueSoundOnMatineeEnd:1;

	/** If true, don't show subtitles for sounds played by this track. */
	UPROPERTY(EditAnywhere, Category=InterpTrackSound)
	uint32 bSuppressSubtitles:1;

	/** If true and track is controlling a pawn, makes the pawn "speak" the given audio. */
	UPROPERTY(EditAnywhere, Category=InterpTrackSound)
	uint32 bTreatAsDialogue:1;

	UPROPERTY(EditAnywhere, Category=InterpTrackSound)
	uint32 bAttach:1;

	// Begin UObject Interface
	virtual void PostLoad() OVERRIDE;
	// End UObject Interface

	// Begin UInterpTrack interface
	virtual int32 GetNumKeyframes() const OVERRIDE;
	virtual void GetTimeRange(float& StartTime, float& EndTime) const OVERRIDE;
	virtual float GetTrackEndTime() const OVERRIDE;
	virtual float GetKeyframeTime(int32 KeyIndex) const OVERRIDE;
	virtual int32 GetKeyframeIndex( float KeyTime ) const OVERRIDE;
	virtual int32 AddKeyframe(float Time, UInterpTrackInst* TrInst, EInterpCurveMode InitInterpMode) OVERRIDE;
	virtual int32 SetKeyframeTime(int32 KeyIndex, float NewKeyTime, bool bUpdateOrder=true) OVERRIDE;
	virtual void RemoveKeyframe(int32 KeyIndex) OVERRIDE;
	virtual int32 DuplicateKeyframe(int32 KeyIndex, float NewKeyTime, UInterpTrack* ToTrack = NULL) OVERRIDE;
	virtual bool GetClosestSnapPosition(float InPosition, TArray<int32> &IgnoreKeys, float& OutPosition) OVERRIDE;
	virtual void PreviewUpdateTrack(float NewPosition, UInterpTrackInst* TrInst) OVERRIDE;
	virtual void UpdateTrack(float NewPosition, UInterpTrackInst* TrInst, bool bJump) OVERRIDE;
	virtual void PreviewStopPlayback(class UInterpTrackInst* TrInst) OVERRIDE;
	virtual const FString	GetEdHelperClassName() const OVERRIDE;
	virtual const FString	GetSlateHelperClassName() const OVERRIDE;
	virtual class UTexture2D* GetTrackIcon() const OVERRIDE;
	virtual void DrawTrack( FCanvas* Canvas, UInterpGroup* Group, const FInterpTrackDrawParams& Params ) OVERRIDE;
	virtual bool AllowStaticActors() OVERRIDE { return true; }
	virtual void SetTrackToSensibleDefault() OVERRIDE;
	// End UInterpTrack interface

	/**
	 * Return the key at the specified position in the track.
	 */
	struct FSoundTrackKey& GetSoundTrackKeyAtPosition(float InPosition);

};



