// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_Splash.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
#include "OculusHMD.h"
#include "RenderingThread.h"
#include "Misc/ScopeLock.h"
#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "Android/AndroidEGL.h"
#include "AndroidApplication.h"
#endif

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FSplash
//-------------------------------------------------------------------------------------------------

FSplash::FSplash(FOculusHMD* InOculusHMD) :
	OculusHMD(InOculusHMD),
	CustomPresent(InOculusHMD->GetCustomPresent_Internal()),
	NextLayerId(1),
	bInitialized(false),
	bTickable(false),
	bLoadingStarted(false),
	bLoadingCompleted(false),
	bLoadingIconMode(false),
	bAutoShow(true),
	bIsBlack(true),
	SystemDisplayInterval(1 / 90.0f),
	ShowFlags(0)
{
	const TCHAR* SplashSettings = TEXT("Oculus.Splash.Settings");
	float f;
	FVector vec;
	FVector2D vec2d;
	FString s;
	bool b;
	FRotator r;
	if (GConfig->GetBool(SplashSettings, TEXT("bAutoEnabled"), b, GEngineIni))
	{
		bAutoShow = b;
	}
	FString num;
	for (int32 i = 0; ; ++i)
	{
		FSplashDesc SplashDesc;
		if (GConfig->GetString(SplashSettings, *(FString(TEXT("TexturePath")) + num), s, GEngineIni))
		{
			SplashDesc.TexturePath = s;
		}
		else
		{
			break;
		}
		if (GConfig->GetVector(SplashSettings, *(FString(TEXT("DistanceInMeters")) + num), vec, GEngineIni))
		{
			SplashDesc.TransformInMeters.SetTranslation(vec);
		}
		if (GConfig->GetRotator(SplashSettings, *(FString(TEXT("Rotation")) + num), r, GEngineIni))
		{
			SplashDesc.TransformInMeters.SetRotation(FQuat(r));
		}
		if (GConfig->GetVector2D(SplashSettings, *(FString(TEXT("SizeInMeters")) + num), vec2d, GEngineIni))
		{
			SplashDesc.QuadSizeInMeters = vec2d;
		}
		if (GConfig->GetRotator(SplashSettings, *(FString(TEXT("DeltaRotation")) + num), r, GEngineIni))
		{
			SplashDesc.DeltaRotation = FQuat(r);
		}
		else
		{
			if (GConfig->GetVector(SplashSettings, *(FString(TEXT("RotationAxis")) + num), vec, GEngineIni))
			{
				if (GConfig->GetFloat(SplashSettings, *(FString(TEXT("RotationDeltaInDegrees")) + num), f, GEngineIni))
				{
					SplashDesc.DeltaRotation = FQuat(vec, FMath::DegreesToRadians(f));
				}
			}
		}
		if (!SplashDesc.TexturePath.IsEmpty())
		{
			AddSplash(SplashDesc);
		}
		num = Lex::ToString(i);
	}

	// Create empty quad layer for black frame
	{
		IStereoLayers::FLayerDesc LayerDesc;
		LayerDesc.QuadSize = FVector2D(0.01f, 0.01f);
		LayerDesc.Priority = 0;
		LayerDesc.PositionType = IStereoLayers::TrackerLocked;
		LayerDesc.ShapeType = IStereoLayers::QuadLayer;
		LayerDesc.Texture = GBlackTexture->TextureRHI;
		BlackLayer = MakeShareable(new FLayer(NextLayerId++, LayerDesc));
	}
}


FSplash::~FSplash()
{
	// Make sure RenTicker is freed in Shutdown
	check(!Ticker.IsValid())
}


void FSplash::AddReferencedObjects(FReferenceCollector& Collector)
{
	FScopeLock ScopeLock(&RenderThreadLock);

	for (int32 SplashLayerIndex = 0; SplashLayerIndex < SplashLayers.Num(); SplashLayerIndex++)
	{
		if (SplashLayers[SplashLayerIndex].Desc.LoadingTexture)
		{
			Collector.AddReferencedObject(SplashLayers[SplashLayerIndex].Desc.LoadingTexture);
		}
	}
}


