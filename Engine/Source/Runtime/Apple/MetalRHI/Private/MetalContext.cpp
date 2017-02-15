// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MetalRHIPrivate.h"
#include "Misc/App.h"
#if PLATFORM_IOS
#include "IOSAppDelegate.h"
#include "IOS/IOSPlatformFramePacer.h"
#endif

#include "MetalContext.h"
#include "MetalProfiler.h"
#include "MetalCommandBuffer.h"

#if METAL_STATISTICS
#include "MetalStatistics.h"
#include "ModuleManager.h"
#endif

int32 GMetalSupportsIntermediateBackBuffer = PLATFORM_MAC ? 1 : 0;
static FAutoConsoleVariableRef CVarMetalSupportsIntermediateBackBuffer(
	TEXT("rhi.Metal.SupportsIntermediateBackBuffer"),
	GMetalSupportsIntermediateBackBuffer,
	TEXT("When enabled (> 0) allocate an intermediate texture to use as the back-buffer & blit from there into the actual device back-buffer, thereby allowing screenshots & video capture that would otherwise be impossible as the texture required has already been released back to the OS as required by Metal's API. (Off by default (0) on iOS/tvOS but enabled (1) on Mac)"));

#if PLATFORM_MAC
static int32 GMetalCommandQueueSize = 5120; // This number is large due to texture streaming - currently each texture is its own command-buffer.
// The whole MetalRHI needs to be changed to use MTLHeaps/MTLFences & reworked so that operations with the same synchronisation requirements are collapsed into a single blit command-encoder/buffer.
#else
static int32 GMetalCommandQueueSize = 0;
#endif
static FAutoConsoleVariableRef CVarMetalCommandQueueSize(
	TEXT("rhi.Metal.CommandQueueSize"),
	GMetalCommandQueueSize,
	TEXT("The maximum number of command-buffers that can be allocated from each command-queue. (Default: 5120 Mac, 64 iOS/tvOS)"));

#if METAL_DEBUG_OPTIONS
int32 GMetalBufferScribble = 0; // Deliberately not static, see InitFrame_UniformBufferPoolCleanup
static FAutoConsoleVariableRef CVarMetalBufferScribble(
	TEXT("rhi.Metal.BufferScribble"),
	GMetalBufferScribble,
	TEXT("Debug option: when enabled will scribble over the buffer contents with 0xCD when releasing Shared & Managed buffer objects. (Default: 0, Off)"));

int32 GMetalBufferZeroFill = 0; // Deliberately not static
static FAutoConsoleVariableRef CVarMetalBufferZeroFill(
	TEXT("rhi.Metal.BufferZeroFill"),
	GMetalBufferZeroFill,
	TEXT("Debug option: when enabled will fill the buffer contents with 0 when allocating Shared & Managed buffer objects, or regions thereof. (Default: 0, Off)"));

static int32 GMetalResourcePurgeOnDelete = 0;
static FAutoConsoleVariableRef CVarMetalResourcePurgeOnDelete(
	TEXT("rhi.Metal.ResourcePurgeOnDelete"),
	GMetalResourcePurgeOnDelete,
	TEXT("Debug option: when enabled all MTLResource objects will have their backing stores purged on release - any subsequent access will be invalid and cause a command-buffer failure. Useful for making intermittent resource lifetime errors more common and easier to track. (Default: 0, Off)"));

static int32 GMetalResourceDeferDeleteNumFrames = 0;
static FAutoConsoleVariableRef CVarMetalResourceDeferDeleteNumFrames(
	TEXT("rhi.Metal.ResourceDeferDeleteNumFrames"),
	GMetalResourcePurgeOnDelete,
	TEXT("Debug option: set to the number of frames that must have passed before resource free-lists are processed and resources disposed of. (Default: 0, Off)"));
#endif

int32 GMetalRuntimeDebugLevel = 0;
static FAutoConsoleVariableRef CVarMetalRuntimeDebugLevel(
	TEXT("rhi.Metal.RuntimeDebugLevel"),
	GMetalRuntimeDebugLevel,
	TEXT("The level of debug validation performed by MetalRHI in addition to the underlying Metal API & validation layer.\n")
	TEXT("Each subsequent level adds more tests and reporting in addition to the previous level.\n")
	TEXT("*IGNORED IN SHIPPING AND TEST BUILDS*. (Default: 0)\n")
	TEXT("\t0: Off,\n")
	TEXT("\t1: Enable validation checks for encoder resets,\n")
	TEXT("\t2: Record the debug-groups issued into a command-buffer and report them on failure,\n")
	TEXT("\t3: Record the draw, blit & dispatch commands issued into a command-buffer and report them on failure,\n")
	TEXT("\t4: Set buffer bindings to nil prior to binding raw bytes so that the GPU tool doesn't show incorrect data,\n")
	TEXT("\t5: Allow rhi.Metal.CommandBufferCommitThreshold to break command-encoders (except when MSAA is enabled),\n")
	TEXT("\t6: Wait for each command-buffer to complete immediately after submission."));

#if SHOULD_TRACK_OBJECTS
TMap<id, int32> ClassCounts;

FCriticalSection* GetClassCountsMutex()
{
	static FCriticalSection Mutex;
	return &Mutex;
}

void TrackMetalObject(NSObject* Obj)
{
	check(Obj);
	
	if (GIsRHIInitialized)
	{
		FScopeLock Lock(GetClassCountsMutex());
		ClassCounts.FindOrAdd([Obj class])++;
	}
}

void UntrackMetalObject(NSObject* Obj)
{
	check(Obj);
	
	if (GIsRHIInitialized)
	{
		FScopeLock Lock(GetClassCountsMutex());
		ClassCounts.FindOrAdd([Obj class])--;
	}
}

#endif

static void* LocalCreateAutoreleasePool()
{
	return [[NSAutoreleasePool alloc] init];
}

static void LocalReleaseAutoreleasePool(void *Pool)
{
	if (Pool)
	{
		[(NSAutoreleasePool*)Pool release];
	}
}

#if PLATFORM_MAC
#if __MAC_OS_X_VERSION_MAX_ALLOWED < 101100
MTL_EXTERN NSArray* MTLCopyAllDevices(void);
#else
MTL_EXTERN NSArray <id<MTLDevice>>* MTLCopyAllDevices(void);
#endif

