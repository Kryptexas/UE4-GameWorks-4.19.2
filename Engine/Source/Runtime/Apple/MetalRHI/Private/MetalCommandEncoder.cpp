// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalCommandEncoder.cpp: Metal command encoder wrapper.
=============================================================================*/

#include "MetalRHIPrivate.h"

#include "MetalCommandEncoder.h"
#include "MetalCommandBuffer.h"
#include "MetalComputeCommandEncoder.h"
#include "MetalRenderCommandEncoder.h"
#include "MetalProfiler.h"

const uint32 EncoderRingBufferSize = 1024 * 1024;

#if METAL_DEBUG_OPTIONS
extern int32 GMetalBufferScribble;
#endif

#pragma mark - Public C++ Boilerplate -

FMetalCommandEncoder::FMetalCommandEncoder(FMetalCommandList& CmdList)
: CommandList(CmdList)
, RenderPassDesc(nil)
, CommandBuffer(nil)
, RenderCommandEncoder(nil)
, ComputeCommandEncoder(nil)
, BlitCommandEncoder(nil)
, DebugGroups([NSMutableArray new])
{
	FMemory::Memzero(ShaderBuffers);

	RingBuffer = MakeShareable(new FRingBuffer(CommandList.GetCommandQueue().GetDevice(), EncoderRingBufferSize, BufferOffsetAlignment));
}

FMetalCommandEncoder::~FMetalCommandEncoder(void)
{
	if(CommandBuffer)
	{
		EndEncoding();
		CommitCommandBuffer(false);
	}
	
	check(!IsRenderCommandEncoderActive());
	check(!IsComputeCommandEncoderActive());
	check(!IsBlitCommandEncoderActive());
	
	[RenderPassDesc release];
	RenderPassDesc = nil;

	if(DebugGroups)
	{
		[DebugGroups release];
	}
}

void FMetalCommandEncoder::Reset(void)
{
    check(!CommandBuffer);
    check(IsRenderCommandEncoderActive() == false
          && IsComputeCommandEncoderActive() == false
          && IsBlitCommandEncoderActive() == false);
	
	if(RenderPassDesc)
	{
		UNTRACK_OBJECT(STAT_MetalRenderPassDescriptorCount, RenderPassDesc);
		[RenderPassDesc release];
		RenderPassDesc = nil;
	}
    
    FMemory::Memzero(ShaderBuffers);
	
	[DebugGroups removeAllObjects];
}

#pragma mark - Public Command Buffer Mutators -

void FMetalCommandEncoder::StartCommandBuffer(void)
{
	check(!CommandBuffer);
	check(IsRenderCommandEncoderActive() == false
          && IsComputeCommandEncoderActive() == false
          && IsBlitCommandEncoderActive() == false);

	CommandBuffer = CommandList.GetCommandQueue().CreateCommandBuffer();
	TRACK_OBJECT(STAT_MetalCommandBufferCount, CommandBuffer);
	CommandBufferPtr = TSharedPtr<MTLCommandBufferRef, ESPMode::ThreadSafe>(new MTLCommandBufferRef([CommandBuffer retain], dispatch_semaphore_create(0)));
	
	if ([DebugGroups count] > 0)
	{
		[CommandBuffer setLabel:[DebugGroups lastObject]];
	}
}
	
