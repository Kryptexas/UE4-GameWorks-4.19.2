// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MetalManager.h"
#include "IOSAppDelegate.h"

const uint32 RingBufferSize = 8 * 1024 * 1024;

#if SHOULD_TRACK_OBJECTS
TMap<id, int32> ClassCounts;
#endif

#define NUMBITS_DEPTH_WRITE_ENABLED				1
#define NUMBITS_STENCIL_WRITE_ENABLED			1
#define NUMBITS_SOURCE_RGB_BLEND_FACTOR			4
#define NUMBITS_DEST_RGB_BLEND_FACTOR			4
#define NUMBITS_RGB_BLEND_OPERATION				3
#define NUMBITS_SOURCE_A_BLEND_FACTOR			4
#define NUMBITS_DEST_A_BLEND_FACTOR				4
#define NUMBITS_A_BLEND_OPERATION				3
#define NUMBITS_WRITE_MASK						4
#define NUMBITS_RENDER_TARGET_FORMAT			9
#define NUMBITS_DEPTH_TARGET_FORMAT				9
#define NUMBITS_SAMPLE_COUNT					3


#define OFFSET_DEPTH_WRITE_ENABLED				(0)
#define OFFSET_STENCIL_WRITE_ENABLED			(OFFSET_DEPTH_WRITE_ENABLED		+ NUMBITS_DEPTH_WRITE_ENABLED)
#define OFFSET_SOURCE_RGB_BLEND_FACTOR			(OFFSET_STENCIL_WRITE_ENABLED	+ NUMBITS_STENCIL_WRITE_ENABLED)
#define OFFSET_DEST_RGB_BLEND_FACTOR			(OFFSET_SOURCE_RGB_BLEND_FACTOR	+ NUMBITS_SOURCE_RGB_BLEND_FACTOR)
#define OFFSET_RGB_BLEND_OPERATION				(OFFSET_DEST_RGB_BLEND_FACTOR	+ NUMBITS_DEST_RGB_BLEND_FACTOR)
#define OFFSET_SOURCE_A_BLEND_FACTOR			(OFFSET_RGB_BLEND_OPERATION		+ NUMBITS_RGB_BLEND_OPERATION)
#define OFFSET_DEST_A_BLEND_FACTOR				(OFFSET_SOURCE_A_BLEND_FACTOR	+ NUMBITS_SOURCE_A_BLEND_FACTOR)
#define OFFSET_A_BLEND_OPERATION				(OFFSET_DEST_A_BLEND_FACTOR		+ NUMBITS_DEST_A_BLEND_FACTOR)
#define OFFSET_WRITE_MASK						(OFFSET_A_BLEND_OPERATION		+ NUMBITS_A_BLEND_OPERATION)
#define OFFSET_RENDER_TARGET_FORMAT				(OFFSET_WRITE_MASK				+ NUMBITS_WRITE_MASK)
#define OFFSET_DEPTH_TARGET_FORMAT				(OFFSET_RENDER_TARGET_FORMAT	+ NUMBITS_RENDER_TARGET_FORMAT)
#define OFFSET_SAMPLE_COUNT						(OFFSET_DEPTH_TARGET_FORMAT		+ NUMBITS_DEPTH_TARGET_FORMAT)

#define SET_HASH(Offset, NumBits, Value) \
	{ \
		uint64 BitMask = ((1ULL << NumBits) - 1) << Offset; \
		Pipeline.Hash = (Pipeline.Hash & ~BitMask) | (((uint64)Value << Offset) & BitMask); \
	}
#define GET_HASH(Offset, NumBits) ((Hash >> Offset) & ((1ULL << NumBits) - 1))

DEFINE_STAT(STAT_MetalMakeDrawableTime);
DEFINE_STAT(STAT_MetalDrawCallTime);
DEFINE_STAT(STAT_MetalPrepareDrawTime);
DEFINE_STAT(STAT_MetalUniformBufferCleanupTime);
DEFINE_STAT(STAT_MetalFreeUniformBufferMemory);
DEFINE_STAT(STAT_MetalNumFreeUniformBuffers);
DEFINE_STAT(STAT_MetalPipelineStateTime);
DEFINE_STAT(STAT_MetalBoundShaderStateTime);
DEFINE_STAT(STAT_MetalVertexDeclarationTime);

	
void FPipelineShadow::SetHash(uint64 InHash)
{
	Hash = InHash;

	bIsDepthWriteEnabled = GET_HASH(OFFSET_DEPTH_WRITE_ENABLED, NUMBITS_DEPTH_WRITE_ENABLED);
	bIsStencilWriteEnabled = GET_HASH(OFFSET_STENCIL_WRITE_ENABLED, NUMBITS_STENCIL_WRITE_ENABLED);
	RenderTargets[0] = [[MTLRenderPipelineAttachmentDescriptor alloc] init];
	FMetalManager::Get()->ReleaseObject(RenderTargets[0]);
	RenderTargets[0].sourceRGBBlendFactor = (MTLBlendFactor)GET_HASH(OFFSET_SOURCE_RGB_BLEND_FACTOR, NUMBITS_SOURCE_RGB_BLEND_FACTOR);
	RenderTargets[0].destinationRGBBlendFactor = (MTLBlendFactor)GET_HASH(OFFSET_DEST_RGB_BLEND_FACTOR, NUMBITS_DEST_RGB_BLEND_FACTOR);
	RenderTargets[0].rgbBlendOperation = (MTLBlendOperation)GET_HASH(OFFSET_RGB_BLEND_OPERATION, NUMBITS_RGB_BLEND_OPERATION);
	RenderTargets[0].sourceAlphaBlendFactor = (MTLBlendFactor)GET_HASH(OFFSET_SOURCE_A_BLEND_FACTOR, NUMBITS_SOURCE_A_BLEND_FACTOR);
	RenderTargets[0].destinationAlphaBlendFactor = (MTLBlendFactor)GET_HASH(OFFSET_DEST_A_BLEND_FACTOR, NUMBITS_DEST_A_BLEND_FACTOR);
	RenderTargets[0].alphaBlendOperation = (MTLBlendOperation)GET_HASH(OFFSET_A_BLEND_OPERATION, NUMBITS_A_BLEND_OPERATION);
	RenderTargets[0].writeMask = GET_HASH(OFFSET_WRITE_MASK, NUMBITS_WRITE_MASK);
	RenderTargets[0].blendingEnabled =
		RenderTargets[0].rgbBlendOperation != MTLBlendOperationAdd || RenderTargets[0].destinationRGBBlendFactor != MTLBlendFactorZero || RenderTargets[0].sourceRGBBlendFactor != MTLBlendFactorOne ||
		RenderTargets[0].alphaBlendOperation != MTLBlendOperationAdd || RenderTargets[0].destinationAlphaBlendFactor != MTLBlendFactorZero || RenderTargets[0].sourceAlphaBlendFactor != MTLBlendFactorOne;

	RenderTargets[0].pixelFormat = (MTLPixelFormat)GET_HASH(OFFSET_RENDER_TARGET_FORMAT, NUMBITS_RENDER_TARGET_FORMAT);
	DepthTargetFormat = (MTLPixelFormat)GET_HASH(OFFSET_DEPTH_TARGET_FORMAT, NUMBITS_DEPTH_TARGET_FORMAT);
	SampleCount = GET_HASH(OFFSET_SAMPLE_COUNT, NUMBITS_SAMPLE_COUNT);
}



