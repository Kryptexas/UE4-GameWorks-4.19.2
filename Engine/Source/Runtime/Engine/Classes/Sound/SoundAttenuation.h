// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/Attenuation.h"
#include "IAudioExtensionPlugin.h"
#include "SoundAttenuation.generated.h"

UENUM()
enum ESoundDistanceCalc
{
	SOUNDDISTANCE_Normal,
	SOUNDDISTANCE_InfiniteXYPlane,
	SOUNDDISTANCE_InfiniteXZPlane,
	SOUNDDISTANCE_InfiniteYZPlane,
	SOUNDDISTANCE_MAX,
};

UENUM()
enum ESoundSpatializationAlgorithm
{
	SPATIALIZATION_Default,
	SPATIALIZATION_HRTF,
};

/*
The settings for attenuating.
*/
USTRUCT(BlueprintType)
struct ENGINE_API FSoundAttenuationSettings : public FBaseAttenuationSettings
{
	GENERATED_USTRUCT_BODY()

	/* Enable attenuation via volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attenuation )
	uint32 bAttenuate:1;

	/* Enable the source to be positioned in 3D. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attenuation )
	uint32 bSpatialize:1;

	/* Enable attenuation via low pass filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LowPassFilter )
	uint32 bAttenuateWithLPF:1;

	/** Whether or not listener-focus calculations are enabled for this attenuation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus, meta = (EditCondition = "bSpatialize"))
	uint32 bEnableListenerFocus:1;

	/** Whether or not smooth volume interpolation is enabled for sounds as they transition in/out of focus */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus, meta = (EditCondition = "bEnableListenerFocus"))
	uint32 bEnableFocusInterpolation:1;

	/** Whether or not to enable line-of-sight occlusion checking for the sound that plays in this audio component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Occlusion, meta = (EditCondition = "bSpatialize"))
	uint32 bEnableOcclusion:1;

	/** Whether or not to enable complex geometry occlusion checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Occlusion)
	uint32 bUseComplexCollisionForOcclusion:1;

	UPROPERTY()
	TEnumAsByte<enum ESoundDistanceCalc> DistanceType_DEPRECATED;

	/** At what distance we start treating the sound source as spatialized */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attenuation, meta=(ClampMin = "0", EditCondition="bSpatialize", DisplayName="Non-Spatialized Radius"))
	float OmniRadius;

	/** The distance between left and right stereo channels when stereo assets spatialized. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation, meta = (ClampMin = "0", EditCondition = "bSpatialize", DisplayName = "3D Stereo Spread"))
	float StereoSpread;

	/** Which spatialization algorithm to use if spatializing mono sources. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation, meta = (ClampMin = "0", EditCondition = "bSpatialize", DisplayName = "Spatialization Algorithm"))
	TEnumAsByte<enum ESoundSpatializationAlgorithm> SpatializationAlgorithm;

	/** Settings to use with occlusion plugin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation, meta = (EditCondition = "bSpatialize"))
	USpatializationPluginSourceSettingsBase* SpatializationPluginSettings;

	UPROPERTY()
	float RadiusMin_DEPRECATED;

	UPROPERTY()
	float RadiusMax_DEPRECATED;

	/* The range at which to start applying a low pass filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LowPassFilter )
	float LPFRadiusMin;

	/* The range at which to apply the maximum amount of low pass filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LowPassFilter )
	float LPFRadiusMax;

	/* The Frequency in hertz at which to set the LPF when the sound is at LPFRadiusMin. (defaults to bypass) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LowPassFilter)
	float LPFFrequencyAtMin;

	/* The Frequency in hertz at which to set the LPF when the sound is at LPFRadiusMax. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LowPassFilter)
	float LPFFrequencyAtMax;

	/** Azimuth angle (in degrees) relative to the listener forward vector which defines the focus region of sounds. Sounds playing at an angle less than this will be in focus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus, meta = (EditCondition = "bEnableListenerFocus"))
	float FocusAzimuth;

	/** Azimuth angle (in degrees) relative to the listener forward vector which defines the non-focus region of sounds. Sounds playing at an angle greater than this will be out of focus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus, meta = (EditCondition = "bEnableListenerFocus"))
	float NonFocusAzimuth;

	/** Amount to scale the distance calculation of sounds that are in-focus. Can be used to make in-focus sounds appear to be closer or further away than they actually are. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableListenerFocus"))
	float FocusDistanceScale;

	/** Amount to scale the distance calculation of sounds that are not in-focus. Can be used to make in-focus sounds appear to be closer or further away than they actually are.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableListenerFocus"))
	float NonFocusDistanceScale;

	/** Amount to scale the priority of sounds that are in focus. Can be used to boost the priority of sounds that are in focus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableListenerFocus"))
	float FocusPriorityScale;

	/** Amount to scale the priority of sounds that are not in-focus. Can be used to reduce the priority of sounds that are not in focus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableListenerFocus"))
	float NonFocusPriorityScale;

	/** Amount to attenuate sounds that are in focus. Can be overridden at the sound-level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableListenerFocus"))
	float FocusVolumeAttenuation;

	/** Amount to attenuate sounds that are not in focus. Can be overridden at the sound-level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableListenerFocus"))
	float NonFocusVolumeAttenuation;

	/** Scalar used to increase interpolation speed upwards to the target Focus value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableFocusInterpolation"))
	float FocusAttackInterpSpeed;

	/** Scalar used to increase interpolation speed downwards to the target Focus value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableFocusInterpolation"))
	float FocusReleaseInterpSpeed;

	/* Which trace channel to use for audio occlusion checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation)
	TEnumAsByte<enum ECollisionChannel> OcclusionTraceChannel;

	/** The low pass filter frequency (in hertz) to apply if the sound playing in this audio component is occluded. This will override the frequency set in LowPassFilterFrequency. A frequency of 0.0 is the device sample rate and will bypass the filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Occlusion, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableOcclusion"))
	float OcclusionLowPassFilterFrequency;

	/** The amount of volume attenuation to apply to sounds which are occluded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Occlusion, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableOcclusion"))
	float OcclusionVolumeAttenuation;

	/** The amount of time in seconds to interpolate to the target OcclusionLowPassFilterFrequency when a sound is occluded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Occlusion, meta = (ClampMin = "0", UIMin = "0.0", EditCondition = "bEnableOcclusion"))
	float OcclusionInterpolationTime;

	/** Settings to use with occlusion plugin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Occlusion, meta=(EditCondition = "bEnableOcclusion"))
	UOcclusionPluginSourceSettingsBase* OcclusionPluginSettings;

	/** Settings to use with reverb plugin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation)
	UReverbPluginSourceSettingsBase* ReverbPluginSettings;

	/** The amount to send to master reverb when sound is ReverbDistanceMin from listener. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation)
	float ReverbWetLevelMin;

	/** The amount to send to master reverb when sound is at ReverbDistanceMax from listener. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation)
	float ReverbWetLevelMax;

	/** The distance which defines the amount of reverb wet level defined in ReverbWetLevelMin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation)
	float ReverbDistanceMin;

	/** The distance which defines the amount of reverb wet level defined in ReverbDistanceMax. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation)
	float ReverbDistanceMax;

	FSoundAttenuationSettings()
		: bAttenuate(true)
		, bSpatialize(true)
		, bAttenuateWithLPF(false)
		, bEnableListenerFocus(false)
		, bEnableFocusInterpolation(false)
		, bEnableOcclusion(false)
		, bUseComplexCollisionForOcclusion(false)
		, DistanceType_DEPRECATED(SOUNDDISTANCE_Normal)
		, OmniRadius(0.0f)
		, StereoSpread(0.0f)
		, SpatializationAlgorithm(ESoundSpatializationAlgorithm::SPATIALIZATION_Default)
		, SpatializationPluginSettings(nullptr)
		, RadiusMin_DEPRECATED(400.f)
		, RadiusMax_DEPRECATED(4000.f)
		, LPFRadiusMin(3000.f)
		, LPFRadiusMax(6000.f)
		, LPFFrequencyAtMin(20000.f)
		, LPFFrequencyAtMax(20000.f)
		, FocusAzimuth(30.0f)
		, NonFocusAzimuth(60.0f)
		, FocusDistanceScale(1.0f)
		, NonFocusDistanceScale(1.0f)
		, FocusPriorityScale(1.0f)
		, NonFocusPriorityScale(1.0f)
		, FocusVolumeAttenuation(1.0f)
		, NonFocusVolumeAttenuation(1.0f)
		, FocusAttackInterpSpeed(1.0f)
		, FocusReleaseInterpSpeed(1.0f)
		, OcclusionTraceChannel(ECC_Visibility)
		, OcclusionLowPassFilterFrequency(20000.f)
		, OcclusionVolumeAttenuation(1.0f)
		, OcclusionInterpolationTime(0.1f)
		, OcclusionPluginSettings(nullptr)
		, ReverbPluginSettings(nullptr)
		, ReverbWetLevelMin(0.3f)
		, ReverbWetLevelMax(0.95f)
		, ReverbDistanceMin(AttenuationShapeExtents.X)
		, ReverbDistanceMax(AttenuationShapeExtents.X + FalloffDistance)
	{
	}

	bool operator==(const FSoundAttenuationSettings& Other) const;
	void PostSerialize(const FArchive& Ar);

	virtual void CollectAttenuationShapesForVisualization(TMultiMap<EAttenuationShape::Type, FBaseAttenuationSettings::AttenuationShapeDetails>& ShapeDetailsMap) const override;
	float GetFocusPriorityScale(const struct FGlobalFocusSettings& FocusSettings, float FocusFactor) const;
	float GetFocusAttenuation(const struct FGlobalFocusSettings& FocusSettings, float FocusFactor) const;
	float GetFocusDistanceScale(const struct FGlobalFocusSettings& FocusSettings, float FocusFactor) const;
};

DEPRECATED(4.15, "FAttenuationSettings has been renamed FSoundAttenuationSettings")
typedef FSoundAttenuationSettings FAttenuationSettings;

template<>
struct TStructOpsTypeTraits<FSoundAttenuationSettings> : public TStructOpsTypeTraitsBase2<FSoundAttenuationSettings>
{
	enum 
	{
		WithPostSerialize = true,
	};
};

/** 
 * Defines how a sound changes volume with distance to the listener
 */
UCLASS(BlueprintType, hidecategories=Object, editinlinenew, MinimalAPI)
class USoundAttenuation : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Settings)
	FSoundAttenuationSettings Attenuation;
};
