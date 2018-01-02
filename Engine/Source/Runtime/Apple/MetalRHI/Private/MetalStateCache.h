// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MetalRHIPrivate.h"
#include "MetalCommandEncoder.h"
#include "MetalPipeline.h"

class FShaderCacheState;

enum EMetalPipelineFlags
{
	EMetalPipelineFlagPipelineState = 1 << 0,
    EMetalPipelineFlagVertexBuffers = 1 << 1,
    EMetalPipelineFlagPixelBuffers = 1 << 2,
    EMetalPipelineFlagDomainBuffers = 1 << 3,
    EMetalPipelineFlagComputeBuffers = 1 << 4,
    EMetalPipelineFlagComputeShader = 1 << 5,
    EMetalPipelineFlagRasterMask = 0xF,
    EMetalPipelineFlagComputeMask = 0x30,
    EMetalPipelineFlagMask = 0x3F
};

enum EMetalRenderFlags
{
    EMetalRenderFlagViewport = 1 << 0,
    EMetalRenderFlagFrontFacingWinding = 1 << 1,
    EMetalRenderFlagCullMode = 1 << 2,
    EMetalRenderFlagDepthBias = 1 << 3,
    EMetalRenderFlagScissorRect = 1 << 4,
    EMetalRenderFlagTriangleFillMode = 1 << 5,
    EMetalRenderFlagBlendColor = 1 << 6,
    EMetalRenderFlagDepthStencilState = 1 << 7,
    EMetalRenderFlagStencilReferenceValue = 1 << 8,
    EMetalRenderFlagVisibilityResultMode = 1 << 9,
    EMetalRenderFlagMask = 0x1FF
};

class FMetalStateCache
{
public:
	FMetalStateCache(bool const bInImmediate);
	~FMetalStateCache();
	
	/** Reset cached state for reuse */
	void Reset(void);

	void SetScissorRect(bool const bEnable, MTLScissorRect const& Rect);
	void SetBlendFactor(FLinearColor const& InBlendFactor);
	void SetStencilRef(uint32 const InStencilRef);
	void SetComputeShader(FMetalComputeShader* InComputeShader);
	bool SetRenderTargetsInfo(FRHISetRenderTargetsInfo const& InRenderTargets, id<MTLBuffer> const QueryBuffer, bool const bRestart);
	void InvalidateRenderTargets(void);
	void SetRenderTargetsActive(bool const bActive);
	void SetViewport(const MTLViewport& InViewport);
	void SetViewports(const MTLViewport InViewport[], uint32 Count);
	void SetVertexStream(uint32 const Index, id<MTLBuffer> Buffer, FMetalBufferData* Bytes, uint32 const Offset, uint32 const Length);
	void SetGraphicsPipelineState(FMetalGraphicsPipelineState* State);
	void SetIndexType(EMetalIndexType IndexType);
	void BindUniformBuffer(EShaderFrequency const Freq, uint32 const BufferIndex, FUniformBufferRHIParamRef BufferRHI);
	void SetDirtyUniformBuffers(EShaderFrequency const Freq, uint32 const Dirty);
	
	/*
	 * Monitor if samples pass the depth and stencil tests.
	 * @param Mode Controls if the counter is disabled or moniters passing samples.
	 * @param Offset The offset relative to the occlusion query buffer provided when the command encoder was created.  offset must be a multiple of 8.
	 */
	void SetVisibilityResultMode(MTLVisibilityResultMode const Mode, NSUInteger const Offset);
	
#pragma mark - Public Shader Resource Mutators -
	/*
	 * Set a global buffer for the specified shader frequency at the given bind point index.
	 * @param Frequency The shader frequency to modify.
	 * @param Buffer The buffer to bind or nil to clear.
	 * @param Bytes The FMetalBufferData to bind or nil to clear.
	 * @param Offset The offset in the buffer or 0 when Buffer is nil.
	 * @param Offset The length of data (caller accounts for Offset) in the buffer or 0 when Buffer is nil.
	 * @param Index The index to modify.
	 * @param Format The UAV pixel format.
	 */
	void SetShaderBuffer(EShaderFrequency const Frequency, id<MTLBuffer> const Buffer, FMetalBufferData* const Bytes, NSUInteger const Offset, NSUInteger const Length, NSUInteger const Index, EPixelFormat const Format = PF_Unknown);
	