static MTLTriangleFillMode TranslateFillMode(ERasterizerFillMode FillMode)
{
	switch (FillMode)
	{
		case FM_Wireframe:	return MTLTriangleFillModeLines;
		case FM_Point:		return MTLTriangleFillModeFill;
		default:			return MTLTriangleFillModeFill;
	};
}

static MTLCullMode TranslateCullMode(ERasterizerCullMode CullMode)
{
	switch (CullMode)
	{
		case CM_CCW:	return MTLCullModeFront;
		case CM_CW:		return MTLCullModeBack;
		default:		return MTLCullModeNone;
	}
}

static MTLBlendOperation TranslateBlendOp(EBlendOperation BlendOp)
{
	switch (BlendOp)
	{
	case BO_Subtract:	return MTLBlendOperationSubtract;
	case BO_Min:		return MTLBlendOperationMin;
	case BO_Max:		return MTLBlendOperationMax;
	default:			return MTLBlendOperationAdd;
	};
}


static MTLBlendFactor TranslateBlendFactor(EBlendFactor BlendFactor)
{
	switch (BlendFactor)
	{
	case BF_One:					return MTLBlendFactorOne;
	case BF_SourceColor:			return MTLBlendFactorSourceColor;
	case BF_InverseSourceColor:		return MTLBlendFactorOneMinusSourceColor;
	case BF_SourceAlpha:			return MTLBlendFactorSourceAlpha;
	case BF_InverseSourceAlpha:		return MTLBlendFactorOneMinusSourceAlpha;
	case BF_DestAlpha:				return MTLBlendFactorDestinationAlpha;
	case BF_InverseDestAlpha:		return MTLBlendFactorOneMinusDestinationAlpha;
	case BF_DestColor:				return MTLBlendFactorDestinationColor;
	case BF_InverseDestColor:		return MTLBlendFactorOneMinusDestinationColor;
	default:						return MTLBlendFactorZero;
	};
}

static MTLColorWriteMask TranslateWriteMask(EColorWriteMask WriteMask)
{
	uint32 Result = 0;
	Result |= (WriteMask & CW_RED) ? (MTLColorWriteMaskRed) : 0;
	Result |= (WriteMask & CW_GREEN) ? (MTLColorWriteMaskGreen) : 0;
	Result |= (WriteMask & CW_BLUE) ? (MTLColorWriteMaskBlue) : 0;
	Result |= (WriteMask & CW_ALPHA) ? (MTLColorWriteMaskAlpha) : 0;

	return (MTLColorWriteMask)Result;
}




id<MTLRenderPipelineState> FPipelineShadow::CreatePipelineStateForBoundShaderState(FMetalBoundShaderState* BSS) const
{
	SCOPE_CYCLE_COUNTER(STAT_MetalPipelineStateTime);
	
	MTLRenderPipelineDescriptor* Desc = [[MTLRenderPipelineDescriptor alloc] init];

	// some basic settings
	Desc.depthWriteEnabled = bIsDepthWriteEnabled;
	Desc.stencilWriteEnabled = bIsStencilWriteEnabled;

	// set per-MRT settings
	for (uint32 RenderTargetIndex = 0; RenderTargetIndex < MaxMetalRenderTargets; ++RenderTargetIndex)
	{
		[Desc.colorAttachments setObject:RenderTargets[RenderTargetIndex] atIndexedSubscript:RenderTargetIndex];
	}

	// depth setting if it's actually used
//	if (DepthTargetFormat != MTLPixelFormatInvalid)
	{
		Desc.depthAttachmentPixelFormat = DepthTargetFormat;
	}

	// set the bound shader state settings
	Desc.vertexDescriptor = BSS->VertexDeclaration->Layout;
	Desc.vertexFunction = BSS->VertexShader->Function;
	Desc.fragmentFunction = BSS->PixelShader ? BSS->PixelShader->Function : nil;
	Desc.sampleCount = SampleCount;

	check(SampleCount > 0);

	NSError* Error = nil;
	id<MTLRenderPipelineState> PipelineState = [FMetalManager::GetDevice() newRenderPipelineStateWithDescriptor:Desc error : &Error];
	TRACK_OBJECT(PipelineState);

	[Desc release];

	if (PipelineState == nil)
	{
		NSLog(@"Failed to generate a pipeline state object: %@", Error);
		return nil;
	}

	return PipelineState;
}

