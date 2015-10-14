// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneParticleTrackInstance.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneParticleTrack.h"
#include "MovieSceneParticleSection.h"
#include "Particles/Emitter.h"


FMovieSceneParticleTrackInstance::~FMovieSceneParticleTrackInstance()
{
}


void FMovieSceneParticleTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	// @todo Sequencer We need something analagous to Matinee 1's particle replay tracks
	// What we have here is simple toggling/triggering

	if (Position > LastPosition && Player.GetPlaybackStatus() == EMovieScenePlayerStatus::Playing)
	{
		
		const TArray<UMovieSceneSection*> Sections = ParticleTrack->GetAllParticleSections();
		EParticleKey::Type ParticleKey = EParticleKey::Active;
		bool bKeyFound = false;
		for (int32 i = 0; i < Sections.Num(); ++i)
		{
			UMovieSceneParticleSection* Section = Cast<UMovieSceneParticleSection>( Sections[i] );
			if (Section->IsActive())
			{
				FIntegralCurve& ParticleKeyCurve = Section->GetParticleCurve();
				FKeyHandle PreviousHandle = ParticleKeyCurve.FindKeyBeforeOrAt( Position );
				if ( ParticleKeyCurve.IsKeyHandleValid( PreviousHandle ) )
				{
					FIntegralKey& PreviousKey = ParticleKeyCurve.GetKey( PreviousHandle );
					if ( PreviousKey.Time > LastPosition )
					{
						ParticleKey = (EParticleKey::Type)PreviousKey.Value;
						bKeyFound = true;
					}
				}
			}
		}
		
		if ( bKeyFound )
		{
			for (int32 i = 0; i < RuntimeObjects.Num(); ++i)
			{
				AEmitter* Emitter = Cast<AEmitter>(RuntimeObjects[i]);
				if (Emitter)
				{
					if ( ParticleKey == EParticleKey::Active )
					{
						if ( Emitter->IsActive() )
						{
							Emitter->Deactivate();
						}
						Emitter->Activate();
					}
					else if( ParticleKey == EParticleKey::Inactive )
					{
						Emitter->Deactivate();
					}
				}
			}
		}
	}
	else
	{
		for (int32 i = 0; i < RuntimeObjects.Num(); ++i)
		{
			AEmitter* Emitter = Cast<AEmitter>(RuntimeObjects[i]);
			if (Emitter)
			{
				Emitter->Deactivate();
			}
		}
	}
}
