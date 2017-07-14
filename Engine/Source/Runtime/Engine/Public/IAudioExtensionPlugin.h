// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Features/IModularFeature.h"
#include "Features/IModularFeatures.h"
#include "IAudioExtensionPlugin.generated.h"

class FAudioDevice;

/**
* FSpatializationParams
* Struct for retrieving parameters needed for computing spatialization and occlusion plugins.
*/
struct FSpatializationParams
{
	/** The listener position (is likely at the origin). */
	FVector ListenerPosition;

	/** The listener orientation. */
	FVector ListenerOrientation;

	/** The emitter position relative to listener. */
	FVector EmitterPosition;

	/** The emitter world position. */
	FVector EmitterWorldPosition;

	/** The left channel position. */
	FVector LeftChannelPosition;

	/** The right channel position. */
	FVector RightChannelPosition;

	/** The distance between listener and emitter. */
	float Distance;

	/** The normalized omni radius, or the radius that will blend a sound to non-3d */
	float NormalizedOmniRadius;

	FSpatializationParams()
		: ListenerPosition(FVector::ZeroVector)
		, ListenerOrientation(FVector::ZeroVector)
		, EmitterPosition(FVector::ZeroVector)
		, EmitterWorldPosition(FVector::ZeroVector)
		, LeftChannelPosition(FVector::ZeroVector)
		, RightChannelPosition(FVector::ZeroVector)
		, Distance(0.0f)
		, NormalizedOmniRadius(0.0f)
	{}
};

struct FAudioPluginSourceInputData
{
	// The ID of the source voice
	int32 SourceId;

	// The ID of the audio component associated with the wave instance.
	int32 AudioComponentId;

	// The audio input buffer
	TArray<float>* AudioBuffer;

	// Number of channels of the source audio buffer.
	int32 NumChannels;

	// Spatialization parameters.
	const FSpatializationParams* SpatializationParams;
};

struct FAudioPluginSourceOutputData
{
	// The audio ouput buffer
	TArray<float> AudioBuffer;
};

/** This is a class which should be overridden to provide users with settings to use for individual sounds */
UCLASS(config = Engine, abstract, editinlinenew, BlueprintType)
class ENGINE_API USpatializationPluginSourceSettingsBase : public UObject
{
	GENERATED_BODY()
};

/**
* IAudioSpatialization
*
* This class represents instances of a plugin that will process spatialization for a stream of audio.
* Currently used to process a mono-stream through an HRTF spatialization algorithm into a stereo stream.
* This algorithm contains an audio effect assigned to every VoiceId (playing sound instance). It assumes
* the effect is updated in the audio engine update loop with new position information.
*
*/
class IAudioSpatialization
{
public:
	/** Virtual destructor */
	virtual ~IAudioSpatialization()
	{
	}

	/** DEPRECATED: sets the spatialization effect parameters. */
	virtual void SetSpatializationParameters(uint32 SourceId, const FSpatializationParams& Params)
	{
	}

	/** DEPRECATED: Gets the spatialization effect parameters. */
	virtual void GetSpatializationParameters(uint32 SourceId, FSpatializationParams& OutParams)
	{
	}
	
	/** DEPRECATED: Initializes the spatialization effect with the given buffer length. */
	virtual void InitializeSpatializationEffect(uint32 BufferLength)
	{
	}

	/** DEPRECATED: Uses the given HRTF algorithm to spatialize a mono audio stream. */
	virtual void ProcessSpatializationForVoice(uint32 SourceId, float* InSamples, float* OutSamples, const FVector& Position)
	{
	}

	/** DEPRECATED: Uses the given HRTF algorithm to spatialize a mono audio stream, assumes the parameters have already been set before processing. */
	virtual void ProcessSpatializationForVoice(uint32 SourceId, float* InSamples, float* OutSamples)
	{
	}

	/** Called when a source is assigned to a voice. */
	virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, USpatializationPluginSourceSettingsBase* InSettings)
	{
	}

	/** Called when a source is done playing and is released. */
	virtual void OnReleaseSource(const uint32 SourceId)
	{
	}

	/** Processes audio with the given input and output data structs.*/
	virtual void ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData)
	{
	}

	/** Returns whether or not the spatialization effect has been initialized */
	virtual bool IsSpatializationEffectInitialized() const
	{
		return false;
	}

	/** Initializes the spatialization plugin with the given buffer length. */
	virtual void Initialize(const uint32 SampleRate, const uint32 NumSources, const uint32 OutputBufferLength)
	{
	}

	/** Creates an audio spatialization effect. */
	virtual bool CreateSpatializationEffect(uint32 SourceId)
	{
		return true;
	}

	/**	Returns the spatialization effect for the given voice id. */
	virtual void* GetSpatializationEffect(uint32 SourceId)
	{
		return nullptr;
	}
};

