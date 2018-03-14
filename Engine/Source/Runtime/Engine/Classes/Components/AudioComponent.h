// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "Components/SceneComponent.h"
#include "Sound/SoundAttenuation.h"

#include "AudioComponent.generated.h"

class FAudioDevice;
class USoundBase;
class USoundClass;

/** called when we finish playing audio, either because it played to completion or because a Stop() call turned it off early */
DECLARE_DYNAMIC_MULTICAST_DELEGATE( FOnAudioFinished );

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_OneParam( FOnAudioFinishedNative, class UAudioComponent* );

/** Called when subtitles are sent to the SubtitleManager.  Set this delegate if you want to hijack the subtitles for other purposes */
DECLARE_DYNAMIC_DELEGATE_TwoParams( FOnQueueSubtitles, const TArray<struct FSubtitleCue>&, Subtitles, float, CueDuration );

/** Called as a sound plays on the audio component to allow BP to perform actions based on playback percentage.
* Computed as samples played divided by total samples, taking into account pitch.
* Not currently implemented on all platforms.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAudioPlaybackPercent, const class USoundWave*, PlayingSoundWave, const float, PlaybackPercent);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAudioPlaybackPercentNative, const class UAudioComponent*, const class USoundWave*, const float);

/** 
* Called while a sound plays and returns the sound's envelope value (using an envelope follower in the audio renderer).
* This only works in the audio mixer.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAudioSingleEnvelopeValue, const class USoundWave*, PlayingSoundWave, const float, EnvelopeValue);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAudioSingleEnvelopeValueNative, const class UAudioComponent*, const class USoundWave*, const float);

/**
* Called while a sound plays and returns the sound's average and max envelope value (using an envelope follower in the audio renderer per wave instance).
* This only works in the audio mixer.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAudioMultiEnvelopeValue, const float, AverageEnvelopeValue, const float, MaxEnvelope, const int32, NumWaveInstances);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_FourParams(FOnAudioMultiEnvelopeValueNative, const class UAudioComponent*, const float, const float, const int32);

/**
 *	Struct used for storing one per-instance named parameter for this AudioComponent.
 *	Certain nodes in the SoundCue may reference parameters by name so they can be adjusted per-instance.
 */
USTRUCT(BlueprintType)
struct FAudioComponentParam
{
	GENERATED_USTRUCT_BODY()

	// Name of the parameter
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AudioComponentParam)
	FName ParamName;

	// Value of the parameter when used as a float
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AudioComponentParam)
	float FloatParam;

	// Value of the parameter when used as a boolean
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AudioComponentParam)
	bool BoolParam;
	
	// Value of the parameter when used as an integer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AudioComponentParam)
	int32 IntParam;

	// Value of the parameter when used as a sound wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AudioComponentParam)
	class USoundWave* SoundWaveParam;

	FAudioComponentParam(const FName& Name)
		: ParamName(Name)
		, FloatParam(0.f)
		, BoolParam(false)
		, IntParam(0)
		, SoundWaveParam(nullptr)
	{}

	FAudioComponentParam()
		: FloatParam(0.f)
		, BoolParam(false)
		, IntParam(0)
		, SoundWaveParam(nullptr)
	{
	}

};

/**
 * AudioComponent is used to play a Sound
 *
 * @see https://docs.unrealengine.com/latest/INT/Audio/Overview/index.html
 * @see USoundBase
 */
