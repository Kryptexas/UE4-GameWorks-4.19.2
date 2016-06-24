// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanRHI.cpp: Vulkan device RHI implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "HardwareInfo.h"
#include "VulkanShaderResources.h"
#include "VulkanSwapChain.h"
#include "VulkanResources.h"
#include "VulkanPendingState.h"
#include "VulkanContext.h"
#include "VulkanManager.h"

#ifdef VK_API_VERSION
// Check the SDK is least the API version we want to use
static_assert(VK_API_VERSION >= UE_VK_API_VERSION, "Vulkan SDK is older than the version we want to support (UE_VK_API_VERSION). Please update your SDK.");
#elif !defined(VK_HEADER_VERSION)
	#error No VulkanSDK defines?
#endif

#if VK_HEADER_VERSION < 8 && (VK_API_VERSION < VK_MAKE_VERSION(1, 0, 3))
	#include <vulkan/vk_ext_debug_report.h>
#endif

///////////////////////////////////////////////////////////////////////////////

#if PLATFORM_ANDROID

// Vulkan function pointers

#define DEFINE_VK_ENTRYPOINTS(Type,Func) Type Func = NULL;
ENUM_VK_ENTRYPOINTS_ALL(DEFINE_VK_ENTRYPOINTS)

#endif

///////////////////////////////////////////////////////////////////////////////

static TAutoConsoleVariable<int32> GLSLCvar(
	TEXT("r.Vulkan.UseGLSL"),
	PLATFORM_ANDROID ? 2 : 0,
	TEXT("2 to use ES GLSL\n1 to use GLSL\n0 to use SPIRV")
	);

static TAutoConsoleVariable<int32> GRHIThreadCvar(
	TEXT("r.Vulkan.RHIThread"),
	0,
	TEXT("1 to use RHI Thread")
	);

#if VULKAN_CUSTOM_MEMORY_MANAGER_ENABLED
VkAllocationCallbacks GCallbacks;
#endif

struct FVulkanMemManager
{
	static VKAPI_ATTR void* Alloc(
		void*                                       pUserData,
		size_t                                      size,
		size_t                                      alignment,
		VkSystemAllocationScope                           allocScope)
	{
		FVulkanMemManager* This = (FVulkanMemManager*)pUserData;
		This->MaxAllocSize = FMath::Max(This->MaxAllocSize, size);
		This->UsedMemory += size;
		void* Data = FMemory::Malloc(size, alignment);
		This->Allocs.Add(Data, size);
		return Data;
	}

	static VKAPI_ATTR void Free(
		void*                                       pUserData,
		void*                                       pMem)
	{
		FVulkanMemManager* This = (FVulkanMemManager*)pUserData;
		size_t Size = This->Allocs.FindAndRemoveChecked(pMem);
		This->UsedMemory -= Size;
		FMemory::Free(pMem);
	}

	static VKAPI_ATTR void* Realloc(
		void*										pUserData,
		void*										pOriginal,
		size_t										size,
		size_t										alignment,
		VkSystemAllocationScope						allocScope)
	{
		FVulkanMemManager* This = (FVulkanMemManager*)pUserData;
		size_t Size = This->Allocs.FindAndRemoveChecked(pOriginal);
		This->UsedMemory -= Size;
		void* Data = FMemory::Realloc(pOriginal, size, alignment);
		This->Allocs.Add(Data, size);
		This->UsedMemory += size;
		This->MaxAllocSize = FMath::Max(This->MaxAllocSize, size);
		return Data;
	}

	static VKAPI_ATTR void InternalAllocationNotification(
		void*										pUserData,
		size_t										size,
		VkInternalAllocationType					allocationType,
		VkSystemAllocationScope						allocationScope)
	{
		//@TODO
	}

	static VKAPI_ATTR void InternalFreeNotification(
		void*										pUserData,
		size_t										size,
		VkInternalAllocationType					allocationType,
		VkSystemAllocationScope						allocationScope)
	{
		//@TODO
	}

	FVulkanMemManager() :
		MaxAllocSize(0),
		UsedMemory(0)
	{
	}

	TMap<void*, size_t> Allocs;
	size_t MaxAllocSize;
	size_t UsedMemory;
};

static FVulkanMemManager GVulkanMemMgr;

static inline VkAttachmentLoadOp RenderTargetLoadActionToVulkan(ERenderTargetLoadAction InLoadAction)
{
	VkAttachmentLoadOp OutLoadAction = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;

	switch (InLoadAction)
	{
	case ERenderTargetLoadAction::ELoad:		OutLoadAction = VK_ATTACHMENT_LOAD_OP_LOAD;			break;
	case ERenderTargetLoadAction::EClear:		OutLoadAction = VK_ATTACHMENT_LOAD_OP_CLEAR;		break;
	case ERenderTargetLoadAction::ENoAction:	OutLoadAction = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	break;
	default:																						break;
	}

	// Check for missing translation
	check(OutLoadAction != VK_ATTACHMENT_LOAD_OP_MAX_ENUM);
	return OutLoadAction;
}

static inline VkAttachmentStoreOp RenderTargetStoreActionToVulkan(ERenderTargetStoreAction InStoreAction)
{
	VkAttachmentStoreOp OutStoreAction = VK_ATTACHMENT_STORE_OP_MAX_ENUM;

	switch (InStoreAction)
	{
	case ERenderTargetStoreAction::EStore:		OutStoreAction = VK_ATTACHMENT_STORE_OP_STORE;		break;
	case ERenderTargetStoreAction::ENoAction:	OutStoreAction = VK_ATTACHMENT_STORE_OP_DONT_CARE;	break;
	default:																						break;
	}

	// Check for missing translation
	check(OutStoreAction != VK_ATTACHMENT_STORE_OP_MAX_ENUM);
	return OutStoreAction;
}

static inline int32 CountSetBits(int32 n)
{
	uint32 Count = 0;
	while (n)
	{
		n &= (n-1);
		Count++;
	}

	return Count;
}


DEFINE_LOG_CATEGORY(LogVulkan)

#if PLATFORM_ANDROID
#include <dlfcn.h>

static void *VulkanLib = nullptr;
static bool bAttemptedLoad = false;

static bool LoadVulkanLibrary()
{
	if (bAttemptedLoad)
	{
		return (VulkanLib != nullptr);
	}
	bAttemptedLoad = true;

	// try to load libvulkan.so
	VulkanLib = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
	if (VulkanLib == nullptr)
	{
		return false;
	}

	bool bFoundAllEntryPoints = true;
#define CHECK_VK_ENTRYPOINTS(Type,Func) if (Func == NULL) { bFoundAllEntryPoints = false; UE_LOG(LogRHI, Warning, TEXT("Failed to find entry point for %s"), TEXT(#Func)); }

	// Initialize all of the entry points we have to query manually
#define GET_VK_ENTRYPOINTS(Type,Func) Func = (Type)dlsym(VulkanLib, #Func);
	ENUM_VK_ENTRYPOINTS_BASE(GET_VK_ENTRYPOINTS);
	ENUM_VK_ENTRYPOINTS_BASE(CHECK_VK_ENTRYPOINTS);
	if (!bFoundAllEntryPoints)
	{
		dlclose(VulkanLib);
		VulkanLib = nullptr;
		return false;
	}

	ENUM_VK_ENTRYPOINTS_OPTIONAL(GET_VK_ENTRYPOINTS);
	//ENUM_VK_ENTRYPOINTS_OPTIONAL(CHECK_VK_ENTRYPOINTS);

	return true;
}