void FSplash::Tick_RenderThread(float DeltaTime)
{
	CheckInRenderThread();

	const double TimeInSeconds = FPlatformTime::Seconds();
	const double DeltaTimeInSeconds = TimeInSeconds - LastTimeInSeconds;

	// Render every 1/3 secs if nothing animating, or every other frame if rotation is needed
	bool bRenderFrame = DeltaTimeInSeconds > 1.f / 3.f;

	if(DeltaTimeInSeconds > 2.f * SystemDisplayInterval)
	{
		FScopeLock ScopeLock(&RenderThreadLock);

		for (int32 SplashLayerIndex = 0; SplashLayerIndex < SplashLayers.Num(); SplashLayerIndex++)
		{
			FSplashLayer& SplashLayer = SplashLayers[SplashLayerIndex];

			if (SplashLayer.Layer.IsValid() && !SplashLayer.Desc.DeltaRotation.Equals(FQuat::Identity))
			{
				IStereoLayers::FLayerDesc LayerDesc = SplashLayer.Layer->GetDesc();
				LayerDesc.Transform.SetRotation(SplashLayer.Desc.DeltaRotation * LayerDesc.Transform.GetRotation());

				FLayer* Layer = new FLayer(*SplashLayer.Layer);
				Layer->SetDesc(LayerDesc);
				SplashLayer.Layer = MakeShareable(Layer);

				bRenderFrame = true;
			}
		}
	}

	if (bRenderFrame)
	{
		RenderFrame_RenderThread(FRHICommandListExecutor::GetImmediateCommandList(), TimeInSeconds);
	}
}


void FSplash::RenderFrame_RenderThread(FRHICommandListImmediate& RHICmdList, double TimeInSeconds)
{
	CheckInRenderThread();

	// RenderFrame
	FSettingsPtr XSettings;
	FGameFramePtr XFrame;
	TArray<FLayerPtr> XLayers;
	{
		FScopeLock ScopeLock(&RenderThreadLock);
		XSettings = Settings->Clone();
		Frame->FrameNumber = ++OculusHMD->FrameNumber;
		XFrame = Frame->Clone();

		if(!bIsBlack)
		{
			for(int32 SplashLayerIndex = 0; SplashLayerIndex < SplashLayers.Num(); SplashLayerIndex++)
			{
				const FSplashLayer& SplashLayer = SplashLayers[SplashLayerIndex];

				if(SplashLayer.Layer.IsValid())
				{
					XLayers.Add(SplashLayer.Layer->Clone());
				}
			}

			XLayers.Sort(FLayerPtr_CompareId());
		}
		else
		{
			XLayers.Add(BlackLayer->Clone());
		}
	}

	ovrp_Update3(ovrpStep_Render, XFrame->FrameNumber, 0.0);

	{
		int32 LayerIndex = 0;
		int32 LayerIndex_RenderThread = 0;

		while(LayerIndex < XLayers.Num() && LayerIndex_RenderThread < Layers_RenderThread.Num())
		{
			uint32 LayerIdA = XLayers[LayerIndex]->GetId();
			uint32 LayerIdB = Layers_RenderThread[LayerIndex_RenderThread]->GetId();

			if (LayerIdA < LayerIdB)
			{
				XLayers[LayerIndex++]->Initialize_RenderThread(CustomPresent);
			}
			else if (LayerIdA > LayerIdB)
			{
				LayerIndex_RenderThread++;
			}
			else
			{
				XLayers[LayerIndex++]->Initialize_RenderThread(CustomPresent, Layers_RenderThread[LayerIndex_RenderThread++].Get());
			}
		}

		while(LayerIndex < XLayers.Num())
		{
			XLayers[LayerIndex++]->Initialize_RenderThread(CustomPresent);
		}
	}

	Layers_RenderThread = XLayers;

	for (int32 LayerIndex = 0; LayerIndex < Layers_RenderThread.Num(); LayerIndex++)
	{
		Layers_RenderThread[LayerIndex]->UpdateTexture_RenderThread(CustomPresent, RHICmdList);
	}


	// RHIFrame
	for (int32 LayerIndex = 0; LayerIndex < XLayers.Num(); LayerIndex++)
	{
		XLayers[LayerIndex] = XLayers[LayerIndex]->Clone();
	}

	ExecuteOnRHIThread_DoNotWait([this, XSettings, XFrame, XLayers]()
	{
		ovrp_BeginFrame2(XFrame->FrameNumber);

		Layers_RHIThread = XLayers;
		Layers_RHIThread.Sort(FLayerPtr_ComparePriority());
		TArray<const ovrpLayerSubmit*> LayerSubmitPtr;
		LayerSubmitPtr.SetNum(Layers_RHIThread.Num());

		for (int32 LayerIndex = 0; LayerIndex < Layers_RHIThread.Num(); LayerIndex++)
		{
			LayerSubmitPtr[LayerIndex] = Layers_RHIThread[LayerIndex]->UpdateLayer_RHIThread(XSettings.Get(), XFrame.Get());
		}

		ovrp_EndFrame2(XFrame->FrameNumber, LayerSubmitPtr.GetData(), LayerSubmitPtr.Num());

		for (int32 LayerIndex = 0; LayerIndex < Layers_RHIThread.Num(); LayerIndex++)
		{
			Layers_RHIThread[LayerIndex]->IncrementSwapChainIndex_RHIThread();
		}
	});

	LastTimeInSeconds = TimeInSeconds;
}


