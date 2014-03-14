// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "AudioComponent.generated.h"

/** called when we finish playing audio, either because it played to completion or because a Stop() call turned it off early */
DECLARE_DYNAMIC_MULTICAST_DELEGATE( FOnAudioFinished );
/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_OneParam( FOnAudioFinishedNative, class UAudioComponent* );
/** Called when subtitles are sent to the SubtitleManager.  Set this delegate if you want to hijack the subtitles for other purposes */
DECLARE_DYNAMIC_DELEGATE_TwoParams( FOnQueueSubtitles, const TArray<struct FSubtitleCue>&, Subtitles, float, CueDuration );

/**
 *	Struct used for storing one per-instance named parameter for this AudioComponent.
 *	Certain nodes in the SoundCue may reference parameters by name so they can be adjusted per-instance.
 */
USTRUCT()
struct FAudioComponentParam
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AudioComponentParam)
	FName ParamName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AudioComponentParam)
	float FloatParam;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AudioComponentParam)
	bool BoolParam;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AudioComponentParam)
	int32 IntParam;

	UPROPERTY()
	class UDEPRECATED_SoundNodeWave* WaveParam_DEPRECATED;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AudioComponentParam)
	class USoundWave* SoundWaveParam;


	FAudioComponentParam()
		: FloatParam(0)
		, BoolParam(false)
		, IntParam(0)
		, WaveParam_DEPRECATED(NULL)
		, SoundWaveParam(NULL)
	{
	}

};

struct FSoundParseParameters
{
	FNotifyBufferFinishedHooks NotifyBufferFinishedHooks;

	class USoundClass* SoundClass;
	
	FTransform Transform;
	FVector Velocity;
	
	float Volume;
	float VolumeMultiplier;

	float Pitch;
	float HighFrequencyGain;

	float StartTime;

	uint32 bUseSpatialization:1;
	uint32 bLooping:1;

	FSoundParseParameters()
		: SoundClass(NULL)
		, Velocity(ForceInit)
		, Volume(1.f)
		, VolumeMultiplier(1.f)
		, Pitch(1.f)
		, HighFrequencyGain(1.f)
		, StartTime(-1.f)
		, bUseSpatialization(false)
		, bLooping(false)
	{
	}
};

/**
 * Used to provide objects with audio.
 */
UCLASS(HeaderGroup=Component, ClassGroup=(Audio, Common), hidecategories=(Object, ActorComponent, Physics, Rendering, Mobility, LOD), ShowCategories=Trigger, dependson=(AReverbVolume, UEngineTypes, USoundAttenuation), meta=(BlueprintSpawnableComponent))
class ENGINE_API UAudioComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sound)
	class USoundBase* Sound;

	/** Array of per-instance parameters for this AudioComponent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, editinline, Category=Sound, AdvancedDisplay)
	TArray<struct FAudioComponentParam> InstanceParameters;

	/** Optional sound group this AudioComponent belongs to */
	UPROPERTY(EditAnywhere, Category=Sound, AdvancedDisplay)
	USoundClass* SoundClassOverride;

	/** Auto start this component on creation */
	UPROPERTY()
	uint32 bAutoPlay_DEPRECATED:1;

	/** Auto destroy this component on completion */
	UPROPERTY()
	uint32 bAutoDestroy:1;

	/** Stop sound when owner is destroyed */
	UPROPERTY()
	uint32 bStopWhenOwnerDestroyed:1;

	/** Whether the wave instances should remain active if they're dropped by the prioritization code. Useful for e.g. vehicle sounds that shouldn't cut out. */
	UPROPERTY()
	uint32 bShouldRemainActiveIfDropped:1;

	/** Is this audio component allowed to be spatialized? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attenuation)
	uint32 bAllowSpatialization:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attenuation)
	uint32 bOverrideAttenuation:1;

	/** Whether or not this sound plays when the game is paused in the UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	uint32 bIsUISound : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Modulation)
	float PitchModulationMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Modulation)
	float PitchModulationMax;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Modulation)
	float VolumeModulationMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Modulation)
	float VolumeModulationMax;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sound)
	float VolumeMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sound)
	float PitchMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Modulation)
	float HighFrequencyGainMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attenuation)
	class USoundAttenuation* AttenuationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attenuation)
	struct FAttenuationSettings AttenuationOverrides;

	/** while playing, this component will check for occlusion from its closest listener every this many seconds */
	float OcclusionCheckInterval;

	/** called when we finish playing audio, either because it played to completion or because a Stop() call turned it off early */
	UPROPERTY(BlueprintAssignable)
	FOnAudioFinished OnAudioFinished;
	/** shadow delegate for non UObject subscribers */
	FOnAudioFinishedNative OnAudioFinishedNative;

	/** Called when subtitles are sent to the SubtitleManager.  Set this delegate if you want to hijack the subtitles for other purposes */
	UPROPERTY()
	FOnQueueSubtitles OnQueueSubtitles;

	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetSound( USoundBase* NewSound );

	/**
	 * This can be used in place of "play" when it is desired to fade in the sound over time.
	 *
	 * If FadeTime is 0.0, the change in volume is instant.
	 * If FadeTime is > 0.0, the multiplier will be increased from 0 to FadeVolumeLevel over FadeIn seconds.
	 *
	 * @param FadeInDuration how long it should take to reach the FadeVolumeLevel
	 * @param FadeVolumeLevel the percentage of the AudioComponents's calculated volume to fade to
	 */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio", meta=(FadeVolumeLevel="1.0"))
	void FadeIn(float FadeInDuration, float FadeVolumeLevel, float StartTime = 0.f);

	/**
	 * This is used in place of "stop" when it is desired to fade the volume of the sound before stopping.
	 *
	 * If FadeTime is 0.0, this is the same as calling Stop().
	 * If FadeTime is > 0.0, this will adjust the volume multiplier to FadeVolumeLevel over FadeInTime seconds
	 * and then stop the sound.
	 *
	 * @param FadeOutDuration how long it should take to reach the FadeVolumeLevel
	 * @param FadeVolumeLevel the percentage of the AudioComponents's calculated volume in which to fade to
	 */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")

	void FadeOut(float FadeOutDuration, float FadeVolumeLevel);

	/** Start a sound playing on an audio component */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void Play(float StartTime = 0.f);

	/** Stop an audio component playing its sound cue, issue any delegates if needed */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void Stop();

	/** This will allow one to adjust the volume of an AudioComponent on the fly */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void AdjustVolume(float AdjustVolumeDuration, float AdjustVolumeLevel);

	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetFloatParameter(FName InName, float InFloat);

	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetWaveParameter(FName InName, class USoundWave* InWave);

	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetBoolParameter(FName InName, bool InBool);
	
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetIntParameter(FName InName, int32 InInt);

	/** Set a new volume multiplier */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetVolumeMultiplier(float NewVolumeMultiplier);

	/** Set a new pitch multiplier */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetPitchMultiplier(float NewPitchMultiplier);

	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetUISound(bool bInUISound);

	/** Called by the ActiveSound to inform the component that playback is finished */
	void PlaybackCompleted(bool bFailedToStart);

