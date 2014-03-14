// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StreamingPauseRendering.cpp: Streaming pause implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "StreamingPauseRendering.h"
#include "TileRendering.h"
#include "TickableObjectRenderThread.h"

#define STREAMING_PAUSE_DELAY (1.0f / 30.0f)

struct FStreamingPauseFlipPumper;
class FFrontBufferTexture;

/** Wheter to use streaming pause instead of default suspend/resume RHI interface. */
bool						GUseStreamingPause = false;

/** Temporary view used for drawing the streaming pause icon. */
// @todo UE4 merge andrew
// FViewInfo*					GStreamingPauseView = NULL;

/** Material used to draw the streaming pause icon. */
FMaterialRenderProxy*		GStreamingPauseMaterialRenderProxy = NULL;

/** Viewport that we are rendering into. */
FViewport*					GStreamingPauseViewport = NULL;

/** Texture used to store the front buffer. */
FFrontBufferTexture*		GStreamingPauseBackground = NULL;

/** An instance of the streaming pause object. */
FStreamingPauseFlipPumper*	GStreamingPause = NULL;

/**
 * A texture used to store the front buffer. 
 */
class FFrontBufferTexture : public FTextureResource
{
public:
	/** Constructor. */
	FFrontBufferTexture( int32 InSizeX, int32 InSizeY )
		: SizeX( InSizeX )
		, SizeY( InSizeY )
	{
	}

	// FResource interface.
	virtual void InitRHI()
	{
		// Create the texture RHI.  	
		uint32 TexCreateFlags = TexCreate_Dynamic;

		Texture2DRHI = RHICreateTexture2D( SizeX, SizeY, PF_B8G8R8A8, 1, 1, TexCreateFlags, NULL );
		TextureRHI = Texture2DRHI;

		// Create the sampler state RHI resource.
		FSamplerStateInitializerRHI SamplerStateInitializer(SF_Point,AM_Wrap,AM_Wrap,AM_Wrap);
		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);
	}

	/** Returns the width of the texture in pixels. */
	virtual uint32 GetSizeX() const
	{
		return SizeX;
	}

	/** Returns the height of the texture in pixels. */
	virtual uint32 GetSizeY() const
	{
		return SizeY;
	}

	/** Returns the texture2D's RHI resource. */
	FTexture2DRHIRef GetTexture2DRHI()
	{
		return Texture2DRHI;
	}

private:
	/** The texture2D's RHI resource. */
	FTexture2DRHIRef Texture2DRHI;

	/** The width of the front buffer texture in pixels. */
	int32 SizeX;

	/** The height of the front buffer texture in pixels. */
	int32 SizeY;
};

/** Returns true if we can render the streaming pause. */
bool CanRenderStreamingPause()
{
	// @todo UE4 merge andrew
	// return GStreamingPauseView && GStreamingPauseMaterialRenderProxy && GStreamingPauseViewport && GStreamingPauseBackground;
	return false;
}

/** Renders the streaming pause icon. */
void FStreamingPause::Render()
{
	if ( CanRenderStreamingPause() )
	{		
		GStreamingPauseViewport->BeginRenderFrame();

		FCanvas Canvas(GStreamingPauseViewport, NULL, GStreamingPauseViewport->GetClient()->GetWorld());

		const float LoadingSize = 0.1f * GStreamingPauseViewport->GetSizeXY().Y;
		const float LoadingOffsetX =  0.8f * GStreamingPauseViewport->GetSizeXY().X - LoadingSize;
		const float LoadingOffsetY =  0.8f * GStreamingPauseViewport->GetSizeXY().Y - LoadingSize;

		Canvas.DrawTile(0, 0, GStreamingPauseViewport->GetSizeXY().X, GStreamingPauseViewport->GetSizeXY().Y, 0.0f, 0.0f, 1.0f, 1.0f, FLinearColor(1.0f, 1.0f, 1.0f, 1), GStreamingPauseBackground, false);

		FCanvasTileItem TileItem( FVector2D( LoadingOffsetX, LoadingOffsetY ), GStreamingPauseMaterialRenderProxy, FVector2D( LoadingSize, LoadingSize) );
		TileItem.bFreezeTime = true;
		Canvas.DrawItem( TileItem );

		if(GEngine)
		{
			TGuardValue<float> OldGamma(GEngine->DisplayGamma,1.0f);
			Canvas.Flush();
		}
		else
		{
			Canvas.Flush();
		}

		GStreamingPauseViewport->EndRenderFrame( true, true );
	}
}