static id<MTLDevice> GetMTLDevice(uint32& DeviceIndex)
{
	SCOPED_AUTORELEASE_POOL;
	
	DeviceIndex = 0;
	
#if METAL_STATISTICS
	IMetalStatisticsModule* StatsModule = FModuleManager::Get().LoadModulePtr<IMetalStatisticsModule>(TEXT("MetalStatistics"));
#endif
	
	NSArray* DeviceList = MTLCopyAllDevices();
	[DeviceList autorelease];
	
	const int32 NumDevices = [DeviceList count];
	
	TArray<FMacPlatformMisc::FGPUDescriptor> const& GPUs = FPlatformMisc::GetGPUDescriptors();
	check(GPUs.Num() > 0);
	
	int32 ExplicitRendererId = FPlatformMisc::GetExplicitRendererIndex();
	if(ExplicitRendererId < 0 && GPUs.Num() > 1 && FMacPlatformMisc::MacOSXVersionCompare(10, 11, 5) == 0)
	{
		int32 OverrideRendererId = -1;
		bool bForceExplicitRendererId = false;
		for(uint32 i = 0; i < GPUs.Num(); i++)
		{
			FMacPlatformMisc::FGPUDescriptor const& GPU = GPUs[i];
			if((GPU.GPUVendorId == 0x10DE))
			{
				OverrideRendererId = i;
				bForceExplicitRendererId = (GPU.GPUMetalBundle && ![GPU.GPUMetalBundle isEqualToString:@"GeForceMTLDriverWeb"]);
			}
			else if(!GPU.GPUHeadless && GPU.GPUVendorId != 0x8086)
			{
				OverrideRendererId = i;
			}
		}
		if (bForceExplicitRendererId)
		{
			ExplicitRendererId = OverrideRendererId;
		}
	}
	
	id<MTLDevice> SelectedDevice = nil;
	if (ExplicitRendererId >= 0 && ExplicitRendererId < GPUs.Num())
	{
		FMacPlatformMisc::FGPUDescriptor const& GPU = GPUs[ExplicitRendererId];
		TArray<FString> NameComponents;
		FString(GPU.GPUName).Trim().ParseIntoArray(NameComponents, TEXT(" "));	
		for (id<MTLDevice> Device in DeviceList)
		{
			if(([Device.name rangeOfString:@"Nvidia" options:NSCaseInsensitiveSearch].location != NSNotFound && GPU.GPUVendorId == 0x10DE)
			   || ([Device.name rangeOfString:@"AMD" options:NSCaseInsensitiveSearch].location != NSNotFound && GPU.GPUVendorId == 0x1002)
			   || ([Device.name rangeOfString:@"Intel" options:NSCaseInsensitiveSearch].location != NSNotFound && GPU.GPUVendorId == 0x8086))
			{
				bool bMatchesName = (NameComponents.Num() > 0);
				for (FString& Component : NameComponents)
				{
					bMatchesName &= FString(Device.name).Contains(Component);
				}
				if((Device.headless == GPU.GPUHeadless || GPU.GPUVendorId != 0x1002) && bMatchesName)
                {
					DeviceIndex = ExplicitRendererId;
					SelectedDevice = Device;
					break;
				}
			}
		}
		if(!SelectedDevice)
		{
			UE_LOG(LogMetal, Warning,  TEXT("Couldn't find Metal device to match GPU descriptor (%s) from IORegistry - using default device."), *FString(GPU.GPUName));
		}
	}
	if (SelectedDevice == nil)
	{
		TArray<FString> NameComponents;
		SelectedDevice = MTLCreateSystemDefaultDevice();
		bool bFoundDefault = false;
		for (uint32 i = 0; i < GPUs.Num(); i++)
		{
			FMacPlatformMisc::FGPUDescriptor const& GPU = GPUs[i];
			if(([SelectedDevice.name rangeOfString:@"Nvidia" options:NSCaseInsensitiveSearch].location != NSNotFound && GPU.GPUVendorId == 0x10DE)
			   || ([SelectedDevice.name rangeOfString:@"AMD" options:NSCaseInsensitiveSearch].location != NSNotFound && GPU.GPUVendorId == 0x1002)
			   || ([SelectedDevice.name rangeOfString:@"Intel" options:NSCaseInsensitiveSearch].location != NSNotFound && GPU.GPUVendorId == 0x8086))
			{
				NameComponents.Empty();
				bool bMatchesName = FString(GPU.GPUName).Trim().ParseIntoArray(NameComponents, TEXT(" ")) > 0;
				for (FString& Component : NameComponents)
				{
					bMatchesName &= FString(SelectedDevice.name).Contains(Component);
				}
				if((SelectedDevice.headless == GPU.GPUHeadless || GPU.GPUVendorId != 0x1002) && bMatchesName)
                {
					DeviceIndex = i;
					bFoundDefault = true;
					break;
				}
			}
		}
		if(!bFoundDefault)
		{
			UE_LOG(LogMetal, Warning,  TEXT("Couldn't find Metal device %s in GPU descriptors from IORegistry - capability reporting may be wrong."), *FString(SelectedDevice.name));
		}
	}
	check(SelectedDevice);
	return SelectedDevice;
}

static MTLPrimitiveTopologyClass TranslatePrimitiveTopology(uint32 PrimitiveType)
{
	switch (PrimitiveType)
	{
		case PT_TriangleList:
		case PT_TriangleStrip:
			return MTLPrimitiveTopologyClassTriangle;
		case PT_LineList:
			return MTLPrimitiveTopologyClassLine;
		case PT_PointList:
			return MTLPrimitiveTopologyClassPoint;
		case PT_1_ControlPointPatchList:
		case PT_2_ControlPointPatchList:
		case PT_3_ControlPointPatchList:
		case PT_4_ControlPointPatchList:
		case PT_5_ControlPointPatchList:
		case PT_6_ControlPointPatchList:
		case PT_7_ControlPointPatchList:
		case PT_8_ControlPointPatchList:
		case PT_9_ControlPointPatchList:
		case PT_10_ControlPointPatchList:
		case PT_11_ControlPointPatchList:
		case PT_12_ControlPointPatchList:
		case PT_13_ControlPointPatchList:
		case PT_14_ControlPointPatchList:
		case PT_15_ControlPointPatchList:
		case PT_16_ControlPointPatchList:
		case PT_17_ControlPointPatchList:
		case PT_18_ControlPointPatchList:
		case PT_19_ControlPointPatchList:
		case PT_20_ControlPointPatchList:
		case PT_21_ControlPointPatchList:
		case PT_22_ControlPointPatchList:
		case PT_23_ControlPointPatchList:
		case PT_24_ControlPointPatchList:
		case PT_25_ControlPointPatchList:
		case PT_26_ControlPointPatchList:
		case PT_27_ControlPointPatchList:
		case PT_28_ControlPointPatchList:
		case PT_29_ControlPointPatchList:
		case PT_30_ControlPointPatchList:
		case PT_31_ControlPointPatchList:
		case PT_32_ControlPointPatchList:
		{
			static uint32 Logged = 0;
			if (!Logged)
			{
				Logged = 1;
				UE_LOG(LogMetal, Warning, TEXT("Untested primitive topology %d"), (int32)PrimitiveType);
			}
			return MTLPrimitiveTopologyClassTriangle;
		}
		default:
			UE_LOG(LogMetal, Fatal, TEXT("Unsupported primitive topology %d"), (int32)PrimitiveType);
			return MTLPrimitiveTopologyClassTriangle;
	}
}
#endif

FMetalDeviceContext* FMetalDeviceContext::CreateDeviceContext()
{
	uint32 DeviceIndex = 0;
#if PLATFORM_IOS
	id<MTLDevice> Device = [IOSAppDelegate GetDelegate].IOSView->MetalDevice;
#else // @todo zebra
	id<MTLDevice> Device = GetMTLDevice(DeviceIndex);
#endif
	FMetalCommandQueue* Queue = new FMetalCommandQueue(Device, GMetalCommandQueueSize);
	check(Queue);
	
	return new FMetalDeviceContext(Device, DeviceIndex, Queue);
}

