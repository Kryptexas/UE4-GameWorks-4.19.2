// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TextureMovie.cpp: UTextureMovie implementation.
=============================================================================*/

#include "EnginePrivate.h"

/*-----------------------------------------------------------------------------
UTextureMovie
-----------------------------------------------------------------------------*/

UTextureMovie::UTextureMovie(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	MovieStreamSource = MovieStream_Memory;
	DecoderClass = UCodecMovieFallback::StaticClass();
	Looping = true;
	AutoPlay = true;
	NeverStream = true;
}


void UTextureMovie::BeginDestroy()
{
	// this will also release the FTextureMovieResource
	Super::BeginDestroy();

	if( Decoder )
	{
		// close the movie
		Decoder->Close();
		// FTextureMovieResource no longer exists and is not trying to use the decoder 
		// to update itself on the render thread so it should be safe to set this
		Decoder = NULL;
	}

	// synchronize with the rendering thread by inserting a fence
 	if( !ReleaseCodecFence )
 	{
 		ReleaseCodecFence = new FRenderCommandFence();
 	}
 	ReleaseCodecFence->BeginFence();
}

bool UTextureMovie::IsReadyForFinishDestroy()
{
	// ready to call FinishDestroy if the codec flushing fence has been hit
	return( 
		Super::IsReadyForFinishDestroy() &&
		ReleaseCodecFence && 
		ReleaseCodecFence->IsFenceComplete() 
		);
}


void UTextureMovie::FinishDestroy()
{
	delete ReleaseCodecFence;
	ReleaseCodecFence = NULL;
	
	Super::FinishDestroy();
}

#if WITH_EDITOR
void UTextureMovie::PreEditChange(UProperty* PropertyAboutToChange)
{
	// this will release the FTextureMovieResource
	Super::PreEditChange(PropertyAboutToChange);

	// synchronize with the rendering thread by flushing all render commands
	FlushRenderingCommands();

	if( Decoder )
	{
		// close the movie
		Decoder->Close();
		// FTextureMovieResource no longer exists and is not trying to use the decoder 
		// to update itself on the render thread so it should be safe to set this
		Decoder = NULL;
	}
}

void UTextureMovie::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// reinit the decoder
	InitDecoder();	

	// Update the internal size and format with what the decoder provides. 
	// This is not necessarily a power of two.	
	SizeX	= Decoder->GetSizeX();
	SizeY	= Decoder->GetSizeY();
	Format	= Decoder->GetFormat();

	// Non power of two textures need to use clamping.
	if( SizeX & (SizeX - 1) ||
		SizeY & (SizeY - 1) )
	{
		AddressX = TA_Clamp;
		AddressY = TA_Clamp;
	}

	// this will recreate the FTextureMovieResource
	Super::PostEditChangeProperty(PropertyChangedEvent);
    
	if( AutoPlay )
	{
		// begin movie playback if AutoPlay is enabled
		Play();
	}
	else
	{
		// begin movie playback so that we have at least one frame from the movie
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			PauseCommand,
			UCodecMovie*,Decoder,Decoder,
		{
			Decoder->Play(false,true,true);
		});
		Paused = true;
	}
}
#endif // WITH_EDITOR


void UTextureMovie::PostLoad()
{
	Super::PostLoad();

	if ( !HasAnyFlags(RF_ClassDefaultObject) && !GIsBuildMachine)  // we won't initialize this on build machines
	{
		// create the decoder and verify the movie stream
		InitDecoder();

		// Update the internal size and format with what the decoder provides. 
		// This is not necessarily a power of two.		
		SizeX	= Decoder->GetSizeX();
		SizeY	= Decoder->GetSizeY();
		Format	= Decoder->GetFormat();

		// Non power of two textures need to use clamping.
		if( SizeX & (SizeX - 1) ||
			SizeY & (SizeY - 1) )
		{
			AddressX = TA_Clamp;
			AddressY = TA_Clamp;
		}

		// recreate the FTextureMovieResource
		UpdateResource();
		
		if( AutoPlay )
		{
			// begin movie playback if AutoPlay is enabled
			Play();
		}
		else
		{			
			// begin movie playback so that we have at least one frame from the movie
			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
				PauseCommand,
				UCodecMovie*,Decoder,Decoder,
			{
				Decoder->Play(false,true,true);
			});
			Paused = true;
		}
	}
}


void UTextureMovie::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Data.Serialize( Ar, this );
}

void UTextureMovie::InitDecoder()
{
	check(Decoder == NULL);

	if( DecoderClass )
	{
		// Constructs a new decoder of the appropriate class.
		// The decoder class is set by the UTextureMovieFactory during import based on the movie file type
		Decoder = ConstructObject<UCodecMovie>( DecoderClass );
	}

	bool bIsStreamValid=false;
	if( Decoder )
	{				
#if 0	//@todo bink - implement stream from file
		// Newly imported objects won't have a linker till they are saved so we have to play from memory. We also play
		// from memory in the Editor to avoid having an open file handle to an existing package.
		if(!GIsEditor && 
			GetLinker() && 
			MovieStreamSource == MovieStream_File )		
		{
			// Have the decoder open the file itself and stream from disk.
			bIsStreamValid = Decoder->Open( GetLinker()->Filename, Data.GetBulkDataOffsetInFile(), Data.GetBulkDataSize() );
		}
		else
#endif
		{
			// allocate memory and copy movie stream into it.
			// decoder is responsible for free'ing this memory on Close.
			void* CopyOfData = NULL;
			Data.GetCopy( &CopyOfData, true );
			// open the movie stream for decoding
			if( Decoder->Open( CopyOfData, Data.GetBulkDataSize() ) )
			{
				bIsStreamValid = true;
			}
			else
			{
				bIsStreamValid = false;
				// free copy of data since we own it now
				FMemory::Free( CopyOfData );
			}
		}
	}

	// The engine always expects a valid decoder so we're going to revert to a fallback decoder
	// if we couldn't associate a decoder with this stream.
	if( !bIsStreamValid )
	{
		UE_LOG(LogTexture, Warning, TEXT("Invalid movie stream data for %s"), *GetFullName() );
		Decoder = ConstructObject<UCodecMovieFallback>( UCodecMovieFallback::StaticClass() );
		verify( Decoder->Open( NULL, 0 ) );
	}
}