static bool LoadVulkanInstanceFunctions(VkInstance inInstance)
{
	bool bFoundAllEntryPoints = true;
#define CHECK_VK_ENTRYPOINTS(Type,Func) if (Func == NULL) { bFoundAllEntryPoints = false; UE_LOG(LogRHI, Warning, TEXT("Failed to find entry point for %s"), TEXT(#Func)); }

#define GETINSTANCE_VK_ENTRYPOINTS(Type, Func) Func = (Type)vkGetInstanceProcAddr(inInstance, #Func);
	ENUM_VK_ENTRYPOINTS_INSTANCE(GETINSTANCE_VK_ENTRYPOINTS);
	ENUM_VK_ENTRYPOINTS_INSTANCE(CHECK_VK_ENTRYPOINTS);

	return bFoundAllEntryPoints;
}

static void FreeVulkanLibrary()
{
	if (VulkanLib != nullptr)
	{
#define CLEAR_VK_ENTRYPOINTS(Type,Func) Func = nullptr;
		ENUM_VK_ENTRYPOINTS_ALL(CLEAR_VK_ENTRYPOINTS);

		dlclose(VulkanLib);
		VulkanLib = nullptr;
	}
	bAttemptedLoad = false;
}

#elif PLATFORM_WINDOWS

#include "AllowWindowsPlatformTypes.h"
static HMODULE GVulkanDLLModule = nullptr;
static bool LoadVulkanLibrary()
{
	// Try to load the vulkan dll, as not everyone has the sdk installed
	GVulkanDLLModule = ::LoadLibraryW(TEXT("vulkan-1.dll"));
	return GVulkanDLLModule != nullptr;
}

static bool LoadVulkanInstanceFunctions(VkInstance inInstance)
{
	return true;
}

static void FreeVulkanLibrary()
{
	if (GVulkanDLLModule != nullptr)
	{
		::FreeLibrary(GVulkanDLLModule);
		GVulkanDLLModule = nullptr;
	}
}
#include "HideWindowsPlatformTypes.h"

#else
#error Unsupported!
#endif // PLATFORM_ANDROID

bool FVulkanDynamicRHIModule::IsSupported()
{
	return true;
}

FDynamicRHI* FVulkanDynamicRHIModule::CreateRHI()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("SM5")))
	{
		GMaxRHIFeatureLevel = ERHIFeatureLevel::SM5;
		GMaxRHIShaderPlatform = SP_VULKAN_SM5;
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("SM4")))
	{
		GMaxRHIFeatureLevel = ERHIFeatureLevel::SM4;
		GMaxRHIShaderPlatform = SP_VULKAN_SM4;
	}
	else
	{
		GMaxRHIFeatureLevel = ERHIFeatureLevel::ES3_1;
		GMaxRHIShaderPlatform = PLATFORM_ANDROID ? SP_VULKAN_ES3_1_ANDROID : SP_VULKAN_PCES3_1;
	}

	//#todo-rco: When is this needed?
#if 0
	// VULKAN_USE_MSAA_RESOLVE_ATTACHMENTS=0 requires separate MSAA and resolve textures
	check(RHISupportsSeparateMSAAAndResolveTextures(GMaxRHIShaderPlatform) == (true && !VULKAN_USE_MSAA_RESOLVE_ATTACHMENTS));
#endif

	return new FVulkanDynamicRHI();
}

IMPLEMENT_MODULE(FVulkanDynamicRHIModule, VulkanRHI);


FVulkanCommandListContext::FVulkanCommandListContext(FVulkanDynamicRHI* InRHI, FVulkanDevice* InDevice, bool bInIsImmediate)
	: RHI(InRHI)
	, Device(InDevice)
	, bIsImmediate(bInIsImmediate)
	, PendingNumVertices(0)
	, PendingVertexDataStride(0)
	, PendingPrimitiveIndexType(VK_INDEX_TYPE_MAX_ENUM)
	, PendingPrimitiveType(0)
	, PendingNumPrimitives(0)
	, PendingMinVertexIndex(0)
	, PendingIndexDataStride(0)
	, TempFrameAllocationBuffer(InDevice, VULKAN_TEMP_FRAME_ALLOCATOR_SIZE)
	, CommandBufferManager(nullptr)
	, PendingState(nullptr)
{
	// Create CommandBufferManager, contain all active buffers
	CommandBufferManager = new FVulkanCommandBufferManager(InDevice);

	// Create Pending state, contains pipeline states such as current shader and etc..
	PendingState = new FVulkanPendingState(InDevice);
}

FVulkanCommandListContext::~FVulkanCommandListContext()
{
	check(CommandBufferManager != nullptr);
	delete CommandBufferManager;
	CommandBufferManager = nullptr;

	delete PendingState;

	TempFrameAllocationBuffer.Destroy();
}

namespace VulkanRHI
{
	FManager GManager;
}

FVulkanDynamicRHI::FVulkanDynamicRHI()
	: Instance(VK_NULL_HANDLE)
	, Device(nullptr)
	, DrawingViewport(nullptr)
#if VULKAN_HAS_DEBUGGING_ENABLED
	, MsgCallback(VK_NULL_HANDLE)
#endif
	, PresentCount(0)
#if VULKAN_ENABLE_PIPELINE_CACHE
	, SavePipelineCacheCmd(nullptr)
	, RebuildPipelineCacheCmd(nullptr)
#endif
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	, DumpMemoryCmd(nullptr)
#endif
{
	// This should be called once at the start 
	check(IsInGameThread());
	check(!GIsThreadedRendering);

	GRHIRequiresEarlyBackBufferRenderTarget = false;
#if PLATFORM_ANDROID
	GHardwareHiddenSurfaceRemoval = true;
#endif
	GPoolSizeVRAMPercentage = 0;
	GTexturePoolSize = 0;
	GConfig->GetInt(TEXT("TextureStreaming"), TEXT("PoolSizeVRAMPercentage"), GPoolSizeVRAMPercentage, GEngineIni);
}

void FVulkanDynamicRHI::Init()
{
	if (!LoadVulkanLibrary())
	{
		UE_LOG(LogVulkanRHI, Fatal, TEXT("Failed to find all required Vulkan entry points; make sure your driver supports Vulkan!"));
	}

	InitInstance();
}