UCLASS(ClassGroup=(Audio, Common), hidecategories=(Object, ActorComponent, Physics, Rendering, Mobility, LOD), ShowCategories=Trigger, meta=(BlueprintSpawnableComponent))
class ENGINE_API UAudioComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** The sound to be played */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sound)
	class USoundBase* Sound;

	/** Array of per-instance parameters for this AudioComponent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sound, AdvancedDisplay)
	TArray<struct FAudioComponentParam> InstanceParameters;

	/** Optional sound group this AudioComponent belongs to */
	UPROPERTY(EditAnywhere, Category=Sound, AdvancedDisplay)
	USoundClass* SoundClassOverride;

	/** Auto destroy this component on completion */
	UPROPERTY()
	uint8 bAutoDestroy:1;

	/** Stop sound when owner is destroyed */
	UPROPERTY()
	uint8 bStopWhenOwnerDestroyed:1;

	/** Whether the wave instances should remain active if they're dropped by the prioritization code. Useful for e.g. vehicle sounds that shouldn't cut out. */
	UPROPERTY()
	uint8 bShouldRemainActiveIfDropped:1;

	/** Overrides spatialization enablement in either the attenuation asset or on this audio component's attenuation settings override. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attenuation)
	uint8 bAllowSpatialization:1;

	/** Allows defining attenuation settings directly on this audio component without using an attenuation settings asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attenuation)
	uint8 bOverrideAttenuation:1;

	/** Whether or not to override the sound's subtitle priority. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	uint32 bOverrideSubtitlePriority:1;

	/** Whether or not this sound plays when the game is paused in the UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	uint8 bIsUISound : 1;

	/** Whether or not to apply a low-pass filter to the sound that plays in this audio component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Filter)
	uint8 bEnableLowPassFilter : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	uint8 bOverridePriority:1;

	/** If true, subtitles in the sound data will be ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	uint8 bSuppressSubtitles:1;

	/** Whether this audio component is previewing a sound */
	uint8 bPreviewComponent:1;

	/** If true, this sound will not be stopped when flushing the audio device. */
	uint8 bIgnoreForFlushing:1;

	/** Whether audio effects are applied */
	uint8 bEQFilterApplied:1;

	/** Whether to artificially prioritize the component to play */
	uint8 bAlwaysPlay:1;

	/** Whether or not this audio component is a music clip */
	uint8 bIsMusic:1;

	/** Whether or not the audio component should be excluded from reverb EQ processing */
	uint8 bReverb:1;

	/** Whether or not this sound class forces sounds to the center channel */
	uint8 bCenterChannelOnly:1;

	/** Whether or not this sound is a preview sound */
	uint8 bIsPreviewSound:1;

	/** Whether or not this audio component has been paused */
	uint8 bIsPaused:1;

	/** The specific audio device to play this component on */
	uint32 AudioDeviceHandle;

	/** Configurable, serialized ID for audio plugins */
	UPROPERTY()
	FName AudioComponentUserID;

	/** The lower bound to use when randomly determining a pitch multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Modulation)
	float PitchModulationMin;

	/** The upper bound to use when randomly determining a pitch multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Modulation)
	float PitchModulationMax;

	/** The lower bound to use when randomly determining a volume multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Modulation)
	float VolumeModulationMin;

	/** The upper bound to use when randomly determining a volume multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Modulation)
	float VolumeModulationMax;

	/** A volume multiplier to apply to sounds generated by this component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sound)
	float VolumeMultiplier;

	/** The attack time in milliseconds for the envelope follower. Delegate callbacks can be registered to get the envelope value of sounds played with this audio component. Only used in audio mixer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (ClampMin = "0", UIMin = "0"))
	int32 EnvelopeFollowerAttackTime;

	/** The release time in milliseconds for the envelope follower. Delegate callbacks can be registered to get the envelope value of sounds played with this audio component. Only used in audio mixer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (ClampMin = "0", UIMin = "0"))
	int32 EnvelopeFollowerReleaseTime;

	/** A priority value that is used for sounds that play on this component that scales against final output volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bOverridePriority"))
	float Priority;

	/** Used by the subtitle manager to prioritize subtitles wave instances spawned by this component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bOverrideSubtitlePriority"))
	float SubtitlePriority;

	UPROPERTY()
	float VolumeWeightedPriorityScale_DEPRECATED;

	/** A pitch multiplier to apply to sounds generated by this component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sound)
	float PitchMultiplier;

	UPROPERTY()
	float HighFrequencyGainMultiplier_DEPRECATED;

	/** The frequency of the lowpass filter (in hertz) to apply to this voice. A frequency of 0.0 is the device sample rate and will bypass the filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Filter, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableLowPassFilter"))
	float LowPassFilterFrequency;

	/** If bOverrideSettings is false, the asset to use to determine attenuation properties for sounds generated by this component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attenuation, meta=(EditCondition="!bOverrideAttenuation"))
	class USoundAttenuation* AttenuationSettings;

	/** If bOverrideSettings is true, the attenuation properties to use for sounds generated by this component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= AttenuationSettings, meta=(EditCondition="bOverrideAttenuation"))
	struct FSoundAttenuationSettings AttenuationOverrides;

	/** What sound concurrency to use for sounds generated by this audio component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Concurrency)
	class USoundConcurrency* ConcurrencySettings;

	/** while playing, this component will check for occlusion from its closest listener every this many seconds */
	float OcclusionCheckInterval;

	/** called when we finish playing audio, either because it played to completion or because a Stop() call turned it off early */
	UPROPERTY(BlueprintAssignable)
	FOnAudioFinished OnAudioFinished;

	/** shadow delegate for non UObject subscribers */
	FOnAudioFinishedNative OnAudioFinishedNative;

	/** Called as a sound plays on the audio component to allow BP to perform actions based on playback percentage.
	* Computed as samples played divided by total samples, taking into account pitch.
	* Not currently implemented on all platforms.
	*/
	UPROPERTY(BlueprintAssignable)
	FOnAudioPlaybackPercent OnAudioPlaybackPercent;

	/** shadow delegate for non UObject subscribers */
	FOnAudioPlaybackPercentNative OnAudioPlaybackPercentNative;

	UPROPERTY(BlueprintAssignable)
	FOnAudioSingleEnvelopeValue OnAudioSingleEnvelopeValue;

	/** shadow delegate for non UObject subscribers */
	FOnAudioSingleEnvelopeValueNative OnAudioSingleEnvelopeValueNative;

	UPROPERTY(BlueprintAssignable)
	FOnAudioMultiEnvelopeValue OnAudioMultiEnvelopeValue;

	/** shadow delegate for non UObject subscribers */
	FOnAudioMultiEnvelopeValueNative OnAudioMultiEnvelopeValueNative;

	/** Called when subtitles are sent to the SubtitleManager.  Set this delegate if you want to hijack the subtitles for other purposes */
	UPROPERTY()
	FOnQueueSubtitles OnQueueSubtitles;

	// Set what sound is played by this component
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
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	virtual void FadeIn(float FadeInDuration, float FadeVolumeLevel = 1.f, float StartTime = 0.f);

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
	virtual	void FadeOut(float FadeOutDuration, float FadeVolumeLevel);

	/** Start a sound playing on an audio component */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	virtual void Play(float StartTime = 0.f);

	/** Stop an audio component playing its sound cue, issue any delegates if needed */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	virtual void Stop();

	/** Pause an audio component playing its sound cue, issue any delegates if needed */
	UFUNCTION(BlueprintCallable, Category = "Audio|Components|Audio")
	void SetPaused(bool bPause);

	/** @return true if this component is currently playing a SoundCue. */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	virtual bool IsPlaying() const;

	/** This will allow one to adjust the volume of an AudioComponent on the fly */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void AdjustVolume(float AdjustVolumeDuration, float AdjustVolumeLevel);

	/**  Set a float instance parameter for use in sound cues played by this audio component */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetFloatParameter(FName InName, float InFloat);

	/**  Set a sound wave instance parameter for use in sound cues played by this audio component */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetWaveParameter(FName InName, class USoundWave* InWave);

	/** Set a boolean instance parameter for use in sound cues played by this audio component */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio", meta=(DisplayName="Set Boolean Parameter"))
	void SetBoolParameter(FName InName, bool InBool);
	
	/** Set an integer instance parameter for use in sound cues played by this audio component */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio", meta=(DisplayName="Set Integer Parameter"))
	void SetIntParameter(FName InName, int32 InInt);

	/** Set a new volume multiplier */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetVolumeMultiplier(float NewVolumeMultiplier);

	/** Set a new pitch multiplier */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetPitchMultiplier(float NewPitchMultiplier);

	/** Set whether sounds generated by this audio component should be considered UI sounds */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void SetUISound(bool bInUISound);

	/** Modify the attenuation settings of the audio component */
	UFUNCTION(BlueprintCallable, Category="Audio|Components|Audio")
	void AdjustAttenuation(const FSoundAttenuationSettings& InAttenuationSettings);

	/** Sets how much audio the sound should send to the given submix. */
	UFUNCTION(BlueprintCallable, Category = "Audio|Components|Audio")
	void SetSubmixSend(USoundSubmix* Submix, float SendLevel);

	/** Sets whether or not the low pass filter is enabled on the audio component. */
	UFUNCTION(BlueprintCallable, Category = "Audio|Components|Audio")
	void SetLowPassFilterEnabled(bool InLowPassFilterEnabled);

	/** Sets lowpass filter frequency of the audio component. */
	UFUNCTION(BlueprintCallable, Category = "Audio|Components|Audio")
	void SetLowPassFilterFrequency(float InLowPassFilterFrequency);

	static void PlaybackCompleted(uint64 AudioComponentID, bool bFailedToStart);