FString UTextureMovie::GetDesc()
{
	if (Decoder)
	{
		return FString::Printf( 
			TEXT("%dx%d [%s], %.1f FPS, %.1f sec"), 
			SizeX, 
			SizeY, 
			GPixelFormats[Format].Name, 
			Decoder->GetFrameRate(), 
			Decoder->GetDuration() 
			);
	}
	else
	{
		return FString();
	}
}


SIZE_T UTextureMovie::GetResourceSize(EResourceSizeMode::Type Mode)
{
	int32 ResourceSize = 0;
	
	// Movie bulk data size.
	ResourceSize += Data.GetBulkDataSize();
	// Cost of render targets.
	ResourceSize += SizeX * SizeY * 4;
	return ResourceSize;
}


void UTextureMovie::Play()
{
	check(Decoder);

	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		PauseCommand,
		UCodecMovie*,Decoder,Decoder,
		bool,Looping,Looping,
		bool,ResetOnLastFrame,ResetOnLastFrame,
	{
		Decoder->Play(Looping,false,ResetOnLastFrame);
	});

	Paused = false;
	Stopped = false;
}

void UTextureMovie::Pause()
{
	if (Decoder)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			PauseCommand,
			UCodecMovie*,Decoder,Decoder,
		{
			Decoder->Pause(true);
		});
	}
	Paused = true;
}


void UTextureMovie::Stop()
{
	if (Decoder)	
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			PauseCommand,
			UCodecMovie*,Decoder,Decoder,
		{
			Decoder->Stop();
			Decoder->ResetStream();
		});
	}
	Stopped	= true;
}


FTextureResource* UTextureMovie::CreateResource()
{
	FTextureMovieResource* Result = new FTextureMovieResource(this);
	return Result;
}


class FTextureMovieResource* UTextureMovie::GetTextureMovieResource()
{
	FTextureMovieResource* Result = NULL;
	if( Resource &&
		Resource->IsInitialized() )
	{
		Result = (FTextureMovieResource*)Resource;
	}
	return Result;
}


EMaterialValueType UTextureMovie::GetMaterialType()
{
	return MCT_Texture2D;    
}


/*-----------------------------------------------------------------------------
FTextureMovieResource
-----------------------------------------------------------------------------*/

/**
* Initializes the dynamic RHI resource and/or RHI render target used by this resource.
* Called when the resource is initialized, or when reseting all RHI resources.
* This is only called by the rendering thread.
*/
void FTextureMovieResource::InitDynamicRHI()
{
	if( Owner->SizeX > 0 && Owner->SizeY > 0 )
	{
		// Create the RHI texture. Only one mip is used and the texture is targetable or resolve.
		uint32 TexCreateFlags = Owner->SRGB ? TexCreate_SRGB : 0;
		RHICreateTargetableShaderResource2D(
			Owner->SizeX, 
			Owner->SizeY, 
			Owner->Format, 
			1,
			TexCreateFlags,
			TexCreate_RenderTargetable,
			false,
			RenderTargetTextureRHI,
			Texture2DRHI
			);
		TextureRHI = (FTextureRHIRef&)Texture2DRHI;

		// add to the list of global deferred updates (updated during scene rendering)
		// since the latest decoded movie frame is rendered to this movie texture target
		AddToDeferredUpdateList(false);
	}

	// Create the sampler state RHI resource.
	FSamplerStateInitializerRHI SamplerStateInitializer
	(
		GSystemSettings.TextureLODSettings.GetSamplerFilter( Owner ),
		Owner->AddressX == TA_Wrap ? AM_Wrap : (Owner->AddressX == TA_Clamp ? AM_Clamp : AM_Mirror),
		Owner->AddressY == TA_Wrap ? AM_Wrap : (Owner->AddressY == TA_Clamp ? AM_Clamp : AM_Mirror),
		AM_Wrap
	);
	SamplerStateRHI = RHICreateSamplerState( SamplerStateInitializer );
}

/**
* Release the dynamic RHI resource and/or RHI render target used by this resource.
* Called when the resource is released, or when reseting all RHI resources.
* This is only called by the rendering thread.
*/
void FTextureMovieResource::ReleaseDynamicRHI()
{
	// release the FTexture RHI resources here as well
	ReleaseRHI();

	Texture2DRHI.SafeRelease();
	RenderTargetTextureRHI.SafeRelease();	

	// remove from global list of deferred updates
	RemoveFromDeferredUpdateList();

	if( Owner->Decoder )
	{
		// release any dynamnic resources allocated by the decoder
		Owner->Decoder->ReleaseDynamicResources();
	}
}

/**
* Decodes the next frame of the movie stream and renders the result to this movie texture target
* This is only called by the rendering thread.
*/
void FTextureMovieResource::UpdateResource()
{
	if( Owner->Decoder )
	{
		SCOPED_DRAW_EVENTF(EventDecode, DEC_SCENE_ITEMS, TEXT("Movie[%s]"),*Owner->GetName());
		Owner->Decoder->GetFrame(this);
	}
}

FIntPoint FTextureMovieResource::GetSizeXY() const 
{
	return FIntPoint(Owner->SizeX, Owner->SizeY);
}