void FMetalCommandEncoder::CommitCommandBuffer(uint32 const Flags)
{
	check(CommandBuffer);
	check(IsRenderCommandEncoderActive() == false
          && IsComputeCommandEncoderActive() == false
          && IsBlitCommandEncoderActive() == false);

	if(CommandBuffer.label == nil && [DebugGroups count] > 0)
	{
		[CommandBuffer setLabel:[DebugGroups lastObject]];
	}
	
	bool const bWait = (Flags & EMetalSubmitFlagsWaitOnCommandBuffer);
	if (!(Flags & EMetalSubmitFlagsBreakCommandBuffer))
	{
		uint32 RingBufferOffset = RingBuffer->GetOffset();
		uint32 StartOffset = RingBuffer->LastWritten;
		RingBuffer->LastWritten = RingBufferOffset;
		id<MTLBuffer> CurrentRingBuffer = RingBuffer->Buffer;
		TWeakPtr<FRingBuffer, ESPMode::ThreadSafe>* WeakRingBufferRef = new TWeakPtr<FRingBuffer, ESPMode::ThreadSafe>(RingBuffer.ToSharedRef());
		[CommandBuffer addCompletedHandler : ^ (id <MTLCommandBuffer> Buffer)
		{
			TSharedPtr<FRingBuffer, ESPMode::ThreadSafe> CmdBufferRingBuffer = WeakRingBufferRef->Pin();
			if(CmdBufferRingBuffer.IsValid() && CmdBufferRingBuffer->Buffer == CurrentRingBuffer)
			{
#if METAL_DEBUG_OPTIONS
				if (GMetalBufferScribble && StartOffset != RingBufferOffset)
				{
					 if (StartOffset < RingBufferOffset)
					 {
						FMemory::Memset(((uint8*)CmdBufferRingBuffer->Buffer.contents) + StartOffset, 0xCD, RingBufferOffset - StartOffset);
					 }
					 else
					 {
						uint32 TrailLen = CmdBufferRingBuffer->Buffer.length - StartOffset;
						FMemory::Memset(((uint8*)CmdBufferRingBuffer->Buffer.contents) + StartOffset, 0xCD, TrailLen);
						FMemory::Memset(((uint8*)CmdBufferRingBuffer->Buffer.contents), 0xCD, RingBufferOffset);
					 }
				}
#endif
				CmdBufferRingBuffer->SetLastRead(RingBufferOffset);
			}
			delete WeakRingBufferRef;
		}];
	}
	
	TSharedPtr<MTLCommandBufferRef, ESPMode::ThreadSafe>* CmdBufRef = new TSharedPtr<MTLCommandBufferRef, ESPMode::ThreadSafe>(CommandBufferPtr);
	[CommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull CompletedBuffer)
	{
		if(CommandBuffer.status == MTLCommandBufferStatusError)
		{
			FMetalCommandList::HandleMetalCommandBufferFailure(CommandBuffer);
		}
		delete CmdBufRef;
	}];

	CommandList.Commit(CommandBuffer, bWait);
	
	CommandBufferPtr.Reset();
	CommandBuffer = nil;
	if (Flags & EMetalSubmitFlagsCreateCommandBuffer)
	{
		StartCommandBuffer();
		check(CommandBuffer);
	}
}

#pragma mark - Public Command Encoder Accessors -
	
bool FMetalCommandEncoder::IsRenderCommandEncoderActive(void) const
{
	return RenderCommandEncoder != nil;
}

bool FMetalCommandEncoder::IsComputeCommandEncoderActive(void) const
{
	return ComputeCommandEncoder != nil;
}

bool FMetalCommandEncoder::IsBlitCommandEncoderActive(void) const
{
	return BlitCommandEncoder != nil;
}

bool FMetalCommandEncoder::IsImmediate(void) const
{
	return CommandList.IsImmediate();
}

bool FMetalCommandEncoder::IsRenderPassDescriptorValid(void) const
{
	return (RenderPassDesc != nil);
}

id<MTLRenderCommandEncoder> FMetalCommandEncoder::GetRenderCommandEncoder(void) const
{
	return RenderCommandEncoder;
}

id<MTLComputeCommandEncoder> FMetalCommandEncoder::GetComputeCommandEncoder(void) const
{
	return ComputeCommandEncoder;
}

id<MTLBlitCommandEncoder> FMetalCommandEncoder::GetBlitCommandEncoder(void) const
{
	return BlitCommandEncoder;
}
	
#pragma mark - Public Command Encoder Mutators -