	/*
	 * Set a global texture for the specified shader frequency at the given bind point index.
	 * @param Frequency The shader frequency to modify.
	 * @param Texture The texture to bind or nil to clear.
	 * @param Index The index to modify.
	 */
	void SetShaderTexture(EShaderFrequency const Frequency, id<MTLTexture> Texture, NSUInteger const Index);
	
	/*
	 * Set a global sampler for the specified shader frequency at the given bind point index.
	 * @param Frequency The shader frequency to modify.
	 * @param Sampler The sampler state to bind or nil to clear.
	 * @param Index The index to modify.
	 */
	void SetShaderSamplerState(EShaderFrequency const Frequency, FMetalSamplerState* const Sampler, NSUInteger const Index);

	void SetShaderResourceView(FMetalContext* Context, EShaderFrequency ShaderStage, uint32 BindIndex, FMetalShaderResourceView* RESTRICT SRV);
	
	void SetShaderUnorderedAccessView(EShaderFrequency ShaderStage, uint32 BindIndex, FMetalUnorderedAccessView* RESTRICT UAV);

	void SetStateDirty(void);
	
	void SetRenderStoreActions(FMetalCommandEncoder& CommandEncoder, bool const bConditionalSwitch);
	
	void SetRenderState(FMetalCommandEncoder& CommandEncoder, FMetalCommandEncoder* PrologueEncoder);

	void CommitRenderResources(FMetalCommandEncoder* Raster);

	void CommitTessellationResources(FMetalCommandEncoder* Raster, FMetalCommandEncoder* Compute);

	void CommitComputeResources(FMetalCommandEncoder* Compute);
	
	void CommitResourceTable(EShaderFrequency const Frequency, MTLFunctionType const Type, FMetalCommandEncoder& CommandEncoder);
	
	bool PrepareToRestart(void);
	
	FMetalShaderParameterCache& GetShaderParameters(uint32 const Stage) { return ShaderParameters[Stage]; }
	FLinearColor const& GetBlendFactor() const { return BlendFactor; }
	uint32 GetStencilRef() const { return StencilRef; }
	FMetalDepthStencilState* GetDepthStencilState() const { return DepthStencilState; }
	FMetalRasterizerState* GetRasterizerState() const { return RasterizerState; }
	FMetalGraphicsPipelineState* GetGraphicsPSO() const { return GraphicsPSO; }
	FMetalComputeShader* GetComputeShader() const { return ComputeShader; }
	CGSize GetFrameBufferSize() const { return FrameBufferSize; }
	FRHISetRenderTargetsInfo const& GetRenderTargetsInfo() const { return RenderTargetsInfo; }
	int32 GetNumRenderTargets() { return bHasValidColorTarget ? RenderTargetsInfo.NumColorRenderTargets : -1; }
	bool GetHasValidRenderTarget() const { return bHasValidRenderTarget; }
	bool GetHasValidColorTarget() const { return bHasValidColorTarget; }
	const MTLViewport& GetViewport(uint32 const Index) const { check(Index < ML_MaxViewports); return Viewport[Index]; }
	uint32 GetVertexBufferSize(uint32 const Index);
	uint32 GetRenderTargetArraySize() const { return RenderTargetArraySize; }
	TRefCountPtr<FRHIUniformBuffer>* GetBoundUniformBuffers(EShaderFrequency const Freq) { return BoundUniformBuffers[Freq]; }
	uint32 GetDirtyUniformBuffers(EShaderFrequency const Freq) const { return DirtyUniformBuffers[Freq]; }
	id<MTLBuffer> GetVisibilityResultsBuffer() const { return VisibilityResults; }
	bool GetScissorRectEnabled() const { return bScissorRectEnabled; }
	bool NeedsToSetRenderTarget(const FRHISetRenderTargetsInfo& RenderTargetsInfo);
	bool HasValidDepthStencilSurface() const { return IsValidRef(DepthStencilSurface); }
	EMetalIndexType GetIndexType() const { return IndexType; }
    bool GetUsingTessellation() const { return bUsingTessellation; }
    bool CanRestartRenderPass() const { return bCanRestartRenderPass; }
	MTLRenderPassDescriptor* GetRenderPassDescriptor(void) const { return RenderPassDesc; }
	uint32 GetSampleCount(void) const { return SampleCount; }
    bool IsLinearBuffer(EShaderFrequency ShaderStage, uint32 BindIndex);
	bool ValidateBufferFormat(EShaderFrequency ShaderStage, uint32 BindIndex, EPixelFormat Format);
    FMetalShaderPipeline* GetPipelineState(uint32 V, uint32 F, uint32 C, EPixelFormat const* const VS, EPixelFormat const* const PS, EPixelFormat const* const DS) const { return GraphicsPSO->GetPipeline(GetIndexType(), V, F, C, VS, PS, DS); }
    FMetalShaderPipeline* GetPipelineState(void) const { return GraphicsPSO->GetPipeline(GetIndexType(), ShaderBuffers[SF_Vertex].FormatHash, ShaderBuffers[SF_Pixel].FormatHash, ShaderBuffers[SF_Domain].FormatHash, nullptr, nullptr, nullptr); }
	