FMetalDeviceContext::FMetalDeviceContext(id<MTLDevice> MetalDevice, uint32 InDeviceIndex, FMetalCommandQueue* Queue)
: FMetalContext(*Queue, true)
, Device(MetalDevice)
, DeviceIndex(InDeviceIndex)
, FreeBuffers([NSMutableSet new])
, SceneFrameCounter(0)
, FrameCounter(0)
, ActiveContexts(1)
, AllocatedContexts(1)
{
#if METAL_DEBUG_OPTIONS
	CommandQueue.SetRuntimeDebuggingLevel(GMetalRuntimeDebugLevel);
#endif
	
	// Hook into the ios framepacer, if it's enabled for this platform.
	FrameReadyEvent = NULL;
#if PLATFORM_IOS
	if( FIOSPlatformRHIFramePacer::IsEnabled() )
	{
		FrameReadyEvent = FPlatformProcess::GetSynchEventFromPool();
		FIOSPlatformRHIFramePacer::InitWithEvent( FrameReadyEvent );
	}
#endif
	
	InitFrame(true);
}

FMetalDeviceContext::~FMetalDeviceContext()
{
	SubmitCommandsHint(EMetalSubmitFlagsWaitOnCommandBuffer);
	delete &(GetCommandQueue());
}



void FMetalDeviceContext::BeginFrame()
{
	FPlatformTLS::SetTlsValue(CurrentContextTLSSlot, this);
}

#if METAL_DEBUG_OPTIONS
void ScribbleBuffer(id<MTLBuffer> Buffer)
{
#if PLATFORM_MAC
	if (Buffer.storageMode != MTLStorageModePrivate)
#endif
	{
		FMemory::Memset(Buffer.contents, 0xCD, Buffer.length);
#if PLATFORM_MAC
		if (Buffer.storageMode == MTLStorageModeManaged)
		{
			[Buffer didModifyRange:NSMakeRange(0, Buffer.length)];
		}
#endif
	}
}
#endif

void FMetalDeviceContext::ClearFreeList()
{
	uint32 Index = 0;
	while(Index < DelayedFreeLists.Num())
	{
		FMetalDelayedFreeList* Pair = DelayedFreeLists[Index];
		if(METAL_DEBUG_OPTION(Pair->DeferCount-- <= 0 &&) dispatch_semaphore_wait(Pair->Signal, DISPATCH_TIME_NOW) == 0)
		{
			dispatch_release(Pair->Signal);
			for( id Entry : Pair->FreeList )
			{
#if METAL_DEBUG_OPTIONS
				if (GMetalBufferScribble && [Entry conformsToProtocol:@protocol(MTLBuffer)])
				{
					ScribbleBuffer((id<MTLBuffer>)Entry);
				}
				if (GMetalResourcePurgeOnDelete && [Entry conformsToProtocol:@protocol(MTLResource)])
				{
					[((id<MTLResource>)Entry) setPurgeableState:MTLPurgeableStateEmpty];
				}
#endif
				
				[Entry release];
			}
			{
				FScopeLock Lock(&PoolMutex);
				for ( id<MTLBuffer> Buffer in Pair->FreeBuffers )
				{
					DEC_MEMORY_STAT_BY(STAT_MetalUsedPooledBufferMem, Buffer.length);
					INC_DWORD_STAT(STAT_MetalBufferFreed);
					INC_DWORD_STAT_BY(STAT_MetalBufferMemFreed, Buffer.length);
					INC_MEMORY_STAT_BY(STAT_MetalFreePooledBufferMem, Buffer.length);
					
#if METAL_DEBUG_OPTIONS
					if (GMetalBufferScribble)
					{
						ScribbleBuffer(Buffer);
					}
					if (GMetalResourcePurgeOnDelete)
					{
						[Buffer setPurgeableState:MTLPurgeableStateEmpty];
					}
#endif
				
					BufferPool.ReleasePooledResource(Buffer);
				}
				[Pair->FreeBuffers release];
			}
			delete Pair;
			DelayedFreeLists.RemoveAt(Index, 1, false);
		}
		else
		{
			Index++;
		}
	}
}

void FMetalDeviceContext::EndFrame()
{
	ClearFreeList();
	
	if(FrameCounter != GFrameNumberRenderThread)
	{
		FrameCounter = GFrameNumberRenderThread;
		
		FScopeLock Lock(&PoolMutex);
		BufferPool.DrainPool(false);
	}
	CommandQueue.InsertDebugCaptureBoundary();
}

void FMetalDeviceContext::BeginScene()
{
	FPlatformTLS::SetTlsValue(CurrentContextTLSSlot, this);
	
	// Increment the frame counter. INDEX_NONE is a special value meaning "uninitialized", so if
	// we hit it just wrap around to zero.
	SceneFrameCounter++;
	if (SceneFrameCounter == INDEX_NONE)
	{
		SceneFrameCounter++;
	}
}

void FMetalDeviceContext::EndScene()
{
}

void FMetalDeviceContext::BeginDrawingViewport(FMetalViewport* Viewport)
{
	FPlatformTLS::SetTlsValue(CurrentContextTLSSlot, this);
}

