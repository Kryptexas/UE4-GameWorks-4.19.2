// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SynthComponent.h"
#include "AudioDevice.h"
#include "AudioMixerLog.h"
#include "Components/BillboardComponent.h"

USynthSound::USynthSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, OwningSynthComponent(nullptr)
{
}

void USynthSound::Init(USynthComponent* InSynthComponent, int32 InNumChannels)
{
	OwningSynthComponent = InSynthComponent;
	bVirtualizeWhenSilent = true;
	NumChannels = InNumChannels;
	bCanProcessAsync = true;
	Duration = INDEFINITELY_LOOPING_DURATION;
	bLooping = true;
	SampleRate = InSynthComponent->GetAudioDevice()->SampleRate;
}

bool USynthSound::OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples)
{
	GeneratedPCMData.Reset();
	GeneratedPCMData.AddZeroed(NumSamples);

	OwningSynthComponent->OnGeneratePCMAudio(GeneratedPCMData);

	OutAudio.Append((uint8*)GeneratedPCMData.GetData(), NumSamples * sizeof(int16));

	return true;
}

USynthComponent::USynthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create the audio component which will be used to play the procedural sound wave
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));

	AudioComponent->bAutoActivate = true;
	AudioComponent->bStopWhenOwnerDestroyed = true;
	AudioComponent->bShouldRemainActiveIfDropped = true;
	AudioComponent->Mobility = EComponentMobility::Movable;

	bAutoActivate = true;
	bStopWhenOwnerDestroyed = true;

	bNeverNeedsRenderUpdate = true;
	bUseAttachParentBound = true; // Avoid CalcBounds() when transform changes.

	bIsSynthPlaying = false;
	bIsInitialized = false;

	// Set the default sound class
	SoundClass = USoundBase::DefaultSoundClassObject;

#if WITH_EDITORONLY_DATA
	AudioComponent->bVisualizeComponent = false;
	bVisualizeComponent = false;
#endif
}

void USynthComponent::Activate(bool bReset)
{
	if (bReset || ShouldActivate() == true)
	{
		Start();
		if (bIsActive)
		{
			OnComponentActivated.Broadcast(this, bReset);
		}
	}
}

void USynthComponent::Deactivate()
{
	if (ShouldActivate() == false)
	{
		Stop();

		if (!bIsActive)
		{
			OnComponentDeactivated.Broadcast(this);
		}
	}
}

void USynthComponent::OnRegister()
{
	if (AudioComponent->GetAttachParent() == nullptr && !AudioComponent->IsAttachedTo(this))
	{
		AudioComponent->SetupAttachment(this);
	}

	Super::OnRegister();

	if (!bIsInitialized)
	{
		bIsInitialized = true;

		const int32 SampleRate = GetAudioDevice()->SampleRate;

#if SYNTH_GENERATOR_TEST_TONE
		NumChannels = 2;
		TestSineLeft.Init(SampleRate, 440.0f, 0.5f);
		TestSineRight.Init(SampleRate, 220.0f, 0.5f);
#else	
		// Initialize the synth component
		this->Init(SampleRate);

		if (NumChannels < 0 || NumChannels > 2)
		{
			UE_LOG(LogAudioMixer, Error, TEXT("Synthesis component '%s' has set an invalid channel count '%d' (only mono and stereo currently supported)."), *GetName(), NumChannels);
		}

		NumChannels = FMath::Clamp(NumChannels, 1, 2);
#endif

		Synth = NewObject<USynthSound>(this, TEXT("Synth"));	

		// Copy sound base data to the sound
		Synth->SourceEffectChain = SourceEffectChain;
		Synth->DefaultMasterReverbSendAmount = DefaultMasterReverbSendAmount;
		Synth->SoundSubmixObject = SoundSubmix;
		Synth->SoundSubmixSends = SoundSubmixSends;

		Synth->Init(this, NumChannels);
	}
}

void USynthComponent::OnUnregister()
{
	// Route OnUnregister event.
	Super::OnUnregister();

	// Don't stop audio and clean up component if owner has been destroyed (default behaviour). This function gets
	// called from AActor::ClearComponents when an actor gets destroyed which is not usually what we want for one-
	// shot sounds.
	AActor* Owner = GetOwner();
	if (!Owner || bStopWhenOwnerDestroyed)
	{
		Stop();
	}
}

