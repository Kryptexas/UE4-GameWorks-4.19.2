// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Audio.cpp: Unreal base audio.
=============================================================================*/

#include "EnginePrivate.h"
#include "SoundDefinitions.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"

#define LOCTEXT_NAMESPACE "AmbientSound"

/*-----------------------------------------------------------------------------
	AAmbientSound implementation.
-----------------------------------------------------------------------------*/
bool AAmbientSound::bUE4AudioRefactorMigrationUnderway = false;

AAmbientSound::AAmbientSound(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	AudioComponent = PCIP.CreateDefaultSubobject<UAudioComponent>(this, TEXT("AudioComponent0"));

	AudioComponent->bAutoActivate = true;
	AudioComponent->bStopWhenOwnerDestroyed = true;
	AudioComponent->bShouldRemainActiveIfDropped = true;
	AudioComponent->Mobility = EComponentMobility::Movable;

	RootComponent = AudioComponent;

	bAutoPlay_DEPRECATED = true;
	bReplicates = false;
	bHidden = true;
	bCanBeDamaged = false;
}

#if WITH_EDITOR
void AAmbientSound::CheckForErrors( void )
{
	Super::CheckForErrors();

	if( !AudioComponent.IsValid() )
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format( LOCTEXT( "MapCheck_Message_AudioComponentNull", "{ActorName} : Ambient sound actor has NULL AudioComponent property - please delete" ), Arguments ) ))
			->AddToken(FMapErrorToken::Create(FMapErrors::AudioComponentNull));
	}
	else if( AudioComponent->Sound == NULL )
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format( LOCTEXT( "MapCheck_Message_SoundCueNull", "{ActorName} : Ambient sound actor has NULL Sound Cue property" ), Arguments ) ))
			->AddToken(FMapErrorToken::Create(FMapErrors::SoundCueNull));
	}
}

bool AAmbientSound::GetReferencedContentObjects( TArray<UObject*>& Objects ) const
{
	if( AudioComponent->Sound )
	{
		Objects.Add( AudioComponent->Sound );
	}
	return true;
}

#endif

void AAmbientSound::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

#if WITH_EDITORONLY_DATA
	if ( AudioComponent && bHiddenEdLevel )
	{
		AudioComponent->Stop();
	}
#endif // WITH_EDITORONLY_DATA
}

FString AAmbientSound::GetInternalSoundCueName()
{
	FString CueName;
#if WITH_EDITORONLY_DATA
	CueName = GetActorLabel();
#endif
	if (CueName.Len() == 0)
	{
		CueName = GetName();
	}
	CueName += TEXT("_SoundCue");

	return CueName;
}