void FMetalDeviceContext::EndDrawingViewport(FMetalViewport* Viewport, bool bPresent)
{
	// kick the whole buffer
	FMetalDelayedFreeList* NewList = new FMetalDelayedFreeList;
	NewList->Signal = dispatch_semaphore_create(0);
	METAL_DEBUG_OPTION(NewList->DeferCount = GMetalResourceDeferDeleteNumFrames);
	if(GUseRHIThread)
	{
		FreeListMutex.Lock();
	}
	NewList->FreeList = FreeList;
	
	PoolMutex.Lock();
	NewList->FreeBuffers = FreeBuffers;
	FreeBuffers = [NSMutableSet new];
	PoolMutex.Unlock();
	
#if STATS
	for (id Obj : FreeList)
	{
		check(Obj);
		
		if([[Obj class] conformsToProtocol:@protocol(MTLBuffer)])
		{
			DEC_DWORD_STAT(STAT_MetalBufferCount);
		}
		else if([[Obj class] conformsToProtocol:@protocol(MTLTexture)])
		{
			DEC_DWORD_STAT(STAT_MetalTextureCount);
		}
		else if([[Obj class] conformsToProtocol:@protocol(MTLSamplerState)])
		{
			DEC_DWORD_STAT(STAT_MetalSamplerStateCount);
		}
		else if([[Obj class] conformsToProtocol:@protocol(MTLDepthStencilState)])
		{
			DEC_DWORD_STAT(STAT_MetalDepthStencilStateCount);
		}
		else if([[Obj class] conformsToProtocol:@protocol(MTLRenderPipelineState)])
		{
			DEC_DWORD_STAT(STAT_MetalRenderPipelineStateCount);
		}
		else if([Obj isKindOfClass:[MTLRenderPipelineColorAttachmentDescriptor class]])
		{
			DEC_DWORD_STAT(STAT_MetalRenderPipelineColorAttachmentDescriptor);
		}
		else if([Obj isKindOfClass:[MTLRenderPassDescriptor class]])
		{
			DEC_DWORD_STAT(STAT_MetalRenderPassDescriptorCount);
		}
		else if([Obj isKindOfClass:[MTLRenderPassColorAttachmentDescriptor class]])
		{
			DEC_DWORD_STAT(STAT_MetalRenderPassColorAttachmentDescriptorCount);
		}
		else if([Obj isKindOfClass:[MTLRenderPassDepthAttachmentDescriptor class]])
		{
			DEC_DWORD_STAT(STAT_MetalRenderPassDepthAttachmentDescriptorCount);
		}
		else if([Obj isKindOfClass:[MTLRenderPassStencilAttachmentDescriptor class]])
		{
			DEC_DWORD_STAT(STAT_MetalRenderPassStencilAttachmentDescriptorCount);
		}
		else if([Obj isKindOfClass:[MTLVertexDescriptor class]])
		{
			DEC_DWORD_STAT(STAT_MetalVertexDescriptorCount);
		}
		
#if SHOULD_TRACK_OBJECTS
		UntrackMetalObject(Obj);
#endif
	}
#endif
	FreeList.Empty(FreeList.Num());
	if(GUseRHIThread)
	{
		FreeListMutex.Unlock();
	}
	
	dispatch_semaphore_t Signal = NewList->Signal;
	dispatch_retain(Signal);
	
	id<MTLCommandBuffer> CurrentCommandBuffer = GetCurrentCommandBuffer();
	check(CurrentCommandBuffer);
	
	[CurrentCommandBuffer addCompletedHandler : ^ (id <MTLCommandBuffer> Buffer)
	{
		dispatch_semaphore_signal(CommandBufferSemaphore);
		dispatch_semaphore_signal(Signal);
		dispatch_release(Signal);
	}];
    DelayedFreeLists.Add(NewList);
	
	// enqueue a present if desired
	if (bPresent)
	{
		id<CAMetalDrawable> Drawable = (id<CAMetalDrawable>)Viewport->GetDrawable(EMetalViewportAccessRHI);
		if (Drawable && Drawable.texture)
		{
			[Drawable retain];
			
			if (GMetalSupportsIntermediateBackBuffer)
			{
				id<MTLTexture> Src = [Viewport->GetBackBuffer(EMetalViewportAccessRHI)->Surface.Texture retain];
				id<MTLTexture> Dst = [Drawable.texture retain];
				
				NSUInteger Width = FMath::Min(Src.width, Dst.width);
				NSUInteger Height = FMath::Min(Src.height, Dst.height);
				
				RenderPass.PresentTexture(Src, 0, 0, MTLOriginMake(0, 0, 0), MTLSizeMake(Width, Height, 1), Dst, 0, 0, MTLOriginMake(0, 0, 0));

				[CurrentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer>) {
					[Src release];
					[Dst release];
				}];
			}
			
			[CurrentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer>) {
				[Drawable release];
			}];
			[CurrentCommandBuffer addScheduledHandler:^(id<MTLCommandBuffer>) {
				[Drawable present];
			}];
		}
	}
	
	// We may be limiting our framerate to the display link
	if( FrameReadyEvent != nullptr )
	{
		FrameReadyEvent->Wait();
	}
	
	// Latched update of whether to use runtime debugging features
	uint32 SubmitFlags = EMetalSubmitFlagsNone;
#if METAL_DEBUG_OPTIONS
	if (GMetalRuntimeDebugLevel != CommandQueue.GetRuntimeDebuggingLevel())
	{
		CommandQueue.SetRuntimeDebuggingLevel(GMetalRuntimeDebugLevel);
		
		// After change the debug features level wait on commit
		SubmitFlags |= EMetalSubmitFlagsWaitOnCommandBuffer;
	}
#endif
	RenderPass.End((EMetalSubmitFlags)SubmitFlags);
	
#if SHOULD_TRACK_OBJECTS
	// print out outstanding objects
	if ((GFrameCounter % 500) == 10)
	{
		for (auto It = ClassCounts.CreateIterator(); It; ++It)
		{
			UE_LOG(LogMetal, Display, TEXT("%s has %d outstanding allocations"), *FString([It.Key() description]), It.Value());
		}
	}
#endif
	
	// drain the pool
	DrainAutoreleasePool();
	
	InitFrame(true);
	
	extern void InitFrame_UniformBufferPoolCleanup();
	InitFrame_UniformBufferPoolCleanup();
	
	Viewport->ReleaseDrawable();
}

void FMetalDeviceContext::ReleaseObject(id Object)
{
	if (GIsRHIInitialized) // @todo zebra: there seems to be some race condition at exit when the framerate is very low
	{
		check(Object);
		if(GUseRHIThread)
		{
			FreeListMutex.Lock();
		}
		if(!FreeList.Contains(Object))
        {
            FreeList.Add(Object);
        }
        else
        {
            [Object release];
        }
		if(GUseRHIThread)
		{
			FreeListMutex.Unlock();
		}
	}
}

FMetalPooledBuffer FMetalDeviceContext::CreatePooledBuffer(FMetalPooledBufferArgs const& Args)
{
	FMetalPooledBuffer Buffer;
	if(GIsRHIInitialized)
	{
		FScopeLock Lock(&PoolMutex);
		Buffer = BufferPool.CreatePooledResource(Args);
#if METAL_DEBUG_OPTIONS
		if (GMetalResourcePurgeOnDelete)
		{
			[Buffer.Buffer setPurgeableState:MTLPurgeableStateNonVolatile];
		}
#endif
	}
	else
	{
		FMetalBufferPoolPolicyData Policy;
		uint32 BufferSize = Policy.GetPoolBucketSize(Policy.GetPoolBucketIndex(Args));
		Buffer.Buffer = [Device newBufferWithLength:BufferSize options: BUFFER_CACHE_MODE
#if METAL_API_1_1
						 | (Args.Storage << MTLResourceStorageModeShift)
#endif
						 ];
		TRACK_OBJECT(STAT_MetalBufferCount, Buffer.Buffer);
		INC_DWORD_STAT(STAT_MetalPooledBufferCount);
		INC_MEMORY_STAT_BY(STAT_MetalPooledBufferMem, BufferSize);
		INC_MEMORY_STAT_BY(STAT_MetalFreePooledBufferMem, BufferSize);
	}
	INC_MEMORY_STAT_BY(STAT_MetalUsedPooledBufferMem, Buffer.Buffer.length);
	INC_DWORD_STAT(STAT_MetalBufferAlloctations);
	INC_DWORD_STAT_BY(STAT_MetalBufferMemAlloc, Buffer.Buffer.length);
	DEC_MEMORY_STAT_BY(STAT_MetalFreePooledBufferMem, Buffer.Buffer.length);
	
#if METAL_DEBUG_OPTIONS
	if (GMetalBufferZeroFill && Args.Storage != MTLStorageModePrivate)
	{
		FMemory::Memset([Buffer.Buffer contents], 0x0, Buffer.Buffer.length);
	}
#endif
	
	return Buffer;
}

void FMetalDeviceContext::ReleasePooledBuffer(FMetalPooledBuffer Buf)
{
	if(GIsRHIInitialized)
	{
		FScopeLock Lock(&FreeListMutex);
		check(Buf.Buffer);
		[FreeBuffers addObject:Buf.Buffer];
		//[Buf.Buffer setPurgeableState:MTLPurgeableStateVolatile];
	}
}