void FVulkanDynamicRHI::Shutdown()
{
	check(IsInGameThread() && IsInRenderingThread());
	check(Device);

	Device->PrepareForDestroy();

	EmptyCachedBoundShaderStates();

	FVulkanVertexDeclaration::EmptyCache();

	if (GIsRHIInitialized)
	{
		// Reset the RHI initialized flag.
		GIsRHIInitialized = false;

		check(!GIsCriticalError);

		// Ask all initialized FRenderResources to release their RHI resources.
		for (TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
		{
			FRenderResource* Resource = *ResourceIt;
			check(Resource->IsInitialized());
			Resource->ReleaseRHI();
		}

		for (TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
		{
			ResourceIt->ReleaseDynamicRHI();
		}

#if 0
		for (auto* Framebuffer : FrameBuffers)
		{
			Framebuffer->Destroy(*Device);
			delete Framebuffer;
		}
#endif

		//EmptyVulkanSamplerStateCache();

		// Flush all pending deletes before destroying the device.
		FRHIResource::FlushPendingDeletes();

		// And again since some might get on a pending queue
		FRHIResource::FlushPendingDeletes();

		//ReleasePooledTextures();
	}

	Device->Destroy();

	delete Device;
	Device = nullptr;

#if !VULKAN_DISABLE_DEBUG_CALLBACK && VULKAN_HAS_DEBUGGING_ENABLED
	RemoveDebugLayerCallback();
#endif

	vkDestroyInstance(Instance, nullptr);

#if VULKAN_ENABLE_PIPELINE_CACHE
	IConsoleManager::Get().UnregisterConsoleObject(SavePipelineCacheCmd);
	IConsoleManager::Get().UnregisterConsoleObject(RebuildPipelineCacheCmd);
#endif

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	IConsoleManager::Get().UnregisterConsoleObject(DumpMemoryCmd);
#endif

	FreeVulkanLibrary();
}

void FVulkanDynamicRHI::CreateInstance()
{
	VkApplicationInfo App;
	FMemory::Memzero(App);
	App.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	App.pApplicationName = "UE4";
	App.applicationVersion = 0;
	App.pEngineName = "UE4";
	App.engineVersion = 0;
	App.apiVersion = UE_VK_API_VERSION;

	VkInstanceCreateInfo InstInfo;
	FMemory::Memzero(InstInfo);
	InstInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	InstInfo.pNext = nullptr;
	InstInfo.pApplicationInfo = &App;

#if VULKAN_CUSTOM_MEMORY_MANAGER_ENABLED
	GCallbacks.pUserData = &GVulkanMemMgr;
	GCallbacks.pfnAllocation = &FVulkanMemManager::Alloc;
	GCallbacks.pfnReallocation = &FVulkanMemManager::Realloc;
	GCallbacks.pfnFree = &FVulkanMemManager::Free;
	GCallbacks.pfnInternalAllocation = &FVulkanMemManager::InternalAllocationNotification;
	GCallbacks.pfnInternalFree = &FVulkanMemManager::InternalFreeNotification;

//@TODO: pass GCallbacks into funcs that take them. currently it's just a nullptr
#endif

	GetInstanceLayersAndExtensions(InstanceExtensions, InstanceLayers);

	InstInfo.enabledExtensionCount = InstanceExtensions.Num();
	InstInfo.ppEnabledExtensionNames = InstInfo.enabledExtensionCount > 0 ? (const ANSICHAR* const*)InstanceExtensions.GetData() : nullptr;
	
	#if VULKAN_HAS_DEBUGGING_ENABLED
		InstInfo.enabledLayerCount = (GValidationCvar.GetValueOnAnyThread() > 0) ? InstanceLayers.Num() : 0;
		InstInfo.ppEnabledLayerNames = InstInfo.enabledLayerCount > 0 ? InstanceLayers.GetData() : nullptr;
	#endif

	VkResult Result = vkCreateInstance(&InstInfo, nullptr, &Instance);
	
	if(Result == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		checkf(0, TEXT(
			"Cannot find a compatible Vulkan installable client driver \
			(ICD).\n\nPlease look at the Getting Started guide for \
			additional information.\n\
			vkCreateInstance Failure"));
	}
	else if(Result == VK_ERROR_EXTENSION_NOT_PRESENT)
	{
		checkf(0, TEXT(
			"Cannot find a specified extension library\
			 .\nMake sure your layers path is set appropriately\n\
			 vkCreateInstance Failure"));
	}
	else if(Result)
	{
		checkf(0, TEXT(
			"vkCreateInstance failed.\n\nDo you have a compatible Vulkan \
			 installable client driver (ICD) installed?\nPlease look at \
			 the Getting Started guide for additional information.\n\
			 vkCreateInstance Failure"));
	}

	VERIFYVULKANRESULT(Result);

	if (!LoadVulkanInstanceFunctions(Instance))
	{
		UE_LOG(LogVulkanRHI, Fatal, TEXT("Failed to find all required Vulkan entry points! Maybe using an older SDK/driver?"));
	}

#if !VULKAN_DISABLE_DEBUG_CALLBACK && VULKAN_HAS_DEBUGGING_ENABLED
	SetupDebugLayerCallback();
#endif
}

void FVulkanDynamicRHI::InitInstance()
{
	check(IsInGameThread());

	// Wait for the rendering thread to go idle.
	SCOPED_SUSPEND_RENDERING_THREAD(false);

	if (!Device)
	{
		check(!GIsRHIInitialized);

#ifdef PLATFORM_ANDROID
		// Want to see the actual crash report on Android so unregister signal handlers
		FPlatformMisc::SetCrashHandler((void(*)(const FGenericCrashContext& Context)) -1);
#endif

		GRHISupportsAsyncTextureCreation = false;

		bool bDeviceSupportsGeometryShaders = true;
		bool bDeviceSupportsTessellation = true;

		{
			CreateInstance();

			uint32 GpuCount = 0;
			VERIFYVULKANRESULT_EXPANDED(vkEnumeratePhysicalDevices(Instance, &GpuCount, nullptr));
			check(GpuCount >= 1);

			TArray<VkPhysicalDevice> PhysicalDevices;
			PhysicalDevices.AddZeroed(GpuCount);
			VERIFYVULKANRESULT_EXPANDED(vkEnumeratePhysicalDevices(Instance, &GpuCount, PhysicalDevices.GetData()));

			UE_LOG(LogVulkanRHI, Display, TEXT("Found %d device(s)"), GpuCount);
			for (uint32 Index = 0; Index < GpuCount; ++Index)
			{
				FVulkanDevice* NewDevice = new FVulkanDevice(PhysicalDevices[Index]);
				Devices.Add(NewDevice);

				bool bCouldBeMainDevice = NewDevice->QueryGPU(Index);
				if (Index == GpuCount - 1 || bCouldBeMainDevice)
				{
					NewDevice->InitGPU(Index);
					Device = NewDevice;

					bDeviceSupportsGeometryShaders = bDeviceSupportsGeometryShaders && NewDevice->GetFeatures().geometryShader;
					bDeviceSupportsTessellation = bDeviceSupportsTessellation && NewDevice->GetFeatures().tessellationShader;
				}
			}

			check(Device);
		}

		const VkPhysicalDeviceProperties& Props = Device->GetDeviceProperties();

		// Initialize the RHI capabilities.
		GRHIVendorId = Device->GetDeviceProperties().vendorID;
		GRHIAdapterName = ANSI_TO_TCHAR(Props.deviceName);
		GSupportsRenderTargetFormat_PF_G8 = false;	// #todo-rco
		GSupportsQuads = false;	// Not supported in Vulkan
		GRHISupportsTextureStreaming = false;	// #todo-rco
		GRHISupportsRHIThread = GRHIThreadCvar->GetInt() != 0;	// #todo-rco

		//#todo-rco: Leaks/issues still present!
		// Indicate that the RHI needs to use the engine's deferred deletion queue.
		GRHINeedsExtraDeletionLatency = true;

		GMaxShadowDepthBufferSizeX =  FPlatformMath::Min<int32>(Props.limits.maxImageDimension2D, GMaxShadowDepthBufferSizeX);
		GMaxShadowDepthBufferSizeY =  FPlatformMath::Min<int32>(Props.limits.maxImageDimension2D, GMaxShadowDepthBufferSizeY);
		GMaxTextureDimensions = Props.limits.maxImageDimension2D;
		GMaxTextureMipCount = FPlatformMath::CeilLogTwo( GMaxTextureDimensions ) + 1;
		GMaxTextureMipCount = FPlatformMath::Min<int32>( MAX_TEXTURE_MIP_COUNT, GMaxTextureMipCount );
		GMaxCubeTextureDimensions = Props.limits.maxImageDimensionCube;
		GMaxTextureArrayLayers = Props.limits.maxImageArrayLayers;
		GMaxVulkanTextureFilterAnisotropic = Props.limits.maxSamplerAnisotropy;
		GRHISupportsBaseVertexIndex = false;

		GShaderPlatformForFeatureLevel[ERHIFeatureLevel::ES2] = GMaxRHIFeatureLevel == ERHIFeatureLevel::ES2 ? GMaxRHIShaderPlatform : SP_NumPlatforms;
		GShaderPlatformForFeatureLevel[ERHIFeatureLevel::ES3_1] = GMaxRHIFeatureLevel == ERHIFeatureLevel::ES3_1 ? GMaxRHIShaderPlatform : SP_NumPlatforms;
		GShaderPlatformForFeatureLevel[ERHIFeatureLevel::SM4] = (GMaxRHIFeatureLevel == ERHIFeatureLevel::SM4 && bDeviceSupportsGeometryShaders) ? GMaxRHIShaderPlatform : SP_NumPlatforms;
		GShaderPlatformForFeatureLevel[ERHIFeatureLevel::SM5] = (GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5 && bDeviceSupportsTessellation) ? GMaxRHIShaderPlatform : SP_NumPlatforms;

		// without that the first RHIClear would get a scissor rect of (0,0)-(0,0) which means we get a draw call clear 
		Device->GetImmediateContext().RHISetScissorRect(false, 0, 0, 0, 0);

		GDynamicRHI = this;

		// Notify all initialized FRenderResources that there's a valid RHI device to create their RHI resources for now.
		for (TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList()); ResourceIt; ResourceIt.Next())
		{
			ResourceIt->InitRHI();
		}
		// Dynamic resources can have dependencies on static resources (with uniform buffers) and must initialized last!
		for (TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList()); ResourceIt; ResourceIt.Next())
		{
			ResourceIt->InitDynamicRHI();
		}

		FHardwareInfo::RegisterHardwareInfo(NAME_RHI, TEXT("Vulkan"));

		GProjectionSignY = 1.0f;

		GIsRHIInitialized = true;

#if VULKAN_ENABLE_PIPELINE_CACHE
		SavePipelineCacheCmd = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("SavePipelineCache"),
			TEXT("Save pipeline cache."),
			FConsoleCommandDelegate::CreateStatic(SavePipelineCache),
			ECVF_Default
			);

		RebuildPipelineCacheCmd = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("RebuildPipelineCache"),
			TEXT("Rebuilds pipeline cache."),
			FConsoleCommandDelegate::CreateStatic(RebuildPipelineCache),
			ECVF_Default
			);