void AAmbientSound::MigrateSoundNodeInstance()
{
	if (SoundNodeInstance_DEPRECATED->SoundSlots.Num() == 0)
	{
		AudioComponent->Sound = NULL;
		return;
	}

	bool bLoopAndDelay = SoundNodeInstance_DEPRECATED->GetClass() == UDEPRECATED_SoundNodeAmbientNonLoop::StaticClass();
	bool bMixer = SoundNodeInstance_DEPRECATED->GetClass() == UDEPRECATED_SoundNodeAmbient::StaticClass();

	int32 ColumnCount = 0;

	USoundCue* SoundCue = ConstructObject<USoundCue>(USoundCue::StaticClass(), this, FName(*GetInternalSoundCueName()));
	AudioComponent->Sound = SoundCue;

	USoundNodeMixer* SoundMixer = NULL;
	USoundNodeRandom* SoundRandom = NULL;
	if (bMixer)
	{
		SoundMixer = SoundCue->ConstructSoundNode<USoundNodeMixer>();
		SoundCue->FirstNode = SoundMixer;
#if WITH_EDITOR
		SoundMixer->PlaceNode(ColumnCount++, 0, 1);
#endif //WITH_EDITOR
	}
	else
	{
		SoundRandom = SoundCue->ConstructSoundNode<USoundNodeRandom>();

		if (bLoopAndDelay)
		{
			UDEPRECATED_SoundNodeAmbientNonLoop* SoundNodeNonLoop = CastChecked<UDEPRECATED_SoundNodeAmbientNonLoop>(SoundNodeInstance_DEPRECATED);
			USoundNodeLooping* SoundLooping = SoundCue->ConstructSoundNode<USoundNodeLooping>();

			SoundLooping->CreateStartingConnectors();
			SoundCue->FirstNode = SoundLooping;
#if WITH_EDITOR
			SoundLooping->PlaceNode(ColumnCount++, 0, 1);
#endif //WITH_EDITOR

			if (SoundNodeNonLoop->DelayMin > 0.f || SoundNodeNonLoop->DelayMax > 0.f)
			{
				USoundNodeDelay* SoundDelay = SoundCue->ConstructSoundNode<USoundNodeDelay>();

				SoundDelay->DelayMin = SoundNodeNonLoop->DelayMin;
				SoundDelay->DelayMax = SoundNodeNonLoop->DelayMax;

				SoundLooping->ChildNodes[0] = SoundDelay;
#if WITH_EDITOR
				SoundDelay->PlaceNode(ColumnCount++, 0, 1);
#endif //WITH_EDITOR

				SoundDelay->CreateStartingConnectors();
				SoundDelay->ChildNodes[0] = SoundRandom;
			}
			else
			{
				SoundLooping->ChildNodes[0] = SoundRandom;
			}
		}
		else
		{
			SoundCue->FirstNode = SoundRandom;
		}
#if WITH_EDITOR
		SoundRandom->PlaceNode(ColumnCount++, 0, 1);
#endif //WITH_EDITOR
	}

	for(int32 SlotIndex = 0;SlotIndex < SoundNodeInstance_DEPRECATED->SoundSlots.Num(); ++SlotIndex)
	{
		USoundNodeWavePlayer* WavePlayer = SoundCue->ConstructSoundNode<USoundNodeWavePlayer>();

		WavePlayer->bLooping = bMixer;
		WavePlayer->SoundWave = SoundNodeInstance_DEPRECATED->SoundSlots[SlotIndex].SoundWave;

		if (bMixer)
		{
			SoundMixer->InsertChildNode(SlotIndex);
			SoundMixer->InputVolume[SlotIndex] = SoundNodeInstance_DEPRECATED->SoundSlots[SlotIndex].VolumeScale;

			if (SoundNodeInstance_DEPRECATED->SoundSlots[SlotIndex].PitchScale != 1.f)
			{
				USoundNodeModulator* SoundMod = SoundCue->ConstructSoundNode<USoundNodeModulator>();

				SoundMod->VolumeMin = SoundMod->VolumeMax = 1.f;
				SoundMod->PitchMin = SoundMod->PitchMax = SoundNodeInstance_DEPRECATED->SoundSlots[SlotIndex].PitchScale;

				SoundMod->CreateStartingConnectors();
				SoundMod->ChildNodes[0] = WavePlayer;
#if WITH_EDITOR
				SoundMod->PlaceNode(ColumnCount, SlotIndex, SoundNodeInstance_DEPRECATED->SoundSlots.Num());
#endif //WITH_EDITOR

				SoundMixer->ChildNodes[SlotIndex] = SoundMod;
			}
			else
			{
				SoundMixer->ChildNodes[SlotIndex] = WavePlayer;
			}
		}
		else
		{
			SoundRandom->InsertChildNode(SlotIndex);
			SoundRandom->Weights[SlotIndex] = SoundNodeInstance_DEPRECATED->SoundSlots[SlotIndex].Weight;

			if (   SoundNodeInstance_DEPRECATED->SoundSlots[SlotIndex].VolumeScale != 1.f
				|| SoundNodeInstance_DEPRECATED->SoundSlots[SlotIndex].PitchScale != 1.f)
			{
				USoundNodeModulator* SoundMod = SoundCue->ConstructSoundNode<USoundNodeModulator>();

				SoundMod->VolumeMin = SoundMod->VolumeMax = SoundNodeInstance_DEPRECATED->SoundSlots[SlotIndex].VolumeScale;
				SoundMod->PitchMin = SoundMod->PitchMax = SoundNodeInstance_DEPRECATED->SoundSlots[SlotIndex].PitchScale;

				SoundMod->CreateStartingConnectors();
				SoundMod->ChildNodes[0] = WavePlayer;
#if WITH_EDITOR
				SoundMod->PlaceNode(ColumnCount, SlotIndex, SoundNodeInstance_DEPRECATED->SoundSlots.Num());
#endif //WITH_EDITOR

				SoundRandom->ChildNodes[SlotIndex] = SoundMod;
			}
			else
			{
				SoundRandom->ChildNodes[SlotIndex] = WavePlayer;
			}
		}

#if WITH_EDITOR
		WavePlayer->PlaceNode(ColumnCount + 1, SlotIndex, SoundNodeInstance_DEPRECATED->SoundSlots.Num());
#endif //WITH_EDITOR
	}

#if WITH_EDITORONLY_DATA
	SoundCue->LinkGraphNodesFromSoundNodes();
#endif //WITH_EDITORONLY_DATA
}

