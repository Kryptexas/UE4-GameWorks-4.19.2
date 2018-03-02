// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MediaTexture.h"
#include "MediaAssetsPrivate.h"

#include "ExternalTexture.h"
#include "IMediaClock.h"
#include "IMediaClockSink.h"
#include "IMediaModule.h"
#include "MediaPlayerFacade.h"
#include "Modules/ModuleManager.h"
#include "RenderUtils.h"
#include "RenderingThread.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "MediaPlayer.h"
#include "Misc/MediaTextureResource.h"


/* Local helpers
 *****************************************************************************/

/**
 * Media clock sink for media textures.
 */
class FMediaTextureClockSink
	: public IMediaClockSink
{
public:

	FMediaTextureClockSink(UMediaTexture& InOwner)
		: Owner(&InOwner)
	{ }

	virtual ~FMediaTextureClockSink() { }

public:

	virtual void TickRender(FTimespan DeltaTime, FTimespan Timecode) override
	{
		if (Owner.IsValid())
		{
			Owner->TickResource(Timecode);
		}
	}

private:

	TWeakObjectPtr<UMediaTexture> Owner;
};


/* UMediaTexture structors
 *****************************************************************************/

UMediaTexture::UMediaTexture(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, AddressX(TA_Clamp)
	, AddressY(TA_Clamp)
	, AutoClear(false)
	, ClearColor(FLinearColor::Black)
	, DefaultGuid(FGuid::NewGuid())
	, Dimensions(FIntPoint::ZeroValue)
	, Size(0)
{
	NeverStream = true;
}


/* UMediaTexture interface
 *****************************************************************************/

float UMediaTexture::GetAspectRatio() const
{
	if (Dimensions.Y == 0)
	{
		return 0.0f;
	}

	return (float)(Dimensions.X) / Dimensions.Y;
}


int32 UMediaTexture::GetHeight() const
{
	return Dimensions.Y;
}


UMediaPlayer* UMediaTexture::GetMediaPlayer() const
{
	return CurrentPlayer.Get();
}


int32 UMediaTexture::GetWidth() const
{
	return Dimensions.X;
}


void UMediaTexture::SetMediaPlayer(UMediaPlayer* NewMediaPlayer)
{
	CurrentPlayer = NewMediaPlayer;
	UpdateQueue();
}


#if WITH_EDITOR

void UMediaTexture::SetDefaultMediaPlayer(UMediaPlayer* NewMediaPlayer)
{
	MediaPlayer = NewMediaPlayer;
	CurrentPlayer = MediaPlayer;
}

#endif


/* UTexture interface
 *****************************************************************************/

FTextureResource* UMediaTexture::CreateResource()
{
	if (!ClockSink.IsValid())
	{
		IMediaModule* MediaModule = FModuleManager::LoadModulePtr<IMediaModule>("Media");

		if (MediaModule != nullptr)
		{
			ClockSink = MakeShared<FMediaTextureClockSink, ESPMode::ThreadSafe>(*this);
			MediaModule->GetClock().AddSink(ClockSink.ToSharedRef());
		}
	}

	return new FMediaTextureResource(*this, Dimensions, Size, ClearColor, CurrentGuid.IsValid() ? CurrentGuid : DefaultGuid);
}


EMaterialValueType UMediaTexture::GetMaterialType() const
{
	return MCT_TextureExternal;
}


float UMediaTexture::GetSurfaceWidth() const
{
	return Dimensions.X;
}


float UMediaTexture::GetSurfaceHeight() const
{
	return Dimensions.Y;
}


FGuid UMediaTexture::GetExternalTextureGuid() const
{
	return CurrentGuid;
}


/* UObject interface
 *****************************************************************************/

void UMediaTexture::BeginDestroy()
{
	if (ClockSink.IsValid())
	{
		IMediaModule* MediaModule = FModuleManager::LoadModulePtr<IMediaModule>("Media");

		if (MediaModule != nullptr)
		{
			MediaModule->GetClock().RemoveSink(ClockSink.ToSharedRef());
		}

		ClockSink.Reset();
	}

	if (CurrentGuid.IsValid())
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(MediaTextureUnregisterGuid,
			FGuid, Guid, CurrentGuid,
			{
				FExternalTextureRegistry::Get().UnregisterExternalTexture(Guid);
			});
	}

	Super::BeginDestroy();
}