public:

	/** If true, subtitles in the sound data will be ignored. */
	uint32 bSuppressSubtitles:1;

	/** Whether this audio component is previewing a sound */
	uint32 bPreviewComponent:1;

	/** If true, this sound will not be stopped when flushing the audio device. */
	uint32 bIgnoreForFlushing:1;

	/**
	 * Properties of the audio component set by its owning sound class
	 */
	
	/** Whether audio effects are applied */
	uint32 bEQFilterApplied:1;

	/** Whether to artificially prioritize the component to play */
	uint32 bAlwaysPlay:1;

	/** Whether or not this audio component is a music clip */
	uint32 bIsMusic:1;

	/** Whether or not the audio component should be excluded from reverb EQ processing */
	uint32 bReverb:1;

	/** Whether or not this sound class forces sounds to the center channel */
	uint32 bCenterChannelOnly:1;

	/** Whether or not the audio component should display an icon in the editor */
	uint32 bVisualizeComponent:1;

	/** Used by the subtitle manager to prioritize subtitles wave instances spawned by this component. */
	float SubtitlePriority;

	// Begin UObject interface.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void PostLoad() OVERRIDE;
	virtual FString GetDetailedInfoInternal() const OVERRIDE;
	// End UObject interface.

	// Begin USceneComponent Interface
	virtual void Activate(bool bReset=false) OVERRIDE;
	virtual void Deactivate() OVERRIDE;
	virtual void OnUpdateTransform(bool bSkipPhysicsMove) OVERRIDE;
	// End USceneComponent Interface

	/** @return true if this component is currently playing a SoundCue. */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	bool IsPlaying();


	// Begin ActorComponent interface.
#if WITH_EDITORONLY_DATA
	virtual void OnRegister() OVERRIDE;
#endif
	virtual void OnUnregister() OVERRIDE;
	// End ActorComponent interface.

	const FAttenuationSettings* GetAttenuationSettingsToApply() const;
	void CollectAttenuationShapesForVisualization(TMap<EAttenuationShape::Type, FAttenuationSettings::AttenuationShapeDetails>& ShapeDetailsMap) const;

private:
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(transient)
	class UBillboardComponent* SpriteComponent;

	void UpdateSpriteTexture();
#endif

	void PlayInternal(const float StartTime = 0.f, const float FadeInDuration = 0.f, const float FadeVolumeLevel = 1.f);
};