private:
	/** Called by the ActiveSound to inform the component that playback is finished */
	void PlaybackCompleted(bool bFailedToStart);

public:

	/** Sets the sound instance parameter. */
	void SetSoundParameter(const FAudioComponentParam& Param);

	//~ Begin UObject Interface.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	virtual FString GetDetailedInfoInternal() const override;
	virtual void PostLoad() override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void BeginDestroy() override;
	//~ End UObject Interface.

	//~ Begin USceneComponent Interface
	virtual void Activate(bool bReset=false) override;
	virtual void Deactivate() override;
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;
	//~ End USceneComponent Interface

	//~ Begin ActorComponent Interface.
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual const UObject* AdditionalStatObject() const override;
	virtual bool IsReadyForOwnerToAutoDestroy() const override;
	//~ End ActorComponent Interface.

	/** Returns a pointer to the attenuation settings to be used (if any) for this audio component dependent on the SoundAttenuation asset or overrides set. */
	const FSoundAttenuationSettings* GetAttenuationSettingsToApply() const;

	UFUNCTION(BlueprintCallable, Category = "Audio|Components|Audio", meta = (DisplayName = "Get Attenuation Settings To Apply", ScriptName="GetAttenuationSettingsToApply"))
	bool BP_GetAttenuationSettingsToApply(FSoundAttenuationSettings& OutAttenuationSettings);

	/** Collects the various attenuation shapes that may be applied to the sound played by the audio component for visualization in the editor or via the in game debug visualization. */
	void CollectAttenuationShapesForVisualization(TMultiMap<EAttenuationShape::Type, FBaseAttenuationSettings::AttenuationShapeDetails>& ShapeDetailsMap) const;

	/** Returns the active audio device to use for this component based on whether or not the component is playing in a world. */
	FAudioDevice* GetAudioDevice() const;

	uint64 GetAudioComponentID() const { return AudioComponentID; }

	FName GetAudioComponentUserID() const { return AudioComponentUserID; }

	static UAudioComponent* GetAudioComponentFromID(uint64 AudioComponentID);

	void UpdateInteriorSettings(bool bFullUpdate);

