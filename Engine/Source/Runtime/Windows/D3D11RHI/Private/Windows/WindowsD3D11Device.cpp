// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	D3D11Device.cpp: D3D device RHI implementation.
=============================================================================*/

#include "D3D11RHIPrivate.h"
#include "AllowWindowsPlatformTypes.h"
	#include <delayimp.h>
#include "HideWindowsPlatformTypes.h"

#include "HardwareInfo.h"

extern bool D3D11RHI_ShouldCreateWithD3DDebug();
extern bool D3D11RHI_ShouldAllowAsyncResourceCreation();

/**
 * Console variables used by the D3D11 RHI device.
 */
namespace RHIConsoleVariables
{
	int32 FeatureSetLimit = -1;
	static FAutoConsoleVariableRef CVarFeatureSetLimit(
		TEXT("RHI.FeatureSetLimit"),
		FeatureSetLimit,
		TEXT("If set to 10, limit D3D RHI to D3D10 feature level. Otherwise, it will use default. Changing this at run-time has no effect. (default is -1)")
		);
};

/** This function is used as a SEH filter to catch only delay load exceptions. */
static bool IsDelayLoadException(PEXCEPTION_POINTERS ExceptionPointers)
{
	switch(ExceptionPointers->ExceptionRecord->ExceptionCode)
	{
	case VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND):
	case VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND):
		return EXCEPTION_EXECUTE_HANDLER;
	default:
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

// We suppress warning C6322: Empty _except block. Appropriate checks are made upon returning. 
#if USING_CODE_ANALYSIS
	MSVC_PRAGMA(warning(push))
	MSVC_PRAGMA(warning(disable:6322))
#endif	// USING_CODE_ANALYSIS
/**
 * Since CreateDXGIFactory is a delay loaded import from the D3D11 DLL, if the user
 * doesn't have Vista/DX10, calling CreateDXGIFactory will throw an exception.
 * We use SEH to detect that case and fail gracefully.
 */
static void SafeCreateDXGIFactory(IDXGIFactory** DXGIFactory)
{
#if !D3D11_CUSTOM_VIEWPORT_CONSTRUCTOR
	__try
	{
		CreateDXGIFactory(__uuidof(IDXGIFactory),(void**)DXGIFactory);
	}
	__except(IsDelayLoadException(GetExceptionInformation()))
	{
	}
#endif	//!D3D11_CUSTOM_VIEWPORT_CONSTRUCTOR
}

/**
 * Returns the highest D3D feature level we are allowed to created based on
 * command line parameters.
 */
static D3D_FEATURE_LEVEL GetAllowedD3DFeatureLevel()
{
	// Default to D3D11 
	D3D_FEATURE_LEVEL AllowedFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	// Use a feature level 10 if specified on the command line.
	if(FParse::Param(FCommandLine::Get(),TEXT("d3d10")) || 
		FParse::Param(FCommandLine::Get(),TEXT("dx10")) ||
		FParse::Param(FCommandLine::Get(),TEXT("sm4")) ||
		RHIConsoleVariables::FeatureSetLimit == 10)
	{
		AllowedFeatureLevel = D3D_FEATURE_LEVEL_10_0;
	}
	return AllowedFeatureLevel;
}

/**
 * Attempts to create a D3D11 device for the adapter using at most MaxFeatureLevel.
 * If creation is successful, true is returned and the supported feature level is set in OutFeatureLevel.
 */
static bool SafeTestD3D11CreateDevice(IDXGIAdapter* Adapter,D3D_FEATURE_LEVEL MaxFeatureLevel,D3D_FEATURE_LEVEL* OutFeatureLevel)
{
	ID3D11Device* D3DDevice = NULL;
	ID3D11DeviceContext* D3DDeviceContext = NULL;
	uint32 DeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;

	// Use a debug device if specified on the command line.
	if(D3D11RHI_ShouldCreateWithD3DDebug())
	{
		DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}

	D3D_FEATURE_LEVEL RequestedFeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0
	};

	int32 FirstAllowedFeatureLevel = 0;
	int32 NumAllowedFeatureLevels = ARRAY_COUNT(RequestedFeatureLevels);
	while (FirstAllowedFeatureLevel < NumAllowedFeatureLevels)
	{
		if (RequestedFeatureLevels[FirstAllowedFeatureLevel] == MaxFeatureLevel)
		{
			break;
		}
		FirstAllowedFeatureLevel++;
	}
	NumAllowedFeatureLevels -= FirstAllowedFeatureLevel;

	if (NumAllowedFeatureLevels == 0)
	{
		return false;
	}

	__try
	{
		if(SUCCEEDED(D3D11CreateDevice(
			Adapter,
			D3D_DRIVER_TYPE_UNKNOWN,
			NULL,
			DeviceFlags,
			&RequestedFeatureLevels[FirstAllowedFeatureLevel],
			NumAllowedFeatureLevels,
			D3D11_SDK_VERSION,
			&D3DDevice,
			OutFeatureLevel,
			&D3DDeviceContext
			)))
		{
			D3DDevice->Release();
			D3DDeviceContext->Release();
			return true;
		}
	}
	__except(IsDelayLoadException(GetExceptionInformation()))
	{
	}

	return false;
}

// Re-enable C6322
#if USING_CODE_ANALYSIS
	MSVC_PRAGMA(warning(pop)) 
#endif // USING_CODE_ANALYSIS

bool FD3D11DynamicRHIModule::IsSupported()
{
	if (MaxSupportedFeatureLevel == 0)
	{
		// Try to create the DXGIFactory.  This will fail if we're not running Vista.
		TRefCountPtr<IDXGIFactory> DXGIFactory;
		SafeCreateDXGIFactory(DXGIFactory.GetInitReference());
		if(!DXGIFactory)
		{
			return false;
		}

		// Enumerate the DXGIFactory's adapters.
		uint32 AdapterIndex = 0;
		TRefCountPtr<IDXGIAdapter> TempAdapter;
		bool bHasD3D11Adapter = false;
		D3D_FEATURE_LEVEL MaxAllowedFeatureLevel = GetAllowedD3DFeatureLevel();
		while(DXGIFactory->EnumAdapters(AdapterIndex,TempAdapter.GetInitReference()) != DXGI_ERROR_NOT_FOUND)
		{
			// Check that if adapter supports D3D11.
			if(TempAdapter)
			{
				D3D_FEATURE_LEVEL ActualFeatureLevel = (D3D_FEATURE_LEVEL)0;
				if(SafeTestD3D11CreateDevice(TempAdapter,MaxAllowedFeatureLevel,&ActualFeatureLevel))
				{
					bHasD3D11Adapter = true;
					// Log some information about the available D3D11 adapters.
					DXGI_ADAPTER_DESC AdapterDesc;
					VERIFYD3D11RESULT(TempAdapter->GetDesc(&AdapterDesc));

					UE_LOG(LogD3D11RHI, Log,
						TEXT("Found D3D11 adapter %u: %s (Feature Level %s)"),
						AdapterIndex,
						AdapterDesc.Description,
						ActualFeatureLevel == D3D_FEATURE_LEVEL_11_0 ? TEXT("11_0") : TEXT("10_0")
						);
					UE_LOG(LogD3D11RHI, Log,
						TEXT("Adapter has %uMB of dedicated video memory, %uMB of dedicated system memory, and %uMB of shared system memory"),
						(uint32)(AdapterDesc.DedicatedVideoMemory / (1024*1024)),
						(uint32)(AdapterDesc.DedicatedSystemMemory / (1024*1024)),
						(uint32)(AdapterDesc.SharedSystemMemory / (1024*1024))
						);

					// We only ever create a device using the first adapter.
					if (AdapterIndex == 0)
					{
						MaxSupportedFeatureLevel = ActualFeatureLevel;
					}
				}
			}

			AdapterIndex++;
		};
	}

	// The hardware must support 11_0 or 10_0.
	return MaxSupportedFeatureLevel == D3D_FEATURE_LEVEL_11_0
		|| MaxSupportedFeatureLevel == D3D_FEATURE_LEVEL_10_0;
}