FMetalManager* FMetalManager::Get()
{
	static FMetalManager Singleton;
	return &Singleton;
}

id<MTLDevice> FMetalManager::GetDevice()
{
	return FMetalManager::Get()->Device;
}

id<MTLRenderCommandEncoder> FMetalManager::GetContext()
{
	return FMetalManager::Get()->CurrentContext;
}

void FMetalManager::ReleaseObject(id Object)
{
	FMetalManager::Get()->DelayedFreeLists[FMetalManager::Get()->WhichFreeList].Add(Object);
}


@interface MetalFramePacer : NSObject
{
@public
	FEvent *FramePacerEvent;
}

-(void)run:(id)param;
-(void)signal:(id)param;

@end

@implementation MetalFramePacer

-(void)run:(id)param
{
	NSRunLoop *runloop = [NSRunLoop currentRunLoop];
	CADisplayLink *displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(signal:)];
	displayLink.frameInterval = 2;
	
	[NSThread currentThread].name = @"FramePacerThread";
	FramePacerEvent	= new FLocalEvent();
	FramePacerEvent->Create(false);
		
	[displayLink addToRunLoop:runloop forMode:NSDefaultRunLoopMode];
	[runloop run];
}

-(void)signal:(id)param
{
	FramePacerEvent->Trigger();
}

@end

MetalFramePacer *FramePacer;

FMetalManager::FMetalManager()
	: Device([IOSAppDelegate GetDelegate].IOSView->MetalDevice)
	, CurrentCommandBuffer(nil)
	, CurrentDrawable(nil)
	, CurrentRenderPass(nil)
	, CurrentContext(nil)
	, CurrentColorRenderTexture(nil)
	, PreviousColorRenderTexture(nil)
	, CurrentDepthRenderTexture(nil)
	, PreviousDepthRenderTexture(nil)
	, RingBuffer(Device, RingBufferSize, BufferOffsetAlignment)
	, QueryBuffer(Device, 64 * 1024, 8)
	, WhichFreeList(0)
	, CommandBufferIndex(0)
	, CompletedCommandBufferIndex(0)
	, SceneFrameCounter(0)
	, ResourceTableFrameCounter(INDEX_NONE)
{
	CommandQueue = [Device newCommandQueue];

	// get the size of the window
	CGRect ViewFrame = [[IOSAppDelegate GetDelegate].IOSView frame];
	FRHIResourceCreateInfo CreateInfo;
	BackBuffer = (FMetalTexture2D*)(FTexture2DRHIParamRef)RHICreateTexture2D(ViewFrame.size.width, ViewFrame.size.height, PF_B8G8R8A8, 1, 1, TexCreate_RenderTargetable | TexCreate_Presentable, CreateInfo);

//@todo-rco: What Size???
	// make a buffer for each shader type
	ShaderParameters = new FMetalShaderParameterCache[METAL_NUM_SHADER_STAGES];
	ShaderParameters[METAL_SHADER_STAGE_VERTEX].InitializeResources(1024*1024);
	ShaderParameters[METAL_SHADER_STAGE_PIXEL].InitializeResources(1024*1024);

	// create a semaphore for multi-buffering the command buffer
	CommandBufferSemaphore = dispatch_semaphore_create(FParse::Param(FCommandLine::Get(),TEXT("gpulockstep")) ? 1 : 3);

	AutoReleasePoolTLSSlot = FPlatformTLS::AllocTlsSlot();

#if GPU_TIMINGS
	// start at start of GPUTimers
	TimerIndex = 0;
	FirstIndexToDump = 0;
	LastIndexToDump = 0;
#endif
	
	// Create display link thread
	FramePacer = [[MetalFramePacer alloc] init];
	[NSThread detachNewThreadSelector:@selector(run:) toTarget:FramePacer withObject:nil];
	
	InitFrame();
}

void FMetalManager::BeginScene()
{
	// Increment the frame counter. INDEX_NONE is a special value meaning "uninitialized", so if
	// we hit it just wrap around to zero.
	SceneFrameCounter++;
	if (SceneFrameCounter == INDEX_NONE)
	{
		SceneFrameCounter++;
	}

	static auto* ResourceTableCachingCvar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("rhi.ResourceTableCaching"));
	if (ResourceTableCachingCvar == NULL || ResourceTableCachingCvar->GetValueOnAnyThread() == 1)
	{
		ResourceTableFrameCounter = SceneFrameCounter;
	}
}

void FMetalManager::EndScene()
{
	ResourceTableFrameCounter = INDEX_NONE;
}

void FMetalManager::BeginFrame()
{
}


static TConsoleVariableData<int32>* GDrawableMode = IConsoleManager::Get().RegisterConsoleVariable(TEXT("u.Drawable"), 1, TEXT("Whether to use new drawable mode or not"), ECVF_Default)->AsVariableInt();
static bool GUseNewDrawable = true;