	FTexture2DRHIRef CreateFallbackDepthStencilSurface(uint32 Width, uint32 Height);
	bool GetFallbackDepthStencilBound(void) const { return bFallbackDepthStencilBound; }
	
	void SetShaderCacheStateObject(FShaderCacheState* CacheState)	{ShaderCacheContextState = CacheState;}
	FShaderCacheState* GetShaderCacheStateObject() const			{return ShaderCacheContextState;}
	
    void SetRenderPipelineState(FMetalCommandEncoder& CommandEncoder, FMetalCommandEncoder* PrologueEncoder);
    void SetComputePipelineState(FMetalCommandEncoder& CommandEncoder);
private:
	void ConditionalUpdateBackBuffer(FMetalSurface& Surface);
	
	void SetDepthStencilState(FMetalDepthStencilState* InDepthStencilState);
	void SetRasterizerState(FMetalRasterizerState* InRasterizerState);

private:
	FORCEINLINE void SetResource(uint32 ShaderStage, uint32 BindIndex, FRHITexture* RESTRICT TextureRHI, float CurrentTime);
	
	FORCEINLINE void SetResource(uint32 ShaderStage, uint32 BindIndex, FMetalShaderResourceView* RESTRICT SRV, float CurrentTime);
	
	FORCEINLINE void SetResource(uint32 ShaderStage, uint32 BindIndex, FMetalSamplerState* RESTRICT SamplerState, float CurrentTime);
	
	FORCEINLINE void SetResource(uint32 ShaderStage, uint32 BindIndex, FMetalUnorderedAccessView* RESTRICT UAV, float CurrentTime);
	
	template <typename MetalResourceType>
	inline int32 SetShaderResourcesFromBuffer(uint32 ShaderStage, FMetalUniformBuffer* RESTRICT Buffer, const uint32* RESTRICT ResourceMap, int32 BufferIndex, float CurrentTime);
	
	template <class ShaderType>
	void SetResourcesFromTables(ShaderType Shader, uint32 ShaderStage);
	
	void SetViewport(uint32 Index, const MTLViewport& InViewport);
	void SetScissorRect(uint32 Index, bool const bEnable, MTLScissorRect const& Rect);

private:
#pragma mark - Private Type Declarations -
	struct FMetalBufferBinding
	{
		/** The bound buffers or nil. */
		id<MTLBuffer> Buffer;
		/** Optional bytes buffer used instead of an id<MTLBuffer> */
		FMetalBufferData* Bytes;
		/** The bound buffer offsets or 0. */
		NSUInteger Offset;
		/** The bound buffer lengths or 0. */
		NSUInteger Length;
	};
	