void FMetalCommandEncoder::BeginRenderCommandEncoding(void)
{
	check(RenderPassDesc);
	check(CommandBuffer);
	check(IsRenderCommandEncoderActive() == false && IsComputeCommandEncoderActive() == false && IsBlitCommandEncoderActive() == false);
    
	RenderCommandEncoder = [[CommandBuffer renderCommandEncoderWithDescriptor:RenderPassDesc] retain];
	
	if(GEmitDrawEvents)
	{
		NSString* Label = [DebugGroups count] > 0 ? [DebugGroups lastObject] : (NSString*)CFSTR("InitialPass");
		[RenderCommandEncoder setLabel:Label];
		
		if([DebugGroups count])
		{
			for (NSString* Group in DebugGroups)
			{
				[RenderCommandEncoder pushDebugGroup:Group];
			}
		}
    }
}

void FMetalCommandEncoder::BeginComputeCommandEncoding(void)
{
	check(CommandBuffer);
	check(IsRenderCommandEncoderActive() == false && IsComputeCommandEncoderActive() == false && IsBlitCommandEncoderActive() == false);
	
	ComputeCommandEncoder = [[CommandBuffer computeCommandEncoder] retain];
	
	if(GEmitDrawEvents)
	{
		NSString* Label = [DebugGroups count] > 0 ? [DebugGroups lastObject] : (NSString*)CFSTR("InitialPass");
		[ComputeCommandEncoder setLabel:Label];
		
		if([DebugGroups count])
		{
			for (NSString* Group in DebugGroups)
			{
				[ComputeCommandEncoder pushDebugGroup:Group];
			}
		}
	}
}

void FMetalCommandEncoder::BeginBlitCommandEncoding(void)
{
	check(CommandBuffer);
	check(IsRenderCommandEncoderActive() == false && IsComputeCommandEncoderActive() == false && IsBlitCommandEncoderActive() == false);
	
	BlitCommandEncoder = [[CommandBuffer blitCommandEncoder] retain];
	
	if(GEmitDrawEvents)
	{
		NSString* Label = [DebugGroups count] > 0 ? [DebugGroups lastObject] : (NSString*)CFSTR("InitialPass");
		[BlitCommandEncoder setLabel:Label];
		
		if([DebugGroups count])
		{
			for (NSString* Group in DebugGroups)
			{
				[BlitCommandEncoder pushDebugGroup:Group];
			}
		}
	}
}

void FMetalCommandEncoder::EndEncoding(void)
{
	@autoreleasepool
	{
		if(IsRenderCommandEncoderActive())
		{
			[RenderCommandEncoder endEncoding];
			[RenderCommandEncoder release];
			RenderCommandEncoder = nil;
		}
		else if(IsComputeCommandEncoderActive())
		{
			[ComputeCommandEncoder endEncoding];
			[ComputeCommandEncoder release];
			ComputeCommandEncoder = nil;
		}
		else if(IsBlitCommandEncoderActive())
		{
			[BlitCommandEncoder endEncoding];
			[BlitCommandEncoder release];
			BlitCommandEncoder = nil;
		}
	}
    
    FMemory::Memzero(ShaderBuffers);
}

void FMetalCommandEncoder::InsertCommandBufferFence(FMetalCommandBufferFence& Fence, MTLCommandBufferHandler Handler)
{
	check(CommandBuffer);
	check(CommandBufferPtr.IsValid());
	
	Fence.CommandBufferRef = CommandBufferPtr;
	
	dispatch_semaphore_t Semaphore = *(CommandBufferPtr->Semaphore);
	[CommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull CompletedBuffer)
	{
		if (Handler)
		{
			Handler(CompletedBuffer);
		}
		dispatch_semaphore_signal(Semaphore);
	}];
}

#pragma mark - Public Debug Support -

void FMetalCommandEncoder::InsertDebugSignpost(NSString* const String)
{
	if (RenderCommandEncoder)
	{
		[RenderCommandEncoder insertDebugSignpost:String];
	}
	if (ComputeCommandEncoder)
	{
		[ComputeCommandEncoder insertDebugSignpost:String];
	}
	if (BlitCommandEncoder)
	{
		[BlitCommandEncoder insertDebugSignpost:String];
	}
}