void FMetalManager::InitFrame()
{
	GUseNewDrawable = GDrawableMode->GetValueOnRenderThread() > 0 ? true : false;

	// start an auto release pool (EndFrame will drain and remake)
	FPlatformTLS::SetTlsValue(AutoReleasePoolTLSSlot, FPlatformMisc::CreateAutoreleasePool());

	// create the command buffer for this frame
	CurrentCommandBuffer = [CommandQueue commandBufferWithUnretainedReferences];
	[CurrentCommandBuffer retain];
	TRACK_OBJECT(CurrentCommandBuffer);
	
#if GPU_TIMINGS
	uint8 LocalTimerIndex = TimerIndex++;
	FirstIndexThisFrame = LocalTimerIndex;
#endif

	uint64 LocalCommandBufferIndex = CommandBufferIndex++;
	[CurrentCommandBuffer addScheduledHandler : ^ (id <MTLCommandBuffer> Buffer)
	{
#if GPU_TIMINGS
		// timer stuff
		GPUTimers[LocalTimerIndex] = FPlatformTime::Seconds();
#endif

		FMetalManager::Get()->SetCompletedCommandBufferIndex(LocalCommandBufferIndex);
	}];

//	double Start = FPlatformTime::Seconds();
	// block on the semaphore
	dispatch_semaphore_wait(CommandBufferSemaphore, DISPATCH_TIME_FOREVER);

//	double Mid = FPlatformTime::Seconds();

	if (GUseNewDrawable)
	{
		// mark us to get later
		BackBuffer->Surface.Texture = nil;
	}
	else
	{
		SCOPE_CYCLE_COUNTER(STAT_MetalMakeDrawableTime);

		// make a drawable object for this frame
		CurrentDrawable = [[IOSAppDelegate GetDelegate].IOSView MakeDrawable];
		TRACK_OBJECT(CurrentDrawable);
		BackBuffer->Surface.Texture = CurrentDrawable.texture;
	}

//	double End = FPlatformTime::Seconds();
//	NSLog(@"Semaphore Block Time: %.2f   -- MakeDrawable Time: %.2f", 1000.0f*(Mid - Start), 1000.0f*(End - Mid));
	

	extern void InitFrame_UniformBufferPoolCleanup();
	InitFrame_UniformBufferPoolCleanup();

	NumDrawCalls = 0;
}

void FMetalManager::EndFrame(bool bPresent)
{
//	NSLog(@"There were %d draw calls for final RT in frame %lld", NumDrawCalls, GFrameCounter);
	// commit the render context to the commandBuffer
	[CurrentContext endEncoding];
	ReleaseObject(CurrentContext);
	CurrentContext = nil;

#if GPU_TIMINGS
	// set up timer
	uint8 LocalTimerIndex = TimerIndex++;
	uint8 LocalFirstIndex = FirstIndexThisFrame;
#endif

	// kick the whole buffer
	[CurrentCommandBuffer addCompletedHandler:^(id <MTLCommandBuffer> Buffer)
	 {
#if GPU_TIMINGS
	 GPUTimers[LocalTimerIndex] = FPlatformTime::Seconds();
	 FirstIndexToDump = LocalFirstIndex;
	 LastIndexToDump = LocalTimerIndex;
#endif
	 dispatch_semaphore_signal(CommandBufferSemaphore);
	 }];

	// Wait until at least 2 VBlanks has passed since last time
	FramePacer->FramePacerEvent->Wait();

	// Commit before waiting to avoid leaving the gpu idle
	[CurrentCommandBuffer commit];
	
	// enqueue a present if desired
	if (CurrentDrawable)
	{
		if (bPresent && GFrameCounter > 3)
		{
			[CurrentCommandBuffer waitUntilScheduled];
			
			[CurrentDrawable present];
		}

		UNTRACK_OBJECT(CurrentDrawable);
		[CurrentDrawable release];
		CurrentDrawable = nil;
	}


	ReleaseObject(CurrentRenderPass);
	CurrentRenderPass = nil;


	UNTRACK_OBJECT(CurrentCommandBuffer);
	[CurrentCommandBuffer release];

#if GPU_TIMINGS
	// print out timings
	bool bDone = false;
	uint8 Index = FirstIndexToDump;
	uint8 RTIndex = 0;
	while (Index != LastIndexToDump)
	{
		// this can wrap around at 256
		uint8 NextIndex = Index + 1;
		double Time = (GPUTimers[NextIndex] - GPUTimers[Index]) * 1000.0;
		NSLog(@"Rende40rTarget %d took %.3fms", RTIndex, (float)Time);

		Index = NextIndex;
		RTIndex++;
	}
#endif

	// xcode helper function
	[CommandQueue insertDebugCaptureBoundary];

	// drain the pool
	FPlatformMisc::ReleaseAutoreleasePool(FPlatformTLS::GetTlsValue(AutoReleasePoolTLSSlot));

	// drain the oldest pool
	uint32 PrevFreeList = (WhichFreeList + 1) % ARRAY_COUNT(DelayedFreeLists);
	for (int32 Index = 0; Index < DelayedFreeLists[PrevFreeList].Num(); Index++)
	{
		id Object = DelayedFreeLists[PrevFreeList][Index];
		UNTRACK_OBJECT(Object);
		[Object release];
	}
	DelayedFreeLists[PrevFreeList].Reset();

	// and switch to it
	WhichFreeList = PrevFreeList;

#if SHOULD_TRACK_OBJECTS
	// print out outstanding objects
	if ((GFrameCounter % 500) == 10)
	{
		for (auto It = ClassCounts.CreateIterator(); It; ++It)
		{
			NSLog(@"%@ has %d outstanding allocations", It.Key(), It.Value());
		}
	}
#endif

	InitFrame();
}


FMetalTexture2D* FMetalManager::GetBackBuffer()
{
	return BackBuffer;
}

