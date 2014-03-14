// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** 
 * The base class for a playable sound object 
 */

#include "SoundBase.generated.h"

struct FActiveSound;
struct FSoundParseParameters;
struct FWaveInstance;
class  USoundClass;

UENUM()
namespace EMaxConcurrentResolutionRule
{
	enum Type
	{
		PreventNew					UMETA(ToolTip = "When Max Concurrent sounds are active do not start a new sound."),
		StopOldest					UMETA(ToolTip = "When Max Concurrent sounds are active stop the oldest and start a new one."),
		StopFarthestThenPreventNew	UMETA(ToolTip = "When Max Concurrent sounds are active stop the furthest sound.  If all sounds are the same distance then do not start a new sound."),
		StopFarthestThenOldest		UMETA(ToolTip = "When Max Concurrent sounds are active stop the furthest sound.  If all sounds are the same distance then stop the oldest.")
	};
}

UCLASS(config=Engine, hidecategories=Object, abstract, editinlinenew, MinimalAPI, BlueprintType)
class USoundBase : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Config)
	FString DefaultSoundClassName;

	UPROPERTY()
	FName SoundClass_DEPRECATED;

	/** Sound group this sound cue belongs to */
	UPROPERTY(EditAnywhere, Category=Sound, meta=(DisplayName = "Sound Class"))
	USoundClass* SoundClassObject;

	/** For debugging purpose only . */
	UPROPERTY(EditAnywhere, Category=Sound)
	uint32 bDebug:1;

	/** If we try to play a new version of this sound when at the max concurrent count how should it be resolved. */
	UPROPERTY(EditAnywhere, Category=Sound)
	TEnumAsByte<EMaxConcurrentResolutionRule::Type> MaxConcurrentResolutionRule;


	/** Maximum number of times this sound can be played concurrently. */
	UPROPERTY(EditAnywhere, Category=Sound)
	int32 MaxConcurrentPlayCount;

	/** Duration of sound in seconds. */
	UPROPERTY(Category=Info, AssetRegistrySearchable, VisibleAnywhere, BlueprintReadOnly)
	float Duration;

	/** Attenuation settings package for the sound */
	UPROPERTY(EditAnywhere, Category=Attenuation)
	USoundAttenuation* AttenuationSettings;

public:	
	/** Number of times this cue is currently being played. */
	int32 CurrentPlayCount;

	// Begin UObject interface.
	virtual void PostInitProperties() OVERRIDE;
	// End UObject interface.
	
	/** Returns whether the sound base is set up in a playable manner */
	virtual bool IsPlayable() const;

	/** Returns a pointer to the attenuation settings that are to be applied for this node */
	virtual const FAttenuationSettings* GetAttenuationSettingsToApply() const;

	/**
	 * Checks to see if a location is audible
	 */
	ENGINE_API bool IsAudible( const FVector& SourceLocation, const FVector& ListenerLocation, AActor* SourceActor, bool& bIsOccluded, bool bCheckOcclusion );

	/** 
	 * Does a simple range check to all listeners to test hearability
	 */
	ENGINE_API bool IsAudibleSimple( const FVector Location );

	/** 
	 * Returns the farthest distance at which the sound could be heard
	 */
	virtual float GetMaxAudibleDistance();

	/** 
	 * Returns the length of the sound
	 */
	virtual float GetDuration();

	virtual float GetVolumeMultiplier();
	virtual float GetPitchMultiplier();

	/** 
	 * Parses the Sound to generate the WaveInstances to play
	 */
	virtual void Parse( class FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& AudioComponent, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) { }
};