FMetalRHICommandContext* FMetalDeviceContext::AcquireContext()
{
	FMetalRHICommandContext* Context = ParallelContexts.Pop();
	if (!Context)
	{
		FMetalContext* MetalContext = new FMetalContext(GetCommandQueue(), false);
		check(MetalContext);
		
		FMetalRHICommandContext* CmdContext = static_cast<FMetalRHICommandContext*>(RHIGetDefaultContext());
		check(CmdContext);
		
		Context = new FMetalRHICommandContext(CmdContext->GetProfiler(), MetalContext);
		
		++AllocatedContexts;
	}
	check(Context);
	FPlatformAtomics::InterlockedIncrement(&ActiveContexts);
	return Context;
}

void FMetalDeviceContext::ReleaseContext(FMetalRHICommandContext* Context)
{
	check(!Context->GetInternalContext().GetCurrentCommandBuffer());
	
	ParallelContexts.Push(Context);
	FPlatformAtomics::InterlockedDecrement(&ActiveContexts);
	check(ActiveContexts >= 1);
}

uint32 FMetalDeviceContext::GetNumActiveContexts(void) const
{
	return ActiveContexts;
}

uint32 FMetalDeviceContext::GetDeviceIndex(void) const
{
	return DeviceIndex;
}

uint32 FMetalContext::AutoReleasePoolTLSSlot = FPlatformTLS::AllocTlsSlot();
uint32 FMetalContext::CurrentContextTLSSlot = FPlatformTLS::AllocTlsSlot();

FMetalContext::FMetalContext(FMetalCommandQueue& Queue, bool const bIsImmediate)
: Device(Queue.GetDevice())
, CommandQueue(Queue)
, CommandList(Queue, bIsImmediate)
, StateCache(bIsImmediate)
, RenderPass(CommandList, StateCache)
, QueryBuffer(new FMetalQueryBufferPool(this))
{
	// create a semaphore for multi-buffering the command buffer
	CommandBufferSemaphore = dispatch_semaphore_create(FParse::Param(FCommandLine::Get(),TEXT("gpulockstep")) ? 1 : 3);
}

FMetalContext::~FMetalContext()
{
	SubmitCommandsHint(EMetalSubmitFlagsWaitOnCommandBuffer);
}

FMetalContext* FMetalContext::GetCurrentContext()
{
	FMetalContext* Current = (FMetalContext*)FPlatformTLS::GetTlsValue(CurrentContextTLSSlot);
	check(Current);
	return Current;
}

id<MTLDevice> FMetalContext::GetDevice()
{
	return Device;
}

FMetalCommandQueue& FMetalContext::GetCommandQueue()
{
	return CommandQueue;
}

FMetalCommandList& FMetalContext::GetCommandList()
{
	return CommandList;
}

id<MTLCommandBuffer> FMetalContext::GetCurrentCommandBuffer()
{
	return RenderPass.GetCurrentCommandBuffer();
}

void FMetalContext::InsertCommandBufferFence(FMetalCommandBufferFence& Fence)
{
	check(GetCurrentCommandBuffer());
	
	TSharedPtr<MTLCommandBufferRef, ESPMode::ThreadSafe>* CmdBufRef = new TSharedPtr<MTLCommandBufferRef, ESPMode::ThreadSafe>(new MTLCommandBufferRef([GetCurrentCommandBuffer() retain]));
	
	[GetCurrentCommandBuffer() addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull CommandBuffer)
	{
		if(CommandBuffer.status == MTLCommandBufferStatusError)
		{
			FMetalCommandList::HandleMetalCommandBufferFailure(CommandBuffer);
		}
		delete CmdBufRef;
	}];
	
	Fence.CommandBufferRef = *CmdBufRef;
}

void FMetalContext::CreateAutoreleasePool()
{
	// Create an autorelease pool. Not on the game thread, though. It's not needed there, as the game thread provides its own pools.
	// Also, it'd cause problems as we sometimes call EndDrawingViewport on the game thread. That would drain the pool for Metal
	// context while some other pool is active, which is illegal.
	if (FPlatformTLS::GetTlsValue(AutoReleasePoolTLSSlot) == NULL && FPlatformTLS::GetCurrentThreadId() != GGameThreadId)
	{
		FPlatformTLS::SetTlsValue(AutoReleasePoolTLSSlot, LocalCreateAutoreleasePool());
	}
}

void FMetalContext::DrainAutoreleasePool()
{
	LocalReleaseAutoreleasePool(FPlatformTLS::GetTlsValue(AutoReleasePoolTLSSlot));
	FPlatformTLS::SetTlsValue(AutoReleasePoolTLSSlot, NULL);
}

void FMetalContext::MakeCurrent(FMetalContext* Context)
{
	FPlatformTLS::SetTlsValue(CurrentContextTLSSlot, Context);
}

void FMetalContext::InitFrame(bool const bImmediateContext)
{
	FPlatformTLS::SetTlsValue(CurrentContextTLSSlot, this);
	
	// Reset cached state in the encoder
	StateCache.Reset();
	
	// start an auto release pool (EndFrame will drain and remake)
	CreateAutoreleasePool();
	
	// Wait for the frame semaphore on the immediate context.
	if (bImmediateContext)
	{
		dispatch_semaphore_wait(CommandBufferSemaphore, DISPATCH_TIME_FOREVER);
	}
	
	// Begin the render pass frame.
	RenderPass.Begin(nil);
	
	// make sure first SetRenderTarget goes through
	StateCache.InvalidateRenderTargets();
}

void FMetalContext::FinishFrame()
{
	// Issue any outstanding commands.
	SubmitCommandsHint(EMetalSubmitFlagsNone);
	
	// make sure first SetRenderTarget goes through
	StateCache.InvalidateRenderTargets();
	
	// Drain the auto-release pool for this context
	DrainAutoreleasePool();
	
	FPlatformTLS::SetTlsValue(CurrentContextTLSSlot, nullptr);
}

void FMetalContext::SubmitCommandsHint(uint32 const Flags)
{
	RenderPass.Submit((EMetalSubmitFlags)Flags);
}

void FMetalContext::SubmitCommandBufferAndWait()
{
	// kick the whole buffer
	// Commit to hand the commandbuffer off to the gpu
	// Wait for completion as requested.
	SubmitCommandsHint((EMetalSubmitFlagsCreateCommandBuffer | EMetalSubmitFlagsWaitOnCommandBuffer));
}

void FMetalContext::ResetRenderCommandEncoder()
{
	SubmitCommandsHint();
	
	StateCache.InvalidateRenderTargets();
	
	SetRenderTargetsInfo(StateCache.GetRenderTargetsInfo(), false);
}