void FMetalManager::PrepareToDraw(uint32 NumVertices)
{
#if NO_MEMORY
	return;
#endif

	SCOPE_CYCLE_COUNTER(STAT_MetalPrepareDrawTime);

	NumDrawCalls++;

	// make sure the BSS has a valid pipeline state object
	CurrentBoundShaderState->PrepareToDraw(Pipeline);
	
	CommitGraphicsResourceTables();
	CommitNonComputeShaderConstants();

	// grow buffers as needed
	extern TRefCountPtr<FMetalVertexBuffer> HACK_ZeroStrideBuffers[16];
	for (int32 Index = 0; Index < 16; Index++)
	{
		if (HACK_ZeroStrideBuffers[Index] != NULL)
		{
			TRefCountPtr<FMetalVertexBuffer> Buffer = HACK_ZeroStrideBuffers[Index];
			
			// how big do we need?
			uint32 NeededSize = Align(NumVertices * Buffer->ZeroStrideElementSize, 128*1024);
			// if too small, then grow and fill in
			if (NeededSize > [Buffer->Buffer length])
			{
				id<MTLBuffer> Orig = Buffer->Buffer;
				Buffer->Buffer = [Device newBufferWithLength:NeededSize options:BUFFER_CACHE_MODE];
				TRACK_OBJECT(Buffer->Buffer);

				uint8* SrcMem = (uint8*)[Orig contents];
				uint8* DestMem = (uint8*)[Buffer->Buffer contents];
				for (int32 CopyIndex = 1; CopyIndex < NumVertices; CopyIndex++)
				{
					FMemory::Memcpy(DestMem + Buffer->ZeroStrideElementSize * CopyIndex, SrcMem, Buffer->ZeroStrideElementSize);
				}

				NSLog(@"Expaning buffer from %d to %d", (int32)[Orig length], NeededSize);
				// set the new buffer object
				[FMetalManager::GetContext() setVertexBuffer:Buffer->Buffer offset:0 atIndex:UNREAL_TO_METAL_BUFFER_INDEX(Index)];

				ReleaseObject(Orig);
			}
		}
	}
}

void FMetalManager::SetDepthStencilWriteEnabled(bool bIsDepthWriteEnabled, bool bIsStencilWriteEnabled)
{
	Pipeline.bIsDepthWriteEnabled = bIsDepthWriteEnabled;
	Pipeline.bIsStencilWriteEnabled = bIsStencilWriteEnabled;

	SET_HASH(OFFSET_DEPTH_WRITE_ENABLED, NUMBITS_DEPTH_WRITE_ENABLED, bIsDepthWriteEnabled);
	SET_HASH(OFFSET_STENCIL_WRITE_ENABLED, NUMBITS_STENCIL_WRITE_ENABLED, bIsStencilWriteEnabled);
}

void FMetalManager::SetBlendState(FMetalBlendState* BlendState)
{
	for(uint32 RenderTargetIndex = 0;RenderTargetIndex < MaxMetalRenderTargets; ++RenderTargetIndex)
	{
		// @todo metal mrt
		check(RenderTargetIndex == 0);

		// get the blend params
		const FBlendStateInitializerRHI::FRenderTarget& Init =
			BlendState->RenderTargetStates.bUseIndependentRenderTargetBlendStates
				? BlendState->RenderTargetStates.RenderTargets[RenderTargetIndex]
				: BlendState->RenderTargetStates.RenderTargets[0];

		// write into the pipline attachment
		MTLRenderPipelineAttachmentDescriptor* Attachment = Pipeline.RenderTargets[RenderTargetIndex];
		
		// set values
		Attachment.blendingEnabled =
			Init.ColorBlendOp != BO_Add || Init.ColorDestBlend != BF_Zero || Init.ColorSrcBlend != BF_One ||
			Init.AlphaBlendOp != BO_Add || Init.AlphaDestBlend != BF_Zero || Init.AlphaSrcBlend != BF_One;
		Attachment.sourceRGBBlendFactor = TranslateBlendFactor(Init.ColorSrcBlend);
		Attachment.destinationRGBBlendFactor = TranslateBlendFactor(Init.ColorDestBlend);
		Attachment.rgbBlendOperation = TranslateBlendOp(Init.ColorBlendOp);
		Attachment.sourceAlphaBlendFactor = TranslateBlendFactor(Init.AlphaSrcBlend);
		Attachment.destinationAlphaBlendFactor = TranslateBlendFactor(Init.AlphaDestBlend);
		Attachment.alphaBlendOperation = TranslateBlendOp(Init.AlphaBlendOp);
		Attachment.writeMask = TranslateWriteMask(Init.ColorWriteMask);

		SET_HASH(OFFSET_SOURCE_RGB_BLEND_FACTOR, NUMBITS_SOURCE_RGB_BLEND_FACTOR, Attachment.sourceRGBBlendFactor);
		SET_HASH(OFFSET_DEST_RGB_BLEND_FACTOR, NUMBITS_DEST_RGB_BLEND_FACTOR, Attachment.destinationRGBBlendFactor);
		SET_HASH(OFFSET_RGB_BLEND_OPERATION, NUMBITS_RGB_BLEND_OPERATION, Attachment.rgbBlendOperation);
		SET_HASH(OFFSET_SOURCE_A_BLEND_FACTOR, NUMBITS_SOURCE_A_BLEND_FACTOR, Attachment.sourceAlphaBlendFactor);
		SET_HASH(OFFSET_DEST_A_BLEND_FACTOR, NUMBITS_DEST_A_BLEND_FACTOR, Attachment.destinationAlphaBlendFactor);
		SET_HASH(OFFSET_A_BLEND_OPERATION, NUMBITS_A_BLEND_OPERATION, Attachment.alphaBlendOperation);
		SET_HASH(OFFSET_WRITE_MASK, NUMBITS_WRITE_MASK, Attachment.writeMask);
	}
}

void FMetalManager::SetBoundShaderState(FMetalBoundShaderState* BoundShaderState)
{
#if NO_DRAW
	return;
#endif
	CurrentBoundShaderState = BoundShaderState;
}

void FMetalManager::SetCurrentRenderTarget(FMetalSurface* RenderSurface)
{
	// update the current rendered-to pixel format
	if (RenderSurface)
	{
		if (GUseNewDrawable)
		{
			// first time in a frame that we are setting the backbuffer, get it
			if (RenderSurface == &BackBuffer->Surface && RenderSurface->Texture == nil && CurrentDrawable == nil)
			{
				SCOPE_CYCLE_COUNTER(STAT_MetalMakeDrawableTime);

				uint32 IdleStart = FPlatformTime::Cycles();

				// make a drawable object for this frame
				CurrentDrawable = [[IOSAppDelegate GetDelegate].IOSView MakeDrawable];
				TRACK_OBJECT(CurrentDrawable);

				GRenderThreadIdle[ERenderThreadIdleTypes::WaitingForGPUPresent] += FPlatformTime::Cycles() - IdleStart;
				GRenderThreadNumIdle[ERenderThreadIdleTypes::WaitingForGPUPresent]++;

				// set the texture into the backbuffer
				RenderSurface->Texture = CurrentDrawable.texture;
			}
		}

		CurrentColorRenderTexture = RenderSurface->Texture;
		CurrentMSAARenderTexture = RenderSurface->MSAATexture;
	}
	else
	{
		CurrentColorRenderTexture = nil;
	}
}

void FMetalManager::SetCurrentDepthStencilTarget(FMetalSurface* RenderSurface)
{
	if (RenderSurface)
	{
		// @todo urban: Handle stencil
		CurrentDepthRenderTexture = RenderSurface->Texture;
	}
	else
	{
		CurrentDepthRenderTexture = nil;
	}
}

void FMetalManager::UpdateContext()
{
	if (CurrentColorRenderTexture == PreviousColorRenderTexture && CurrentDepthRenderTexture == PreviousDepthRenderTexture)
	{
		return;
	}

	// if we are setting them to nothing, then this is probably end of frame, and we can't make a framebuffer
	// with nothng, so just abort this
	if (CurrentColorRenderTexture == nil && CurrentDepthRenderTexture == nil)
	{
		return;
	}

	// toss previous render pass object
	ReleaseObject(CurrentRenderPass);

	// make a new one
	CurrentRenderPass = [MTLRenderPassDescriptor renderPassDescriptor];
	[CurrentRenderPass retain];
	TRACK_OBJECT(CurrentRenderPass);
	// if we need to do queries, write to the ring buffer (we set the offset into the ring buffer per query)
	CurrentRenderPass.visibilityResultBuffer = QueryBuffer.Buffer;

	// @todo metal mrt: Set an attachment index properly
	uint32 AttachmentIndex = 0;

	MTLRenderPassAttachmentDescriptor* ColorAttachment = nil;
	MTLRenderPassAttachmentDescriptor* DepthAttachment = nil;
	// @todo urban: Handle stencil
	MTLRenderPassAttachmentDescriptor* StencilAttachment = nil;

	Pipeline.SampleCount = 0;
	if (CurrentColorRenderTexture)
	{
		ColorAttachment = [[MTLRenderPassAttachmentDescriptor alloc] init];

		if (CurrentMSAARenderTexture)
		{
			ColorAttachment.texture = CurrentMSAARenderTexture;
			[ColorAttachment setLoadAction:MTLLoadActionDontCare];
			[ColorAttachment setStoreAction:MTLStoreActionMultisampleResolve];
			[ColorAttachment setResolveTexture:CurrentColorRenderTexture];
			Pipeline.SampleCount = CurrentMSAARenderTexture.sampleCount;
		}
		else
		{
			ColorAttachment.texture = CurrentColorRenderTexture;
			[ColorAttachment setLoadAction:MTLLoadActionDontCare];
			[ColorAttachment setStoreAction:MTLStoreActionStore];
			Pipeline.SampleCount = 1;
		}

		[CurrentRenderPass.colorAttachments setObject:ColorAttachment atIndexedSubscript:AttachmentIndex];
		[ColorAttachment release];

		Pipeline.RenderTargets[AttachmentIndex].pixelFormat = CurrentColorRenderTexture.pixelFormat;
	}
	else
	{
		Pipeline.RenderTargets[AttachmentIndex].pixelFormat = MTLPixelFormatInvalid;
	}
	SET_HASH(OFFSET_RENDER_TARGET_FORMAT, NUMBITS_RENDER_TARGET_FORMAT, Pipeline.RenderTargets[AttachmentIndex].pixelFormat);
	
	if (CurrentDepthRenderTexture)
	{
		DepthAttachment = [[MTLRenderPassAttachmentDescriptor alloc] init];

		DepthAttachment.texture = CurrentDepthRenderTexture;
		[DepthAttachment setLoadAction:MTLLoadActionClear];
		[DepthAttachment setStoreAction:MTLStoreActionDontCare];
		[DepthAttachment setClearValue:MTLClearValueMakeDepth(0.0f)];

		Pipeline.DepthTargetFormat = CurrentDepthRenderTexture.pixelFormat;
		if (Pipeline.SampleCount == 0)
		{
			Pipeline.SampleCount = CurrentDepthRenderTexture.sampleCount;
		}

		CurrentRenderPass.depthAttachment = DepthAttachment;
		[DepthAttachment release];
	}
	else
	{
		Pipeline.DepthTargetFormat = MTLPixelFormatInvalid;
	}
	SET_HASH(OFFSET_DEPTH_TARGET_FORMAT, NUMBITS_DEPTH_TARGET_FORMAT, Pipeline.DepthTargetFormat);
	SET_HASH(OFFSET_SAMPLE_COUNT, NUMBITS_SAMPLE_COUNT, Pipeline.SampleCount);

	PreviousColorRenderTexture = CurrentColorRenderTexture;
	PreviousDepthRenderTexture = CurrentDepthRenderTexture;

	// commit pending commands on the old render target
	if (CurrentContext)
	{
		if (NumDrawCalls == 0)
		{
			NSLog(@"There were %d draw calls for an RT in frame %lld", NumDrawCalls, GFrameCounter);
		}

		if (true)//NumDrawCalls > 0)
		{
			[CurrentContext endEncoding];
			NumDrawCalls = 0;
		}

		ReleaseObject(CurrentContext);

		// if we are doing occlusion queries, we could use this method, along with a completion callback
		// to set a "render target complete" flag that the OQ code next frame would wait on
		// commit the buffer for this context
		[CurrentCommandBuffer commit];
		UNTRACK_OBJECT(CurrentCommandBuffer);
		[CurrentCommandBuffer release];

		// create the command buffer for this frame
		CurrentCommandBuffer = [CommandQueue commandBufferWithUnretainedReferences];
		[CurrentCommandBuffer retain];
		TRACK_OBJECT(CurrentCommandBuffer);

#if GPU_TIMINGS
		uint8 LocalTimerIndex = TimerIndex++;
#endif

		uint64 LocalCommandBufferIndex = CommandBufferIndex++;

		[CurrentCommandBuffer addCompletedHandler : ^ (id <MTLCommandBuffer> Buffer)
		{
#if GPU_TIMINGS
			// timer stuff
			GPUTimers[LocalTimerIndex] = FPlatformTime::Seconds();
#endif
			FMetalManager::Get()->SetCompletedCommandBufferIndex(LocalCommandBufferIndex);
		}];
	}
		
	// make a new render context to use to render to the framebuffer
	CurrentContext = [CurrentCommandBuffer renderCommandEncoderWithDescriptor:CurrentRenderPass];
	[CurrentContext retain];
	TRACK_OBJECT(CurrentContext);

	// make suire the rasterizer state is set the first time for each new encoder
	bFirstRasterizerState = true;
}



uint32 FRingBuffer::Allocate(uint32 Size, uint32 Alignment)
{
	if (Alignment == 0)
	{
		Alignment = DefaultAlignment;
	}

	// align the offset
	Offset = Align(Offset, Alignment);
	
	// wrap if needed
	if (Offset + Size > Buffer.length)
	{
// 		NSLog(@"Wrapping at frame %lld [size = %d]", GFrameCounter, (uint32)Buffer.length);
		Offset = 0;
	}
	
	// get current location
	uint32 ReturnOffset = Offset;
	
	// allocate
	Offset += Size;
	
	return ReturnOffset;
}

uint32 FMetalManager::AllocateFromRingBuffer(uint32 Size, uint32 Alignment)
{
	return RingBuffer.Allocate(Size, Alignment);
}

uint32 FMetalManager::AllocateFromQueryBuffer()
{
 	return QueryBuffer.Allocate(8, 0);
}




FORCEINLINE void SetResource(uint32 ShaderStage, uint32 BindIndex, FMetalSurface* RESTRICT Surface)
{
	check(Surface->Texture != nil);
	if (ShaderStage == METAL_SHADER_STAGE_PIXEL)
	{
		[FMetalManager::GetContext() setFragmentTexture:Surface->Texture atIndex:BindIndex];
	}
	else
	{
		[FMetalManager::GetContext() setVertexTexture:Surface->Texture atIndex : BindIndex];
	}
}

FORCEINLINE void SetResource(uint32 ShaderStage, uint32 BindIndex, FMetalSamplerState* RESTRICT SamplerState)
{
	check(SamplerState->State != nil);
	if (ShaderStage == METAL_SHADER_STAGE_PIXEL)
	{
		[FMetalManager::GetContext() setFragmentSamplerState:SamplerState->State atIndex:BindIndex];
	}
	else
	{
		[FMetalManager::GetContext() setFragmentSamplerState:SamplerState->State atIndex:BindIndex];
	}
}

template <typename MetalResourceType>
inline int32 SetShaderResourcesFromBuffer(uint32 ShaderStage, FMetalUniformBuffer* RESTRICT Buffer, const uint32 * RESTRICT ResourceMap, int32 BufferIndex)
{
	int32 NumSetCalls = 0;
	uint32 BufferOffset = ResourceMap[BufferIndex];
	if (BufferOffset > 0)
	{
		const uint32* RESTRICT ResourceInfos = &ResourceMap[BufferOffset];
		uint32 ResourceInfo = *ResourceInfos++;
		do
		{
			checkSlow(FRHIResourceTableEntry::GetUniformBufferIndex(ResourceInfo) == BufferIndex);
			const uint16 ResourceIndex = FRHIResourceTableEntry::GetResourceIndex(ResourceInfo);
			const uint8 BindIndex = FRHIResourceTableEntry::GetBindIndex(ResourceInfo);

			// @todo metal log: remove
			NSLog(@"Binding a resource at BindIndex %d, Resource Index %d", BindIndex, ResourceIndex);

			// todo: could coalesce adjacent bound resources.
			MetalResourceType* ResourcePtr = (MetalResourceType*)Buffer->RawResourceTable[ResourceIndex];
			SetResource(ShaderStage, BindIndex, ResourcePtr);

			NumSetCalls++;
			ResourceInfo = *ResourceInfos++;
		} while (FRHIResourceTableEntry::GetUniformBufferIndex(ResourceInfo) == BufferIndex);
	}

	return NumSetCalls;
}

template <class ShaderType>
void SetResourcesFromTables(ShaderType Shader, uint32 ShaderStage, uint32 ResourceTableFrameCounter)
{
	checkSlow(Shader);

	// @todo metal log: remove
	NSLog(@"Resource Bits %x, DirtyBits: %llx", Shader->Bindings.ShaderResourceTable.ResourceTableBits, Shader->DirtyUniformBuffers);

	// Mask the dirty bits by those buffers from which the shader has bound resources.
	uint32 DirtyBits = Shader->Bindings.ShaderResourceTable.ResourceTableBits & Shader->DirtyUniformBuffers;
	uint32 NumSetCalls = 0;
	while (DirtyBits)
	{
		// Scan for the lowest set bit, compute its index, clear it in the set of dirty bits.
		const uint32 LowestBitMask = (DirtyBits)& (-(int32)DirtyBits);
		const int32 BufferIndex = FMath::FloorLog2(LowestBitMask); // todo: This has a branch on zero, we know it could never be zero...
		DirtyBits ^= LowestBitMask;
		FMetalUniformBuffer* Buffer = (FMetalUniformBuffer*)Shader->BoundUniformBuffers[BufferIndex].GetReference();
		check(Buffer);
		check(BufferIndex < Shader->Bindings.ShaderResourceTable.ResourceTableLayoutHashes.Num());
		check(Buffer->GetLayout().GetHash() == Shader->Bindings.ShaderResourceTable.ResourceTableLayoutHashes[BufferIndex]);
		Buffer->CacheResources(ResourceTableFrameCounter);

		// @todo metal log: remove
		NSLog(@"Setting BufferIndex %d", BufferIndex);

		// todo: could make this two pass: gather then set
		NumSetCalls += SetShaderResourcesFromBuffer<FMetalSurface>(ShaderStage, Buffer, Shader->Bindings.ShaderResourceTable.ShaderResourceViewMap.GetData(), BufferIndex);
		NumSetCalls += SetShaderResourcesFromBuffer<FMetalSamplerState>(ShaderStage, Buffer, Shader->Bindings.ShaderResourceTable.SamplerMap.GetData(), BufferIndex);
	}
	Shader->DirtyUniformBuffers = 0;
//	SetTextureInTableCalls += NumSetCalls;
}

void FMetalManager::CommitGraphicsResourceTables()
{
	uint32 Start = FPlatformTime::Cycles();

// 	GRHICommandList.Verify();

	check(CurrentBoundShaderState);

	SetResourcesFromTables(CurrentBoundShaderState->VertexShader, METAL_SHADER_STAGE_VERTEX, ResourceTableFrameCounter);
	if (IsValidRef(CurrentBoundShaderState->PixelShader))
	{
		SetResourcesFromTables(CurrentBoundShaderState->PixelShader, METAL_SHADER_STAGE_PIXEL, ResourceTableFrameCounter);
	}

//	CommitResourceTableCycles += (FPlatformTime::Cycles() - Start);
}

void FMetalManager::CommitNonComputeShaderConstants()
{
#if NO_MEMORY
	return;
#endif
	ShaderParameters[METAL_SHADER_STAGE_VERTEX].CommitPackedUniformBuffers(CurrentBoundShaderState, METAL_SHADER_STAGE_VERTEX, CurrentBoundShaderState->VertexShader->BoundUniformBuffers, CurrentBoundShaderState->VertexShader->UniformBuffersCopyInfo);
	ShaderParameters[METAL_SHADER_STAGE_VERTEX].CommitPackedGlobals(METAL_SHADER_STAGE_VERTEX, CurrentBoundShaderState->VertexShader->Bindings);
	
	if (IsValidRef(CurrentBoundShaderState->PixelShader))
	{
		ShaderParameters[METAL_SHADER_STAGE_PIXEL].CommitPackedUniformBuffers(CurrentBoundShaderState, METAL_SHADER_STAGE_PIXEL, CurrentBoundShaderState->PixelShader->BoundUniformBuffers, CurrentBoundShaderState->PixelShader->UniformBuffersCopyInfo);
		ShaderParameters[METAL_SHADER_STAGE_PIXEL].CommitPackedGlobals(METAL_SHADER_STAGE_PIXEL, CurrentBoundShaderState->PixelShader->Bindings);
	}
}

bool FMetalManager::WaitForCommandBufferComplete(uint64 IndexToWaitFor, double Timeout)
{
	// don't track a block if not needed
	if (CompletedCommandBufferIndex >= IndexToWaitFor)
	{
		return true;
	}

	// if we don't want to wait, then we have failed
	if (Timeout == 0.0)
	{
		return false;
	}

	// if we block until it's ready, loop here until it is
	SCOPE_CYCLE_COUNTER(STAT_RenderQueryResultTime);
	uint32 IdleStart = FPlatformTime::Cycles();
	double StartTime = FPlatformTime::Seconds();

	while (CompletedCommandBufferIndex < IndexToWaitFor)
	{
		FPlatformProcess::Sleep(0);

		// look for gpu stuck/crashed
		if ((FPlatformTime::Seconds() - StartTime) > Timeout)
		{
			UE_LOG(LogRHI, Log, TEXT("Timed out while waiting for GPU to catch up on occlusion/timer results. (%.1f s)"), Timeout);
			return false;
		}
	}

	// track idle time blocking on GPU
	GRenderThreadIdle[ERenderThreadIdleTypes::WaitingForGPUQuery] += FPlatformTime::Cycles() - IdleStart;
	GRenderThreadNumIdle[ERenderThreadIdleTypes::WaitingForGPUQuery]++;

	return true;
}


void FMetalManager::SetRasterizerState(const FRasterizerStateInitializerRHI& State)
{
	if (bFirstRasterizerState)
	{
		[CurrentContext setFrontFacingWinding:MTLWindingCounterClockwise];
	}

	if (bFirstRasterizerState || ShadowRasterizerState.CullMode != State.CullMode)
	{
		[CurrentContext setCullMode:TranslateCullMode(State.CullMode)];
		ShadowRasterizerState.CullMode = State.CullMode;
	}

	// @todo urban: Shoud the clamp be FLT_MAX or 0 for "don't bother"
	if (bFirstRasterizerState || ShadowRasterizerState.DepthBias != State.DepthBias || ShadowRasterizerState.SlopeScaleDepthBias != State.SlopeScaleDepthBias)
	{
		[CurrentContext setDepthBias:State.DepthBias slopeScale:State.SlopeScaleDepthBias clamp : FLT_MAX];
		ShadowRasterizerState.DepthBias = State.DepthBias;
		ShadowRasterizerState.SlopeScaleDepthBias = State.SlopeScaleDepthBias;
	}

	// @todo urban: Would we ever need this in a shipping app?
#if !UE_BUILD_SHIPPING
	if (bFirstRasterizerState || ShadowRasterizerState.FillMode != State.FillMode)
	{
		[CurrentContext setTriangleFillMode:TranslateFillMode(State.FillMode)];
		ShadowRasterizerState.FillMode = State.FillMode;
	}
#endif

	bFirstRasterizerState = false;
}