bool FSplash::IsShown() const
{
	return (ShowFlags != 0) || (bAutoShow && bLoadingStarted && !bLoadingCompleted);
}


void FSplash::Startup()
{
	CheckInGameThread();

	if (!bInitialized)
	{
		Ticker = MakeShareable(new FTicker(this));

		ExecuteOnRenderThread_DoNotWait([this]()
		{
			Ticker->Register();
		});

		// Check to see if we want to use autoloading splash screens from the config
		const TCHAR* OculusSettings = TEXT("Oculus.Settings");
		bool bUseAutoShow = true;
		GConfig->GetBool(OculusSettings, TEXT("bUseAutoLoadingSplashScreen"), bUseAutoShow, GEngineIni);
		bAutoShow = bUseAutoShow;

		// Add a delegate to start playing movies when we start loading a map
		FCoreUObjectDelegates::PreLoadMap.AddSP(this, &FSplash::OnPreLoadMap);
		FCoreUObjectDelegates::PostLoadMapWithWorld.AddSP(this, &FSplash::OnPostLoadMap);

		bInitialized = true;
	}
}


void FSplash::PreShutdown()
{
	CheckInGameThread();

	// force Ticks to stop
	bTickable = false;
}


void FSplash::Shutdown()
{
	CheckInGameThread();

	if (bInitialized)
	{
		bTickable = false;

		ExecuteOnRenderThread([this]()
		{
			Ticker->Unregister();
			Ticker = nullptr;

			ExecuteOnRHIThread([this]()
			{
				SplashLayers.Reset();
				Layers_RenderThread.Reset();
				Layers_RHIThread.Reset();
			});
		});

		FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
		FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

		ShowFlags = 0;
		bIsBlack = false;
		bLoadingCompleted = false;
		bLoadingStarted = false;
		bInitialized = false;
	}
}


void FSplash::OnLoadingBegins()
{
	CheckInGameThread();

	if (bAutoShow)
	{
		UE_LOG(LogLoadingSplash, Log, TEXT("Loading begins"));
		bLoadingStarted = true;
		bLoadingCompleted = false;
		Show(ShowAutomatically);
	}
}


void FSplash::OnLoadingEnds()
{
	CheckInGameThread();

	if (bAutoShow)
	{
		UE_LOG(LogLoadingSplash, Log, TEXT("Loading ends"));
		bLoadingStarted = false;
		bLoadingCompleted = true;
		Hide(ShowAutomatically);
	}
}