	/** A structure of arrays for the current buffer binding settings. */
	struct FMetalBufferBindings
	{
		/** The bound buffers/bytes or nil. */
		FMetalBufferBinding Buffers[ML_MaxBuffers];
		/** The pixel formats for buffers bound so that we emulate [RW]Buffer<T> type conversion */
		EPixelFormat Formats[ML_MaxBuffers];
		/** The hash of the pixel formats for the formats above */
		uint32 FormatHash;
		/** A bitmask for which buffers were bound by the application where a bit value of 1 is bound and 0 is unbound. */
		uint32 Bound;
	};
	
	/** A structure of arrays for the current texture binding settings. */
	struct FMetalTextureBindings
	{
		/** The bound textures or nil. */
		id<MTLTexture> Textures[ML_MaxTextures];
		/** A bitmask for which textures were bound by the application where a bit value of 1 is bound and 0 is unbound. */
		FMetalTextureMask Bound;
	};
	
	/** A structure of arrays for the current sampler binding settings. */
	struct FMetalSamplerBindings
	{
		/** The bound sampler states or nil. */
		TRefCountPtr<FMetalSamplerState> Samplers[ML_MaxSamplers];
		/** A bitmask for which samplers were bound by the application where a bit value of 1 is bound and 0 is unbound. */
		uint16 Bound;
	};
	
private:

    EMetalBufferType GetShaderBufferBindingType(FMetalBufferBindings& BufferBindings, uint32 BoundBuffers, uint32 ShaderBindingHash);
    
private:
	FMetalShaderParameterCache ShaderParameters[CrossCompiler::NUM_SHADER_STAGES];

	EMetalIndexType IndexType;
	uint32 SampleCount;

	TRefCountPtr<FRHIUniformBuffer> BoundUniformBuffers[SF_NumFrequencies][ML_MaxBuffers];
	
	/** Bitfield for which uniform buffers are dirty */
	uint64 DirtyUniformBuffers[SF_NumFrequencies];
	
	/** Vertex attribute buffers */
	FMetalBufferBinding VertexBuffers[MaxVertexElementCount];
	
	/** Bound shader resource tables. */
	FMetalBufferBindings ShaderBuffers[SF_NumFrequencies];
	FMetalTextureBindings ShaderTextures[SF_NumFrequencies];
	FMetalSamplerBindings ShaderSamplers[SF_NumFrequencies];
	
	MTLStoreAction ColorStore[MaxSimultaneousRenderTargets];
	MTLStoreAction DepthStore;
	MTLStoreAction StencilStore;

	id<MTLBuffer> VisibilityResults;
	MTLVisibilityResultMode VisibilityMode;
	NSUInteger VisibilityOffset;
	
	TRefCountPtr<FMetalDepthStencilState> DepthStencilState;
	TRefCountPtr<FMetalRasterizerState> RasterizerState;
	TRefCountPtr<FMetalGraphicsPipelineState> GraphicsPSO;
	TRefCountPtr<FMetalComputeShader> ComputeShader;
	uint32 StencilRef;
	
	FLinearColor BlendFactor;
	CGSize FrameBufferSize;
	
	uint32 RenderTargetArraySize;

	MTLViewport Viewport[ML_MaxViewports];
	MTLScissorRect Scissor[ML_MaxViewports];
	
	uint32 ActiveViewports;
	uint32 ActiveScissors;
	
	FRHISetRenderTargetsInfo RenderTargetsInfo;
	FTextureRHIRef ColorTargets[MaxSimultaneousRenderTargets];
	FTextureRHIRef DepthStencilSurface;
	/** A fallback depth-stencil surface for draw calls that write to depth without a depth-stencil surface bound. */
	FTexture2DRHIRef FallbackDepthStencilSurface;
	MTLRenderPassDescriptor* RenderPassDesc;
	uint32 RasterBits;
    uint8 PipelineBits;
	bool bIsRenderTargetActive;
	bool bHasValidRenderTarget;
	bool bHasValidColorTarget;
	bool bScissorRectEnabled;
    bool bUsingTessellation;
    bool bCanRestartRenderPass;
    bool bImmediate;
	bool bFallbackDepthStencilBound;
	
	FShaderCacheState* ShaderCacheContextState;
};