void FMetalCommandEncoder::PushDebugGroup(NSString* const String)
{
	if (String)
	{
		[DebugGroups addObject:String];
		if (RenderCommandEncoder)
		{
			[RenderCommandEncoder pushDebugGroup:String];
		}
		else if (ComputeCommandEncoder)
		{
			[ComputeCommandEncoder pushDebugGroup:String];
		}
		else if (BlitCommandEncoder)
		{
			[BlitCommandEncoder pushDebugGroup:String];
		}
	}
}

void FMetalCommandEncoder::PopDebugGroup(void)
{
	if (DebugGroups.count > 0)
	{
		[DebugGroups removeLastObject];
		if (RenderCommandEncoder)
		{
			[RenderCommandEncoder popDebugGroup];
		}
		else if (ComputeCommandEncoder)
		{
			[ComputeCommandEncoder popDebugGroup];
		}
		else if (BlitCommandEncoder)
		{
			[BlitCommandEncoder popDebugGroup];
		}
	}
}
	
#pragma mark - Public Render State Mutators -

void FMetalCommandEncoder::SetRenderPassDescriptor(MTLRenderPassDescriptor* const RenderPass)
{
	check(IsRenderCommandEncoderActive() == false && IsComputeCommandEncoderActive() == false && IsBlitCommandEncoderActive() == false);
	check(RenderPass);
	
	if(RenderPass != RenderPassDesc)
	{
		[RenderPass retain];
		if(RenderPassDesc)
		{
			GetMetalDeviceContext().ReleaseObject(RenderPassDesc);
		}
		RenderPassDesc = RenderPass;
	}
	check(RenderPassDesc);
	
	FMemory::Memzero(ShaderBuffers);
}

void FMetalCommandEncoder::SetRenderPipelineState(id<MTLRenderPipelineState> PipelineState, MTLRenderPipelineReflection* Reflection, NSString* VertexSource, NSString* FragmentSource)
{
	check (RenderCommandEncoder);
	{
		[RenderCommandEncoder setRenderPipelineState:PipelineState];
        METAL_SET_RENDER_REFLECTION(RenderCommandEncoder, Reflection, VertexSource, FragmentSource);
	}
}

void FMetalCommandEncoder::SetViewport(MTLViewport const& InViewport)
{
    check (RenderCommandEncoder);
	{
		[RenderCommandEncoder setViewport:InViewport];
	}
}

void FMetalCommandEncoder::SetFrontFacingWinding(MTLWinding const InFrontFacingWinding)
{
    check (RenderCommandEncoder);
	{
		[RenderCommandEncoder setFrontFacingWinding:InFrontFacingWinding];
	}
}

void FMetalCommandEncoder::SetCullMode(MTLCullMode const InCullMode)
{
    check (RenderCommandEncoder);
	{
		[RenderCommandEncoder setCullMode:InCullMode];
	}
}

void FMetalCommandEncoder::SetDepthBias(float const InDepthBias, float const InSlopeScale, float const InClamp)
{
    check (RenderCommandEncoder);
	{
		[RenderCommandEncoder setDepthBias:InDepthBias slopeScale:InSlopeScale clamp:InClamp];
	}
}

void FMetalCommandEncoder::SetScissorRect(MTLScissorRect const& Rect)
{
    check(RenderCommandEncoder);
	{
		[RenderCommandEncoder setScissorRect:Rect];
	}
}

void FMetalCommandEncoder::SetTriangleFillMode(MTLTriangleFillMode const InFillMode)
{
    check(RenderCommandEncoder);
	{
		[RenderCommandEncoder setTriangleFillMode:InFillMode];
	}
}

void FMetalCommandEncoder::SetBlendColor(float const Red, float const Green, float const Blue, float const Alpha)
{
	check(RenderCommandEncoder);
	{
		[RenderCommandEncoder setBlendColorRed:Red green:Green blue:Blue alpha:Alpha];
	}
}

void FMetalCommandEncoder::SetDepthStencilState(id<MTLDepthStencilState> const InDepthStencilState)
{
    check (RenderCommandEncoder);
	{
		[RenderCommandEncoder setDepthStencilState:InDepthStencilState];
	}
}