/** Streaming pause object. */
struct FStreamingPauseFlipPumper : public FTickableObjectRenderThread
{
	FStreamingPauseFlipPumper()
		: DelayCountdown(STREAMING_PAUSE_DELAY)
	{
	}

	/**
	 * Pure virtual that must be overloaded by the inheriting class. It is
	 * used to determine whether an object is ready to be ticked. This is 
	 * required for example for all UObject derived classes as they might be
	 * loaded async and therefore won't be ready immediately.
	 *
	 * @return	true if class is ready to be ticked, false otherwise.
	 */
	virtual bool IsTickable() const
	{
		return true;
	}

	/**
	 * Pure virtual that must be overloaded by the inheriting class. It will
	 * be called from within LevelTick.cpp after ticking all actors.
	 *
	 * @param DeltaTime	Game time passed since the last call.
	 */
	void Tick(float DeltaTime)
	{
		// countdown to a flip
		DelayCountdown -= DeltaTime;
		
		// if we reached the flip time, flip
		if (DelayCountdown <= 0.0f)
		{
			FStreamingPause::Render();

			// reset timer by adding 1 full delay, but don't allow it to be negative
			DelayCountdown = FMath::Max(0.0f, DelayCountdown + STREAMING_PAUSE_DELAY);
		}
	}
	virtual TStatId GetStatId() const OVERRIDE
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FStreamingPauseFlipPumper, STATGROUP_Tickables);
	}

	/** Delay until next flip */
	float DelayCountdown;
};

/** Initializes the streaming pause object. */
void FStreamingPause::Init()
{
	static bool TestedEnable = false;
	if (!TestedEnable)
	{
		TestedEnable = true;
		GConfig->GetBool( TEXT("/Script/Engine.Engine"), TEXT("bUseStreamingPause"), GUseStreamingPause, GEngineIni );
	}

	if( GStreamingPauseBackground == NULL && GUseStreamingPause )
	{
		// @todo UE4 merge andrew
		// GStreamingPauseBackground = new FFrontBufferTexture( GSceneRenderTargets.GetBufferSizeXY().X, GSceneRenderTargets.GetBufferSizeXY().Y );
		GStreamingPauseBackground->InitRHI();
	}
}

/** Suspends the title rendering and enables the streaming pause rendering. */
void FStreamingPause::SuspendRendering()
{
	if (GStreamingPause == NULL)
	{
		if ( CanRenderStreamingPause() )
		{
			check( GStreamingPauseBackground );
			
			// @todo UE4 merge andrew
			//		FResolveParams ResolveParams( GStreamingPauseBackground->GetTexture2DRHI() );
			//		RHICopyFrontBufferToTexture( ResolveParams );
		}
		
		// start the flip pumper
		GStreamingPause = new FStreamingPauseFlipPumper;
	}	
}

/** Resumes the title rendering and deletes the streaming pause rendering. */
void FStreamingPause::ResumeRendering()
{
	/* @todo UE4 merge andrew
	if ( GStreamingPauseView )
	{
		delete GStreamingPauseView->Family;
		delete GStreamingPauseView;
		GStreamingPauseView = NULL;
	}
	*/
	GStreamingPauseMaterialRenderProxy = NULL;

	delete GStreamingPause;
	GStreamingPause = NULL;

	GStreamingPauseViewport = NULL;
}

/** Returns true if the title rendering is suspended. Otherwise, returns false. */
bool FStreamingPause::IsRenderingSuspended()
{
	return GStreamingPause ? true : false;
}

/** Updates the streaming pause rendering. */
void FStreamingPause::Tick(float DeltaTime)
{
	if (GStreamingPause)
	{
		GStreamingPause->Tick(DeltaTime);
	}
}

/** Enqueue the streaming pause to suspend rendering during blocking load. */
void FStreamingPause::GameThreadWantsToSuspendRendering( FViewport* GameViewport )
{
#if WITH_STREAMING_PAUSE
	if( GUseStreamingPause )
	{
		FSceneViewFamily* StreamingPauseViewFamily = NULL;
		FViewInfo* StreamingPauseView = NULL;
		FMaterialRenderProxy* StreamingPauseMaterialProxy = NULL;
		UCanvas* CanvasObject = FindObject<UCanvas>(UObject::GetTransientPackage(),TEXT("CanvasObject"));

		if ( GameViewport != NULL )
		{
			FCanvas Canvas(GameViewport,NULL);

			const FRenderTarget* CanvasRenderTarget = Canvas.GetRenderTarget();

			StreamingPauseViewFamily = new FSceneViewFamily(
				CanvasRenderTarget,
				NULL,
				SHOW_DefaultGame,
				GetWorld()->GetTimeSeconds(),
				GetWorld()->GetDeltaSeconds(),
				GetWorld()->GetRealTimeSeconds(),
				false,false,false,true,true,
				CanvasRenderTarget->GetDisplayGamma(),
				false
				);

			// make a temporary view
			StreamingPauseView = new FViewInfo(StreamingPauseViewFamily, 
				NULL, 
				-1,
				NULL, 
				NULL, 
				NULL, 
				NULL, 
				NULL,
				NULL, 
				0, 
				0, 
				CanvasRenderTarget->GetSizeX(), 
				CanvasRenderTarget->GetSizeY(), 
				FMatrix::Identity, 
				FMatrix::Identity, 
				FLinearColor::Black, 
				FLinearColor::White, 
				FLinearColor::White, 
				TSet<UPrimitiveComponent*>()
				);
		}

		struct FSuspendRenderingParameters
		{
			FViewInfo*  StreamingPauseView;
			FMaterialRenderProxy* StreamingPauseMaterialRenderProxy;
			FViewport* Viewport;
		};

		FSuspendRenderingParameters SuspendRenderingParameters =
		{
			StreamingPauseView,
			StreamingPauseMaterialProxy,
			GameViewport,
		};

		// Enqueue command to suspend rendering during blocking load.
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			SuspendRendering,
			FSuspendRenderingParameters, Parameters, SuspendRenderingParameters,
		{ 
			extern RENDERCORE_API bool GGameThreadWantsToSuspendRendering; GGameThreadWantsToSuspendRendering = true; 
			extern FViewInfo* GStreamingPauseView; GStreamingPauseView = Parameters.StreamingPauseView; 
			extern FMaterialRenderProxy* GStreamingPauseMaterialRenderProxy; GStreamingPauseMaterialRenderProxy = Parameters.StreamingPauseMaterialRenderProxy; 
			extern FViewport* GStreamingPauseViewport; GStreamingPauseViewport = Parameters.Viewport;
		} );
	}
	else
#endif // WITH_STREAMING_PAUSE
	{
		// Enqueue command to suspend rendering during blocking load.
		ENQUEUE_UNIQUE_RENDER_COMMAND( SuspendRendering, { extern RENDERCORE_API bool GGameThreadWantsToSuspendRendering; GGameThreadWantsToSuspendRendering = true; } );
	}

	// Flush rendering commands to ensure we don't have any outstanding work on rendering thread
	// and rendering is indeed suspended.
	FlushRenderingCommands();
}

/** Enqueue the streaming pause to resume rendering after blocking load is completed. */
void FStreamingPause::GameThreadWantsToResumeRendering()
{
	// Resume rendering again now that we're done loading.
	ENQUEUE_UNIQUE_RENDER_COMMAND( ResumeRendering, { extern RENDERCORE_API bool GGameThreadWantsToSuspendRendering; GGameThreadWantsToSuspendRendering = false; } );

	FlushRenderingCommands();
}