bool FMetalContext::PrepareToDraw(uint32 PrimitiveType, EMetalIndexType IndexType)
{
	SCOPE_CYCLE_COUNTER(STAT_MetalPrepareDrawTime);
	TRefCountPtr<FMetalBoundShaderState> CurrentBoundShaderState = StateCache.GetBoundShaderState();
	check(IsValidRef(CurrentBoundShaderState));
	
	// Enforce calls to SetRenderTarget prior to issuing draw calls.
#if PLATFORM_MAC
	check(StateCache.GetHasValidRenderTarget());
#else
	if (!StateCache.GetHasValidRenderTarget())
	{
		return false;
	}
#endif
	
	bool bUpdatedStrides = false;
	uint32 StrideHash = CurrentBoundShaderState->VertexDeclaration->BaseHash;
	
	MTLVertexDescriptor* Layout = CurrentBoundShaderState->VertexDeclaration->Layout.VertexDesc;
	if(Layout && Layout.layouts)
	{
		SCOPE_CYCLE_COUNTER(STAT_MetalPrepareVertexDescTime);
		
		for (uint32 ElementIndex = 0; ElementIndex < CurrentBoundShaderState->VertexDeclaration->Elements.Num(); ElementIndex++)
		{
			const FVertexElement& Element = CurrentBoundShaderState->VertexDeclaration->Elements[ElementIndex];
			
			uint32 StreamStride = StateCache.GetVertexStride(Element.StreamIndex);
			StrideHash = FCrc::MemCrc32(&StreamStride, sizeof(StreamStride), StrideHash);
			
			bool const bStrideMistmatch = (StreamStride > 0 && Element.Stride != StreamStride);
			
			uint32 const ActualStride = bStrideMistmatch ? StreamStride : (Element.Stride ? Element.Stride : TranslateElementTypeToSize(Element.Type));
			
			bool const bStepRateMistmatch = StreamStride == 0 && Element.Stride != StreamStride;

			if (bStrideMistmatch || bStepRateMistmatch)
			{
				if (!bUpdatedStrides)
				{
					bUpdatedStrides = true;
					Layout = [Layout copy];
					TRACK_OBJECT(STAT_MetalVertexDescriptorCount, Layout);
				}
				auto BufferLayout = [Layout.layouts objectAtIndexedSubscript:UNREAL_TO_METAL_BUFFER_INDEX(Element.StreamIndex)];
				check(BufferLayout);
				if (bStrideMistmatch)
				{
					BufferLayout.stride = ActualStride;
				}
				if (bStepRateMistmatch)
				{
					BufferLayout.stepFunction = MTLVertexStepFunctionConstant;
					BufferLayout.stepRate = 0;
				}
			}
		}
	}
	
	FMetalHashedVertexDescriptor VertexDesc;
	{
		SCOPE_CYCLE_COUNTER(STAT_MetalPrepareVertexDescTime);
		VertexDesc = !bUpdatedStrides ? CurrentBoundShaderState->VertexDeclaration->Layout : FMetalHashedVertexDescriptor(Layout, StrideHash);
	}
	
	// Validate the vertex layout in debug mode, or when the validation layer is enabled for development builds.
	// Other builds will just crash & burn if it is incorrect.
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if(CommandQueue.GetRuntimeDebuggingLevel() >= EMetalDebugLevelValidation)
	{
		if(Layout && Layout.layouts)
		{
			for (uint32 i = 0; i < MaxVertexElementCount; i++)
			{
				auto Attribute = [Layout.attributes objectAtIndexedSubscript:i];
				if(Attribute && Attribute.format > MTLVertexFormatInvalid)
				{
					auto BufferLayout = [Layout.layouts objectAtIndexedSubscript:Attribute.bufferIndex];
					uint32 BufferLayoutStride = BufferLayout ? BufferLayout.stride : 0;
					
					uint32 BufferIndex = METAL_TO_UNREAL_BUFFER_INDEX(Attribute.bufferIndex);
					
					uint64 MetalSize = StateCache.GetVertexBufferSize(BufferIndex);
					
					uint32 MetalStride = StateCache.GetVertexStride(BufferIndex);
					
					// If the vertex attribute is required and either no Metal buffer is bound or the size of the buffer is smaller than the stride, or the stride is explicitly specified incorrectly then the layouts don't match.
					if (BufferLayoutStride > 0 && (MetalSize < BufferLayoutStride || (MetalStride > 0 && MetalStride != BufferLayoutStride)))
					{
						FString Report = FString::Printf(TEXT("Vertex Layout Mismatch: Index: %d, Len: %lld, Decl. Stride: %d, Stream Stride: %d"), Attribute.bufferIndex, MetalSize, BufferLayoutStride, StateCache.GetVertexStride(BufferIndex));
						UE_LOG(LogMetal, Warning, TEXT("%s"), *Report);
					}
				}
			}
		}
	}
#endif
	
#if PLATFORM_MAC
	StateCache.SetPrimitiveTopology(TranslatePrimitiveTopology(PrimitiveType));
#endif
	
	// @todo Handle the editor not setting a depth-stencil target for the material editor's tiles which render to depth even when they shouldn't.
	bool const bNeedsDepthStencilWrite = (IsValidRef(CurrentBoundShaderState->PixelShader) && (CurrentBoundShaderState->PixelShader->Bindings.InOutMask & 0x8000) && !StateCache.HasValidDepthStencilSurface() && StateCache.GetRenderPipelineDesc().PipelineDescriptor.depthAttachmentPixelFormat == MTLPixelFormatInvalid && !FShaderCache::IsPredrawCall());
	
	// @todo Improve the way we handle binding a dummy depth/stencil so we can get pure UAV raster operations...
	bool const bNeedsDepthStencilForUAVRaster = (StateCache.GetRenderTargetsInfo().NumColorRenderTargets == 0 && StateCache.GetRenderTargetsInfo().NumUAVs > 0 && !StateCache.HasValidDepthStencilSurface());
	
	if (bNeedsDepthStencilWrite || bNeedsDepthStencilForUAVRaster)
	{
#if UE_BUILD_DEBUG
		if (bNeedsDepthStencilWrite)
		{
			UE_LOG(LogMetal, Warning, TEXT("Binding a temporary depth-stencil surface as the bound shader pipeline writes to depth/stencil but no depth/stencil surface was bound!"));
		}
		else
		{
			check(bNeedsDepthStencilForUAVRaster);
			UE_LOG(LogMetal, Warning, TEXT("Binding a temporary depth-stencil surface as the bound shader pipeline needs a texture bound - even when only writing to UAVs!"));
		}
#endif
		check(StateCache.GetRenderTargetArraySize() <= 1);
		CGSize FBSize;
		if (bNeedsDepthStencilWrite)
		{
			FBSize = StateCache.GetFrameBufferSize();
		}
		else
		{
			check(bNeedsDepthStencilForUAVRaster);
			FBSize = CGSizeMake(StateCache.GetViewport().width, StateCache.GetViewport().height);
		}
		
		FRHISetRenderTargetsInfo Info = StateCache.GetRenderTargetsInfo();
		
		FTexture2DRHIRef FallbackDepthStencilSurface = StateCache.CreateFallbackDepthStencilSurface(FBSize.width, FBSize.height);
		check(IsValidRef(FallbackDepthStencilSurface));
		
		if (bNeedsDepthStencilWrite)
		{
			Info.DepthStencilRenderTarget.Texture = FallbackDepthStencilSurface;
		}
		else
		{
			check(bNeedsDepthStencilForUAVRaster);
			Info.DepthStencilRenderTarget = FRHIDepthRenderTargetView(FallbackDepthStencilSurface, ERenderTargetLoadAction::ELoad, ERenderTargetStoreAction::ENoAction, FExclusiveDepthStencil::DepthRead_StencilRead);
		}
		
		if (StateCache.SetRenderTargetsInfo(Info, StateCache.GetVisibilityResultsBuffer(), false))
		{
			RenderPass.RestartRenderPass(StateCache.GetRenderPassDescriptor());
		}
		
		// Enforce calls to SetRenderTarget prior to issuing draw calls.
		check(StateCache.GetHasValidRenderTarget());
	}
	
	// make sure the BSS has a valid pipeline state object
	StateCache.SetIndexType(IndexType);
	FMetalShaderPipeline* PipelineState = CurrentBoundShaderState->PrepareToDraw(VertexDesc, StateCache.GetRenderPipelineDesc());
	
	StateCache.SetPipelineState(PipelineState);
	
	return true;
}