FString UMediaTexture::GetDesc()
{
	return FString::Printf(TEXT("%ix%i [%s]"), Dimensions.X,  Dimensions.Y, GPixelFormats[PF_B8G8R8A8].Name);
}


void UMediaTexture::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	Super::GetResourceSizeEx(CumulativeResourceSize);

	CumulativeResourceSize.AddUnknownMemoryBytes(Size);
}


void UMediaTexture::PostLoad()
{
	Super::PostLoad();

	CurrentPlayer = MediaPlayer;
}


#if WITH_EDITOR

void UMediaTexture::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName AddressXName = GET_MEMBER_NAME_CHECKED(UMediaTexture, AddressX);
	static const FName AddressYName = GET_MEMBER_NAME_CHECKED(UMediaTexture, AddressY);
	static const FName AutoClearName = GET_MEMBER_NAME_CHECKED(UMediaTexture, AutoClear);
	static const FName ClearColorName = GET_MEMBER_NAME_CHECKED(UMediaTexture, ClearColor);
	static const FName MediaPlayerName = GET_MEMBER_NAME_CHECKED(UMediaTexture, MediaPlayer);

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	
	if (PropertyThatChanged == nullptr)
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		return;
	}

	const FName PropertyName = PropertyThatChanged->GetFName();

	if (PropertyName == MediaPlayerName)
	{
		CurrentPlayer = MediaPlayer;
	}

	// don't update resource for these properties
	if ((PropertyName == AutoClearName) ||
		(PropertyName == ClearColorName) ||
		(PropertyName == MediaPlayerName))
	{
		UObject::PostEditChangeProperty(PropertyChangedEvent);

		return;
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);

	// notify materials for these properties
	if ((PropertyName == AddressXName) ||
		(PropertyName == AddressYName))
	{
		NotifyMaterials();
	}
}

#endif // WITH_EDITOR


/* UMediaTexture implementation
 *****************************************************************************/

void UMediaTexture::TickResource(FTimespan Timecode)
{
	if (Resource == nullptr)
	{
		return;
	}

	const FGuid PreviousGuid = CurrentGuid;

	// media player bookkeeping
	if (CurrentPlayer.IsValid())
	{
		UpdateQueue();
	}
	else if (CurrentGuid != DefaultGuid)
	{
		SampleQueue.Reset();
		CurrentGuid = DefaultGuid;
	}
	else if ((LastClearColor == ClearColor) && (LastSrgb == SRGB))
	{
		return; // nothing to render
	}

	LastClearColor = ClearColor;
	LastSrgb = SRGB;

	// set up render parameters
	FMediaTextureResource::FRenderParams RenderParams;

	if (CurrentPlayer.IsValid())
	{
		const bool PlayerActive = CurrentPlayer->IsPaused() || CurrentPlayer->IsPlaying() || CurrentPlayer->IsPreparing();

		if (PlayerActive)
		{
			RenderParams.SampleSource = SampleQueue;
		}
		else if (!AutoClear)
		{
			return; // retain last frame
		}

		RenderParams.Rate = CurrentPlayer->GetRate();
		RenderParams.Time = CurrentPlayer->GetTime();
	}
	else if (!AutoClear && (CurrentGuid == PreviousGuid))
	{
		return; // retain last frame
	}

	RenderParams.CanClear = AutoClear;
	RenderParams.ClearColor = ClearColor;
	RenderParams.PreviousGuid = PreviousGuid;
	RenderParams.CurrentGuid = CurrentGuid;
	RenderParams.SrgbOutput = SRGB;

	// redraw texture resource on render thread
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(MediaTextureResourceRender,
		FMediaTextureResource*, ResourceParam, (FMediaTextureResource*)Resource,
		FMediaTextureResource::FRenderParams, RenderParamsParam, RenderParams,
		{
			ResourceParam->Render(RenderParamsParam);
		});
}


void UMediaTexture::UpdateQueue()
{
	if (CurrentPlayer.IsValid())
	{
		const FGuid PlayerGuid = CurrentPlayer->GetGuid();

		if (CurrentGuid != PlayerGuid)
		{
			SampleQueue = MakeShared<FMediaTextureSampleQueue, ESPMode::ThreadSafe>();
			CurrentPlayer->GetPlayerFacade()->AddVideoSampleSink(SampleQueue.ToSharedRef());
			CurrentGuid = PlayerGuid;
		}
	}
	else
	{
		SampleQueue.Reset();
	}
}