bool FSplash::AddSplash(const FSplashDesc& Desc)
{
	CheckInGameThread();

	FScopeLock ScopeLock(&RenderThreadLock);
	SplashLayers.Add(FSplashLayer(Desc));
	return true;
}


void FSplash::ClearSplashes()
{
	CheckInGameThread();

	FScopeLock ScopeLock(&RenderThreadLock);
	SplashLayers.Reset();
}


bool FSplash::GetSplash(unsigned InSplashLayerIndex, FSplashDesc& OutDesc)
{
	CheckInGameThread();

	FScopeLock ScopeLock(&RenderThreadLock);
	if (InSplashLayerIndex < unsigned(SplashLayers.Num()))
	{
		OutDesc = SplashLayers[int32(InSplashLayerIndex)].Desc;
		return true;
	}
	return false;
}


void FSplash::Show(uint32 InShowFlags)
{
	CheckInGameThread();

	uint32 OldShowFlags = ShowFlags;
	ShowFlags |= InShowFlags;

	if (ShowFlags && !OldShowFlags)
	{
		OnShow();
	}
}


void FSplash::Hide(uint32 InShowFlags)
{
	CheckInGameThread();

	uint32 OldShowFlags = ShowFlags;
	ShowFlags &= ~InShowFlags;

	if (!ShowFlags && OldShowFlags)
	{
		OnHide();
	}
}


void FSplash::OnShow()
{
	CheckInGameThread();

	// Create new textures
	{
		FScopeLock ScopeLock(&RenderThreadLock);
			
		UnloadTextures();

		// Make sure all UTextures are loaded and contain Resource->TextureRHI
		bool bWaitForRT = false;

		for (int32 SplashLayerIndex = 0; SplashLayerIndex < SplashLayers.Num(); ++SplashLayerIndex)
		{
			FSplashLayer& SplashLayer = SplashLayers[SplashLayerIndex];

			if (!SplashLayer.Desc.TexturePath.IsEmpty())
			{
				// load temporary texture (if TexturePath was specified)
				LoadTexture(SplashLayer);
			}
			if (SplashLayer.Desc.LoadingTexture && SplashLayer.Desc.LoadingTexture->IsValidLowLevel())
			{
				SplashLayer.Desc.LoadingTexture->UpdateResource();
				bWaitForRT = true;
			}
		}

		if (bWaitForRT)
		{
			FlushRenderingCommands();
		}

		bIsBlack = true;

		for (int32 SplashLayerIndex = 0; SplashLayerIndex < SplashLayers.Num(); ++SplashLayerIndex)
		{
			FSplashLayer& SplashLayer = SplashLayers[SplashLayerIndex];

			//@DBG BEGIN
			if (SplashLayer.Desc.LoadingTexture->IsValidLowLevel())
			{
				if (SplashLayer.Desc.LoadingTexture->Resource && SplashLayer.Desc.LoadingTexture->Resource->TextureRHI)
				{
					SplashLayer.Desc.LoadedTexture = SplashLayer.Desc.LoadingTexture->Resource->TextureRHI;
				}
				else
				{
					UE_LOG(LogHMD, Warning, TEXT("Splash, %s - no Resource"), *SplashLayer.Desc.LoadingTexture->GetDesc());
				}
			}
			//@DBG END

			if (SplashLayer.Desc.LoadedTexture)
			{
				IStereoLayers::FLayerDesc LayerDesc;
				LayerDesc.Transform = SplashLayer.Desc.TransformInMeters;
				LayerDesc.QuadSize = SplashLayer.Desc.QuadSizeInMeters;
				LayerDesc.UVRect = FBox2D(SplashLayer.Desc.TextureOffset, SplashLayer.Desc.TextureOffset + SplashLayer.Desc.TextureScale);
				LayerDesc.Priority = INT32_MAX - (int32) (SplashLayer.Desc.TransformInMeters.GetTranslation().X * 1000.f);
				LayerDesc.PositionType = IStereoLayers::TrackerLocked;
				LayerDesc.ShapeType = IStereoLayers::QuadLayer;
				LayerDesc.Texture = SplashLayer.Desc.LoadedTexture;
				LayerDesc.Flags = SplashLayer.Desc.bNoAlphaChannel ? IStereoLayers::LAYER_FLAG_TEX_NO_ALPHA_CHANNEL : 0;

				SplashLayer.Layer = MakeShareable(new FLayer(NextLayerId++, LayerDesc));

				bIsBlack = false;
			}
		}
	}

	// If no textures are loaded, this will push black frame
	if (PushFrame())
	{
		bTickable = true;
	}

	UE_LOG(LogHMD, Log, TEXT("FSplash::OnShow"));
}