FDynamicRHI* FD3D11DynamicRHIModule::CreateRHI()
{
	TRefCountPtr<IDXGIFactory> DXGIFactory;
	SafeCreateDXGIFactory(DXGIFactory.GetInitReference());
	check(DXGIFactory);
	return new FD3D11DynamicRHI(DXGIFactory,MaxSupportedFeatureLevel);
}

void FD3D11DynamicRHI::Init()
{
	InitD3DDevice();
}

void FD3D11DynamicRHI::InitD3DDevice()
{
	check( IsInGameThread() );

	// Wait for the rendering thread to go idle.
	SCOPED_SUSPEND_RENDERING_THREAD(false);

	// If the device we were using has been removed, release it and the resources we created for it.
	if(bDeviceRemoved)
	{
		check(Direct3DDevice);

		HRESULT hRes = Direct3DDevice->GetDeviceRemovedReason();

		const TCHAR* Reason = TEXT("?");
		switch(hRes)
		{
			case DXGI_ERROR_DEVICE_HUNG:			Reason = TEXT("HUNG"); break;
			case DXGI_ERROR_DEVICE_REMOVED:			Reason = TEXT("REMOVED"); break;
			case DXGI_ERROR_DEVICE_RESET:			Reason = TEXT("RESET"); break;
			case DXGI_ERROR_DRIVER_INTERNAL_ERROR:	Reason = TEXT("INTERNAL_ERROR"); break;
			case DXGI_ERROR_INVALID_CALL:			Reason = TEXT("INVALID_CALL"); break;
		}

		bDeviceRemoved = false;

		// Cleanup the D3D device.
		CleanupD3DDevice();

		// We currently don't support removed devices because FTexture2DResource can't recreate its RHI resources from scratch.
		// We would also need to recreate the viewport swap chains from scratch.
		UE_LOG(LogD3D11RHI, Fatal, TEXT("The Direct3D 11 device that was being used has been removed (Error: %d '%s').  Please restart the game."), hRes, Reason);
	}

	// If we don't have a device yet, either because this is the first viewport, or the old device was removed, create a device.
	if(!Direct3DDevice)
	{
		check(!GIsRHIInitialized);

		// Clear shadowed shader resources.
		ClearState();

		// Determine the adapter and device type to use.
		TRefCountPtr<IDXGIAdapter> Adapter;
		
		// In Direct3D 11, if you are trying to create a hardware or a software device, set pAdapter != NULL which constrains the other inputs to be:
		//		DriverType must be D3D_DRIVER_TYPE_UNKNOWN 
		//		Software must be NULL. 
		D3D_DRIVER_TYPE DriverType = D3D_DRIVER_TYPE_UNKNOWN;	

		uint32 DeviceFlags = D3D11RHI_ShouldAllowAsyncResourceCreation() ? 0 : D3D11_CREATE_DEVICE_SINGLETHREADED;

		// Use a debug device if specified on the command line.
		const bool bWithD3DDebug = D3D11RHI_ShouldCreateWithD3DDebug();

		if (bWithD3DDebug)
		{
			DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

			UE_LOG(LogD3D11RHI, Log, TEXT("InitD3DDevice: -D3DDebug = %s"), bWithD3DDebug ? TEXT("on") : TEXT("off"));
		}

		// Don't allow NV perf HUD for rocket builds.
		bool bAllowPerfHUD = FRocketSupport::IsRocket() == false;

		GTexturePoolSize = 0;

		TRefCountPtr<IDXGIAdapter> EnumAdapter;
		uint32 CurrentAdapter = 0;
		while (DXGIFactory->EnumAdapters(CurrentAdapter,EnumAdapter.GetInitReference()) != DXGI_ERROR_NOT_FOUND)
		{
			if (EnumAdapter)// && EnumAdapter->CheckInterfaceSupport(__uuidof(ID3D11Device),NULL) == S_OK)
			{
				DXGI_ADAPTER_DESC AdapterDesc;
				if (SUCCEEDED(EnumAdapter->GetDesc(&AdapterDesc)))
				{
					const bool bIsPerfHUD = !FCString::Stricmp(AdapterDesc.Description,TEXT("NVIDIA PerfHUD"));

					// Select the first adapter in normal circumstances or the PerfHUD one if it exists and we are allowed to use it.
					const bool bUseAdapter = CurrentAdapter == 0 || (bAllowPerfHUD && bIsPerfHUD);
					if (bUseAdapter)
					{
						Adapter = EnumAdapter;

						GRHIAdapterName = AdapterDesc.Description;
						GRHIVendorId = AdapterDesc.VendorId;

						extern int64 GDedicatedVideoMemory;
						extern int64 GDedicatedSystemMemory;
						extern int64 GSharedSystemMemory;
						extern int64 GTotalGraphicsMemory;

						// Issue: 32bit windows doesn't report 64bit value, we take what we get.
						GDedicatedVideoMemory = int64(AdapterDesc.DedicatedVideoMemory);
						GDedicatedSystemMemory = int64(AdapterDesc.DedicatedSystemMemory);
						GSharedSystemMemory = int64(AdapterDesc.SharedSystemMemory);

						// Total amount of system memory, clamped to 8 GB
						int64 TotalPhysicalMemory = FMath::Min(int64(FPlatformMemory::GetConstants().TotalPhysicalGB), 8ll) * (1024ll * 1024ll * 1024ll);

						// Consider 50% of the shared memory but max 25% of total system memory.
						int64 ConsideredSharedSystemMemory = FMath::Min( GSharedSystemMemory / 2ll, TotalPhysicalMemory / 4ll );

						GTotalGraphicsMemory = 0;
						if ( IsRHIDeviceIntel() )
						{
							// It's all system memory.
							GTotalGraphicsMemory = GDedicatedVideoMemory;
							GTotalGraphicsMemory += GDedicatedSystemMemory;
							GTotalGraphicsMemory += ConsideredSharedSystemMemory;
						}
						else if ( GDedicatedVideoMemory >= 200*1024*1024 )
						{
							// Use dedicated video memory, if it's more than 200 MB
							GTotalGraphicsMemory = GDedicatedVideoMemory;
						} else if ( GDedicatedSystemMemory >= 200*1024*1024 )
						{
							// Use dedicated system memory, if it's more than 200 MB
							GTotalGraphicsMemory = GDedicatedSystemMemory;
						} else if ( GSharedSystemMemory >= 400*1024*1024 )
						{
							// Use some shared system memory, if it's more than 400 MB
							GTotalGraphicsMemory = ConsideredSharedSystemMemory;
						}
						else
						{
							// Otherwise consider 25% of total system memory for graphics.
							GTotalGraphicsMemory = TotalPhysicalMemory / 4ll;
						}

						if ( sizeof(SIZE_T) < 8 )
						{
							// Clamp to 1 GB if we're less than 64-bit
							GTotalGraphicsMemory = FMath::Min( GTotalGraphicsMemory, 1024ll * 1024ll * 1024ll );
						}
						else
						{
							// Clamp to 1.9 GB if we're 64-bit
							GTotalGraphicsMemory = FMath::Min( GTotalGraphicsMemory, 1945ll * 1024ll * 1024ll );
						}

						if ( GPoolSizeVRAMPercentage > 0 )
						{
							float PoolSize = float(GPoolSizeVRAMPercentage) * 0.01f * float(GTotalGraphicsMemory);

							// Truncate GTexturePoolSize to MB (but still counted in bytes)
							GTexturePoolSize = int64(FGenericPlatformMath::TruncFloat(PoolSize / 1024.0f / 1024.0f)) * 1024 * 1024;

							UE_LOG(LogRHI,Log,TEXT("Texture pool is %u MB (%d%% of %u MB)"),
								GTexturePoolSize / 1024 / 1024,
								GPoolSizeVRAMPercentage,
								GTotalGraphicsMemory / 1024 / 1024);
						}
					}
					if(bIsPerfHUD)
					{
						DriverType =  D3D_DRIVER_TYPE_REFERENCE;
					}
				}
			}
			++CurrentAdapter;
		}

		D3D_FEATURE_LEVEL ActualFeatureLevel = (D3D_FEATURE_LEVEL)0;

		if(IsRHIDeviceAMD())
		{
			DeviceFlags &= ~D3D11_CREATE_DEVICE_SINGLETHREADED;
		}

		// Creating the Direct3D device.
		VERIFYD3D11RESULT(D3D11CreateDevice(
			Adapter,
			DriverType,
			NULL,
			DeviceFlags,
			&FeatureLevel,
			1,
			D3D11_SDK_VERSION,
			Direct3DDevice.GetInitReference(),
			&ActualFeatureLevel,
			Direct3DDeviceIMContext.GetInitReference()
			));

		// We should get the feature level we asked for as earlier we checked to ensure it is supported.
		check(ActualFeatureLevel == FeatureLevel);

		StateCache.Init(Direct3DDeviceIMContext);

#if (UE_BUILD_SHIPPING && WITH_EDITOR) && PLATFORM_WINDOWS && !PLATFORM_64BITS
		// Disable PIX for windows in the shipping editor builds
		D3DPERF_SetOptions(1);
#endif

		// Determine PF_G8 usage as a render target
		uint32 G8FormatSupport = 0;
		VERIFYD3D11RESULT(Direct3DDevice->CheckFormatSupport(DXGI_FORMAT_R8_UNORM,&G8FormatSupport));
		GSupportsRenderTargetFormat_PF_G8 = !!(G8FormatSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET);

		// Check for async texture creation support.
		D3D11_FEATURE_DATA_THREADING ThreadingSupport = {0};
		VERIFYD3D11RESULT(Direct3DDevice->CheckFeatureSupport(D3D11_FEATURE_THREADING,&ThreadingSupport,sizeof(ThreadingSupport)));
		GRHISupportsAsyncTextureCreation = !!ThreadingSupport.DriverConcurrentCreates
			&& (DeviceFlags & D3D11_CREATE_DEVICE_SINGLETHREADED) == 0;

		GShaderPlatformForFeatureLevel[ERHIFeatureLevel::ES2] = SP_PCD3D_ES2;
		GShaderPlatformForFeatureLevel[ERHIFeatureLevel::SM3] = SP_NumPlatforms;
		GShaderPlatformForFeatureLevel[ERHIFeatureLevel::SM4] = SP_PCD3D_SM4;
		GShaderPlatformForFeatureLevel[ERHIFeatureLevel::SM5] = SP_PCD3D_SM5;

		if(IsRHIDeviceAMD())
		{
			GRHISupportsAsyncTextureCreation = false;
		}

		if (GRHISupportsAsyncTextureCreation)
		{
			UE_LOG(LogD3D11RHI,Log,TEXT("Async texture creation enabled"));
		}
		else
		{
			UE_LOG(LogD3D11RHI,Log,TEXT("Async texture creation disabled: %s"),
				D3D11RHI_ShouldAllowAsyncResourceCreation() ? TEXT("no driver support") : TEXT("disabled by user"));
		}

		UpdateMSAASettings();

		// Notify all initialized FRenderResources that there's a valid RHI device to create their RHI resources for now.
		for(TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
		{
			ResourceIt->InitDynamicRHI();
		}
		for(TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
		{
			ResourceIt->InitRHI();
		}

#if !(UE_BUILD_SHIPPING && WITH_EDITOR)
		// Add some filter outs for known debug spew messages (that we don't care about)
		if(DeviceFlags & D3D11_CREATE_DEVICE_DEBUG)
		{
			ID3D11InfoQueue *pd3dInfoQueue = NULL;
			VERIFYD3D11RESULT(Direct3DDevice->QueryInterface( IID_ID3D11InfoQueue, (void**)&pd3dInfoQueue));
			if(pd3dInfoQueue)
			{
				D3D11_INFO_QUEUE_FILTER NewFilter;
				FMemory::Memzero(&NewFilter,sizeof(NewFilter));

				// Turn off info msgs as these get really spewy
				D3D11_MESSAGE_SEVERITY DenySeverity = D3D11_MESSAGE_SEVERITY_INFO;
				NewFilter.DenyList.NumSeverities = 1;
				NewFilter.DenyList.pSeverityList = &DenySeverity;

				// Be sure to carefully comment the reason for any additions here!  Someone should be able to look at it later and get an idea of whether it is still necessary.
				D3D11_MESSAGE_ID DenyIds[]  = {
					// OMSETRENDERTARGETS_INVALIDVIEW - d3d will complain if depth and color targets don't have the exact same dimensions, but actually
					//	if the color target is smaller then things are ok.  So turn off this error.  There is a manual check in FD3D11DynamicRHI::SetRenderTarget
					//	that tests for depth smaller than color and MSAA settings to match.
					D3D11_MESSAGE_ID_OMSETRENDERTARGETS_INVALIDVIEW, 

					// QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS - The RHI exposes the interface to make and issue queries and a separate interface to use that data.
					//		Currently there is a situation where queries are issued and the results may be ignored on purpose.  Filtering out this message so it doesn't
					//		swarm the debug spew and mask other important warnings
					D3D11_MESSAGE_ID_QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS,
					D3D11_MESSAGE_ID_QUERY_END_ABANDONING_PREVIOUS_RESULTS,

					// D3D11_MESSAGE_ID_CREATEINPUTLAYOUT_EMPTY_LAYOUT - This is a warning that gets triggered if you use a null vertex declaration,
					//       which we want to do when the vertex shader is generating vertices based on ID.
					D3D11_MESSAGE_ID_CREATEINPUTLAYOUT_EMPTY_LAYOUT,

					// D3D11_MESSAGE_ID_DEVICE_DRAW_INDEX_BUFFER_TOO_SMALL - This warning gets triggered by Slate draws which are actually using a valid index range.
					//		The invalid warning seems to only happen when VS 2012 is installed.  Reported to MS.  
					//		There is now an assert in DrawIndexedPrimitive to catch any valid errors reading from the index buffer outside of range.
					D3D11_MESSAGE_ID_DEVICE_DRAW_INDEX_BUFFER_TOO_SMALL,

					// D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET - This warning gets triggered by shadow depth rendering because the shader outputs
					//		a color but we don't bind a color render target. That is safe as writes to unbound render targets are discarded.
					//		Also, batched elements triggers it when rendering outside of scene rendering as it outputs to the GBuffer containing normals which is not bound.
					(D3D11_MESSAGE_ID)3146081, // D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET,
				};

				NewFilter.DenyList.NumIDs = sizeof(DenyIds)/sizeof(D3D11_MESSAGE_ID);
				NewFilter.DenyList.pIDList = (D3D11_MESSAGE_ID*)&DenyIds;

				pd3dInfoQueue->PushStorageFilter(&NewFilter);

				// Break on D3D debug errors.
				pd3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR,true);

				// Enable this to break on a specific id in order to quickly get a callstack
				//pd3dInfoQueue->SetBreakOnID(D3D11_MESSAGE_ID_DEVICE_DRAW_CONSTANT_BUFFER_TOO_SMALL, true);

				if(FParse::Param(FCommandLine::Get(),TEXT("d3dbreakonwarning")))
				{
					pd3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING,true);
				}

				pd3dInfoQueue->Release();
			}
		}
#endif

		FHardwareInfo::RegisterHardwareInfo( NAME_RHI, TEXT( "D3D11" ) );

		GRHISupportsTextureStreaming = true;

		// Set the RHI initialized flag.
		GIsRHIInitialized = true;
	}
}