void AAmbientSound::PostLoad()
{
	Super::PostLoad();

	if (GetLinkerUE4Version() < VER_UE4_UNIFY_AMBIENT_SOUND_ACTORS)
	{
		AudioComponent->ConditionalPostLoad();
		AudioComponent->bAutoActivate = bAutoPlay_DEPRECATED;

		if (SoundNodeInstance_DEPRECATED)
		{
			SoundNodeInstance_DEPRECATED->ConditionalPostLoad();

			AudioComponent->bOverrideAttenuation = true;
			AudioComponent->AttenuationOverrides.bAttenuate			= SoundNodeInstance_DEPRECATED->bAttenuate;
			AudioComponent->AttenuationOverrides.bSpatialize		= SoundNodeInstance_DEPRECATED->bSpatialize;
			AudioComponent->AttenuationOverrides.dBAttenuationAtMax	= SoundNodeInstance_DEPRECATED->dBAttenuationAtMax;
			AudioComponent->AttenuationOverrides.DistanceAlgorithm	= SoundNodeInstance_DEPRECATED->DistanceModel;
			AudioComponent->AttenuationOverrides.bAttenuateWithLPF	= SoundNodeInstance_DEPRECATED->bAttenuateWithLPF;
			AudioComponent->AttenuationOverrides.LPFRadiusMin		= SoundNodeInstance_DEPRECATED->LPFRadiusMin;
			AudioComponent->AttenuationOverrides.LPFRadiusMax		= SoundNodeInstance_DEPRECATED->LPFRadiusMax;

			AudioComponent->AttenuationOverrides.RadiusMin_DEPRECATED = SoundNodeInstance_DEPRECATED->RadiusMin;
			AudioComponent->AttenuationOverrides.RadiusMax_DEPRECATED = SoundNodeInstance_DEPRECATED->RadiusMax;
			AudioComponent->AttenuationOverrides.FalloffDistance	  = SoundNodeInstance_DEPRECATED->RadiusMax - SoundNodeInstance_DEPRECATED->RadiusMin;
			AudioComponent->AttenuationOverrides.AttenuationShapeExtents.X = SoundNodeInstance_DEPRECATED->RadiusMin;
			
			AudioComponent->VolumeModulationMin			= SoundNodeInstance_DEPRECATED->VolumeMin;
			AudioComponent->VolumeModulationMax			= SoundNodeInstance_DEPRECATED->VolumeMax;
			AudioComponent->PitchModulationMin			= SoundNodeInstance_DEPRECATED->PitchMin;
			AudioComponent->PitchModulationMax			= SoundNodeInstance_DEPRECATED->PitchMax;

			if (!bUE4AudioRefactorMigrationUnderway)
			{
				MigrateSoundNodeInstance();
			}
		}
	}
}

void AAmbientSound::FadeIn(float FadeInDuration, float FadeVolumeLevel)
{
	if (AudioComponent)
	{
		AudioComponent->FadeIn(FadeInDuration, FadeVolumeLevel);
	}
}

void AAmbientSound::FadeOut(float FadeOutDuration, float FadeVolumeLevel)
{
	if (AudioComponent)
	{
		AudioComponent->FadeOut(FadeOutDuration, FadeVolumeLevel);
	}
}

void AAmbientSound::AdjustVolume(float AdjustVolumeDuration, float AdjustVolumeLevel)
{
	if (AudioComponent)
	{
		AudioComponent->AdjustVolume(AdjustVolumeDuration, AdjustVolumeLevel);
	}
}

void AAmbientSound::Play(float StartTime)
{
	if (AudioComponent)
	{
		AudioComponent->Play(StartTime);
	}
}

void AAmbientSound::Stop()
{
	if (AudioComponent)
	{
		AudioComponent->Stop();
	}
}

#undef LOCTEXT_NAMESPACE