/** This is a class which should be overridden to provide users with settings to use for individual sounds */
UCLASS(config = Engine, abstract, editinlinenew, BlueprintType)
class ENGINE_API UOcclusionPluginSourceSettingsBase : public UObject
{
	GENERATED_BODY()
};

class IAudioOcclusion
{
public:
	/** Virtual destructor */
	virtual ~IAudioOcclusion()
	{
	}

	/** Initialize the occlusion plugin with the same rate and number of sources. */
	virtual void Initialize(const int32 SampleRate, const int32 NumSources)
	{
	}

	/** Called when a source is assigned to a voice. */
	virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, UOcclusionPluginSourceSettingsBase* InSettings)
	{
	}

	/** Called when a source is done playing and is released. */
	virtual void OnReleaseSource(const uint32 SourceId)
	{
	}

	/** Processes audio with the given input and output data structs.*/
	virtual void ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData)
	{
	}
};

/** This is a class which should be overridden to provide users with settings to use for individual sounds */
UCLASS(config = Engine, abstract, editinlinenew, BlueprintType)
class ENGINE_API UReverbPluginSourceSettingsBase : public UObject
{
	GENERATED_BODY()
};
 
class IAudioReverb
{
public:
	/** Virtual destructor */
	virtual ~IAudioReverb()
	{
	}

	/** Initialize the reverb plugin with the same rate and number of sources. */
	virtual void Initialize(const int32 SampleRate, const int32 NumSources, const int32 FrameSize)
	{
	}

	/** Called when a source is assigned to a voice. */
	virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, UReverbPluginSourceSettingsBase* InSettings) = 0;

	/** Called when a source is done playing and is released. */
	virtual void OnReleaseSource(const uint32 SourceId) = 0;

	virtual class FSoundEffectSubmix* GetEffectSubmix(class USoundSubmix* Submix) = 0;

	/** Processes audio with the given input and output data structs.*/
	virtual void ProcessSourceAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData)
	{
	}
};

/** Override this and create it to receive listener changes. */
class IAudioListenerObserver
{
public:
	virtual ~IAudioListenerObserver()
	{
	}

	// Called when the listener is updated on the given audio device.
	virtual void OnListenerUpdated(FAudioDevice* AudioDevice, UWorld* ListenerWorld, const int32 ViewportIndex, const FTransform& ListenerTransform, const float InDeltaSeconds) = 0;
	virtual void OnListenerShutdown(FAudioDevice* AudioDevice) = 0;
};

typedef TSharedPtr<IAudioSpatialization, ESPMode::ThreadSafe> TAudioSpatializationPtr;
typedef TSharedPtr<IAudioOcclusion, ESPMode::ThreadSafe> TAudioOcclusionPtr;
typedef TSharedPtr<IAudioReverb, ESPMode::ThreadSafe> TAudioReverbPtr;
typedef TSharedPtr<IAudioListenerObserver, ESPMode::ThreadSafe> TAudioListenerObserverPtr;

/**
* The public interface of an audio plugin. Plugins that extend core features of the audio engine.
*/
class IAudioPlugin : public IModuleInterface, public IModularFeature
{
public:
	// IModularFeature
	static FName GetModularFeatureName()
	{
		static FName AudioExtFeatureName = FName(TEXT("AudioPlugin"));
		return AudioExtFeatureName;
	}

	// IModuleInterface
	virtual void StartupModule() override
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}

	/**
	* Singleton-like access to IAudioExtensionPlugin
	*
	* @return Returns IAudioExtensionPlugin singleton instance, loading the module on demand if needed
	*/
	static inline IAudioPlugin& Get()
	{
		return FModuleManager::LoadModuleChecked<IAudioPlugin>("AudioPlugin");
	}

	/**
	* Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	*
	* @return True if the module is loaded and ready to use
	*/
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("AudioPlugin");
	}

	/**
	* Initializes the audio plugin.
	*
	*/
	virtual void Initialize()
	{
	}

	/**
	* Shuts down the audio plugin.
	*
	*/
	virtual void Shutdown()
	{
	}

	virtual void OnDeviceShutdown(FAudioDevice* AudioDevice)
	{
	}

	virtual bool ImplementsSpatialization() const
	{
		return false;
	}

	virtual bool ImplementsOcclusion() const
	{
		return false;
	}

	virtual bool ImplementsReverb() const
	{
		return false;
	}

	virtual bool SupportsMultipleAudioDevices() const
	{
		return true;
	}

	virtual TAudioSpatializationPtr CreateSpatializationInterface(class FAudioDevice* AudioDevice)
	{
		return nullptr;
	}

	virtual TAudioOcclusionPtr CreateOcclusionInterface(class FAudioDevice* AudioDevice)
	{
		return nullptr;
	}

	virtual TAudioReverbPtr CreateReverbInterface(class FAudioDevice* AudioDevice)
	{
		return nullptr;
	}

	virtual TAudioListenerObserverPtr CreateListenerObserverInterface(class FAudioDevice* AudioDevice)
	{
		return nullptr;
	}
};
