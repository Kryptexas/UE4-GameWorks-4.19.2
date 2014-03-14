// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ALAudioDevice.cpp: Unreal OpenAL Audio interface object.

	Unreal is RHS with Y and Z swapped (or technically LHS with flipped axis)
=============================================================================*/

/*------------------------------------------------------------------------------------
	Audio includes.
------------------------------------------------------------------------------------*/

#include "HTML5AudioDevice.h"
#include "AudioDecompress.h"
#include "AudioEffect.h"
#include "Engine.h"


//     2 UU == 1"
// <=> 1 UU == 0.0127 m
#define AUDIO_DISTANCE_FACTOR ( 0.0127f )

ALCdevice* FALAudioDevice::HardwareDevice = NULL;
ALCcontext*	FALAudioDevice::SoundContext = NULL;


class FHTML5AudioDeviceModule : public IAudioDeviceModule
{
public:

	/** Creates a new instance of the audio device implemented by the module. */
	virtual FAudioDevice* CreateAudioDevice() OVERRIDE
	{
		return new FALAudioDevice;
	}
};

IMPLEMENT_MODULE(FHTML5AudioDeviceModule, HTML5Audio );
/*------------------------------------------------------------------------------------
	UALAudioDevice constructor and UObject interface.
------------------------------------------------------------------------------------*/

void FALAudioDevice::Teardown( void )
{
	// Flush stops all sources and deletes all buffers so sources can be safely deleted below.
	Flush( NULL );
	
	// Push any pending data to the hardware
	if( &alcProcessContext )
	{	
		alcProcessContext( SoundContext );
	}

	// Destroy all sound sources
	for( int i = 0; i < Sources.Num(); i++ )
	{
		delete Sources[ i ];
	}

	// Disable the context
	if( &alcMakeContextCurrent )
	{	
		alcMakeContextCurrent( NULL );
	}

	// Destroy the context
	if( &alcDestroyContext )
	{	
		alcDestroyContext( SoundContext );
		SoundContext = NULL;
	}

	// Close the hardware device
	if( &alcCloseDevice )
	{	
		alcCloseDevice( HardwareDevice );
		HardwareDevice = NULL;
	}

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
bool FALAudioDevice::InitializeHardware( void )
{
	// Make sure no interface classes contain any garbage
	Effects = NULL;
	DLLHandle = NULL;

	// Default to sensible channel count.
	if( MaxChannels < 1 )
	{
		MaxChannels = 32;
	}
	// Load ogg and vorbis dlls if they haven't been loaded yet
	//LoadVorbisLibraries();

	
	// Open device
	HardwareDevice = alcOpenDevice(  NULL );
	if( !HardwareDevice )
	{
		//UE_LOG(LogHTML5Audio, Log, TEXT( "ALAudio: no OpenAL devices found." ) );
		return  false  ;
	}

	// Display the audio device that was actually opened
	const ALCchar* OpenedDeviceName = alcGetString( HardwareDevice, ALC_DEVICE_SPECIFIER );
	//UE_LOG(LogHTML5Audio, Log, TEXT( "ALAudio: no OpenAL devices found." ) );
	//UE_LOG(LogHTML5Audio, Log, TEXT( "ALAudio device requested : %s" ), *DeviceName );
	//UE_LOG(LogHTML5Audio, Log, TEXT( "ALAudio device Opened : %s" ), OpenedDeviceName );

	// Create a context
	int Caps[] = 
	{ 
		ALC_FREQUENCY, 44100, 
		ALC_STEREO_SOURCES, 4, 
		0, 0 
    };
#if PLATFORM_HTML5_WIN32
	SoundContext = alcCreateContext( HardwareDevice, Caps );
#elif PLATFORM_HTML5
	SoundContext = alcCreateContext( HardwareDevice, 0 );
#endif 

	if( !SoundContext )
	{
		return false ;
	}

	alcMakeContextCurrent( SoundContext );
	

	// Make sure everything happened correctly
	if( alError( TEXT( "Init" ) ) )
	{
//		debugf( NAME_Init, TEXT( "ALAudio: alcMakeContextCurrent failed." ) );
		return false ;
	}

// 	debugf( NAME_Init, TEXT( "AL_VENDOR      : %s" ), ANSI_TO_TCHAR( ( ANSICHAR* )alGetString( AL_VENDOR ) ) );
// 	debugf( NAME_Init, TEXT( "AL_RENDERER    : %s" ), ANSI_TO_TCHAR( ( ANSICHAR* )alGetString( AL_RENDERER ) ) );
// 	debugf( NAME_Init, TEXT( "AL_VERSION     : %s" ), ANSI_TO_TCHAR( ( ANSICHAR* )alGetString( AL_VERSION ) ) );
// 	debugf( NAME_Init, TEXT( "AL_EXTENSIONS  : %s" ), ANSI_TO_TCHAR( ( ANSICHAR* )alGetString( AL_EXTENSIONS ) ) );
 
	// Get the enums for multichannel support
#if !PLATFORM_HTML5
	Surround40Format = alGetEnumValue( "AL_FORMAT_QUAD16" );
	Surround51Format = alGetEnumValue( "AL_FORMAT_51CHN16" );
	Surround61Format = alGetEnumValue( "AL_FORMAT_61CHN16" );
	Surround71Format = alGetEnumValue( "AL_FORMAT_71CHN16" );
#endif
	// Initialize channels.
	alError( TEXT( "Emptying error stack" ), 0 );
	for( int i = 0; i < (( MaxChannels >  MAX_AUDIOCHANNELS ) ? MAX_AUDIOCHANNELS : MaxChannels); i++ )
	{
		ALuint SourceId;
		alGenSources( 1, &SourceId );
		if( !alError( TEXT( "Init (creating sources)" ), 0 ) )
		{
			FALSoundSource* Source = new FALSoundSource( this );
			Source->SourceId = SourceId;
			Sources.Add( Source );
			FreeSources.Add( Source );
		}
		else
		{
			break;
		}
	}

	if( Sources.Num() < 1 )
	{
//		debugf( NAME_Error,TEXT( "ALAudio: couldn't allocate any sources" ) );
		return false ;
	}

	// Update MaxChannels in case we couldn't create enough sources.
	MaxChannels = Sources.Num();
/*	debugf( NAME_Init, TEXT( "ALAudioDevice: Allocated %i sources" ), MaxChannels );*/

	// Use our own distance model.
	alDistanceModel( AL_NONE );

	// Set up a default (nop) effects manager 
	Effects = new FAudioEffectsManager( this );

	// Initialized.
	NextResourceID = 1;


	return true ;
}

/**
 * Update the audio device and calculates the cached inverse transform later
 * on used for spatialization.
 *
 * @param	Realtime	whether we are paused or not
 */
void FALAudioDevice::Update( bool Realtime )
{
	FVector ListenerLocation	= Listeners[ 0 ].Transform.GetLocation();
	FVector ListenerFront		= Listeners[ 0 ].GetFront();
	FVector ListenerUp			= Listeners[ 0 ].GetUp();


	// Set Player position
	FVector Location;

	// See file header for coordinate system explanation.
	Location.X = ListenerLocation.X;
	Location.Y = ListenerLocation.Z; // Z/Y swapped on purpose, see file header
	Location.Z = ListenerLocation.Y; // Z/Y swapped on purpose, see file header
	Location *= AUDIO_DISTANCE_FACTOR;
	
	// Set Player orientation.
	FVector Orientation[2];

	// See file header for coordinate system explanation.
	Orientation[0].X = ListenerFront.X;
	Orientation[0].Y = ListenerFront.Z; // Z/Y swapped on purpose, see file header	
	Orientation[0].Z = ListenerFront.Y; // Z/Y swapped on purpose, see file header
	
	// See file header for coordinate system explanation.
	Orientation[1].X = ListenerUp.X;
	Orientation[1].Y = ListenerUp.Z; // Z/Y swapped on purpose, see file header
	Orientation[1].Z = ListenerUp.Y; // Z/Y swapped on purpose, see file header

	// Make the listener still and the sounds move relatively -- this allows 
	// us to scale the doppler effect on a per-sound basis.
	FVector Velocity = FVector( 0.0f, 0.0f, 0.0f );
	
	alListenerfv( AL_POSITION, ( ALfloat * )&Location );
	alListenerfv( AL_ORIENTATION, ( ALfloat * )&Orientation[0] );
	alListenerfv( AL_VELOCITY, ( ALfloat * )&Velocity );

	alError( TEXT( "UALAudioDevice::Update" ) );
}	


/**
 * Frees the bulk resource data associated with this SoundNodeWave.
 *
 * @param	SoundNodeWave	wave object to free associated bulk data
 */
void FALAudioDevice::FreeResource( USoundWave* SoundNodeWave )
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
		FALSoundBuffer* Buffer = WaveBufferMap.FindRef( SoundNodeWave->ResourceID );
		if( Buffer )
		{
			// Remove from buffers array.
			Buffers.Remove( Buffer );

			// See if it is being used by a sound source...
			for( int SrcIndex = 0; SrcIndex < Sources.Num(); SrcIndex++ )
			{
				FALSoundSource* Src = ( FALSoundSource* )( Sources[ SrcIndex ] );
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
	// @to-do 
// 	DEC_DWORD_STAT_BY( STAT_AudioMemorySize, SoundNodeWave->GetResourceSize() );
// 	DEC_DWORD_STAT_BY( STAT_AudioMemory, SoundNodeWave->GetResourceSize() );
}



/** 
 * Displays debug information about the loaded sounds
 */
void FALAudioDevice::ListSounds( const TCHAR* Cmd, FOutputDevice& Ar )
{
	int	TotalSoundSize = 0;

	Ar.Logf( TEXT( "Sound resources:" ) );

	TArray<FALSoundBuffer*> AllSounds = Buffers;

	for( int i = 0; i < AllSounds.Num(); ++i )
	{
		FALSoundBuffer* Buffer = AllSounds[i];
		Ar.Logf( TEXT( "RawData: %8.2f Kb (%d channels at %d Hz) in sound %s" ), Buffer->GetSize() / 1024.0f, Buffer->GetNumChannels(), Buffer->SampleRate, *Buffer->ResourceName );
		TotalSoundSize += Buffer->GetSize();
	}

	Ar.Logf( TEXT( "%8.2f Kb for %d sounds" ), TotalSoundSize / 1024.0f, AllSounds.Num() );
}

ALuint FALAudioDevice::GetInternalFormat( int NumChannels )
{
	ALuint InternalFormat = 0;

	switch( NumChannels )
	{
	case 0:
	case 3:
	case 5:
		break;
	case 1:
		InternalFormat = AL_FORMAT_MONO16;
		break;
	case 2:
		InternalFormat = AL_FORMAT_STEREO16;
		break;
#if !PLATFORM_HTML5
	case 4:
		InternalFormat = Surround40Format;
		break;
	case 6:
		InternalFormat = Surround51Format;
		break;	
	case 7:
		InternalFormat = Surround61Format;
		break;	
	case 8:
		InternalFormat = Surround71Format;
		break;
#endif 
	}

	return( InternalFormat );
}

/*------------------------------------------------------------------------------------
OpenAL utility functions
------------------------------------------------------------------------------------*/
//
//	FindExt
//
bool FALAudioDevice::FindExt( const TCHAR* Name )
{
	if( alIsExtensionPresent( TCHAR_TO_ANSI( Name ) ) || alcIsExtensionPresent( HardwareDevice, TCHAR_TO_ANSI( Name ) ) )
	{
		//UE_LOG ( LogHTML5Audio, log,TEXT( "Device supports: %s" )); 
		return( true );
	}
	return( true );
}

//
//	FindProc
//
void FALAudioDevice::FindProc( void*& ProcAddress, char* Name, char* SupportName, bool& Supports, bool AllowExt )
{
	
}

//
//	FindProcs
//
void FALAudioDevice::FindProcs( bool AllowExt )
{

}

//
//	alError
//
bool FALAudioDevice::alError( const TCHAR* Text, bool Log )
{
	ALenum Error = alGetError();
	if( Error != AL_NO_ERROR )
	{
		do 
		{		
			if( Log )
			{
				switch ( Error )
				{
				case AL_INVALID_NAME:
					UE_LOG ( LogInit, Log, TEXT( "ALAudio: AL_INVALID_NAME in %s" ), Text ); 
					break;
				case AL_INVALID_VALUE:
					UE_LOG ( LogInit, Log, TEXT( "ALAudio: AL_INVALID_VALUE in %s" ), Text ); 
					break;
				case AL_OUT_OF_MEMORY:
					UE_LOG ( LogInit, Log, TEXT( "ALAudio: AL_OUT_OF_MEMORY in %s" ), Text ); 
					break;
				case AL_INVALID_ENUM:
					UE_LOG ( LogInit, Log, TEXT( "ALAudio: AL_INVALID_ENUM in %s" ), Text ); 
					break;
				case AL_INVALID_OPERATION:
					UE_LOG ( LogInit, Log, TEXT( "ALAudio: AL_INVALID_OPERATION in %s" ), Text ); 
					break;
				default:
					UE_LOG ( LogInit, Log, TEXT( "ALAudio: Unknown Error NUM in %s" ), Text ); 
					break;
				}
			}
		}
		while( ( Error = alGetError() ) != AL_NO_ERROR );

		return true ; 
	}

	return false ;
}

FSoundSource* FALAudioDevice::CreateSoundSource()
{
	return new FALSoundSource(this);
}