/**
 *	Retrieve available screen resolutions.
 *
 *	@param	Resolutions			TArray<FScreenResolutionRHI> parameter that will be filled in.
 *	@param	bIgnoreRefreshRate	If true, ignore refresh rates.
 *
 *	@return	bool				true if successfully filled the array
 */
bool FD3D11DynamicRHI::RHIGetAvailableResolutions(FScreenResolutionArray& Resolutions, bool bIgnoreRefreshRate)
{
	int32 MinAllowableResolutionX = 0;
	int32 MinAllowableResolutionY = 0;
	int32 MaxAllowableResolutionX = 10480;
	int32 MaxAllowableResolutionY = 10480;
	int32 MinAllowableRefreshRate = 0;
	int32 MaxAllowableRefreshRate = 10480;

	if (MaxAllowableResolutionX == 0)
	{
		MaxAllowableResolutionX = 10480;
	}
	if (MaxAllowableResolutionY == 0)
	{
		MaxAllowableResolutionY = 10480;
	}
	if (MaxAllowableRefreshRate == 0)
	{
		MaxAllowableRefreshRate = 10480;
	}

	// Check the default adapter only.
	int32 CurrentAdapter = 0;
	HRESULT hr = S_OK;
	TRefCountPtr<IDXGIAdapter> Adapter;
	hr = DXGIFactory->EnumAdapters(CurrentAdapter,Adapter.GetInitReference());

	if( DXGI_ERROR_NOT_FOUND == hr )
		return false;
	if( FAILED(hr) )
		return false;

	// get the description of the adapter
	DXGI_ADAPTER_DESC AdapterDesc;
	VERIFYD3D11RESULT(Adapter->GetDesc(&AdapterDesc));

	int32 CurrentOutput = 0;
	do 
	{
		TRefCountPtr<IDXGIOutput> Output;
		hr = Adapter->EnumOutputs(CurrentOutput,Output.GetInitReference());
		if(DXGI_ERROR_NOT_FOUND == hr)
			break;
		if(FAILED(hr))
			return false;

		// TODO: GetDisplayModeList is a terribly SLOW call.  It can take up to a second per invocation.
		//  We might want to work around some DXGI badness here.
		DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		uint32 NumModes = 0;
		hr = Output->GetDisplayModeList(Format, 0, &NumModes, NULL);
		if(hr == DXGI_ERROR_NOT_FOUND)
		{
			continue;
		}
		else if(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
		{
			UE_LOG(LogD3D11RHI, Fatal,
				TEXT("This application cannot be run over a remote desktop configuration")
				);
			return false;
		}

		DXGI_MODE_DESC* ModeList = new DXGI_MODE_DESC[ NumModes ];
		VERIFYD3D11RESULT(Output->GetDisplayModeList(Format, 0, &NumModes, ModeList));

		for(uint32 m = 0;m < NumModes;m++)
		{
			if (((int32)ModeList[m].Width >= MinAllowableResolutionX) &&
				((int32)ModeList[m].Width <= MaxAllowableResolutionX) &&
				((int32)ModeList[m].Height >= MinAllowableResolutionY) &&
				((int32)ModeList[m].Height <= MaxAllowableResolutionY)
				)
			{
				bool bAddIt = true;
				if (bIgnoreRefreshRate == false)
				{
					if (((int32)ModeList[m].RefreshRate.Numerator < MinAllowableRefreshRate * ModeList[m].RefreshRate.Denominator) ||
						((int32)ModeList[m].RefreshRate.Numerator > MaxAllowableRefreshRate * ModeList[m].RefreshRate.Denominator)
						)
					{
						continue;
					}
				}
				else
				{
					// See if it is in the list already
					for (int32 CheckIndex = 0; CheckIndex < Resolutions.Num(); CheckIndex++)
					{
						FScreenResolutionRHI& CheckResolution = Resolutions[CheckIndex];
						if ((CheckResolution.Width == ModeList[m].Width) &&
							(CheckResolution.Height == ModeList[m].Height))
						{
							// Already in the list...
							bAddIt = false;
							break;
						}
					}
				}

				if (bAddIt)
				{
					// Add the mode to the list
					int32 Temp2Index = Resolutions.AddZeroed();
					FScreenResolutionRHI& ScreenResolution = Resolutions[Temp2Index];

					ScreenResolution.Width = ModeList[m].Width;
					ScreenResolution.Height = ModeList[m].Height;
					ScreenResolution.RefreshRate = ModeList[m].RefreshRate.Numerator / ModeList[m].RefreshRate.Denominator;
				}
			}
		}

		delete[] ModeList;

		++CurrentOutput;

	// TODO: Cap at 1 for default output
	} while(CurrentOutput < 1);

	return true;
}
