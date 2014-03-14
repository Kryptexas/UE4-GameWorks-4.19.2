// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*------------------------------------------------------------------------------------
	Audio includes.
------------------------------------------------------------------------------------*/

#include "AndroidAudioDevice.h"
#include "AudioDecompress.h"
#include "AudioEffect.h"
#include "Engine.h"

DEFINE_LOG_CATEGORY(LogAndroidAudio);


class FSLESAudioDeviceModule : public IAudioDeviceModule
{
public:

	/** Creates a new instance of the audio device implemented by the module. */
	virtual FAudioDevice* CreateAudioDevice() OVERRIDE
	{
		return new FSLESAudioDevice;
	}
};

IMPLEMENT_MODULE(FSLESAudioDeviceModule, AndroidAudio );
/*------------------------------------------------------------------------------------
	UALAudioDevice constructor and UObject interface.
------------------------------------------------------------------------------------*/

void FSLESAudioDevice::Teardown( void )
{
	// Flush stops all sources and deletes all buffers so sources can be safely deleted below.
	Flush( NULL );	
	
	// Destroy all sound sources
	for( int32 i = 0; i < Sources.Num(); i++ )
	{
		delete Sources[ i ];
	}
	
	//@todo android: teardown OpenSLES
}

/*------------------------------------------------------------------------------------
	UAudioDevice Interface.
------------------------------------------------------------------------------------*/
/**
 * Initializes the audio device and creates sources.
 *
 * @warning: 
 *
 * @return TRUE if initialization was successful, FALSE otherwise
 */
bool FSLESAudioDevice::InitializeHardware( void )
{
	UE_LOG( LogAndroidAudio, Warning, TEXT("SL Entered Init HW"));

	SLresult result;

	UE_LOG( LogAndroidAudio, Warning, TEXT("OpenSLES Initializing HW"));
	
	SLEngineOption EngineOption[] = { {(SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE} };
	
    // create engine
    result = slCreateEngine( &SL_EngineObject, 1, EngineOption, 0, NULL, NULL);
    //check(SL_RESULT_SUCCESS == result);
	if (SL_RESULT_SUCCESS != result)
	{
		UE_LOG( LogAndroidAudio, Error, TEXT("Engine create failed %d"), int32(result));
	}
	
    // realize the engine
    result = (*SL_EngineObject)->Realize(SL_EngineObject, SL_BOOLEAN_FALSE);
    check(SL_RESULT_SUCCESS == result);
	
    // get the engine interface, which is needed in order to create other objects
    result = (*SL_EngineObject)->GetInterface(SL_EngineObject, SL_IID_ENGINE, &SL_EngineEngine);
    check(SL_RESULT_SUCCESS == result);
	
	// create output mix, with environmental reverb specified as a non-required interface    
    result = (*SL_EngineEngine)->CreateOutputMix( SL_EngineEngine, &SL_OutputMixObject, 0, NULL, NULL );
    check(SL_RESULT_SUCCESS == result);
	
    // realize the output mix
    result = (*SL_OutputMixObject)->Realize(SL_OutputMixObject, SL_BOOLEAN_FALSE);
    check(SL_RESULT_SUCCESS == result);
	
	UE_LOG( LogAndroidAudio, Warning, TEXT("OpenSLES Initialized"));
    // ignore unsuccessful result codes for env

	
	// Default to sensible channel count.
	if( MaxChannels < 1 )
	{  
		MaxChannels = 12;
	}
	
	
	// Initialize channels.
	for( int32 i = 0; i < FMath::Min( MaxChannels, 12 ); i++ )
	{
		FSLESSoundSource* Source = new FSLESSoundSource( this );		
		Sources.Add( Source );
		FreeSources.Add( Source );
	}
	
	if( Sources.Num() < 1 )
	{
		UE_LOG( LogAndroidAudio,  Warning, TEXT( "OpenSLAudio: couldn't allocate any sources" ) );
		return false;
	}
	
	// Update MaxChannels in case we couldn't create enough sources.
	MaxChannels = Sources.Num();
	UE_LOG( LogAndroidAudio, Warning, TEXT( "OpenSLAudioDevice: Allocated %i sources" ), MaxChannels );
	
	// Set up a default (nop) effects manager 
	Effects = new FAudioEffectsManager( this );
	
	// Initialized.
	NextResourceID = 1;

	return true;
}

/**
 * Update the audio device and calculates the cached inverse transform later
 * on used for spatialization.
 *
 * @param	Realtime	whether we are paused or not
 */
void FSLESAudioDevice::Update( bool Realtime )
{
	//Super::Update( Realtime );
	
	//@todo android: UDPATE LISTENERS - Android OpenSLES doesn't support 3D, is there anything to do here?
}


/**
 * Frees the bulk resource data associated with this SoundNodeWave.
 *
 * @param	SoundNodeWave	wave object to free associated bulk data
 */
void FSLESAudioDevice::FreeResource( USoundWave* SoundNodeWave )
{
	// Just in case the data was created but never uploaded
	if( SoundNodeWave->RawPCMData )
	{
		FMemory::Free( SoundNodeWave->RawPCMData );
		SoundNodeWave->RawPCMData = NULL;
	}
	
	// Find buffer for resident wavs
	if( SoundNodeWave->ResourceID )
	{
		// Find buffer associated with resource id.
		FSLESSoundBuffer* Buffer = WaveBufferMap.FindRef( SoundNodeWave->ResourceID );
		if( Buffer )
		{
			// Remove from buffers array.
			Buffers.Remove( Buffer );
			
			// See if it is being used by a sound source...
			for( int32 SrcIndex = 0; SrcIndex < Sources.Num(); SrcIndex++ )
			{
				FSLESSoundSource* Src = ( FSLESSoundSource* )( Sources[ SrcIndex ] );
				if( Src && Src->Buffer && ( Src->Buffer == Buffer ) )
				{
					Src->Stop();
					break;
				}
			}
			
			delete Buffer;
		}
		
		SoundNodeWave->ResourceID = 0;
	}
	
	// .. or reference to compressed data
	SoundNodeWave->RemoveAudioResource();
	
	// Stat housekeeping
	//DEC_DWORD_STAT_BY( STAT_AudioMemorySize, SoundNodeWave->GetResourceSize() );
	//DEC_DWORD_STAT_BY( STAT_AudioMemory, SoundNodeWave->GetResourceSize() );
}



/** 
 * Displays debug information about the loaded sounds
 */
void FSLESAudioDevice::ListSounds( const TCHAR* Cmd, FOutputDevice& Ar )
{
	int	TotalSoundSize = 0;

	Ar.Logf( TEXT( "Sound resources:" ) );

	TArray<FSLESSoundBuffer*> AllSounds = Buffers;

	for( int i = 0; i < AllSounds.Num(); ++i )
	{
		FSLESSoundBuffer* Buffer = AllSounds[i];
		Ar.Logf( TEXT( "RawData: %8.2f Kb (%d channels at %d Hz) in sound %s" ), Buffer->GetSize() / 1024.0f, Buffer->GetNumChannels(), Buffer->SampleRate, *Buffer->ResourceName );
		TotalSoundSize += Buffer->GetSize();
	}

	Ar.Logf( TEXT( "%8.2f Kb for %d sounds" ), TotalSoundSize / 1024.0f, AllSounds.Num() );
}

FSoundSource* FSLESAudioDevice::CreateSoundSource()
{
	return new FSLESSoundSource(this);
}