void FMetalContext::SetRenderTargetsInfo(const FRHISetRenderTargetsInfo& RenderTargetsInfo, bool const bReset)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (!CommandList.IsImmediate())
	{
		bool bClearInParallelBuffer = false;
		
		for (uint32 RenderTargetIndex = 0; RenderTargetIndex < MaxMetalRenderTargets; RenderTargetIndex++)
		{
			if (RenderTargetIndex < RenderTargetsInfo.NumColorRenderTargets && RenderTargetsInfo.ColorRenderTarget[RenderTargetIndex].Texture != nullptr)
			{
				const FRHIRenderTargetView& RenderTargetView = RenderTargetsInfo.ColorRenderTarget[RenderTargetIndex];
				if(RenderTargetView.LoadAction == ERenderTargetLoadAction::EClear)
				{
					bClearInParallelBuffer = true;
				}
			}
		}
		
		if (bClearInParallelBuffer)
		{
			UE_LOG(LogMetal, Warning, TEXT("One or more render targets bound for clear during parallel encoding: this will not behave as expected because each command-buffer will clear the target of the previous contents."));
		}
		
		if (RenderTargetsInfo.DepthStencilRenderTarget.Texture != nullptr)
		{
			if(RenderTargetsInfo.DepthStencilRenderTarget.DepthLoadAction == ERenderTargetLoadAction::EClear)
			{
				UE_LOG(LogMetal, Warning, TEXT("Depth-target bound for clear during parallel encoding: this will not behave as expected because each command-buffer will clear the target of the previous contents."));
			}
			if(RenderTargetsInfo.DepthStencilRenderTarget.StencilLoadAction == ERenderTargetLoadAction::EClear)
			{
				UE_LOG(LogMetal, Warning, TEXT("Stencil-target bound for clear during parallel encoding: this will not behave as expected because each command-buffer will clear the target of the previous contents."));
			}
		}
	}
#endif
	
	bool bSet = false;
	if (IsFeatureLevelSupported( GMaxRHIShaderPlatform, ERHIFeatureLevel::SM4 ))
	{
		bSet = StateCache.SetRenderTargetsInfo(RenderTargetsInfo, QueryBuffer->GetCurrentQueryBuffer()->Buffer, bReset);
	}
	else
	{
		bSet = StateCache.SetRenderTargetsInfo(RenderTargetsInfo, NULL, bReset);
	}
	
	if (bSet && StateCache.GetHasValidRenderTarget())
	{
		RenderPass.EndRenderPass();
		RenderPass.BeginRenderPass(StateCache.GetRenderPassDescriptor());
	}
}

uint32 FMetalContext::AllocateFromRingBuffer(uint32 Size, uint32 Alignment)
{
	return RenderPass.GetRingBuffer()->Allocate(Size, Alignment);
}

id<MTLBuffer> FMetalContext::GetRingBuffer()
{
	return RenderPass.GetRingBuffer()->Buffer;
}