#endif

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		DumpMemoryCmd = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("DumpVulkanMemory"),
			TEXT("Dumps memory map."),
			FConsoleCommandDelegate::CreateStatic(DumpMemory),
			ECVF_Default
			);
#endif
	}
}

void FVulkanCommandListContext::RHIBeginFrame()
{
	PendingState->GetGlobalUniformPool().BeginFrame();

	VulkanRHI::GManager.GPUProfilingData.BeginFrame(this, Device->GetTimestampQueryPool(RHI->GetPresentCount() % FVulkanDevice::NumTimestampPools));
}


void FVulkanCommandListContext::RHIBeginScene()
{
}

void FVulkanCommandListContext::RHIEndScene()
{
	FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
	if (PendingState->IsRenderPassActive())
	{
		PendingState->RenderPassEnd(CmdBuffer);
	}
}

void FVulkanCommandListContext::RHIBeginDrawingViewport(FViewportRHIParamRef ViewportRHI, FTextureRHIParamRef RenderTargetRHI)
{
	static bool bHackClear = false;
	FVulkanViewport* Viewport = ResourceCast(ViewportRHI);
	if (Viewport->GetBackBufferIndex() < 0)
	{
		Viewport->AcquireBackBuffer(Device->GetImmediateContext().GetCommandBufferManager()->GetActiveCmdBuffer());
	}
	RHI->DrawingViewport = Viewport;

	if (!RenderTargetRHI)
	{
		FVulkanTexture2D* BackBuffer = Viewport->GetBackBuffer();
		RenderTargetRHI = BackBuffer;
		if (bHackClear)
		{
			bHackClear = false;
		}
	}

	FRHIRenderTargetView RTView(RenderTargetRHI);
	if (bHackClear)
	{
		RTView.LoadAction = ERenderTargetLoadAction::EClear;
	}

	RHISetRenderTargets(1, &RTView, nullptr, 0, nullptr);
}

void FVulkanCommandListContext::RHIEndDrawingViewport(FViewportRHIParamRef ViewportRHI, bool bPresent, bool bLockToVsync)
{
	FVulkanViewport* Viewport = ResourceCast(ViewportRHI);
	check(Viewport == RHI->DrawingViewport);

	RHI->Present();

	PendingState->InitFrame();
}

void FVulkanCommandListContext::RHIEndFrame()
{
	Device->GetDeferredDeletionQueue().Clear();

	Device->GetStagingManager().ProcessPendingFree();
	Device->GetResourceHeapManager().ReleaseFreedPages();

	Device->FrameCounter++;
}