public:

	/**
	 * True if we should automatically attach to AutoAttachParent when Played, and detach from our parent when playback is completed.
	 * This overrides any current attachment that may be present at the time of activation (deferring initial attachment until activation, if AutoAttachParent is null).
	 * If enabled, this AudioComponent's WorldLocation will no longer be reliable when not currently playing audio, and any attach children will also be detached/attached along with it.
	 * When enabled, detachment occurs regardless of whether AutoAttachParent is assigned, and the relative transform from the time of activation is restored.
	 * This also disables attachment on dedicated servers, where we don't actually activate even if bAutoActivate is true.
	 * @see AutoAttachParent, AutoAttachSocketName, AutoAttachLocationType
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Attachment)
	uint8 bAutoManageAttachment:1;

	/**
	 * Component we automatically attach to when activated, if bAutoManageAttachment is true.
	 * If null during registration, we assign the existing AttachParent and defer attachment until we activate.
	 * @see bAutoManageAttachment
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category=Attachment, meta=(EditCondition="bAutoManageAttachment"))
	TWeakObjectPtr<USceneComponent> AutoAttachParent;

	/**
	 * Socket we automatically attach to on the AutoAttachParent, if bAutoManageAttachment is true.
	 * @see bAutoManageAttachment
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attachment, meta=(EditCondition="bAutoManageAttachment"))
	FName AutoAttachSocketName;

	/**
	 * Options for how we handle our location when we attach to the AutoAttachParent, if bAutoManageAttachment is true.
	 * @see bAutoManageAttachment, EAttachmentRule
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment, meta = (EditCondition = "bAutoManageAttachment"))
	EAttachmentRule AutoAttachLocationRule;

	/**
	 * Options for how we handle our rotation when we attach to the AutoAttachParent, if bAutoManageAttachment is true.
	 * @see bAutoManageAttachment, EAttachmentRule
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment, meta = (EditCondition = "bAutoManageAttachment"))
	EAttachmentRule AutoAttachRotationRule;

	/**
	 * Options for how we handle our scale when we attach to the AutoAttachParent, if bAutoManageAttachment is true.
	 * @see bAutoManageAttachment, EAttachmentRule
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment, meta = (EditCondition = "bAutoManageAttachment"))
	EAttachmentRule AutoAttachScaleRule;

private:

	/** Saved relative transform before auto attachement. Used during detachment to restore the transform if we had automatically attached. */
	FVector SavedAutoAttachRelativeLocation;
	FRotator SavedAutoAttachRelativeRotation;
	FVector SavedAutoAttachRelativeScale3D;

	/** Did we auto attach during activation? Used to determine if we should restore the relative transform during detachment. */
	uint8 bDidAutoAttach : 1;

	/** Restore relative transform from auto attachment and optionally detach from parent (regardless of whether it was an auto attachment). */
	void CancelAutoAttachment(bool bDetachFromParent);

protected:
	/** Utility function called by Play and FadeIn to start a sound playing. */
	void PlayInternal(const float StartTime = 0.f, const float FadeInDuration = 0.f, const float FadeVolumeLevel = 1.f);

private:
	
#if WITH_EDITORONLY_DATA
	/** Utility function that updates which texture is displayed on the sprite dependent on the properties of the Audio Component. */
	void UpdateSpriteTexture();
#endif

	/** A count of how many times we've started playing */
	int32 ActiveCount;

	static uint64 AudioComponentIDCounter;
	static TMap<uint64, UAudioComponent*> AudioIDToComponentMap;

	uint64 AudioComponentID;

};