bool USynthComponent::IsReadyForOwnerToAutoDestroy() const
{
	return !AudioComponent || (AudioComponent && !AudioComponent->IsPlaying());
}

#if WITH_EDITOR
void USynthComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (bIsActive)
	{
		// If this is an auto destroy component we need to prevent it from being auto-destroyed since we're really just restarting it
		const bool bWasAutoDestroy = bAutoDestroy;
		bAutoDestroy = false;
		Stop();
		bAutoDestroy = bWasAutoDestroy;
		Start();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void USynthComponent::PumpPendingMessages()
{
	TFunction<void()> Command;
	while (CommandQueue.Dequeue(Command))
	{
		Command();
	}

	ESynthEvent SynthEvent;
	while (PendingSynthEvents.Dequeue(SynthEvent))
	{
		switch (SynthEvent)
		{
			case ESynthEvent::Start:
				bIsSynthPlaying = true;
				this->OnStart();
				break;

			case ESynthEvent::Stop:
				bIsSynthPlaying = false;
				this->OnStop();
				break;

			default:
				break;
		}
	}
}

void USynthComponent::OnGeneratePCMAudio(TArray<int16>& GeneratedPCMData)
{
	PumpPendingMessages();
	check(GeneratedPCMData.Num() > 0);

	AudioFloatData.Reset();

#if SYNTH_GENERATOR_TEST_TONE
	if (NumChannels == 1)
	{
		for (int32 i = 0; i < SamplesNeeded; ++i)
		{
			const float SampleFloat = TestSineLeft.ProcessAudio();
			const int16 SamplePCM = (int16)(32767.0f * SampleFloat);
			AudioData.Add(SamplePCM);
		}
	}
	else
	{
		for (int32 i = 0; i < SamplesNeeded; ++i)
		{
			float SampleFloat = TestSineLeft.ProcessAudio();
			int16 SamplePCM = (int16)(32767.0f * SampleFloat);
			AudioData.Add(SamplePCM);

			SampleFloat = TestSineRight.ProcessAudio();
			SamplePCM = (int16)(32767.0f * SampleFloat);
			AudioData.Add(SamplePCM);
		}
	}
#else
	AudioFloatData.AddZeroed(GeneratedPCMData.Num());

	// Only call into the synth if we're actually playing, otherwise, we'll write out zero's
	if (bIsSynthPlaying)
	{
		this->OnGenerateAudio(AudioFloatData);

		// Convert the float data to int16 data
		const float* AudioFloatDataPtr = AudioFloatData.GetData();
		for (int32 i = 0; i < GeneratedPCMData.Num(); ++i)
		{
			GeneratedPCMData[i] = (int16)(32767.0f * AudioFloatDataPtr[i]);
		}
	}
#endif
}

void USynthComponent::Start()
{
	if (AudioComponent)
	{
		// Copy the attenuation and concurrency data from the synth component to the audio component
		AudioComponent->AttenuationSettings = AttenuationSettings;
		AudioComponent->bOverrideAttenuation = bOverrideAttenuation;
		AudioComponent->ConcurrencySettings = ConcurrencySettings;
		AudioComponent->AttenuationOverrides = AttenuationOverrides;
		AudioComponent->SoundClassOverride = SoundClass;

		// Set the audio component's sound to be our procedural sound wave
		AudioComponent->SetSound(Synth);
		AudioComponent->Play(0);

		bIsActive = AudioComponent->IsActive();

		if (bIsActive)
		{
			PendingSynthEvents.Enqueue(ESynthEvent::Start);
		}
	}
}

void USynthComponent::Stop()
{
	if (bIsActive)
	{
		PendingSynthEvents.Enqueue(ESynthEvent::Stop);

		if (AudioComponent)
		{
			AudioComponent->Stop();
		}

		bIsActive = false;
	}
}

bool USynthComponent::IsPlaying() const
{
	return AudioComponent && AudioComponent->IsPlaying();
}

void USynthComponent::SetSubmixSend(USoundSubmix* Submix, float SendLevel)
{
	if (AudioComponent)
	{
		AudioComponent->SetSubmixSend(Submix, SendLevel);
	}
}


void USynthComponent::SynthCommand(TFunction<void()> Command)
{
	CommandQueue.Enqueue(MoveTemp(Command));
}