void FVulkanCommandListContext::RHIPushEvent(const TCHAR* Name, FColor Color)
{
	FString EventName = Name;
	EventStack.Add(Name);

	//only valid on immediate context currently.  needs to be fixed for parallel rhi execute
	if (IsImmediate())
	{
#if VULKAN_ENABLE_DRAW_MARKERS
		if (auto VkCmdDbgMarkerBegin = Device->GetCmdDbgMarkerBegin())
		{
			VkCmdDbgMarkerBegin(GetCommandBufferManager()->GetActiveCmdBuffer()->GetHandle(), TCHAR_TO_ANSI(Name));
		}
#endif

		VulkanRHI::GManager.GPUProfilingData.PushEvent(Name, Color);
	}
}

void FVulkanCommandListContext::RHIPopEvent()
{
	//only valid on immediate context currently.  needs to be fixed for parallel rhi execute
	if (IsImmediate())
	{
#if VULKAN_ENABLE_DRAW_MARKERS
		if (auto VkCmdDbgMarkerEnd = Device->GetCmdDbgMarkerEnd())
		{
			VkCmdDbgMarkerEnd(GetCommandBufferManager()->GetActiveCmdBuffer()->GetHandle());
		}
#endif

		VulkanRHI::GManager.GPUProfilingData.PopEvent();
	}

	check(EventStack.Num() > 0);
	EventStack.Pop(false);
}

void FVulkanDynamicRHI::RHIGetSupportedResolution( uint32 &Width, uint32 &Height )
{
}

bool FVulkanDynamicRHI::RHIGetAvailableResolutions(FScreenResolutionArray& Resolutions, bool bIgnoreRefreshRate)
{
	return false;
}

void FVulkanDynamicRHI::RHIFlushResources()
{
}

void FVulkanDynamicRHI::RHIAcquireThreadOwnership()
{
}

void FVulkanDynamicRHI::RHIReleaseThreadOwnership()
{
}

void* FVulkanDynamicRHI::RHIGetNativeDevice()
{
	return (void*)Device->GetInstanceHandle();
}

IRHICommandContext* FVulkanDynamicRHI::RHIGetDefaultContext()
{
	return &Device->GetImmediateContext();
}

IRHICommandContextContainer* FVulkanDynamicRHI::RHIGetCommandContextContainer()
{
	return nullptr;
}


FVulkanBuffer::FVulkanBuffer(FVulkanDevice& InDevice, uint32 InSize, VkFlags InUsage, VkMemoryPropertyFlags InMemPropertyFlags, bool bInAllowMultiLock, const char* File, int32 Line) :
	Device(InDevice),
	Buf(VK_NULL_HANDLE),
	Allocation(nullptr),
	Size(InSize),
	Usage(InUsage),
	BufferPtr(nullptr),
	bAllowMultiLock(bInAllowMultiLock),
	LockStack(0)
{
	VkBufferCreateInfo BufInfo;
	FMemory::Memzero(BufInfo);
	BufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufInfo.size = Size;
	BufInfo.usage = Usage;
	VERIFYVULKANRESULT_EXPANDED(vkCreateBuffer(Device.GetInstanceHandle(), &BufInfo, nullptr, &Buf));

	VkMemoryRequirements MemoryRequirements;
	vkGetBufferMemoryRequirements(Device.GetInstanceHandle(), Buf, &MemoryRequirements);

	Allocation = InDevice.GetMemoryManager().Alloc(MemoryRequirements.size, MemoryRequirements.memoryTypeBits, InMemPropertyFlags, File ? File : __FILE__, Line ? Line : __LINE__);
	check(Allocation);
	//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("** vkBindImageMemory Buf %p MemHandle %p MemOffset %d Size %u\n"), (void*)Buf, (void*)Allocation->GetHandle(), (uint32)0, (uint32)MemoryRequirements.size);
	VERIFYVULKANRESULT_EXPANDED(vkBindBufferMemory(Device.GetInstanceHandle(), Buf, Allocation->GetHandle(), 0));
}

FVulkanBuffer::~FVulkanBuffer()
{
	// The buffer should be unmapped
	check(BufferPtr == nullptr);

	vkDestroyBuffer(Device.GetInstanceHandle(), Buf, nullptr);
	Buf = VK_NULL_HANDLE;

	Device.GetMemoryManager().Free(Allocation);
	Allocation = nullptr;
}

void* FVulkanBuffer::Lock(uint32 InSize, uint32 InOffset)
{
	check(InSize + InOffset <= Size);

	// The buffer should be unmapped, before it can be mapped 
	// @todo vulkan: RingBuffer can be mapped multiple times

	uint32 BufferPtrOffset = 0;
	if (bAllowMultiLock)
	{
		if (LockStack == 0)
		{
			// lock the whole range
			BufferPtr = Allocation->Map(GetSize(), 0);
		}
		// offset the whole range by the requested offset
		BufferPtrOffset = InOffset;
		LockStack++;
	}
	else
	{
		check(BufferPtr == nullptr);
		BufferPtr = Allocation->Map(InSize, InOffset);
	}

	return (uint8*)BufferPtr + BufferPtrOffset;
}

void FVulkanBuffer::Unlock()
{
	// The buffer should be mapped, before it can be unmapped
	check(BufferPtr != nullptr);

	// for multi-lock, if not down to 0, do nothing
	if (bAllowMultiLock && --LockStack > 0)
	{
		return;
	}

	Allocation->Unmap();
	BufferPtr = nullptr;
}


FVulkanDescriptorSetsLayout::FVulkanDescriptorSetsLayout(FVulkanDevice* InDevice) :
	Device(InDevice)
{
	FMemory::Memzero(LayoutTypes);
}

FVulkanDescriptorSetsLayout::~FVulkanDescriptorSetsLayout()
{
	check(Device);
	for (VkDescriptorSetLayout Handle : LayoutHandles)
	{
		vkDestroyDescriptorSetLayout(Device->GetInstanceHandle(), Handle, nullptr);
	}
}

void FVulkanDescriptorSetsLayout::AddDescriptor(int32 DescriptorSetIndex, const VkDescriptorSetLayoutBinding& Descriptor, int32 BindingIndex)
{
	// Setting descriptor is only allowed prior to compiling the layout
	check(LayoutHandles.Num() == 0);

	// Increment type usage
	LayoutTypes[Descriptor.descriptorType]++;

	FSetLayout* DescSetLayout = nullptr;
	for (int32 Index = 0; Index < Layouts.Num(); ++Index)
	{
		if (Layouts[Index].DescriptorSetIndex == DescriptorSetIndex)
		{
			DescSetLayout = &Layouts[Index];
			break;
		}
	}

	if (!DescSetLayout)
	{
		DescSetLayout = new(Layouts) FSetLayout;
		DescSetLayout->DescriptorSetIndex = DescriptorSetIndex;
	}

	VkDescriptorSetLayoutBinding* Binding = new(DescSetLayout->LayoutBindings) VkDescriptorSetLayoutBinding;
	FMemory::Memzero(*Binding);
	*Binding = Descriptor;
}