void FMetalCommandEncoder::SetStencilReferenceValue(uint32 const ReferenceValue)
{
    check (RenderCommandEncoder);
	{
		[RenderCommandEncoder setStencilReferenceValue:ReferenceValue];
	}
}

void FMetalCommandEncoder::SetVisibilityResultMode(MTLVisibilityResultMode const Mode, NSUInteger const Offset)
{
    check (RenderCommandEncoder);
	{
		check(Mode == MTLVisibilityResultModeDisabled || RenderPassDesc.visibilityResultBuffer);
		[RenderCommandEncoder setVisibilityResultMode: Mode offset:Offset];
	}
}
	
#pragma mark - Public Shader Resource Mutators -

void FMetalCommandEncoder::SetShaderBuffer(MTLFunctionType const FunctionType, id<MTLBuffer> Buffer, NSUInteger const Offset, NSUInteger const Length, NSUInteger index)
{
	check(index < ML_MaxBuffers);
    if(GetMetalDeviceContext().SupportsFeature(EMetalFeaturesSetBufferOffset) && Buffer && (ShaderBuffers[FunctionType].Bound & (1 << index)) && ShaderBuffers[FunctionType].Buffers[index] == Buffer)
    {
		SetShaderBufferOffset(FunctionType, Offset, Length, index);
    }
    else
    {
		if(Buffer)
		{
			ShaderBuffers[FunctionType].Bound |= (1 << index);
		}
		else
		{
			ShaderBuffers[FunctionType].Bound &= ~(1 << index);
		}
		ShaderBuffers[FunctionType].Buffers[index] = Buffer;
		ShaderBuffers[FunctionType].Bytes[index] = nil;
		ShaderBuffers[FunctionType].Offsets[index] = Offset;
		ShaderBuffers[FunctionType].Lengths[index] = Length;
		
		SetShaderBufferInternal(FunctionType, index);
    }
}

void FMetalCommandEncoder::SetShaderData(MTLFunctionType const FunctionType, NSData* Data, NSUInteger const Offset, NSUInteger const Index)
{
	check(Index < ML_MaxBuffers);
	
#if METAL_DEBUG_OPTIONS
	if (CommandList.GetCommandQueue().GetRuntimeDebuggingLevel() > EMetalDebugLevelResetOnBind)
	{
		SetShaderBuffer(FunctionType, nil, 0, 0, Index);
	}
#endif
	
	if(Data)
	{
		ShaderBuffers[FunctionType].Bound |= (1 << Index);
	}
	else
	{
		ShaderBuffers[FunctionType].Bound &= ~(1 << Index);
	}
	
	ShaderBuffers[FunctionType].Buffers[Index] = nil;
	ShaderBuffers[FunctionType].Bytes[Index] = Data;
	ShaderBuffers[FunctionType].Offsets[Index] = Offset;
	ShaderBuffers[FunctionType].Lengths[Index] = Data ? (Data.length - Offset) : 0;
	
	SetShaderBufferInternal(FunctionType, Index);
}

void FMetalCommandEncoder::SetShaderBytes(MTLFunctionType const FunctionType, uint8 const* Bytes, NSUInteger const Length, NSUInteger const Index)
{
	check(Index < ML_MaxBuffers);
	
#if METAL_DEBUG_OPTIONS
	if (CommandList.GetCommandQueue().GetRuntimeDebuggingLevel() > EMetalDebugLevelResetOnBind)
	{
		SetShaderBuffer(FunctionType, nil, 0, 0, Index);
	}
#endif
	
	if(Bytes && Length)
	{
		ShaderBuffers[FunctionType].Bound |= (1 << Index);

		uint32 Offset = RingBuffer->Allocate(Length, BufferOffsetAlignment);
		id<MTLBuffer> Buffer = RingBuffer->Buffer;
		
		ShaderBuffers[FunctionType].Buffers[Index] = Buffer;
		ShaderBuffers[FunctionType].Bytes[Index] = nil;
		ShaderBuffers[FunctionType].Offsets[Index] = Offset;
		ShaderBuffers[FunctionType].Lengths[Index] = Length;
		
		FMemory::Memcpy(((uint8*)[Buffer contents]) + Offset, Bytes, Length);
	}
	else
	{
		ShaderBuffers[FunctionType].Bound &= ~(1 << Index);
		
		ShaderBuffers[FunctionType].Buffers[Index] = nil;
		ShaderBuffers[FunctionType].Bytes[Index] = nil;
		ShaderBuffers[FunctionType].Offsets[Index] = 0;
		ShaderBuffers[FunctionType].Lengths[Index] = 0;
	}
	
	SetShaderBufferInternal(FunctionType, Index);
}

void FMetalCommandEncoder::SetShaderBufferOffset(MTLFunctionType FunctionType, NSUInteger const Offset, NSUInteger const Length, NSUInteger const index)
{
	check(index < ML_MaxBuffers);
    checkf(ShaderBuffers[FunctionType].Buffers[index] && (ShaderBuffers[FunctionType].Bound & (1 << index)), TEXT("Buffer must already be bound"));
	check(GetMetalDeviceContext().SupportsFeature(EMetalFeaturesSetBufferOffset));
#if METAL_API_1_1
	ShaderBuffers[FunctionType].Offsets[index] = Offset;
	ShaderBuffers[FunctionType].Lengths[index] = Length;
	switch (FunctionType)
	{
		case MTLFunctionTypeVertex:
			check (RenderCommandEncoder);
			[RenderCommandEncoder setVertexBufferOffset:Offset atIndex:index];
			break;
		case MTLFunctionTypeFragment:
			check(RenderCommandEncoder);
			[RenderCommandEncoder setFragmentBufferOffset:Offset atIndex:index];
			break;
		case MTLFunctionTypeKernel:
			check (ComputeCommandEncoder);
			[ComputeCommandEncoder setBufferOffset:Offset atIndex:index];
			break;
		default:
			check(false);
			break;
	}
#endif
}

void FMetalCommandEncoder::SetShaderTexture(MTLFunctionType FunctionType, id<MTLTexture> Texture, NSUInteger index)
{
	check(index < ML_MaxTextures);
	switch (FunctionType)
	{
		case MTLFunctionTypeVertex:
			check (RenderCommandEncoder);
			[RenderCommandEncoder setVertexTexture:Texture atIndex:index];
			break;
		case MTLFunctionTypeFragment:
			check(RenderCommandEncoder);
			[RenderCommandEncoder setFragmentTexture:Texture atIndex:index];
			break;
		case MTLFunctionTypeKernel:
			check (ComputeCommandEncoder);
			[ComputeCommandEncoder setTexture:Texture atIndex:index];
			break;
		default:
			check(false);
			break;
	}
}

void FMetalCommandEncoder::SetShaderSamplerState(MTLFunctionType FunctionType, id<MTLSamplerState> Sampler, NSUInteger index)
{
	check(index < ML_MaxSamplers);
	switch (FunctionType)
	{
		case MTLFunctionTypeVertex:
       		check (RenderCommandEncoder);
			[RenderCommandEncoder setVertexSamplerState:Sampler atIndex:index];
			break;
		case MTLFunctionTypeFragment:
			check (RenderCommandEncoder);
			[RenderCommandEncoder setFragmentSamplerState:Sampler atIndex:index];
			break;
		case MTLFunctionTypeKernel:
			check (ComputeCommandEncoder);
			[ComputeCommandEncoder setSamplerState:Sampler atIndex:index];
			break;
		default:
			check(false);
			break;
	}
}

void FMetalCommandEncoder::SetShaderSideTable(MTLFunctionType const FunctionType, NSUInteger const Index)
{
	if (Index < ML_MaxBuffers)
	{
		SetShaderBytes(FunctionType, (uint8*)ShaderBuffers[FunctionType].Lengths, sizeof(ShaderBuffers[FunctionType].Lengths), Index);
	}
}

#pragma mark - Public Compute State Mutators -

void FMetalCommandEncoder::SetComputePipelineState(id<MTLComputePipelineState> const State, MTLComputePipelineReflection* Reflection, NSString* Source)
{
	check (ComputeCommandEncoder);
	{
        METAL_SET_COMPUTE_REFLECTION(ComputeCommandEncoder, Reflection, Source);
		[ComputeCommandEncoder setComputePipelineState:State];
	}
}

#pragma mark - Public Ring-Buffer Accessor -
	
TSharedRef<FRingBuffer, ESPMode::ThreadSafe> FMetalCommandEncoder::GetRingBuffer(void) const
{
	return RingBuffer.ToSharedRef();
}

#pragma mark - Private Functions -

void FMetalCommandEncoder::SetShaderBufferInternal(MTLFunctionType Function, uint32 Index)
{
	id<MTLBuffer> Buffer = ShaderBuffers[Function].Buffers[Index];
	NSUInteger Offset = ShaderBuffers[Function].Offsets[Index];
	
	if (!Buffer && ShaderBuffers[Function].Bytes[Index] && !CommandList.GetCommandQueue().SupportsFeature(EMetalFeaturesSetBytes))
	{
		uint8 const* Bytes = (((uint8 const*)ShaderBuffers[Function].Bytes[Index].bytes) + ShaderBuffers[Function].Offsets[Index]);
		uint32 Len = ShaderBuffers[Function].Bytes[Index].length - ShaderBuffers[Function].Offsets[Index];
		
		Offset = RingBuffer->Allocate(Len, BufferOffsetAlignment);
		Buffer = RingBuffer->Buffer;
		
		FMemory::Memcpy(((uint8*)[Buffer contents]) + Offset, Bytes, Len);
	}
	
	if (Buffer != nil)
	{
		switch (Function)
		{
			case MTLFunctionTypeVertex:
				ShaderBuffers[Function].Bound |= (1 << Index);
				check(RenderCommandEncoder);
				[RenderCommandEncoder setVertexBuffer:Buffer offset:Offset atIndex:Index];
				break;
			case MTLFunctionTypeFragment:
				ShaderBuffers[Function].Bound |= (1 << Index);
				check(RenderCommandEncoder);
				[RenderCommandEncoder setFragmentBuffer:Buffer offset:Offset atIndex:Index];
				break;
			case MTLFunctionTypeKernel:
				ShaderBuffers[Function].Bound |= (1 << Index);
				check(ComputeCommandEncoder);
				[ComputeCommandEncoder setBuffer:Buffer offset:Offset atIndex:Index];
				break;
			default:
				check(false);
				break;
		}
	}
#if METAL_API_1_1
	else if (ShaderBuffers[Function].Bytes[Index] != nil && CommandList.GetCommandQueue().SupportsFeature(EMetalFeaturesSetBytes))
	{
		uint8 const* Bytes = (((uint8 const*)ShaderBuffers[Function].Bytes[Index].bytes) + ShaderBuffers[Function].Offsets[Index]);
		uint32 Len = ShaderBuffers[Function].Bytes[Index].length - ShaderBuffers[Function].Offsets[Index];
		
		if (CommandList.GetCommandQueue().SupportsFeature(EMetalFeaturesSetBytes))
		{
			switch (Function)
			{
				case MTLFunctionTypeVertex:
					ShaderBuffers[Function].Bound |= (1 << Index);
					check(RenderCommandEncoder);
					[RenderCommandEncoder setVertexBytes:Bytes length:Len atIndex:Index];
					break;
				case MTLFunctionTypeFragment:
					ShaderBuffers[Function].Bound |= (1 << Index);
					check(RenderCommandEncoder);
					[RenderCommandEncoder setFragmentBytes:Bytes length:Len atIndex:Index];
					break;
				case MTLFunctionTypeKernel:
					ShaderBuffers[Function].Bound |= (1 << Index);
					check(ComputeCommandEncoder);
					[ComputeCommandEncoder setBytes:Bytes length:Len atIndex:Index];
					break;
				default:
					check(false);
					break;
			}
		}
#endif
	}
}