void FMetalContext::DrawPrimitive(uint32 PrimitiveType, uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances)
{
	// finalize any pending state
	if(!PrepareToDraw(PrimitiveType))
	{
		return;
	}
	
	RenderPass.DrawPrimitive(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
	
	if(!FShaderCache::IsPredrawCall())
	{
		FShaderCache::LogDraw(0);
	}
}

void FMetalContext::DrawPrimitiveIndirect(uint32 PrimitiveType, FMetalVertexBuffer* VertexBuffer, uint32 ArgumentOffset)
{
	// finalize any pending state
	if(!PrepareToDraw(PrimitiveType))
	{
		return;
	}
	
	RenderPass.DrawPrimitiveIndirect(PrimitiveType, VertexBuffer, ArgumentOffset);
	
	if(!FShaderCache::IsPredrawCall())
	{
		FShaderCache::LogDraw(0);
	}
}

void FMetalContext::DrawIndexedPrimitive(id<MTLBuffer> IndexBuffer, uint32 IndexStride, MTLIndexType IndexType, uint32 PrimitiveType, int32 BaseVertexIndex, uint32 FirstInstance, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
{
	// finalize any pending state
	if(!PrepareToDraw(PrimitiveType, GetRHIMetalIndexType(IndexType)))
	{
		return;
	}
	
	RenderPass.DrawIndexedPrimitive(IndexBuffer, IndexStride, PrimitiveType, BaseVertexIndex, FirstInstance, NumVertices, StartIndex, NumPrimitives, NumInstances);
	
	if(!FShaderCache::IsPredrawCall())
	{
		FShaderCache::LogDraw(IndexStride);
	}
}

void FMetalContext::DrawIndexedIndirect(FMetalIndexBuffer* IndexBuffer, uint32 PrimitiveType, FMetalStructuredBuffer* VertexBuffer, int32 DrawArgumentsIndex, uint32 NumInstances)
{
	// finalize any pending state
	if(!PrepareToDraw(PrimitiveType))
	{
		return;
	}
	
	RenderPass.DrawIndexedIndirect(IndexBuffer, PrimitiveType, VertexBuffer, DrawArgumentsIndex, NumInstances);
	
	if(!FShaderCache::IsPredrawCall())
	{
		FShaderCache::LogDraw(IndexBuffer->GetStride());
	}
}

void FMetalContext::DrawIndexedPrimitiveIndirect(uint32 PrimitiveType,FMetalIndexBuffer* IndexBuffer,FMetalVertexBuffer* VertexBuffer, uint32 ArgumentOffset)
{	
	// finalize any pending state
	if(!PrepareToDraw(PrimitiveType))
	{
		return;
	}
	
	RenderPass.DrawIndexedPrimitiveIndirect(PrimitiveType, IndexBuffer, VertexBuffer, ArgumentOffset);
	
	if(!FShaderCache::IsPredrawCall())
	{
		FShaderCache::LogDraw(IndexBuffer->GetStride());
	}
}

void FMetalContext::CopyFromTextureToBuffer(id<MTLTexture> Texture, uint32 sourceSlice, uint32 sourceLevel, MTLOrigin sourceOrigin, MTLSize sourceSize, id<MTLBuffer> toBuffer, uint32 destinationOffset, uint32 destinationBytesPerRow, uint32 destinationBytesPerImage, MTLBlitOption options)
{
	RenderPass.CopyFromTextureToBuffer(Texture, sourceSlice, sourceLevel, sourceOrigin, sourceSize, toBuffer, destinationOffset, destinationBytesPerRow, destinationBytesPerImage, options);
}

void FMetalContext::CopyFromBufferToTexture(id<MTLBuffer> Buffer, uint32 sourceOffset, uint32 sourceBytesPerRow, uint32 sourceBytesPerImage, MTLSize sourceSize, id<MTLTexture> toTexture, uint32 destinationSlice, uint32 destinationLevel, MTLOrigin destinationOrigin)
{
	RenderPass.CopyFromBufferToTexture(Buffer, sourceOffset, sourceBytesPerRow, sourceBytesPerImage, sourceSize, toTexture, destinationSlice, destinationLevel, destinationOrigin);
}

void FMetalContext::CopyFromTextureToTexture(id<MTLTexture> Texture, uint32 sourceSlice, uint32 sourceLevel, MTLOrigin sourceOrigin, MTLSize sourceSize, id<MTLTexture> toTexture, uint32 destinationSlice, uint32 destinationLevel, MTLOrigin destinationOrigin)
{
	RenderPass.CopyFromTextureToTexture(Texture, sourceSlice, sourceLevel, sourceOrigin, sourceSize, toTexture, destinationSlice, destinationLevel, destinationOrigin);
}

void FMetalContext::AsyncCopyFromBufferToTexture(id<MTLCommandBuffer> CmdBuf, id<MTLBuffer> Buffer, uint32 sourceOffset, uint32 sourceBytesPerRow, uint32 sourceBytesPerImage, MTLSize sourceSize, id<MTLTexture> toTexture, uint32 destinationSlice, uint32 destinationLevel, MTLOrigin destinationOrigin)
{
    id<MTLBlitCommandEncoder> Blit = nil;
    
    Blit = [CmdBuf blitCommandEncoder];
		
	[Blit copyFromBuffer:Buffer sourceOffset:sourceOffset sourceBytesPerRow:sourceBytesPerRow sourceBytesPerImage:sourceBytesPerImage sourceSize:sourceSize toTexture:toTexture destinationSlice:destinationSlice destinationLevel:destinationLevel destinationOrigin:destinationOrigin];
    
    [Blit endEncoding];
}

void FMetalContext::AsyncCopyFromTextureToTexture(id<MTLCommandBuffer> CmdBuf, id<MTLTexture> Texture, uint32 sourceSlice, uint32 sourceLevel, MTLOrigin sourceOrigin, MTLSize sourceSize, id<MTLTexture> toTexture, uint32 destinationSlice, uint32 destinationLevel, MTLOrigin destinationOrigin)
{
    id<MTLBlitCommandEncoder> Blit = nil;
    
    Blit = [CmdBuf blitCommandEncoder];
		
	[Blit copyFromTexture:Texture sourceSlice:sourceSlice sourceLevel:sourceLevel sourceOrigin:sourceOrigin sourceSize:sourceSize toTexture:toTexture destinationSlice:destinationSlice destinationLevel:destinationLevel destinationOrigin:destinationOrigin];
    
    [Blit endEncoding];
}

void FMetalContext::GenerateMipmapsForTexture(id<MTLCommandBuffer> CmdBuf, id<MTLTexture> Texture)
{
    id<MTLBlitCommandEncoder> Blit = nil;
    
    Blit = [CmdBuf blitCommandEncoder];
		
    [Blit generateMipmapsForTexture:Texture];
    
    [Blit endEncoding];
}

id<MTLCommandBuffer> FMetalContext::BeginAsyncCommands(void)
{
    id<MTLCommandBuffer> CmdBuffer = CommandQueue.CreateCommandBuffer();
    return CmdBuffer;
}

void FMetalContext::EndAsyncCommands(id<MTLCommandBuffer> CmdBuffer, bool const bWait)
{
    CommandList.Commit(CmdBuffer, bWait);
}

void FMetalContext::SynchronizeTexture(id<MTLTexture> Texture, uint32 Slice, uint32 Level)
{
	RenderPass.SynchronizeTexture(Texture, Slice, Level);
}

void FMetalContext::SynchroniseResource(id<MTLResource> Resource)
{
	RenderPass.SynchroniseResource(Resource);
}

void FMetalContext::FillBuffer(id<MTLBuffer> Buffer, NSRange Range, uint8 Value)
{
	RenderPass.FillBuffer(Buffer, Range, Value);
}

void FMetalContext::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
	RenderPass.Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

#if METAL_API_1_1
void FMetalContext::DispatchIndirect(FMetalVertexBuffer* ArgumentBuffer, uint32 ArgumentOffset)
{
	RenderPass.DispatchIndirect(ArgumentBuffer, ArgumentOffset);
}
#endif

void FMetalContext::StartTiming(class FMetalEventNode* EventNode)
{
	if(EventNode && GetCurrentCommandBuffer())
	{
		EventNode->Start(GetCurrentCommandBuffer());
	}
	
	SubmitCommandsHint(EMetalSubmitFlagsCreateCommandBuffer);
}

void FMetalContext::EndTiming(class FMetalEventNode* EventNode)
{
	bool const bWait = EventNode->Wait();
	EventNode->Stop(GetCurrentCommandBuffer());
	
	if (!bWait)
	{
		SubmitCommandsHint(EMetalSubmitFlagsCreateCommandBuffer);
	}
	else
	{
		SubmitCommandBufferAndWait();
	}
}

#if METAL_SUPPORTS_PARALLEL_RHI_EXECUTE

class FMetalCommandContextContainer : public IRHICommandContextContainer
{
	FMetalRHICommandContext* CmdContext;
	
public:
	
	/** Custom new/delete with recycling */
	void* operator new(size_t Size);
	void operator delete(void *RawMemory);
	
	FMetalCommandContextContainer()
	: CmdContext(nullptr)
	{
	}
	
	virtual ~FMetalCommandContextContainer() override
	{
	}
	
	virtual IRHICommandContext* GetContext() override
	{
		CmdContext = GetMetalDeviceContext().AcquireContext();
		check(CmdContext);
		CmdContext->GetInternalContext().InitFrame(false);
		return CmdContext;
	}
	virtual void FinishContext() override
	{
		if (CmdContext)
		{
			CmdContext->GetInternalContext().FinishFrame();
		}
	}
	virtual void SubmitAndFreeContextContainer(int32 Index, int32 Num) override
	{
		if (CmdContext)
		{
			check(CmdContext->GetInternalContext().GetCurrentCommandBuffer() == nil);
			CmdContext->GetInternalContext().GetCommandList().Submit(Index, Num);
			
			GetMetalDeviceContext().ReleaseContext(CmdContext);
			CmdContext = nullptr;
			check(!CmdContext);
		}
		delete this;
	}
};

static TLockFreeFixedSizeAllocator<sizeof(FMetalCommandContextContainer), PLATFORM_CACHE_LINE_SIZE, FThreadSafeCounter> FMetalCommandContextContainerAllocator;

void* FMetalCommandContextContainer::operator new(size_t Size)
{
	return FMemory::Malloc(Size);
}

/**
 * Custom delete
 */
void FMetalCommandContextContainer::operator delete(void *RawMemory)
{
	FMemory::Free(RawMemory);
}

IRHICommandContextContainer* FMetalDynamicRHI::RHIGetCommandContextContainer()
{
	return new FMetalCommandContextContainer();
}

#else

IRHICommandContextContainer* FMetalDynamicRHI::RHIGetCommandContextContainer()
{
	return nullptr;
}

#endif