void FVulkanDescriptorSetsLayout::Compile()
{
	check(LayoutHandles.Num() == 0);
	check(Device);

	// Check if we obey limits
	const VkPhysicalDeviceLimits& Limits = Device->GetLimits();
	
	// Check for maxDescriptorSetSamplers
	check(		LayoutTypes[VK_DESCRIPTOR_TYPE_SAMPLER]
			+	LayoutTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER]
			<	Limits.maxDescriptorSetSamplers);

	// Check for maxDescriptorSetUniformBuffers
	check(		LayoutTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER]
			+	LayoutTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC]
			<	Limits.maxDescriptorSetUniformBuffers);

	// Check for maxDescriptorSetUniformBuffersDynamic
	check(		LayoutTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC]
			<	Limits.maxDescriptorSetUniformBuffersDynamic);

	// Check for maxDescriptorSetStorageBuffers
	check(		LayoutTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER]
			+	LayoutTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC]
			<	Limits.maxDescriptorSetStorageBuffers);

	// Check for maxDescriptorSetStorageBuffersDynamic
	check(		LayoutTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC]
			<	Limits.maxDescriptorSetStorageBuffersDynamic);

	// Check for maxDescriptorSetSampledImages
	check(LayoutTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER]
			+	LayoutTypes[VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE]
			+	LayoutTypes[VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER]
			<	Limits.maxDescriptorSetSampledImages);

	// Check for maxDescriptorSetStorageImages
	check(		LayoutTypes[VK_DESCRIPTOR_TYPE_STORAGE_IMAGE]
			+	LayoutTypes[VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER]
			<	Limits.maxDescriptorSetStorageImages);

	LayoutHandles.Empty(Layouts.Num());

	for (FSetLayout& Layout : Layouts)
	{
		VkDescriptorSetLayoutCreateInfo DescriptorLayoutInfo;
		FMemory::Memzero(DescriptorLayoutInfo);
		DescriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		DescriptorLayoutInfo.pNext = nullptr;
		DescriptorLayoutInfo.bindingCount = Layout.LayoutBindings.Num();
		DescriptorLayoutInfo.pBindings = Layout.LayoutBindings.GetData();

		VkDescriptorSetLayout* LayoutHandle = new(LayoutHandles) VkDescriptorSetLayout;
		VERIFYVULKANRESULT(vkCreateDescriptorSetLayout(Device->GetInstanceHandle(), &DescriptorLayoutInfo, nullptr, LayoutHandle));
	}
}


FVulkanDescriptorSets::FVulkanDescriptorSets(FVulkanDevice* InDevice, const FVulkanBoundShaderState* InState, FVulkanDescriptorPool* InPool)
	: Device(InDevice)
	, Pool(InPool)
	, Layout(InState->GetDescriptorSetsLayout())
{
	Pool->TrackAddUsage(Layout);

	const TArray<VkDescriptorSetLayout>& LayoutHandles = Layout.GetHandles();

	VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
	FMemory::Memzero(DescriptorSetAllocateInfo);
	DescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	DescriptorSetAllocateInfo.descriptorPool = Pool->GetHandle();
	DescriptorSetAllocateInfo.descriptorSetCount = LayoutHandles.Num();
	DescriptorSetAllocateInfo.pSetLayouts = LayoutHandles.GetData();

	Sets.AddZeroed(LayoutHandles.Num());
	VERIFYVULKANRESULT(vkAllocateDescriptorSets(Device->GetInstanceHandle(), &DescriptorSetAllocateInfo, Sets.GetData()));
}

FVulkanDescriptorSets::~FVulkanDescriptorSets()
{
	Pool->TrackRemoveUsage(Layout);

	if (Sets.Num() > 0)
	{
		VERIFYVULKANRESULT(vkFreeDescriptorSets(Device->GetInstanceHandle(), Pool->GetHandle(), Sets.Num(), Sets.GetData()));
	}
}

void FVulkanBufferView::Create(FVulkanBuffer& Buffer, EPixelFormat Format, uint32 Offset, uint32 Size)
{
	VkBufferViewCreateInfo ViewInfo;
	FMemory::Memzero(ViewInfo);
	ViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
	ViewInfo.buffer = Buffer.GetBufferHandle();

	check(Format != PF_Unknown);
	const FPixelFormatInfo& FormatInfo = GPixelFormats[Format];
	check(FormatInfo.Supported);
	ViewInfo.format = (VkFormat)FormatInfo.PlatformFormat;

	// @todo vulkan: Because the buffer could be the ring buffer, maybe we should pass in a size here as well (for the sub part of the ring buffer)
	ViewInfo.offset = Offset;
	ViewInfo.range = Size;
	Flags = Buffer.GetFlags() & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
	check(Flags);

	VERIFYVULKANRESULT(vkCreateBufferView(GetParent()->GetInstanceHandle(), &ViewInfo, nullptr, &View));
}

void FVulkanBufferView::Create(FVulkanResourceMultiBuffer* Buffer, EPixelFormat Format, uint32 Offset, uint32 Size)
{
	VkBufferViewCreateInfo ViewInfo;
	FMemory::Memzero(ViewInfo);
	ViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
	ViewInfo.buffer = Buffer->GetHandle();

	check(Format != PF_Unknown);
	const FPixelFormatInfo& FormatInfo = GPixelFormats[Format];
	check(FormatInfo.Supported);
	ViewInfo.format = (VkFormat)FormatInfo.PlatformFormat;

	// @todo vulkan: Because the buffer could be the ring buffer, maybe we should pass in a size here as well (for the sub part of the ring buffer)
	ViewInfo.offset = Offset;
	ViewInfo.range = Size;
	Flags = Buffer->GetBufferUsageFlags() & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
	check(Flags);

	VERIFYVULKANRESULT(vkCreateBufferView(GetParent()->GetInstanceHandle(), &ViewInfo, nullptr, &View));
}

void FVulkanBufferView::Destroy()
{
	if (View != VK_NULL_HANDLE)
	{
		vkDestroyBufferView(GetParent()->GetInstanceHandle(), View, nullptr);
		View = VK_NULL_HANDLE;
	}
}