void FSplash::OnHide()
{
	CheckInGameThread();

	UE_LOG(LogHMD, Log, TEXT("FSplash::OnHide"));
	bTickable = false;
	bIsBlack = true;
	PushFrame();
	UnloadTextures();

#if PLATFORM_ANDROID
	ExecuteOnRenderThread([this]()
	{
		ExecuteOnRHIThread([this]()
		{
			FAndroidApplication::DetachJavaEnv();
		});
	});
#endif
}


bool FSplash::PushFrame()
{
	CheckInGameThread();

	check(!bTickable);

	if (!OculusHMD->InitDevice())
	{
		return false;
	}

	{
		FScopeLock ScopeLock(&RenderThreadLock);
		Settings = OculusHMD->CreateNewSettings();
		Frame = OculusHMD->CreateNewGameFrame();
		// keep units in meters rather than UU (because UU make not much sense).
		Frame->WorldToMetersScale = 1.0f;

		float SystemDisplayFrequency;
		if (OVRP_SUCCESS(ovrp_GetSystemDisplayFrequency2(&SystemDisplayFrequency)))
		{
			SystemDisplayInterval = 1.0f / SystemDisplayFrequency;
		}
	}

	ExecuteOnRenderThread([this](FRHICommandListImmediate& RHICmdList)
	{
		RenderFrame_RenderThread(RHICmdList, FPlatformTime::Seconds());
	});

	return true;
}


void FSplash::UnloadTextures()
{
	CheckInGameThread();

	// unload temporary loaded textures
	FScopeLock ScopeLock(&RenderThreadLock);
	for (int32 SplashLayerIndex = 0; SplashLayerIndex < SplashLayers.Num(); ++SplashLayerIndex)
	{
		if (!SplashLayers[SplashLayerIndex].Desc.TexturePath.IsEmpty())
		{
			UnloadTexture(SplashLayers[SplashLayerIndex]);
		}
	}
}


void FSplash::LoadTexture(FSplashLayer& InSplashLayer)
{
	CheckInGameThread();

	UnloadTexture(InSplashLayer);

	UE_LOG(LogLoadingSplash, Log, TEXT("Loading texture for splash %s..."), *InSplashLayer.Desc.TexturePath);
	InSplashLayer.Desc.LoadingTexture = LoadObject<UTexture2D>(NULL, *InSplashLayer.Desc.TexturePath, NULL, LOAD_None, NULL);
	if (InSplashLayer.Desc.LoadingTexture != nullptr)
	{
		UE_LOG(LogLoadingSplash, Log, TEXT("...Success. "));
	}
	InSplashLayer.Desc.LoadedTexture = nullptr;
	InSplashLayer.Layer.Reset();
}


void FSplash::UnloadTexture(FSplashLayer& InSplashLayer)
{
	CheckInGameThread();

	if (InSplashLayer.Desc.LoadingTexture && InSplashLayer.Desc.LoadingTexture->IsValidLowLevel())
	{
		InSplashLayer.Desc.LoadingTexture = nullptr;
	}

	InSplashLayer.Desc.LoadedTexture = nullptr;
	InSplashLayer.Layer.Reset();
}


} // namespace OculusHMD

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS