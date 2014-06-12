// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
//
#include "OculusRiftPrivate.h"
#include "OculusRiftHMD.h"
#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "PostProcess/PostProcessHMD.h"

#ifndef OVR_DIRECT_RENDERING
void FOculusRiftHMD::FDistortionMesh::Clear()
{
	delete[] pVertices;
	delete[] pIndices;
	pVertices = NULL;
	pIndices = NULL;
	NumVertices = NumIndices = NumTriangles = 0;
}
#endif

FOculusRiftHMD::FRenderParams::FRenderParams(FOculusRiftHMD* plugin)
	: 
#ifndef OVR_DIRECT_RENDERING
	  CurHmdOrientation(FQuat::Identity)
	, CurHmdPosition(FVector::ZeroVector)
	,
#else
	mD3D11Bridge(plugin)
	,
#endif
	 bFrameBegun(false)
	, ShowFlags(ESFIM_All0)
{
	#ifndef OVR_DIRECT_RENDERING
	FMemory::Memset(UVScale, 0, sizeof(UVScale));
	FMemory::Memset(UVOffset, 0, sizeof(UVOffset));
	#endif
}

void FOculusRiftHMD::PrecalculatePostProcess_NoLock()
{
#ifdef OVR_DIRECT_RENDERING
// 	HmdCaps = ovrHmdCap_Orientation | (bVSync ? 0 : ovrHmdCap_NoVSync); 
 	GDynamicRHI->InitHMDDevice(this);
#else
	for (unsigned eyeNum = 0; eyeNum < 2; ++eyeNum)
	{
		// Allocate & generate distortion mesh vertices.
		ovrDistortionMesh meshData;

		if (!ovrHmd_CreateDistortionMesh(Hmd, EyeRenderDesc[eyeNum].Eye, EyeRenderDesc[eyeNum].Fov, DistortionCaps, &meshData))
		{
			check(false);
			continue;
		}
		ovrHmd_GetRenderScaleAndOffset(EyeRenderDesc[eyeNum].Fov,
			TextureSize, EyeRenderViewport[eyeNum],
			(ovrVector2f*) UVScaleOffset[eyeNum]);

		// alloc the data
		ovrDistortionVertex* ovrMeshVertData = new ovrDistortionVertex[meshData.VertexCount];

		// Convert to final vertex data.
		FDistortionVertex *pVerts = new FDistortionVertex[meshData.VertexCount];
		FDistortionVertex *pCurVert = pVerts;
		ovrDistortionVertex* pCurOvrVert = meshData.pVertexData;

		for (unsigned vertNum = 0; vertNum < meshData.VertexCount; ++vertNum)
		{
			pCurVert->Position.X = pCurOvrVert->Pos.x;
			pCurVert->Position.Y = pCurOvrVert->Pos.y;
			pCurVert->TexR = FVector2D(pCurOvrVert->TexR.x, pCurOvrVert->TexR.y);
			pCurVert->TexG = FVector2D(pCurOvrVert->TexG.x, pCurOvrVert->TexG.y);
			pCurVert->TexB = FVector2D(pCurOvrVert->TexB.x, pCurOvrVert->TexB.y);
			pCurVert->Color.X = pCurVert->Color.Y = pCurVert->Color.Z = pCurOvrVert->VignetteFactor;
			pCurVert->Color.W = pCurOvrVert->TimeWarpFactor;
			pCurOvrVert++;
			pCurVert++;
		}

		pDistortionMesh[eyeNum] = *new FDistortionMesh();
		pDistortionMesh[eyeNum]->NumTriangles = meshData.IndexCount / 3; 
		pDistortionMesh[eyeNum]->NumIndices = meshData.IndexCount;
		pDistortionMesh[eyeNum]->NumVertices = meshData.VertexCount;
		pDistortionMesh[eyeNum]->pVertices = pVerts;

		check(sizeof(*meshData.pIndexData) == sizeof(uint16));
		pDistortionMesh[eyeNum]->pIndices = new uint16[meshData.IndexCount];
		FMemory::Memcpy(pDistortionMesh[eyeNum]->pIndices, meshData.pIndexData, sizeof(uint16)*meshData.IndexCount);

		ovrHmd_DestroyDistortionMesh(&meshData);
	}
#endif
}

void FOculusRiftHMD::DrawDistortionMesh_RenderThread(FRenderingCompositePassContext& Context, const FSceneView& View, const FIntPoint& TextureSize)
{
#ifndef OVR_DIRECT_RENDERING
	float ClipSpaceQuadZ = 0.0f;
	FMatrix QuadTexTransform = FMatrix::Identity;
	FMatrix QuadPosTransform = FMatrix::Identity;
	const FIntRect SrcRect = View.ViewRect;

	const FSceneViewFamily& ViewFamily = *(View.Family);
	FIntPoint ViewportSize = ViewFamily.RenderTarget->GetSizeXY();
	RHISetViewport(0, 0, 0.0f, ViewportSize.X, ViewportSize.Y, 1.0f);

	Ptr<FDistortionMesh> mesh = RenderParams_RenderThread.pDistortionMesh[(View.StereoPass == eSSP_LEFT_EYE) ? 0 : 1];

	DrawIndexedPrimitiveUP(Context.RHICmdList, PT_TriangleList, 0, mesh->NumVertices, mesh->NumTriangles, mesh->pIndices,
		sizeof(mesh->pIndices[0]), mesh->pVertices, sizeof(mesh->pVertices[0]));
#else
	check(0);
#endif
}

void FOculusRiftHMD::GetEyeRenderParams_RenderThread(EStereoscopicPass StereoPass, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const
{
#ifndef OVR_DIRECT_RENDERING
	const unsigned eyeIdx = (StereoPass == eSSP_LEFT_EYE) ? 0 : 1;
	EyeToSrcUVOffsetValue.X = RenderParams_RenderThread.UVOffset[eyeIdx].x;
	EyeToSrcUVOffsetValue.Y = RenderParams_RenderThread.UVOffset[eyeIdx].y;

	EyeToSrcUVScaleValue.X = RenderParams_RenderThread.UVScale[eyeIdx].x;
	EyeToSrcUVScaleValue.Y = RenderParams_RenderThread.UVScale[eyeIdx].y;
#else
	check(0);
#endif
}

void FOculusRiftHMD::PreRenderViewFamily_RenderThread(FSceneViewFamily& ViewFamily)
{
	check(IsInRenderingThread());

	RenderParams_RenderThread.ShowFlags = ViewFamily.EngineShowFlags;

#ifndef OVR_DIRECT_RENDERING 
	{
		// make a copy of StereoParams to access from the RenderThread.
		Lock::Locker lock(&StereoParamsLock);
		RenderParams_RenderThread.UVScale[0] = UVScaleOffset[0][0];
		RenderParams_RenderThread.UVScale[1] = UVScaleOffset[1][0];
		RenderParams_RenderThread.UVOffset[0] = UVScaleOffset[0][1];
		RenderParams_RenderThread.UVOffset[1] = UVScaleOffset[1][1];
		RenderParams_RenderThread.pDistortionMesh[0] = pDistortionMesh[0];
		RenderParams_RenderThread.pDistortionMesh[1] = pDistortionMesh[1];
	}

	// get latest orientation/position and cache it
	if (bUpdateOnRT)
	{
		Lock::Locker lock(&UpdateOnRTLock);
		GetCurrentOrientationAndPosition(CurHmdOrientation, CurHmdPosition);

		ProcessLatencyTesterInput();
		LatencyTestFrameNumber = GFrameNumberRenderThread;
		RenderParams_RenderThread.CurHmdOrientation = CurHmdOrientation;
		RenderParams_RenderThread.CurHmdPosition = CurHmdPosition;
	}
#else
	BeginRendering_RenderThread();
#endif
}

void FOculusRiftHMD::PreRenderView_RenderThread(FSceneView& View)
{
	check(IsInRenderingThread());

	if (!RenderParams_RenderThread.ShowFlags.Rendering)
		return;

#ifdef OVR_DIRECT_RENDERING
	FQuat		CurrentHmdOrientation;
	FVector		CurrentHmdPosition;
	if (RenderParams_RenderThread.bFrameBegun)
	{
		// Get new predicted pose to corresponding eye.
		const ovrEyeType eyeIdx = (View.StereoPass == eSSP_LEFT_EYE) ? ovrEye_Left : ovrEye_Right;
		RenderParams_RenderThread.EyeRenderPose[eyeIdx] = ovrHmd_BeginEyeRender(Hmd, eyeIdx);
		PoseToOrientationAndPosition(RenderParams_RenderThread.EyeRenderPose[eyeIdx], CurrentHmdOrientation, CurrentHmdPosition);
	}
	else
	{
		CurrentHmdOrientation = FQuat::Identity;
		CurrentHmdPosition    = FVector::ZeroVector;
	}
#else
	const FQuat		CurrentHmdOrientation = RenderParams_RenderThread.CurHmdOrientation;
	const FVector	CurrentHmdPosition    = RenderParams_RenderThread.CurHmdPosition;
#endif

	if (bUpdateOnRT)
	{
		// Apply updated pose to corresponding View at recalc matrices.
		UpdatePlayerViewPoint(CurrentHmdOrientation, CurrentHmdPosition,
			View.BaseHmdOrientation, View.BaseHmdLocation,
			View.ViewRotation, View.ViewLocation);
		View.UpdateViewMatrix();
	}
}

#ifdef OVR_DIRECT_RENDERING 
void FOculusRiftHMD::BeginRendering_RenderThread()
{
	check(IsInRenderingThread());
	{
		// Copy main D3D11Bridge into rendering thread's one
		Lock::Locker lock(&StereoParamsLock);

		if (mD3D11Bridge.bNeedReinitRendererAPI)
		{
			check(mD3D11Bridge.Cfg.D3D11.pSwapChain); // make sure Config is initialized
			(bTimeWarp) ? DistortionCaps |= ovrDistortion_TimeWarp : DistortionCaps &= ~ovrDistortion_TimeWarp;
			(bVSync) ? HmdCaps &= ~ovrHmdCap_NoVSync : HmdCaps |= ovrHmdCap_NoVSync;
			if (!ovrHmd_ConfigureRendering(Hmd, &mD3D11Bridge.Cfg.Config, HmdCaps, DistortionCaps, Eyes, EyeRenderDesc))
			{
				UE_LOG(LogHMD, Warning, TEXT("ovrHmd_ConfigureRenderAPI failed."));
				return;
			}
			mD3D11Bridge.bNeedReinitRendererAPI = false;
		}

		RenderParams_RenderThread.mD3D11Bridge = mD3D11Bridge;
		RenderParams_RenderThread.mD3D11Bridge.bReadOnly = true;
	}

	ovrHmd_BeginFrame(Hmd, 0);
	RenderParams_RenderThread.bFrameBegun = true;
}

void FOculusRiftHMD::FinishRendering_RenderThread()
{
	check(IsInRenderingThread())

	if (RenderParams_RenderThread.bFrameBegun)
	{
		if (RenderParams_RenderThread.ShowFlags.Rendering)
		{
			ovrHmd_EndEyeRender(Hmd, ovrEye_Left, RenderParams_RenderThread.EyeRenderPose[0], &RenderParams_RenderThread.mD3D11Bridge.EyeTexture[0].Texture);
			ovrHmd_EndEyeRender(Hmd, ovrEye_Right, RenderParams_RenderThread.EyeRenderPose[1], &RenderParams_RenderThread.mD3D11Bridge.EyeTexture[1].Texture);
		}

		// Finish the frame and let OVR do buffer swap (Present) and flush/sync.
		ovrHmd_EndFrame(Hmd); // This function will present
		RenderParams_RenderThread.bFrameBegun = false;
	}

	RenderParams_RenderThread.mD3D11Bridge.Reset();
}

void FOculusRiftHMD::UpdateRenderTarget(FRHITexture* TargetableTexture, FRHITexture* ShaderResourceTexture, uint32 SizeX, uint32 SizeY, int NumSamples)
{
	check(TargetableTexture && ShaderResourceTexture);
	check(IsInGameThread());

	const FIntPoint NewRTSize = FIntPoint(SizeX, SizeY);
	if (RenderTargetSize != NewRTSize)
	{
		EyeViewportSize.X = (NewRTSize.X + 1) / 2;
		EyeViewportSize.Y = NewRTSize.Y;
		bNeedUpdateStereoRenderingParams = true;
	}
	if (bNeedUpdateStereoRenderingParams)
		UpdateStereoRenderingParams();
	
	// Only RHI knows how to extract actual HW-specific texture from FTexture
	GDynamicRHI->SetHMDTexture(this, TargetableTexture, ShaderResourceTexture, SizeX, SizeY, (NumSamples > 0) ? NumSamples : 1);
	RenderTargetSize = NewRTSize;

	mD3D11Bridge.RenderTargetTexture = (FRHITexture2D*)TargetableTexture;
}

void FOculusRiftHMD::CalculateRenderTargetSize(uint32& InOutSizeX, uint32& InOutSizeY) const
{
	check(IsInGameThread());

	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.ScreenPercentage"));
	float value = CVar->GetValueOnGameThread();
	if (value > 0.0f)
	{
		InOutSizeX = FMath::Ceil(InOutSizeX * value/100.f);
		InOutSizeY = FMath::Ceil(InOutSizeY * value/100.f);
	}
}

bool FOculusRiftHMD::NeedReAllocateRenderTarget(uint32 InSizeX, uint32 InSizeY) const
{
	check(IsInGameThread());
	if (IsStereoEnabled())
	{
		uint32 NewSizeX = InSizeX, NewSizeY = InSizeY;
		CalculateRenderTargetSize(NewSizeX, NewSizeY);
		if (NewSizeX != RenderTargetSize.X || NewSizeY != RenderTargetSize.Y)
		{
			return true;
		}
	}
	return false;
}
#endif

bool FOculusRiftHMD::GetLatencyTesterColor_RenderThread(FColor& color, const FSceneView& view)
{
// #if !UE_BUILD_SHIPPING          
// 	if (pLatencyTest && pLatencyTester && pLatencyTest->IsMeasuringNow())
// 	{
// 		if (view.StereoPass == eSSP_LEFT_EYE || view.StereoPass == eSSP_FULL)
// 		{
// 			uint32 fr = GFrameNumberRenderThread;
// 			//UE_LOG(LogHMD, Warning, TEXT("fr %d lfr %d"), fr, LatencyTestFrameNumber);
// 			if (fr >= LatencyTestFrameNumber)
// 			{
// 				// Right eye will use the same color
// 				OVR::Lock::Locker lock(&LatencyTestLock);
// 				pLatencyTest->DisplayScreenColor(LatencyTestColor);
// 				LatencyTestFrameNumber = 0;
// 			}
// 		}
// 		color.R = LatencyTestColor.R;
// 		color.G = LatencyTestColor.G;
// 		color.B = LatencyTestColor.B;
// 		color.A = LatencyTestColor.A;
// 		return true;
// 	}
// #endif
	return false;
}

static const char* FormatLatencyReading(char* buff, UPInt size, float val)
{
	if (val < 0.000001f)
		OVR_strcpy(buff, size, "N/A   ");
	else
		OVR_sprintf(buff, size, "%4.2fms", val * 1000.0f);
	return buff;
}

void FOculusRiftHMD::DrawDebug(UCanvas* Canvas)
{
#if !UE_BUILD_SHIPPING
	check(IsInGameThread());
	if (IsStereoEnabled() && bShowStats)
	{
		static const FColor TextColor(0,255,0);
		// Pick a larger font on console.
		UFont* const Font = FPlatformProperties::SupportsWindowedMode() ? GEngine->GetSmallFont() : GEngine->GetMediumFont();
		const int32 RowHeight = FMath::TruncToInt(Font->GetMaxCharHeight() * 1.1f);

		float ClipX = Canvas->ClipX;
		float ClipY = Canvas->ClipY;
		float LeftPos = 0;

		ClipX -= 100;
		//ClipY = ClipY * 0.60;
		LeftPos = ClipX * 0.3f;
		float TopPos = ClipY * 0.4f;

		int32 X = (int32)LeftPos;
		int32 Y = (int32)TopPos;

		FString Str;
		Str = FString::Printf(TEXT("TimeWarp: %s"), (bTimeWarp) ? TEXT("ON") : TEXT("OFF"));
		Canvas->Canvas->DrawShadowedString(X, Y, *Str, Font, TextColor);

		Y += RowHeight;
		Str = FString::Printf(TEXT("VSync: %s"), (bVSync) ? TEXT("ON") : TEXT("OFF"));
		Canvas->Canvas->DrawShadowedString(X, Y, *Str, Font, TextColor);
		
		Y += RowHeight;
		Str = FString::Printf(TEXT("UpdRT: %s"), (bUpdateOnRT) ? TEXT("ON") : TEXT("OFF"));
		Canvas->Canvas->DrawShadowedString(X, Y, *Str, Font, TextColor);

		Y += RowHeight;
		static IConsoleVariable* CFinishFrameVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FinishCurrentFrame"));
		int finFr = CFinishFrameVar->GetInt();
		Str = FString::Printf(TEXT("FinFr: %s"), (finFr) ? TEXT("ON") : TEXT("OFF"));
		Canvas->Canvas->DrawShadowedString(X, Y, *Str, Font, TextColor);

		Y += RowHeight;
		static IConsoleVariable* CScrPercVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
		int32 sp = (int32)CScrPercVar->GetFloat();
		Str = FString::Printf(TEXT("SP: %d"), sp);
		Canvas->Canvas->DrawShadowedString(X, Y, *Str, Font, TextColor);

		float latencies[3] = { 0.0f, 0.0f, 0.0f };
		if (ovrHmd_GetFloatArray(Hmd, "DK2Latency", latencies, 3) == 3)
		{
			Y += RowHeight;

			char buf[3][20];
			char destStr[100];

			OVR_sprintf(destStr, sizeof(destStr), "Latency, ren: %s tw: %s pp: %s",
				FormatLatencyReading(buf[0], sizeof(buf[0]), latencies[0]),
				FormatLatencyReading(buf[1], sizeof(buf[1]), latencies[1]),
				FormatLatencyReading(buf[2], sizeof(buf[2]), latencies[2]));

			Str = ANSI_TO_TCHAR(destStr);
			Canvas->Canvas->DrawShadowedString(X, Y, *Str, Font, TextColor);
		}
	}
#endif // #if !UE_BUILD_SHIPPING
}

#ifdef OVR_DIRECT_RENDERING
void FOculusRiftHMD::D3D11Bridge::Init(void* InD3DDevice, void* InD3DDeviceContext)
{
	Cfg.D3D11.Header.API = ovrRenderAPI_D3D11;
	Cfg.D3D11.Header.Multisample = 1; //?? RenderParams.Multisample;
	Cfg.D3D11.pDevice			= (ID3D11Device*)InD3DDevice;
	Cfg.D3D11.pDeviceContext	= (ID3D11DeviceContext*)InD3DDeviceContext;
	bNeedReinitRendererAPI = true;
}

void FOculusRiftHMD::D3D11Bridge::Reset()
{
	Cfg.D3D11.pDevice			= (ID3D11Device*)nullptr;
	Cfg.D3D11.pDeviceContext	= (ID3D11DeviceContext*)nullptr;

	EyeTexture[0].D3D11.pTexture = (ID3D11Texture2D*)nullptr;
	EyeTexture[0].D3D11.pSRView = (ID3D11ShaderResourceView*)nullptr;
	EyeTexture[1].D3D11.pTexture = (ID3D11Texture2D*)nullptr;
	EyeTexture[1].D3D11.pSRView = (ID3D11ShaderResourceView*)nullptr;

	ResetViewport();
}

void FOculusRiftHMD::D3D11Bridge::ResetViewport()
{
	Cfg.D3D11.pBackBufferRT		= (ID3D11RenderTargetView*)nullptr;
	Cfg.D3D11.pSwapChain		= (IDXGISwapChain*)nullptr;

	bNeedReinitRendererAPI = true;

	// if we are in the middle of rendering: prevent from calling EndFrame
	Plugin->RenderParams_RenderThread.bFrameBegun = false;

	//ovrHmd_ConfigureRenderAPI(Plugin->Hmd, Plugin->EyeRenderDesc, Plugin->Eyes, nullptr, Plugin->HmdCaps, Plugin->DistortionCaps);
}

void FOculusRiftHMD::D3D11Bridge::InitViewport(void* InRenderTargetView, void* InSwapChain, int ResX, int ResY)
{
	check(!bReadOnly);

	Cfg.D3D11.Header.RTSize = Sizei(ResX, ResY);
	Cfg.D3D11.pBackBufferRT		= (ID3D11RenderTargetView*)InRenderTargetView;
	Cfg.D3D11.pSwapChain		= (IDXGISwapChain*)InSwapChain;
	bNeedReinitRendererAPI = true;

	// if we are in the middle of rendering: prevent from calling EndFrame
	Plugin->RenderParams_RenderThread.bFrameBegun = false;
}

void FOculusRiftHMD::D3D11Bridge::SetRenderTargetTexture(void* TargetableTexture, void* ShaderResourceTexture, int SizeX, int SizeY, int NumSamples)
{
	check(!bReadOnly);

	Cfg.D3D11.Header.Multisample = NumSamples;

	EyeTexture[0].D3D11.Header.API            = ovrRenderAPI_D3D11;
	EyeTexture[0].D3D11.Header.TextureSize    = Sizei(SizeX, SizeY);
	EyeTexture[0].D3D11.Header.RenderViewport = Plugin->Eyes[0].RenderViewport;
	EyeTexture[0].D3D11.pTexture = (ID3D11Texture2D*)TargetableTexture;
	EyeTexture[0].D3D11.pSRView =  (ID3D11ShaderResourceView*)ShaderResourceTexture;

	// Right eye uses the same texture, but different rendering viewport.
	EyeTexture[1] = EyeTexture[0];
	EyeTexture[1].D3D11.Header.RenderViewport = Plugin->Eyes[1].RenderViewport;

	bNeedReinitRendererAPI = true;
}

bool FOculusRiftHMD::D3D11Bridge::FinishFrame(int SyncInterval)
{
	check(IsInRenderingThread());

	Plugin->FinishRendering_RenderThread();
	return false; // indicates that we are presenting here, UE shouldn't do Present.
}

#endif