FVulkanRenderTargetLayout::FVulkanRenderTargetLayout(const FRHISetRenderTargetsInfo& RTInfo)
	: NumAttachments(0)
	, NumColorAttachments(0)
	, bHasDepthStencil(false)
	, bHasResolveAttachments(false)
	, Hash(0)
{
	FMemory::Memzero(ColorReferences);
	FMemory::Memzero(ResolveReferences);
	FMemory::Memzero(DepthStencilReference);
	FMemory::Memzero(Desc);
	FMemory::Memzero(Extent);

	bool bSetExtent = false;
		
	for (int32 Index = 0; Index < RTInfo.NumColorRenderTargets; ++Index)
	{
		const FRHIRenderTargetView& rtv = RTInfo.ColorRenderTarget[Index];
		if(rtv.Texture)
		{
		    FVulkanTextureBase* Texture = FVulkanTextureBase::Cast(rtv.Texture);
		    check(Texture);
    
		    if (!bSetExtent)
		    {
			    bSetExtent = true;
			    Extent.Extent3D.width = Texture->Surface.Width;
			    Extent.Extent3D.height = Texture->Surface.Height;
			    Extent.Extent3D.depth = 1;
		    }
    
		    uint32 NumSamples = rtv.Texture->GetNumSamples();
	    
		    VkAttachmentDescription& CurrDesc = Desc[NumAttachments];
		    
		    //@TODO: Check this, it should be a power-of-two. Might be a VulkanConvert helper function.
		    CurrDesc.samples = static_cast<VkSampleCountFlagBits>(NumSamples);
		    CurrDesc.format = (VkFormat)GPixelFormats[rtv.Texture->GetFormat()].PlatformFormat;
		    CurrDesc.loadOp = RenderTargetLoadActionToVulkan(rtv.LoadAction);
		    CurrDesc.storeOp = RenderTargetStoreActionToVulkan(rtv.StoreAction);
		    CurrDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
#if !VULKAN_USE_MSAA_RESOLVE_ATTACHMENTS
		    CurrDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
#endif
		    CurrDesc.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
		    CurrDesc.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    
		    ColorReferences[NumColorAttachments].attachment = NumAttachments;
		    ColorReferences[NumColorAttachments].layout = VK_IMAGE_LAYOUT_GENERAL;
		    NumAttachments++;
    
#if VULKAN_USE_MSAA_RESOLVE_ATTACHMENTS
			if (NumSamples == 1)
			{
			    CurrDesc.storeOp = RenderTargetStoreActionToVulkan(rtv.StoreAction);
    
			    ResolveReferences[NumColorAttachments].attachment = VK_ATTACHMENT_UNUSED;
			    ResolveReferences[NumColorAttachments].layout = VK_IMAGE_LAYOUT_UNDEFINED;
		    }
			else
		    {
			    // discard MSAA color target after resolve to 1x surface
			    CurrDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    
			    // Resolve attachment
			    VkAttachmentDescription& ResolveDesc = Desc[NumAttachments];
			    ResolveDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			    ResolveDesc.format = (VkFormat)GPixelFormats[rtv.Texture->GetFormat()].PlatformFormat;
			    ResolveDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			    ResolveDesc.storeOp = RenderTargetStoreActionToVulkan(rtv.StoreAction);
			    ResolveDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			    ResolveDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			    ResolveDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			    ResolveDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
			    ResolveReferences[NumColorAttachments].attachment = NumAttachments;
			    ResolveReferences[NumColorAttachments].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			    NumAttachments++;
			    bHasResolveAttachments = true;
		    }
#endif
	    
		    NumColorAttachments++;
		}
	}

	if (RTInfo.DepthStencilRenderTarget.Texture)
	{
		VkAttachmentDescription& CurrDesc = Desc[NumAttachments];
		FMemory::Memzero(CurrDesc);
		FVulkanTextureBase* Texture = FVulkanTextureBase::Cast(RTInfo.DepthStencilRenderTarget.Texture);
		check(Texture);

		//@TODO: Check this, it should be a power-of-two. Might be a VulkanConvert helper function.
		CurrDesc.samples = static_cast<VkSampleCountFlagBits>(RTInfo.DepthStencilRenderTarget.Texture->GetNumSamples());
		CurrDesc.format = (VkFormat)GPixelFormats[RTInfo.DepthStencilRenderTarget.Texture->GetFormat()].PlatformFormat;
		CurrDesc.loadOp = RenderTargetLoadActionToVulkan(RTInfo.DepthStencilRenderTarget.DepthLoadAction);
		CurrDesc.stencilLoadOp = RenderTargetLoadActionToVulkan(RTInfo.DepthStencilRenderTarget.StencilLoadAction);
		if (CurrDesc.samples == VK_SAMPLE_COUNT_1_BIT)
		{
			CurrDesc.storeOp = RenderTargetStoreActionToVulkan(RTInfo.DepthStencilRenderTarget.DepthStoreAction);
			CurrDesc.stencilStoreOp = RenderTargetStoreActionToVulkan(RTInfo.DepthStencilRenderTarget.GetStencilStoreAction());
		}
		else
		{
			// Never want to store MSAA depth/stencil
			CurrDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			CurrDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
		CurrDesc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		CurrDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		
		DepthStencilReference.attachment = NumAttachments;
		DepthStencilReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		bHasDepthStencil = true;
		NumAttachments++;

		if (!bSetExtent)
		{
			bSetExtent = true;
			Extent.Extent3D.width = Texture->Surface.Width;
			Extent.Extent3D.height = Texture->Surface.Height;
			Extent.Extent3D.depth = 1;
		}
	}

	check(bSetExtent);

	//@TODO: remove loads on device
#if 0
	//@HACK: don't used the load/store info in the hash (avoids a store/resolve between the tonemapping and slate passes)
	FRHISetRenderTargetsInfo HashedRTInfo;
	FMemory::Memcpy(HashedRTInfo, RTInfo);
	for (int32 Index = 0; Index < HashedRTInfo.NumColorRenderTargets; ++Index)
	{
		FRHIRenderTargetView* View = &HashedRTInfo.ColorRenderTarget[Index];
		View->LoadAction = ERenderTargetLoadAction::ENoAction;
		View->StoreAction = ERenderTargetStoreAction::ENoAction;
	}

	if (HashedRTInfo.DepthStencilRenderTarget.Texture)
	{
		FRHIDepthRenderTargetView* View = &HashedRTInfo.DepthStencilRenderTarget;
		View->DepthLoadAction = ERenderTargetLoadAction::ENoAction;
		View->DepthStoreAction = ERenderTargetStoreAction::ENoAction;
		View->StencilLoadAction = ERenderTargetLoadAction::ENoAction;
		View->StencilStoreAction = ERenderTargetStoreAction::ENoAction;
		View->DepthStencilAccess = FExclusiveDepthStencil();
		// doubt we care about the stencil store action anyway
	}
	HashedRTInfo.bClearDepth = false;
	HashedRTInfo.bClearStencil = false;
	HashedRTInfo.bClearColor = false;

	Hash = HashedRTInfo.CalculateHash();
#else
	Hash = RTInfo.CalculateHash();
#endif
}

FVulkanRenderPass::FVulkanRenderPass(FVulkanDevice& InDevice, const FVulkanRenderTargetLayout& RTLayout) :
	Layout(RTLayout),
	RenderPass(VK_NULL_HANDLE),
	Device(InDevice)
{
	INC_DWORD_STAT(STAT_VulkanNumRenderPasses);

	VkSubpassDescription Subpass;
	FMemory::Memzero(Subpass);
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.flags = 0;
	Subpass.inputAttachmentCount = 0;
	Subpass.pInputAttachments = nullptr;
	Subpass.colorAttachmentCount = RTLayout.GetNumColorAttachments();
	Subpass.pColorAttachments = RTLayout.GetColorAttachmentReferences();
	Subpass.pResolveAttachments = RTLayout.GetResolveAttachmentReferences();
	Subpass.pDepthStencilAttachment = RTLayout.GetDepthStencilAttachmentReference();
	Subpass.preserveAttachmentCount = 0;
	Subpass.pPreserveAttachments = nullptr;

	VkRenderPassCreateInfo RenderPassInfo;
	FMemory::Memzero(RenderPassInfo);
	RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	RenderPassInfo.pNext = nullptr;
	RenderPassInfo.attachmentCount = RTLayout.GetNumAttachments();
	RenderPassInfo.pAttachments = RTLayout.GetAttachmentDescriptions();
	RenderPassInfo.subpassCount = 1;
	RenderPassInfo.pSubpasses = &Subpass;
	RenderPassInfo.dependencyCount = 0;
	RenderPassInfo.pDependencies = nullptr;

	VERIFYVULKANRESULT_EXPANDED(vkCreateRenderPass(Device.GetInstanceHandle(), &RenderPassInfo, nullptr, &RenderPass));
}

FVulkanRenderPass::~FVulkanRenderPass()
{
	DEC_DWORD_STAT(STAT_VulkanNumRenderPasses);

	vkDestroyRenderPass(Device.GetInstanceHandle(), RenderPass, nullptr);
	RenderPass = VK_NULL_HANDLE;
}


void VulkanSetImageLayout(
	VkCommandBuffer CmdBuffer,
	VkImage Image,
	VkImageLayout OldLayout,
	VkImageLayout NewLayout,
	const VkImageSubresourceRange& SubresourceRange)
{
	check(CmdBuffer != VK_NULL_HANDLE);

	VkImageMemoryBarrier ImageBarrier;
	FMemory::Memzero(ImageBarrier);
	ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	ImageBarrier.oldLayout = OldLayout;
	ImageBarrier.newLayout = NewLayout;
	ImageBarrier.image = Image;
	ImageBarrier.subresourceRange = SubresourceRange;
	ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	switch (NewLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		ImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		if (OldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			ImageBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		}
		ImageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		ImageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			ImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		ImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		if (OldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			ImageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}
		ImageBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_GENERAL:
		if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			ImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		break;
	default:
		check(0);
		break;
	}

	VkImageMemoryBarrier BarrierList[] ={ImageBarrier};

	VkPipelineStageFlags SourceStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags DestStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	if (OldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		SourceStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		DestStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (NewLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		SourceStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		DestStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}

	vkCmdPipelineBarrier(CmdBuffer, SourceStages, DestStages, 0, 0, nullptr, 0, nullptr, 1, BarrierList);
}

void VulkanResolveImage(VkCommandBuffer Cmd, FTextureRHIParamRef SourceTextureRHI, FTextureRHIParamRef DestTextureRHI)
{
	FVulkanTextureBase* Src = FVulkanTextureBase::Cast(SourceTextureRHI);
	FVulkanTextureBase* Dst = FVulkanTextureBase::Cast(DestTextureRHI);

	check(Src->Surface.GetAspectMask() == Dst->Surface.GetAspectMask());
	const VkImageAspectFlags AspectMask = Src->Surface.GetAspectMask();

	VkImageResolve ResolveDesc;
	FMemory::Memzero(ResolveDesc);
	ResolveDesc.srcSubresource.aspectMask = AspectMask;
	ResolveDesc.srcSubresource.baseArrayLayer = 0;
	ResolveDesc.srcSubresource.mipLevel = 0;
	ResolveDesc.srcSubresource.layerCount = 1;
    ResolveDesc.srcOffset.x = 0;
    ResolveDesc.srcOffset.y = 0;
    ResolveDesc.srcOffset.z = 0;
	ResolveDesc.dstSubresource.aspectMask = AspectMask;
	ResolveDesc.dstSubresource.baseArrayLayer = 0;
	ResolveDesc.dstSubresource.mipLevel = 0;
	ResolveDesc.dstSubresource.layerCount = 1;
	ResolveDesc.dstOffset.x = 0;
	ResolveDesc.dstOffset.y = 0;
	ResolveDesc.dstOffset.z = 0;
    ResolveDesc.extent.width = Src->Surface.Width;
    ResolveDesc.extent.height = Src->Surface.Height;
    ResolveDesc.extent.depth = 1;

	vkCmdResolveImage(Cmd,
		Src->Surface.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		Dst->Surface.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &ResolveDesc);
}

FVulkanRingBuffer::FVulkanRingBuffer(FVulkanDevice* InDevice, uint64 TotalSize, VkFlags Usage)
	: VulkanRHI::FDeviceChild(InDevice)
	, BufferOffset(0)
	, BufferSize(TotalSize)
	, MinAlignment(0)
{
	FRHIResourceCreateInfo CreateInfo;
	BufferSuballocation = InDevice->GetResourceHeapManager().AllocateBuffer(TotalSize, Usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, __FILE__, __LINE__);
	MinAlignment = BufferSuballocation->GetBufferAllocation()->GetAlignment();
}

FVulkanRingBuffer::~FVulkanRingBuffer()
{
	delete BufferSuballocation;
}

uint64 FVulkanRingBuffer::AllocateMemory(uint64 Size, uint32 Alignment)
{
	Alignment = FMath::Max(Alignment, MinAlignment);
	uint64 AllocOffset = Align<uint64>(BufferOffset, Alignment);

	// wrap around if needed
	if (AllocOffset + Size >= BufferSize)
	{
		UE_LOG(LogVulkanRHI, Display, TEXT("Wrapped around the ring buffer. Frame = %lld, size = %lld"), Device->FrameCounter, BufferSize);
		AllocOffset = 0;
	}

	// point to location after this guy
	BufferOffset = AllocOffset + Size;

	return AllocOffset;
}

#if VULKAN_ENABLE_PIPELINE_CACHE
void FVulkanDynamicRHI::SavePipelineCache()
{
	FString CacheFile = FPaths::GameSavedDir() / TEXT("VulkanPSO.cache");

	FVulkanDynamicRHI* RHI = (FVulkanDynamicRHI*)GDynamicRHI;
	RHI->Device->PipelineStateCache->Save(CacheFile);
}

void FVulkanDynamicRHI::RebuildPipelineCache()
{
	FVulkanDynamicRHI* RHI = (FVulkanDynamicRHI*)GDynamicRHI;
	RHI->Device->PipelineStateCache->RebuildCache();
}
#endif

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
void FVulkanDynamicRHI::DumpMemory()
{
	FVulkanDynamicRHI* RHI = (FVulkanDynamicRHI*)GDynamicRHI;
	RHI->Device->GetMemoryManager().DumpMemory();
	RHI->Device->GetResourceHeapManager().DumpMemory();
	RHI->Device->GetStagingManager().DumpMemory();
}
#endif
