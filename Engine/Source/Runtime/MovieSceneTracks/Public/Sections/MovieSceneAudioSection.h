// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Curves/KeyHandle.h"
#include "MovieSceneSection.h"
#include "MovieSceneAudioSection.generated.h"

class USoundBase;

/**
 * Audio section, for use in the master audio, or by attached audio objects
 */
UCLASS( MinimalAPI )
class UMovieSceneAudioSection
	: public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()

public:

	/** Sets this section's sound */
	void SetSound(class USoundBase* InSound) {Sound = InSound;}

	/** Gets the sound for this section */
	class USoundBase* GetSound() const {return Sound;}
	
	/** Set the offset into the beginning of the audio clip */
	void SetStartOffset(float InStartOffset) {StartOffset = InStartOffset;}
	
	/** Get the offset into the beginning of the audio clip */
	float GetStartOffset() const {return StartOffset;}
	
	/**
	 * @return The range of times that the sound plays, truncated by the section limits
	 */
	TRange<float> MOVIESCENETRACKS_API GetAudioRange() const;
	
	DEPRECATED(4.15, "Audio true range no longer supported.")
	TRange<float> GetAudioTrueRange() const;
	
	/**
	 * Gets the sound volume curve
	 *
	 * @return The rich curve for this sound volume
	 */
	FRichCurve& GetSoundVolumeCurve() { return SoundVolume; }
	const FRichCurve& GetSoundVolumeCurve() const { return SoundVolume; }

	/**
	 * Gets the sound pitch curve
	 *
	 * @return The rich curve for this sound pitch
	 */
	FRichCurve& GetPitchMultiplierCurve() { return PitchMultiplier; }
	const FRichCurve& GetPitchMultiplierCurve() const { return PitchMultiplier; }

	/**
	 * Return the sound volume
	 *
	 * @param InTime	The position in time within the movie scene
	 * @return The volume the sound will be played with.
	 */
	float GetSoundVolume(float InTime) const { return SoundVolume.Eval(InTime); }

	/**
	 * Sets the sound volume
	 *
	 * @param InTime	The position in time within the movie scene
	 * @param InVolume	The volume to set
	 */
	void SetSoundVolume(float InTime, float InVolume) { SoundVolume.AddKey(InTime, InVolume); }

	/**
	 * Return the pitch multiplier
	 *
	 * @param Position	The position in time within the movie scene
	 * @return The pitch multiplier the sound will be played with.
	 */
	float GetPitchMultiplier(float InTime) const { return PitchMultiplier.Eval(InTime); }

	/**
	 * Set the pitch multiplier
	 *
	 * @param Position	The position in time within the movie scene
	 * @param InPitch The pitch multiplier to set
	 */
	void SetPitchMultiplier(float InTime, float InPitchMultiplier) { PitchMultiplier.AddKey(InTime, InPitchMultiplier); }

	/**
	 * Returns whether or not a provided position in time is within the timespan of the audio range
	 *
	 * @param Position	The position to check
	 * @return true if the position is within the timespan, false otherwise
	 */
	bool IsTimeWithinAudioRange( float Position ) const 
	{
		TRange<float> AudioRange = GetAudioRange();
		return Position >= AudioRange.GetLowerBoundValue() && Position <= AudioRange.GetUpperBoundValue();
	}

#if WITH_EDITORONLY_DATA
	/**
	 * @return Whether to show actual intensity on the waveform thumbnail or not
	 */
	bool ShouldShowIntensity() const
	{
		return bShowIntensity;
	}
#endif

	/**
	 * @return Whether subtitles should be suppressed
	 */
	bool GetSuppressSubtitles() const
	{
		return bSuppressSubtitles;
	}
	/** ~UObject interface */
	virtual void PostLoad() override;

public:

	// MovieSceneSection interface

	virtual void MoveSection( float DeltaPosition, TSet<FKeyHandle>& KeyHandles ) override;
	virtual void DilateSection( float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles ) override;
	virtual UMovieSceneSection* SplitSection(float SplitTime) override;
	virtual void GetKeyHandles(TSet<FKeyHandle>& OutKeyHandles, TRange<float> TimeRange) const override;
	virtual void GetSnapTimes(TArray<float>& OutSnapTimes, bool bGetSectionBorders) const override;
	virtual TOptional<float> GetKeyTime( FKeyHandle KeyHandle ) const override { return TOptional<float>(); }
	virtual void SetKeyTime( FKeyHandle KeyHandle, float Time ) override { }
	virtual FMovieSceneEvalTemplatePtr GenerateTemplate() const override;

private:

	/** The sound cue or wave that this section plays */
	UPROPERTY(EditAnywhere, Category="Audio")
	USoundBase* Sound;

	/** The offset into the beginning of the audio clip */
	UPROPERTY(EditAnywhere, Category="Audio")
	float StartOffset;

	/** The absolute time that the sound starts playing at */
	UPROPERTY( )
	float AudioStartTime_DEPRECATED;
	
	/** The amount which this audio is time dilated by */
	UPROPERTY( )
	float AudioDilationFactor_DEPRECATED;

	/** The volume the sound will be played with. */
	UPROPERTY( )
	float AudioVolume_DEPRECATED;

	/** The volume the sound will be played with. */
	UPROPERTY( EditAnywhere, Category = "Audio" )
	FRichCurve SoundVolume;

	/** The pitch multiplier the sound will be played with. */
	UPROPERTY( EditAnywhere, Category = "Audio" )
	FRichCurve PitchMultiplier;

#if WITH_EDITORONLY_DATA
	/** Whether to show the actual intensity of the wave on the thumbnail, as well as the smoothed RMS */
	UPROPERTY(EditAnywhere, Category="Audio")
	bool bShowIntensity;
#endif

	UPROPERTY(EditAnywhere, Category="Audio")
	bool bSuppressSubtitles;
};
