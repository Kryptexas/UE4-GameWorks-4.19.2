// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHIMETHOD_SPECIFIERSs.h: The RHI method definitions.  The same methods are defined multiple places, so they're simply included from this file where necessary.
=============================================================================*/

// DEFINE_RHIMETHOD is used by the includer to modify the RHI method definitions.
// It's defined by the dynamic RHI to pass parameters from the statically bound RHI method to the dynamically bound RHI method.
// To enable that, the parameter list must be given a second time, as they will be passed to the dynamically bound RHI method.
// The last parameter should be return if the method returns a value, and nothing otherwise.
#ifndef DEFINE_RHIMETHOD
#define GENERATE_VAX_FUNCTION_DECLARATIONS
#endif

#ifdef GENERATE_VAX_FUNCTION_DECLARATIONS
	#define DEFINE_RHIMETHOD_0(ReturnType,MethodName,ReturnStatement,NullImplementation) 
		ReturnType MethodName();
	#define DEFINE_RHIMETHOD_1(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ReturnStatement,NullImplementation) \
		ReturnType MethodName(ParameterTypeA ParameterNameA);
	#define DEFINE_RHIMETHOD_2(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ReturnStatement,NullImplementation) \
		ReturnType MethodName(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB);
	#define DEFINE_RHIMETHOD_3(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ReturnStatement,NullImplementation) \
		ReturnType MethodName(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC);
	#define DEFINE_RHIMETHOD_4(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ParameterTypeD,ParameterNameD,ReturnStatement,NullImplementation) \
		ReturnType MethodName(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC,ParameterTypeD ParameterNameD);
	#define DEFINE_RHIMETHOD_5(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ParameterTypeD,ParameterNameD,ParameterTypeE,ParameterNameE,ReturnStatement,NullImplementation) \
		ReturnType MethodName(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC,ParameterTypeD ParameterNameD,ParameterTypeE ParameterNameE);
	#define DEFINE_RHIMETHOD_6(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ParameterTypeD,ParameterNameD,ParameterTypeE,ParameterNameE,ParameterTypeF,ParameterNameF,ReturnStatement,NullImplementation) \
		ReturnType MethodName(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC,ParameterTypeD ParameterNameD,ParameterTypeE ParameterNameE,ParameterTypeF ParameterNameF);
	#define DEFINE_RHIMETHOD_7(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ParameterTypeD,ParameterNameD,ParameterTypeE,ParameterNameE,ParameterTypeF,ParameterNameF,ParameterTypeG,ParameterNameG,ReturnStatement,NullImplementation) \
		ReturnType MethodName(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC,ParameterTypeD ParameterNameD,ParameterTypeE ParameterNameE,ParameterTypeF ParameterNameF,ParameterTypeG ParameterNameG);
	#define DEFINE_RHIMETHOD_8(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ParameterTypeD,ParameterNameD,ParameterTypeE,ParameterNameE,ParameterTypeF,ParameterNameF,ParameterTypeG,ParameterNameG,ParameterTypeH,ParameterNameH,ReturnStatement,NullImplementation) \
		ReturnType MethodName(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC,ParameterTypeD ParameterNameD,ParameterTypeE ParameterNameE,ParameterTypeF ParameterNameF,ParameterTypeG ParameterNameG,ParameterTypeH ParameterNameH);
	#define DEFINE_RHIMETHOD_9(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ParameterTypeD,ParameterNameD,ParameterTypeE,ParameterNameE,ParameterTypeF,ParameterNameF,ParameterTypeG,ParameterNameG,ParameterTypeH,ParameterNameH,ParameterTypeI,ParameterNameI,ReturnStatement,NullImplementation) \
		ReturnType MethodName(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC,ParameterTypeD ParameterNameD,ParameterTypeE ParameterNameE,ParameterTypeF ParameterNameF,ParameterTypeG ParameterNameG,ParameterTypeH ParameterNameH,ParameterTypeI ParameterNameI);
#else
	#define DEFINE_RHIMETHOD_0(ReturnType,MethodName,ReturnStatement,NullImplementation) \
		DEFINE_RHIMETHOD(ReturnType,MethodName,(),(),ReturnStatement,NullImplementation)
	#define DEFINE_RHIMETHOD_1(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ReturnStatement,NullImplementation) \
		DEFINE_RHIMETHOD(ReturnType,MethodName,(ParameterTypeA ParameterNameA),(ParameterNameA),ReturnStatement,NullImplementation)
	#define DEFINE_RHIMETHOD_2(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ReturnStatement,NullImplementation) \
		DEFINE_RHIMETHOD(ReturnType,MethodName,(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB),(ParameterNameA,ParameterNameB),ReturnStatement,NullImplementation)
	#define DEFINE_RHIMETHOD_3(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ReturnStatement,NullImplementation) \
		DEFINE_RHIMETHOD(ReturnType,MethodName,(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC),(ParameterNameA,ParameterNameB,ParameterNameC),ReturnStatement,NullImplementation)
	#define DEFINE_RHIMETHOD_4(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ParameterTypeD,ParameterNameD,ReturnStatement,NullImplementation) \
		DEFINE_RHIMETHOD(ReturnType,MethodName,(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC,ParameterTypeD ParameterNameD),(ParameterNameA,ParameterNameB,ParameterNameC,ParameterNameD),ReturnStatement,NullImplementation)
	#define DEFINE_RHIMETHOD_5(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ParameterTypeD,ParameterNameD,ParameterTypeE,ParameterNameE,ReturnStatement,NullImplementation) \
		DEFINE_RHIMETHOD(ReturnType,MethodName,(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC,ParameterTypeD ParameterNameD,ParameterTypeE ParameterNameE),(ParameterNameA,ParameterNameB,ParameterNameC,ParameterNameD,ParameterNameE),ReturnStatement,NullImplementation)
	#define DEFINE_RHIMETHOD_6(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ParameterTypeD,ParameterNameD,ParameterTypeE,ParameterNameE,ParameterTypeF,ParameterNameF,ReturnStatement,NullImplementation) \
		DEFINE_RHIMETHOD(ReturnType,MethodName,(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC,ParameterTypeD ParameterNameD,ParameterTypeE ParameterNameE,ParameterTypeF ParameterNameF),(ParameterNameA,ParameterNameB,ParameterNameC,ParameterNameD,ParameterNameE,ParameterNameF),ReturnStatement,NullImplementation)
	#define DEFINE_RHIMETHOD_7(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ParameterTypeD,ParameterNameD,ParameterTypeE,ParameterNameE,ParameterTypeF,ParameterNameF,ParameterTypeG,ParameterNameG,ReturnStatement,NullImplementation) \
		DEFINE_RHIMETHOD(ReturnType,MethodName,(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC,ParameterTypeD ParameterNameD,ParameterTypeE ParameterNameE,ParameterTypeF ParameterNameF,ParameterTypeG ParameterNameG),(ParameterNameA,ParameterNameB,ParameterNameC,ParameterNameD,ParameterNameE,ParameterNameF,ParameterNameG),ReturnStatement,NullImplementation)
	#define DEFINE_RHIMETHOD_8(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ParameterTypeD,ParameterNameD,ParameterTypeE,ParameterNameE,ParameterTypeF,ParameterNameF,ParameterTypeG,ParameterNameG,ParameterTypeH,ParameterNameH,ReturnStatement,NullImplementation) \
		DEFINE_RHIMETHOD(ReturnType,MethodName,(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC,ParameterTypeD ParameterNameD,ParameterTypeE ParameterNameE,ParameterTypeF ParameterNameF,ParameterTypeG ParameterNameG,ParameterTypeH ParameterNameH),(ParameterNameA,ParameterNameB,ParameterNameC,ParameterNameD,ParameterNameE,ParameterNameF,ParameterNameG,ParameterNameH),ReturnStatement,NullImplementation)
	#define DEFINE_RHIMETHOD_9(ReturnType,MethodName,ParameterTypeA,ParameterNameA,ParameterTypeB,ParameterNameB,ParameterTypeC,ParameterNameC,ParameterTypeD,ParameterNameD,ParameterTypeE,ParameterNameE,ParameterTypeF,ParameterNameF,ParameterTypeG,ParameterNameG,ParameterTypeH,ParameterNameH,ParameterTypeI,ParameterNameI,ReturnStatement,NullImplementation) \
		DEFINE_RHIMETHOD(ReturnType,MethodName,(ParameterTypeA ParameterNameA,ParameterTypeB ParameterNameB,ParameterTypeC ParameterNameC,ParameterTypeD ParameterNameD,ParameterTypeE ParameterNameE,ParameterTypeF ParameterNameF,ParameterTypeG ParameterNameG,ParameterTypeH ParameterNameH,ParameterTypeI ParameterNameI),(ParameterNameA,ParameterNameB,ParameterNameC,ParameterNameD,ParameterNameE,ParameterNameF,ParameterNameG,ParameterNameH,ParameterNameI),ReturnStatement,NullImplementation)
#endif

//
// RHI resource management functions.
//

DEFINE_RHIMETHOD_1(
	FSamplerStateRHIRef,RHICreateSamplerState,
	const FSamplerStateInitializerRHI&,Initializer,
	return,return new FRHISamplerState();
	);
DEFINE_RHIMETHOD_1(
	FRasterizerStateRHIRef,RHICreateRasterizerState,
	const FRasterizerStateInitializerRHI&,Initializer,
	return,return new FRHIRasterizerState();
	);
DEFINE_RHIMETHOD_1(
	FDepthStencilStateRHIRef,RHICreateDepthStencilState,
	const FDepthStencilStateInitializerRHI&,Initializer,
	return,return new FRHIDepthStencilState();
	);
DEFINE_RHIMETHOD_1(
	FBlendStateRHIRef,RHICreateBlendState,
	const FBlendStateInitializerRHI&,Initializer,
	return,return new FRHIBlendState();
	);

DEFINE_RHIMETHOD_1(
	FVertexDeclarationRHIRef,RHICreateVertexDeclaration,
	const FVertexDeclarationElementList&,Elements,
	return,return new FRHIVertexDeclaration();
	);

DEFINE_RHIMETHOD_1(
	FPixelShaderRHIRef,RHICreatePixelShader,
	const TArray<uint8>&,Code,
	return,return new FRHIPixelShader();
	);

DEFINE_RHIMETHOD_1(
	FVertexShaderRHIRef,RHICreateVertexShader,
	const TArray<uint8>&,Code,
	return,return new FRHIVertexShader();
	);

DEFINE_RHIMETHOD_1(
	FHullShaderRHIRef,RHICreateHullShader,
	const TArray<uint8>&,Code,
	return,return new FRHIHullShader();
	);

DEFINE_RHIMETHOD_1(
	FDomainShaderRHIRef,RHICreateDomainShader,
	const TArray<uint8>&,Code,
	return,return new FRHIDomainShader();
	);
	
DEFINE_RHIMETHOD_1(
	FGeometryShaderRHIRef,RHICreateGeometryShader,
	const TArray<uint8>&,Code,
	return,return new FRHIGeometryShader();
	);

/** Creates a geometry shader with stream output ability, defined by ElementList. */
DEFINE_RHIMETHOD_5(
	FGeometryShaderRHIRef,RHICreateGeometryShaderWithStreamOutput,
	const TArray<uint8>&,Code,
	const FStreamOutElementList&,ElementList,
	uint32,NumStrides,
	const uint32*,Strides,
	int32,RasterizedStream,
	return,return new FRHIGeometryShader();
	);

DEFINE_RHIMETHOD_1(
	FComputeShaderRHIRef,RHICreateComputeShader,
	const TArray<uint8>&,Code,
	return,return new FRHIComputeShader();
	);

/**
 * Creates a bound shader state instance which encapsulates a decl, vertex shader, hull shader, domain shader and pixel shader
 * @param VertexDeclaration - existing vertex decl
 * @param VertexShader - existing vertex shader
 * @param HullShader - existing hull shader
 * @param DomainShader - existing domain shader
 * @param GeometryShader - existing geometry shader
 * @param PixelShader - existing pixel shader
 */
DEFINE_RHIMETHOD_6(
	FBoundShaderStateRHIRef,RHICreateBoundShaderState,
	FVertexDeclarationRHIParamRef,VertexDeclaration,
	FVertexShaderRHIParamRef,VertexShader,
	FHullShaderRHIParamRef,HullShader,
	FDomainShaderRHIParamRef,DomainShader,
	FPixelShaderRHIParamRef,PixelShader,
	FGeometryShaderRHIParamRef,GeometryShader,
	return,return new FRHIBoundShaderState();
	);

/**
 *Sets the current compute shader.  Mostly for compliance with platforms
 *that require shader setting before resource binding.
 */
DEFINE_RHIMETHOD_1(
	void,RHISetComputeShader,
	FComputeShaderRHIParamRef,ComputeShader,
	,
	);

DEFINE_RHIMETHOD_3(
	void,RHIDispatchComputeShader,	
	uint32,ThreadGroupCountX,
	uint32,ThreadGroupCountY,
	uint32,ThreadGroupCountZ,
	,
	);

DEFINE_RHIMETHOD_2(
	void,RHIDispatchIndirectComputeShader,	
	FVertexBufferRHIParamRef,ArgumentBuffer,
	uint32,ArgumentOffset,
	,
	);

DEFINE_RHIMETHOD_1(
	void,RHIAutomaticCacheFlushAfterComputeShader,
	bool,bEnable,
	,
	);

DEFINE_RHIMETHOD_0(
	void,RHIFlushComputeShaderCache,
	,
	);

// Useful when used with geometry shader (emit polygons to different viewports), otherwise SetViewPort() is simpler
// @param Count >0
// @param Data must not be 0
DEFINE_RHIMETHOD_2(
	void,RHISetMultipleViewports,
	uint32,Count,
	const FViewportBounds*,Data,
	,
	);

/**
 * Creates a uniform buffer.  The contents of the uniform buffer are provided in a parameter, and are immutable.
 * @param Contents - A pointer to a memory block of size NumBytes that is copied into the new uniform buffer.
 * @param NumBytes - The number of bytes the uniform buffer should contain.
 * @return The new uniform buffer.
 */
DEFINE_RHIMETHOD_3(
	FUniformBufferRHIRef,RHICreateUniformBuffer,
	const void*,Contents,
	uint32,NumBytes,
	EUniformBufferUsage,Usage,
	return,return new FRHIUniformBuffer(NumBytes);
	);


DEFINE_RHIMETHOD_4(
	FIndexBufferRHIRef,RHICreateIndexBuffer,
	uint32,Stride,
	uint32,Size,
	FResourceArrayInterface*,ResourceArray,
	uint32,InUsage,
	return,if (ResourceArray) { ResourceArray->Discard(); } return new FRHIIndexBuffer(Stride,Size,InUsage);
	);

DEFINE_RHIMETHOD_4(
	void*,RHILockIndexBuffer,
	FIndexBufferRHIParamRef,IndexBuffer,
	uint32,Offset,
	uint32,Size,
	EResourceLockMode,LockMode,
	return,return GetStaticBuffer();
	);
DEFINE_RHIMETHOD_1(
	void,RHIUnlockIndexBuffer,
	FIndexBufferRHIParamRef,IndexBuffer,
	,
	);

/**
 * @param ResourceArray - An optional pointer to a resource array containing the resource's data.
 */
DEFINE_RHIMETHOD_3(
	FVertexBufferRHIRef,RHICreateVertexBuffer,
	uint32,Size,
	FResourceArrayInterface*,ResourceArray,
	uint32,InUsage,
	return,if (ResourceArray) { ResourceArray->Discard(); } return new FRHIVertexBuffer(Size,InUsage);
	);

DEFINE_RHIMETHOD_4(
	void*,RHILockVertexBuffer,
	FVertexBufferRHIParamRef,VertexBuffer,
	uint32,Offset,
	uint32,SizeRHI,
	EResourceLockMode,LockMode,
	return,return GetStaticBuffer();
	);
DEFINE_RHIMETHOD_1(
	void,RHIUnlockVertexBuffer,
	FVertexBufferRHIParamRef,VertexBuffer,
	,
	);

/** Copies the contents of one vertex buffer to another vertex buffer.  They must have identical sizes. */
DEFINE_RHIMETHOD_2(
	void,RHICopyVertexBuffer,
	FVertexBufferRHIParamRef,SourceBuffer,
	FVertexBufferRHIParamRef,DestBuffer,
	,
	);

/**
 * @param ResourceArray - An optional pointer to a resource array containing the resource's data.
 */
DEFINE_RHIMETHOD_4(
	FStructuredBufferRHIRef,RHICreateStructuredBuffer,
	uint32,Stride,
	uint32,Size,
	FResourceArrayInterface*,ResourceArray,
	uint32,InUsage,
	return,if (ResourceArray) { ResourceArray->Discard(); } return new FRHIStructuredBuffer(Stride,Size,InUsage);
	);

DEFINE_RHIMETHOD_4(
	void*,RHILockStructuredBuffer,
	FStructuredBufferRHIParamRef,StructuredBuffer,
	uint32,Offset,
	uint32,SizeRHI,
	EResourceLockMode,LockMode,
	return,return GetStaticBuffer();
	);
DEFINE_RHIMETHOD_1(
	void,RHIUnlockStructuredBuffer,
	FStructuredBufferRHIParamRef,StructuredBuffer,
	,
	);

/** Creates an unordered access view of the given structured buffer. */
DEFINE_RHIMETHOD_3(
	FUnorderedAccessViewRHIRef,RHICreateUnorderedAccessView,
	FStructuredBufferRHIParamRef,StructuredBuffer,
	bool,bUseUAVCounter,
	bool,bAppendBuffer,
	return,return new FRHIUnorderedAccessView();
	);

/** Creates an unordered access view of the given texture. */
DEFINE_RHIMETHOD_1(
	FUnorderedAccessViewRHIRef,RHICreateUnorderedAccessView,
	FTextureRHIParamRef,Texture,
	return,return new FRHIUnorderedAccessView();
	);
	
/** Creates an unordered access view of the given texture. */
DEFINE_RHIMETHOD_2(
	FUnorderedAccessViewRHIRef,RHICreateUnorderedAccessView,
	FVertexBufferRHIParamRef,VertexBuffer,
	uint8,Format,
	return,return new FRHIUnorderedAccessView();
	);

/** Creates a shader resource view of the given structured buffer. */
DEFINE_RHIMETHOD_1(
	FShaderResourceViewRHIRef,RHICreateShaderResourceView,
	FStructuredBufferRHIParamRef,StructuredBuffer,
	return,return new FRHIShaderResourceView();
	);

/** Creates a shader resource view of the given vertex buffer. */
DEFINE_RHIMETHOD_3(
	FShaderResourceViewRHIRef,RHICreateShaderResourceView,
	FVertexBufferRHIParamRef,VertexBuffer,
	uint32,Stride,
	uint8,Format,
	return,return new FRHIShaderResourceView();
	);

/** Clears a UAV to the multi-component value provided. */
DEFINE_RHIMETHOD_2(
	void,RHIClearUAV,
	FUnorderedAccessViewRHIParamRef,UnorderedAccessViewRHI,
	const uint32*,Values,
	,
	);

/**
 * Retrieves texture memory stats.
 * safe to call on the main thread
 */
DEFINE_RHIMETHOD_1(
	void,RHIGetTextureMemoryStats,
	FTextureMemoryStats&,OutStats,
	,
	);

/**
 * Fills a texture with to visualize the texture pool memory.
 *
 * @param	TextureData		Start address
 * @param	SizeX			Number of pixels along X
 * @param	SizeY			Number of pixels along Y
 * @param	Pitch			Number of bytes between each row
 * @param	PixelSize		Number of bytes each pixel represents
 *
 * @return true if successful, false otherwise
 */
DEFINE_RHIMETHOD_5(
	bool,RHIGetTextureMemoryVisualizeData,
	FColor*,TextureData,
	int32,SizeX,
	int32,SizeY,
	int32,Pitch,
	int32,PixelSize,
	return,return false
	);

/**
* Creates a 2D RHI texture resource
* @param SizeX - width of the texture to create
* @param SizeY - height of the texture to create
* @param Format - EPixelFormat texture format
* @param NumMips - number of mips to generate or 0 for full mip pyramid
* @param NumSamples - number of MSAA samples, usually 1
* @param Flags - ETextureCreateFlags creation flags
*/
DEFINE_RHIMETHOD_7(
	FTexture2DRHIRef,RHICreateTexture2D,
	uint32,SizeX,
	uint32,SizeY,
	uint8,Format,
	uint32,NumMips,
	uint32,NumSamples,
	uint32,Flags,
	FResourceBulkDataInterface*,BulkData,
	return,return new FRHITexture2D(SizeX,SizeY,NumMips,NumSamples,(EPixelFormat)Format,Flags);
	);

/**
 * Thread-safe function that can be used to create a texture outside of the
 * rendering thread. This function can ONLY be called if GRHISupportsAsyncTextureCreation
 * is true.
 * @param SizeX - width of the texture to create
 * @param SizeY - height of the texture to create
 * @param Format - EPixelFormat texture format
 * @param NumMips - number of mips to generate or 0 for full mip pyramid
 * @param Flags - ETextureCreateFlags creation flags
 * @param InitialMipData - pointers to mip data with which to create the texture
 * @param NumInitialMips - how many mips are provided in InitialMipData
 * @returns a reference to a 2D texture resource
 */
DEFINE_RHIMETHOD_7(
	FTexture2DRHIRef,RHIAsyncCreateTexture2D,
	uint32,SizeX,
	uint32,SizeY,
	uint8,Format,
	uint32,NumMips,
	uint32,Flags,
	void**,InitialMipData,
	uint32,NumInitialMips,
	return,return FTexture2DRHIRef();
	);

/**
 * Copies shared mip levels from one texture to another. The textures must have
 * full mip chains, share the same format, and have the same aspect ratio. This
 * copy will not cause synchronization with the GPU.
 * @param DestTexture2D - destination texture
 * @param SrcTexture2D - source texture
 */
DEFINE_RHIMETHOD_2(
	void,RHICopySharedMips,
	FTexture2DRHIParamRef,DestTexture2D,
	FTexture2DRHIParamRef,SrcTexture2D,
	return,return;
	);

/**
* Creates a Array RHI texture resource
* @param SizeX - width of the texture to create
* @param SizeY - height of the texture to create
* @param SizeZ - depth of the texture to create
* @param Format - EPixelFormat texture format
* @param NumMips - number of mips to generate or 0 for full mip pyramid
* @param Flags - ETextureCreateFlags creation flags
*/
DEFINE_RHIMETHOD_7(
	FTexture2DArrayRHIRef,RHICreateTexture2DArray,
	uint32,SizeX,
	uint32,SizeY,
	uint32,SizeZ,
	uint8,Format,
	uint32,NumMips,
	uint32,Flags,
	FResourceBulkDataInterface*,BulkData,
	return,return new FRHITexture2DArray(SizeX,SizeY,SizeZ,NumMips,(EPixelFormat)Format,Flags);
	);

/**
* Creates a 3d RHI texture resource
* @param SizeX - width of the texture to create
* @param SizeY - height of the texture to create
* @param SizeZ - depth of the texture to create
* @param Format - EPixelFormat texture format
* @param NumMips - number of mips to generate or 0 for full mip pyramid
* @param Flags - ETextureCreateFlags creation flags
* @param Data - Data to initialize the texture with
*/
DEFINE_RHIMETHOD_7(
	FTexture3DRHIRef,RHICreateTexture3D,
	uint32,SizeX,
	uint32,SizeY,
	uint32,SizeZ,
	uint8,Format,
	uint32,NumMips,
	uint32,Flags,
	FResourceBulkDataInterface*,BulkData,
	return,return new FRHITexture3D(SizeX,SizeY,SizeZ,NumMips,(EPixelFormat)Format,Flags);
	);

/**
* Creates a shader resource view for a 2d texture, viewing only a single
* mip level. Useful when rendering to one mip while sampling from another.
*/
DEFINE_RHIMETHOD_2(
	FShaderResourceViewRHIRef,RHICreateShaderResourceView,
	FTexture2DRHIParamRef,Texture2DRHI,
	uint8, MipLevel,
	return,return new FRHIShaderResourceView();
	);

/**
* Creates a shader resource view for a 2d texture, with a different
* format from the original.  Useful when sampling stencil.
*/
DEFINE_RHIMETHOD_4(
	FShaderResourceViewRHIRef,RHICreateShaderResourceView,
	FTexture2DRHIParamRef,Texture2DRHI,
	uint8, MipLevel,
	uint8, NumMipLevels,
	uint8, Format,		
	return,return new FRHIShaderResourceView();
);

/**
* Generates mip maps for a texture.
*/
DEFINE_RHIMETHOD_1(
	void,RHIGenerateMips,
	FTextureRHIParamRef,Texture,
	return,return;
	);

/**
 * Computes the size in memory required by a given texture.
 *
 * @param	TextureRHI		- Texture we want to know the size of, 0 is safely ignored
 * @return					- Size in Bytes
 */
DEFINE_RHIMETHOD_1(
	uint32,RHIComputeMemorySize,
	FTextureRHIParamRef,TextureRHI,
	return,return 0;
	);

/**
 * Starts an asynchronous texture reallocation. It may complete immediately if the reallocation
 * could be performed without any reshuffling of texture memory, or if there isn't enough memory.
 * The specified status counter will be decremented by 1 when the reallocation is complete (success or failure).
 *
 * Returns a new reference to the texture, which will represent the new mip count when the reallocation is complete.
 * RHIFinalizeAsyncReallocateTexture2D() must be called to complete the reallocation.
 *
 * @param Texture2D		- Texture to reallocate
 * @param NewMipCount	- New number of mip-levels
 * @param NewSizeX		- New width, in pixels
 * @param NewSizeY		- New height, in pixels
 * @param RequestStatus	- Will be decremented by 1 when the reallocation is complete (success or failure).
 * @return				- New reference to the texture, or an invalid reference upon failure
 */
DEFINE_RHIMETHOD_5(
	FTexture2DRHIRef,RHIAsyncReallocateTexture2D,
	FTexture2DRHIParamRef,Texture2D,
	int32,NewMipCount,
	int32,NewSizeX,
	int32,NewSizeY,
	FThreadSafeCounter*,RequestStatus,
	return,return new FRHITexture2D(NewSizeX,NewSizeY,NewMipCount,1,Texture2D->GetFormat(),Texture2D->GetFlags());
	);

/**
 * Finalizes an async reallocation request.
 * If bBlockUntilCompleted is false, it will only poll the status and finalize if the reallocation has completed.
 *
 * @param Texture2D				- Texture to finalize the reallocation for
 * @param bBlockUntilCompleted	- Whether the function should block until the reallocation has completed
 * @return						- Current reallocation status:
 *	TexRealloc_Succeeded	Reallocation succeeded
 *	TexRealloc_Failed		Reallocation failed
 *	TexRealloc_InProgress	Reallocation is still in progress, try again later
 */
DEFINE_RHIMETHOD_2(
	ETextureReallocationStatus,RHIFinalizeAsyncReallocateTexture2D,
	FTexture2DRHIParamRef,Texture2D,
	bool,bBlockUntilCompleted,
	return,return TexRealloc_Succeeded;
	);

/**
 * Cancels an async reallocation for the specified texture.
 * This should be called for the new texture, not the original.
 *
 * @param Texture				Texture to cancel
 * @param bBlockUntilCompleted	If true, blocks until the cancellation is fully completed
 * @return						Reallocation status
 */
DEFINE_RHIMETHOD_2(
	ETextureReallocationStatus,RHICancelAsyncReallocateTexture2D,
	FTexture2DRHIParamRef,Texture2D,
	bool,bBlockUntilCompleted,
	return,return TexRealloc_Succeeded;
	);

/**
* Locks an RHI texture's mip-map for read/write operations on the CPU
* @param Texture - the RHI texture resource to lock, must not be 0
* @param MipIndex - index of the mip level to lock
* @param LockMode - Whether to lock the texture read-only instead of write-only
* @param DestStride - output to retrieve the textures row stride (pitch)
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
* @return pointer to the CPU accessible resource data
*/
DEFINE_RHIMETHOD_5(
	void*,RHILockTexture2D,
	FTexture2DRHIParamRef,Texture,
	uint32,MipIndex,
	EResourceLockMode,LockMode,
	uint32&,DestStride,
	bool,bLockWithinMiptail,
	return,DestStride = 0; return GetStaticBuffer()
	);

/**
* Unlocks a previously locked RHI texture resource
* @param Texture - the RHI texture resource to unlock, must not be 0
* @param MipIndex - index of the mip level to unlock
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
*/
DEFINE_RHIMETHOD_3(
	void,RHIUnlockTexture2D,
	FTexture2DRHIParamRef,Texture,
	uint32,MipIndex,
	bool,bLockWithinMiptail,
	,
	);

/**
* Locks an RHI texture's mip-map for read/write operations on the CPU
* @param Texture - the RHI texture resource to lock
* @param MipIndex - index of the mip level to lock
* @param LockMode - Whether to lock the texture read-only instead of write-only
* @param DestStride - output to retrieve the textures row stride (pitch)
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
* @return pointer to the CPU accessible resource data
*/
DEFINE_RHIMETHOD_6(
	void*,RHILockTexture2DArray,
	FTexture2DArrayRHIParamRef,Texture,
	uint32,TextureIndex,
	uint32,MipIndex,
	EResourceLockMode,LockMode,
	uint32&,DestStride,
	bool,bLockWithinMiptail,
	return,DestStride = 0; return GetStaticBuffer()
	);

/**
* Unlocks a previously locked RHI texture resource
* @param Texture - the RHI texture resource to unlock
* @param MipIndex - index of the mip level to unlock
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
*/
DEFINE_RHIMETHOD_4(
	void,RHIUnlockTexture2DArray,
	FTexture2DArrayRHIParamRef,Texture,
	uint32,TextureIndex,
	uint32,MipIndex,
	bool,bLockWithinMiptail,
	,
	);

/**
* Updates a region of a 2D texture from system memory
* @param Texture - the RHI texture resource to update
* @param MipIndex - mip level index to be modified
* @param UpdateRegion - The rectangle to copy source image data from
* @param SourcePitch - size in bytes of each row of the source image
* @param SourceData - source image data, starting at the upper left corner of the source rectangle (in same pixel format as texture)
*/
DEFINE_RHIMETHOD_5(
	void,RHIUpdateTexture2D,
	FTexture2DRHIParamRef,Texture,
	uint32,MipIndex,
	const struct FUpdateTextureRegion2D&,UpdateRegion,
	uint32,SourcePitch,
	const uint8*,SourceData,
	,
	);

/**
* Updates a region of a 3D texture from system memory
* @param Texture - the RHI texture resource to update
* @param MipIndex - mip level index to be modified
* @param UpdateRegion - The rectangle to copy source image data from
* @param SourceRowPitch - size in bytes of each row of the source image, usually Bpp * SizeX
* @param SourceDepthPitch - size in bytes of each depth slice of the source image, usually Bpp * SizeX * SizeY
* @param SourceData - source image data, starting at the upper left corner of the source rectangle (in same pixel format as texture)
*/
DEFINE_RHIMETHOD_6(
	void,RHIUpdateTexture3D,
	FTexture3DRHIParamRef,Texture,
	uint32,MipIndex,
	const struct FUpdateTextureRegion3D&,UpdateRegion,
	uint32,SourceRowPitch,
	uint32,SourceDepthPitch,
	const uint8*,SourceData,
	,
	);

/**
* Creates a Cube RHI texture resource
* @param Size - width/height of the texture to create
* @param Format - EPixelFormat texture format
* @param NumMips - number of mips to generate or 0 for full mip pyramid
* @param Flags - ETextureCreateFlags creation flags
*/
DEFINE_RHIMETHOD_5(
	FTextureCubeRHIRef,RHICreateTextureCube,
	uint32,Size,
	uint8,Format,
	uint32,NumMips,
	uint32,Flags,
	FResourceBulkDataInterface*,BulkData,
	return,return new FRHITextureCube(Size,NumMips,(EPixelFormat)Format,Flags);
	);


	/**
	* Creates a Cube Array RHI texture resource
	* @param Size - width/height of the texture to create
	* @param ArraySize - number of array elements of the texture to create
	* @param Format - EPixelFormat texture format
	* @param NumMips - number of mips to generate or 0 for full mip pyramid
	* @param Flags - ETextureCreateFlags creation flags
	*/
	DEFINE_RHIMETHOD_6(
		FTextureCubeRHIRef,RHICreateTextureCubeArray,
		uint32,Size,
		uint32,ArraySize,
		uint8,Format,
		uint32,NumMips,
		uint32,Flags,
		FResourceBulkDataInterface*,BulkData,
		return,return new FRHITextureCube(Size,NumMips,(EPixelFormat)Format,Flags);
	);

/**
* Locks an RHI texture's mip-map for read/write operations on the CPU
* @param Texture - the RHI texture resource to lock
* @param MipIndex - index of the mip level to lock
* @param LockMode - Whether to lock the texture read-only instead of write-only.
* @param DestStride - output to retrieve the textures row stride (pitch)
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
* @return pointer to the CPU accessible resource data
*/
DEFINE_RHIMETHOD_7(
	void*,RHILockTextureCubeFace,
	FTextureCubeRHIParamRef,Texture,
	uint32,FaceIndex,
	uint32,ArrayIndex,
	uint32,MipIndex,
	EResourceLockMode,LockMode,
	uint32&,DestStride,
	bool,bLockWithinMiptail,
	return,DestStride = 0; return GetStaticBuffer();
	);

/**
* Unlocks a previously locked RHI texture resource
* @param Texture - the RHI texture resource to unlock
* @param MipIndex - index of the mip level to unlock
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
*/
DEFINE_RHIMETHOD_5(
	void,RHIUnlockTextureCubeFace,
	FTextureCubeRHIParamRef,Texture,
	uint32,FaceIndex,
	uint32,ArrayIndex,
	uint32,MipIndex,
	bool,bLockWithinMiptail,
	,
	);

/**
* Resolves from one texture to another.
* @param SourceTexture - texture to resolve from, 0 is silenty ignored
* @param DestTexture - texture to resolve to, 0 is silenty ignored
* @param bKeepOriginalSurface - true if the original surface will still be used after this function so must remain valid
* @param ResolveParams - optional resolve params
*/
DEFINE_RHIMETHOD_4(
	void,RHICopyToResolveTarget,
	FTextureRHIParamRef,SourceTexture,
	FTextureRHIParamRef,DestTexture,
	bool,bKeepOriginalSurface,
	const FResolveParams&,ResolveParams,
	,
	);

DEFINE_RHIMETHOD_2(
	void, RHIBindDebugLabelName,
	FTextureRHIParamRef, Texture,
	const TCHAR*, Name,
	,
	);

/**
 * Reads the contents of a texture to an output buffer (non MSAA and MSAA) and returns it as a FColor array.
 * If the format or texture type is unsupported the OutData array will be size 0
 */
DEFINE_RHIMETHOD_4(
	void,RHIReadSurfaceData,
	FTextureRHIParamRef,Texture,
	FIntRect,Rect,
	TArray<FColor>&,OutData,
	FReadSurfaceDataFlags,InFlags,
	, OutData.AddZeroed(Rect.Width() * Rect.Height());
	);

/** Watch out for OutData to be 0 (can happen on DXGI_ERROR_DEVICE_REMOVED), don't call RHIUnmapStagingSurface in that case. */
DEFINE_RHIMETHOD_4(
	void,RHIMapStagingSurface,
	FTextureRHIParamRef,Texture,
	void*&,OutData,
	int32&,OutWidth,
	int32&,OutHeight,
	,
	);

/** call after a succesful RHIMapStagingSurface() call */
DEFINE_RHIMETHOD_1(
	void,RHIUnmapStagingSurface,
	FTextureRHIParamRef,Texture,
	,
	);

DEFINE_RHIMETHOD_6(
	void,RHIReadSurfaceFloatData,
	FTextureRHIParamRef,Texture,
	FIntRect,Rect,
	TArray<FFloat16Color>&,OutData,
	ECubeFace,CubeFace,
	int32,ArrayIndex,
	int32,MipIndex,
	,
	);

DEFINE_RHIMETHOD_4(
	void,RHIRead3DSurfaceFloatData,
	FTextureRHIParamRef,Texture,
	FIntRect,Rect,
	FIntPoint,ZMinMax,
	TArray<FFloat16Color>&,OutData,
	,
	);


// Occlusion/Timer queries.
DEFINE_RHIMETHOD_1(
	FRenderQueryRHIRef,RHICreateRenderQuery,
	ERenderQueryType, QueryType,
	return,return new FRHIRenderQuery();
	);
DEFINE_RHIMETHOD_1(
	void,RHIResetRenderQuery,
	FRenderQueryRHIParamRef,RenderQuery,
	,
	);
DEFINE_RHIMETHOD_3(
	bool,RHIGetRenderQueryResult,
	FRenderQueryRHIParamRef,RenderQuery,
	uint64&,OutResult,
	bool,bWait,
	return,return true;
	);
DEFINE_RHIMETHOD_1(
	void,RHIBeginRenderQuery,
	FRenderQueryRHIParamRef,RenderQuery,
	,
	);
DEFINE_RHIMETHOD_1(
	void,RHIEndRenderQuery,
	FRenderQueryRHIParamRef,RenderQuery,
	,
	);


DEFINE_RHIMETHOD_2(
	void,RHIBeginDrawingViewport,
	FViewportRHIParamRef,Viewport,
	FTextureRHIParamRef,RenderTargetRHI,
	,
	);
DEFINE_RHIMETHOD_3(
	void,RHIEndDrawingViewport,
	FViewportRHIParamRef,Viewport,
	bool,bPresent,
	bool,bLockToVsync,
	,
	);
/**
 * Determine if currently drawing the viewport
 *
 * @return true if currently within a BeginDrawingViewport/EndDrawingViewport block
 */
DEFINE_RHIMETHOD_0(
	bool,RHIIsDrawingViewport,
	return,return false;
	);
DEFINE_RHIMETHOD_1(
	FTexture2DRHIRef,RHIGetViewportBackBuffer,
	FViewportRHIParamRef,Viewport,
	return,return new FRHITexture2D(1,1,1,1,PF_B8G8R8A8,TexCreate_RenderTargetable);
	);

DEFINE_RHIMETHOD_0(
	void,RHIBeginFrame,
	return,return;
	);
DEFINE_RHIMETHOD_0(
	void,RHIEndFrame,
	return,return;
	);

DEFINE_RHIMETHOD_0(
	void,RHIBeginScene,
	return,return;
	);
DEFINE_RHIMETHOD_0(
	void,RHIEndScene,
	return,return;
	);

/*
 * Acquires or releases ownership of the platform-specific rendering context for the calling thread
 */
DEFINE_RHIMETHOD_0(
	void,RHIAcquireThreadOwnership,
	return,return;
	);
DEFINE_RHIMETHOD_0(
	void,RHIReleaseThreadOwnership,
	return,return;
	);

// Flush driver resources. Typically called when switching contexts/threads
DEFINE_RHIMETHOD_0(
	void,RHIFlushResources,
	return,return;
	);
/*
 * Returns the total GPU time taken to render the last frame. Same metric as FPlatformTime::Cycles().
 */
DEFINE_RHIMETHOD_0(
	uint32,RHIGetGPUFrameCycles,
	return,return 0;
	);

/**
 * The following RHI functions must be called from the main thread.
 */
DEFINE_RHIMETHOD_4(
	FViewportRHIRef,RHICreateViewport,
	void*,WindowHandle,
	uint32,SizeX,
	uint32,SizeY,
	bool,bIsFullscreen,
	return,return new FRHIViewport();
	);
DEFINE_RHIMETHOD_4(
	void,RHIResizeViewport,
	FViewportRHIParamRef,Viewport,
	uint32,SizeX,
	uint32,SizeY,
	bool,bIsFullscreen,
	,
	);
DEFINE_RHIMETHOD_1(
	void,RHITick,
	float,DeltaTime,
	,
	);

//
// RHI commands.
//

// Vertex state.
DEFINE_RHIMETHOD_4(
	void,RHISetStreamSource,
	uint32,StreamIndex,
	FVertexBufferRHIParamRef,VertexBuffer,
	uint32,Stride,
	uint32,Offset,
	,
	);

/** Sets stream output targets, for use with a geometry shader created with RHICreateGeometryShaderWithStreamOutput. */
DEFINE_RHIMETHOD_3(
	void,RHISetStreamOutTargets,
	uint32,NumTargets,
	const FVertexBufferRHIParamRef*,VertexBuffers,
	const uint32*,Offsets,
	,
	);

// Rasterizer state.
DEFINE_RHIMETHOD_1(
	void,RHISetRasterizerState,
	FRasterizerStateRHIParamRef,NewState,
	,
	);
// @param MinX including like Win32 RECT
// @param MinY including like Win32 RECT
// @param MaxX excluding like Win32 RECT
// @param MaxY excluding like Win32 RECT
DEFINE_RHIMETHOD_6(
	void,RHISetViewport,
	uint32,MinX,
	uint32,MinY,
	float,MinZ,
	uint32,MaxX,
	uint32,MaxY,
	float,MaxZ,
	,
	);
// @param MinX including like Win32 RECT
// @param MinY including like Win32 RECT
// @param MaxX excluding like Win32 RECT
// @param MaxY excluding like Win32 RECT
DEFINE_RHIMETHOD_5(
	void,RHISetScissorRect,
	bool,bEnable,
	uint32,MinX,
	uint32,MinY,
	uint32,MaxX,
	uint32,MaxY,
	,
	);

// Shader state.
/**
 * Set bound shader state. This will set the vertex decl/shader, and pixel shader
 * @param BoundShaderState - state resource
 */
DEFINE_RHIMETHOD_1(
	void,RHISetBoundShaderState,
	FBoundShaderStateRHIParamRef,BoundShaderState,
	,
	);

/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderTexture,
	FVertexShaderRHIParamRef,VertexShader,
	uint32,TextureIndex,
	FTextureRHIParamRef,NewTexture,
	,
	);

/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderTexture,
	FHullShaderRHIParamRef,HullShader,
	uint32,TextureIndex,
	FTextureRHIParamRef,NewTexture,
	,
	);

/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderTexture,
	FDomainShaderRHIParamRef,DomainShader,
	uint32,TextureIndex,
	FTextureRHIParamRef,NewTexture,
	,
	);

/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderTexture,
	FGeometryShaderRHIParamRef,GeometryShader,
	uint32,TextureIndex,
	FTextureRHIParamRef,NewTexture,
	,
	);

/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderTexture,
	FPixelShaderRHIParamRef,PixelShader,
	uint32,TextureIndex,
	FTextureRHIParamRef,NewTexture,
	,
	);

/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderTexture,
	FComputeShaderRHIParamRef,PixelShader,
	uint32,TextureIndex,
	FTextureRHIParamRef,NewTexture,
	,
	);

/**
 * Sets sampler state.
 * @param ComputeShader		The compute shader to set the sampler for.
 * @param SamplerIndex		The index of the sampler.
 * @param NewState			The new sampler state.
 */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderSampler,
	FComputeShaderRHIParamRef,ComputeShader,
	uint32,SamplerIndex,
	FSamplerStateRHIParamRef,NewState,
	,
	);

/**
 * Sets sampler state.
 * @param VertexShader		The vertex shader to set the sampler for.
 * @param SamplerIndex		The index of the sampler.
 * @param NewState			The new sampler state.
 */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderSampler,
	FVertexShaderRHIParamRef,VertexShader,
	uint32,SamplerIndex,
	FSamplerStateRHIParamRef,NewState,
	,
	);

/**
 * Sets sampler state.
 * @param GeometryShader	The geometry shader to set the sampler for.
 * @param SamplerIndex		The index of the sampler.
 * @param NewState			The new sampler state.
 */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderSampler,
	FGeometryShaderRHIParamRef,GeometryShader,
	uint32,SamplerIndex,
	FSamplerStateRHIParamRef,NewState,
	,
	);

/**
 * Sets sampler state.
 * @param DomainShader		The domain shader to set the sampler for.
 * @param SamplerIndex		The index of the sampler.
 * @param NewState			The new sampler state.
 */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderSampler,
	FDomainShaderRHIParamRef,DomainShader,
	uint32,SamplerIndex,
	FSamplerStateRHIParamRef,NewState,
	,
	);

/**
 * Sets sampler state.
 * @param HullShader		The hull shader to set the sampler for.
 * @param SamplerIndex		The index of the sampler.
 * @param NewState			The new sampler state.
 */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderSampler,
	FHullShaderRHIParamRef,HullShader,
	uint32,SamplerIndex,
	FSamplerStateRHIParamRef,NewState,
	,
	);

/**
 * Sets sampler state.
 * @param PixelShader		The pixel shader to set the sampler for.
 * @param SamplerIndex		The index of the sampler.
 * @param NewState			The new sampler state.
 */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderSampler,
	FPixelShaderRHIParamRef,PixelShader,
	uint32,SamplerIndex,
	FSamplerStateRHIParamRef,NewState,
	,
	);

/** Sets a compute shader UAV parameter. */
DEFINE_RHIMETHOD_3(
	 void,RHISetUAVParameter,
	 FComputeShaderRHIParamRef,ComputeShader,
	 uint32,UAVIndex,
	 FUnorderedAccessViewRHIParamRef,UAV,
	 ,
	 );

/** Sets a compute shader UAV parameter and initial count */
DEFINE_RHIMETHOD_4(
	 void,RHISetUAVParameter,
	 FComputeShaderRHIParamRef,ComputeShader,
	 uint32,UAVIndex,
	 FUnorderedAccessViewRHIParamRef,UAV,
	 uint32,InitialCount,
	 , 
	 );

/** Sets a pixel shader resource view parameter. */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderResourceViewParameter,
	FPixelShaderRHIParamRef,PixelShader,
	uint32,SamplerIndex,
	FShaderResourceViewRHIParamRef,SRV,
	,
	);

/** Sets a vertex shader resource view parameter. */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderResourceViewParameter,
	FVertexShaderRHIParamRef,VertexShader,
	uint32,SamplerIndex,
	FShaderResourceViewRHIParamRef,SRV,
	,
	);

/** Sets a compute shader resource view parameter. */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderResourceViewParameter,
	FComputeShaderRHIParamRef,ComputeShader,
	uint32,SamplerIndex,
	FShaderResourceViewRHIParamRef,SRV,
	,
	);

/** Sets a hull shader resource view parameter. */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderResourceViewParameter,
	FHullShaderRHIParamRef,HullShader,
	uint32,SamplerIndex,
	FShaderResourceViewRHIParamRef,SRV,
	,
	);

/** Sets a domain shader resource view parameter. */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderResourceViewParameter,
	FDomainShaderRHIParamRef,DomainShader,
	uint32,SamplerIndex,
	FShaderResourceViewRHIParamRef,SRV,
	,
	);

/** Sets a geometry shader resource view parameter. */
DEFINE_RHIMETHOD_3(
	void,RHISetShaderResourceViewParameter,
	FGeometryShaderRHIParamRef,GeometryShader,
	uint32,SamplerIndex,
	FShaderResourceViewRHIParamRef,SRV,
	,
	);

DEFINE_RHIMETHOD_3(
	 void,RHISetShaderUniformBuffer,
	 FVertexShaderRHIParamRef,VertexShader,
	 uint32,BufferIndex,
	 FUniformBufferRHIParamRef,Buffer,
	 ,
	 );

DEFINE_RHIMETHOD_3(
	 void,RHISetShaderUniformBuffer,
	 FHullShaderRHIParamRef,HullShader,
	 uint32,BufferIndex,
	 FUniformBufferRHIParamRef,Buffer,
	 ,
	 );

DEFINE_RHIMETHOD_3(
	 void,RHISetShaderUniformBuffer,
	 FDomainShaderRHIParamRef,DomainShader,
	 uint32,BufferIndex,
	 FUniformBufferRHIParamRef,Buffer,
	 ,
	 );

DEFINE_RHIMETHOD_3(
	 void,RHISetShaderUniformBuffer,
	 FGeometryShaderRHIParamRef,GeometryShader,
	 uint32,BufferIndex,
	 FUniformBufferRHIParamRef,Buffer,
	 ,
	 );

DEFINE_RHIMETHOD_3(
	 void,RHISetShaderUniformBuffer,
	 FPixelShaderRHIParamRef,PixelShader,
	 uint32,BufferIndex,
	 FUniformBufferRHIParamRef,Buffer,
	 ,
	 );

DEFINE_RHIMETHOD_3(
	void,RHISetShaderUniformBuffer,
	FComputeShaderRHIParamRef,ComputeShader,
	uint32,BufferIndex,
	FUniformBufferRHIParamRef,Buffer,
	,
	);

DEFINE_RHIMETHOD_5(
	void,RHISetShaderParameter,
	FVertexShaderRHIParamRef,VertexShader,
	uint32,BufferIndex,
	uint32,BaseIndex,
	uint32,NumBytes,
	const void*,NewValue,
	,
	);
DEFINE_RHIMETHOD_5(
	 void,RHISetShaderParameter,
	 FPixelShaderRHIParamRef,PixelShader,
	 uint32,BufferIndex,
	 uint32,BaseIndex,
	 uint32,NumBytes,
	 const void*,NewValue,
	 ,
	 );

DEFINE_RHIMETHOD_5(
	 void,RHISetShaderParameter,
	 FHullShaderRHIParamRef,HullShader,
	 uint32,BufferIndex,
	 uint32,BaseIndex,
	 uint32,NumBytes,
	 const void*,NewValue,
	 ,
	 );
DEFINE_RHIMETHOD_5(
	 void,RHISetShaderParameter,
	 FDomainShaderRHIParamRef,DomainShader,
	 uint32,BufferIndex,
	 uint32,BaseIndex,
	 uint32,NumBytes,
	 const void*,NewValue,
	 ,
	 );
DEFINE_RHIMETHOD_5(
	 void,RHISetShaderParameter,
	 FGeometryShaderRHIParamRef,GeometryShader,
	 uint32,BufferIndex,
	 uint32,BaseIndex,
	 uint32,NumBytes,
	 const void*,NewValue,
	 ,
	 );
DEFINE_RHIMETHOD_5(
	void,RHISetShaderParameter,
	FComputeShaderRHIParamRef,ComputeShader,
	uint32,BufferIndex,
	uint32,BaseIndex,
	uint32,NumBytes,
	const void*,NewValue,
	,
	);

// Output state.
DEFINE_RHIMETHOD_2(
	void,RHISetDepthStencilState,
	FDepthStencilStateRHIParamRef,NewState,
	uint32,StencilRef,
	,
	);
// Allows to set the blend state, parameter can be created with RHICreateBlendState()
DEFINE_RHIMETHOD_2(
	void,RHISetBlendState,
	FBlendStateRHIParamRef,NewState,
	const FLinearColor&,BlendFactor,
	,
	);
DEFINE_RHIMETHOD_5(
	void,RHISetRenderTargets,
	uint32,NumSimultaneousRenderTargets,
	const FRHIRenderTargetView*,NewRenderTargets,
	FTextureRHIParamRef,NewDepthStencilTarget,
	uint32,NumUAVs,
	const FUnorderedAccessViewRHIParamRef*,UAVs,
	,
	);
DEFINE_RHIMETHOD_3(
	void,RHIDiscardRenderTargets,
	bool,Depth,
	bool,Stencil,
	uint32,ColorBitMask,
	,
	);

// Primitive drawing.
DEFINE_RHIMETHOD_4(
	void,RHIDrawPrimitive,
	uint32,PrimitiveType,
	uint32,BaseVertexIndex,
	uint32,NumPrimitives,
	uint32,NumInstances,
	,
	);
DEFINE_RHIMETHOD_3(
	void,RHIDrawPrimitiveIndirect,
	uint32,PrimitiveType,
	FVertexBufferRHIParamRef,ArgumentBuffer,
	uint32,ArgumentOffset,
	,
	);

DEFINE_RHIMETHOD_5(
	void,RHIDrawIndexedIndirect,
	FIndexBufferRHIParamRef,IndexBufferRHI,
	uint32,PrimitiveType,
	FStructuredBufferRHIParamRef,ArgumentsBufferRHI,
	int32,DrawArgumentsIndex,
	uint32,NumInstances,
	,
	);

// @param NumPrimitives need to be >0 
DEFINE_RHIMETHOD_8(
	void,RHIDrawIndexedPrimitive,
	FIndexBufferRHIParamRef,IndexBuffer,
	uint32,PrimitiveType,
	int32,BaseVertexIndex,
	uint32,MinIndex,
	uint32,NumVertices,
	uint32,StartIndex,
	uint32,NumPrimitives,
	uint32,NumInstances,
	,
	);
DEFINE_RHIMETHOD_4(
	void,RHIDrawIndexedPrimitiveIndirect,
	uint32,PrimitiveType,
	FIndexBufferRHIParamRef,IndexBuffer,
	FVertexBufferRHIParamRef,ArgumentBuffer,
	uint32,ArgumentOffset,
	,
	);

// Immediate Primitive drawing
/**
 * Preallocate memory or get a direct command stream pointer to fill up for immediate rendering . This avoids memcpys below in DrawPrimitiveUP
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param NumPrimitives The number of primitives in the VertexData buffer
 * @param NumVertices The number of vertices to be written
 * @param VertexDataStride Size of each vertex 
 * @param OutVertexData Reference to the allocated vertex memory
 */
DEFINE_RHIMETHOD_5(
	void,RHIBeginDrawPrimitiveUP,
	uint32,PrimitiveType,
	uint32,NumPrimitives,
	uint32,NumVertices,
	uint32,VertexDataStride,
	void*&,OutVertexData,
	,OutVertexData = GetStaticBuffer();
	);

/**
 * Draw a primitive using the vertex data populated since RHIBeginDrawPrimitiveUP and clean up any memory as needed
 */
DEFINE_RHIMETHOD_0(
	void,RHIEndDrawPrimitiveUP,
	,
	);

/**
 * Preallocate memory or get a direct command stream pointer to fill up for immediate rendering . This avoids memcpys below in DrawIndexedPrimitiveUP
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param NumPrimitives The number of primitives in the VertexData buffer
 * @param NumVertices The number of vertices to be written
 * @param VertexDataStride Size of each vertex
 * @param OutVertexData Reference to the allocated vertex memory
 * @param MinVertexIndex The lowest vertex index used by the index buffer
 * @param NumIndices Number of indices to be written
 * @param IndexDataStride Size of each index (either 2 or 4 bytes)
 * @param OutIndexData Reference to the allocated index memory
 */
DEFINE_RHIMETHOD_9(
	void,RHIBeginDrawIndexedPrimitiveUP,
	uint32,PrimitiveType,
	uint32,NumPrimitives,
	uint32,NumVertices,
	uint32,VertexDataStride,
	void*&,OutVertexData,
	uint32,MinVertexIndex,
	uint32,NumIndices,
	uint32,IndexDataStride,
	void*&,OutIndexData,
	,OutVertexData = GetStaticBuffer(); OutIndexData = GetStaticBuffer();
	);

/**
 * Draw a primitive using the vertex and index data populated since RHIBeginDrawIndexedPrimitiveUP and clean up any memory as needed
 */
DEFINE_RHIMETHOD_0(
	void,RHIEndDrawIndexedPrimitiveUP,
	,
	);

/*
 * Raster operations.
 * This method clears all MRT's, but to only one color value
 * @param ExcludeRect within the viewport in pixels, is only a hint to optimize - if a fast clear can be done this is preferred
 */
DEFINE_RHIMETHOD_7(
	void,RHIClear,
	bool,bClearColor,
	const FLinearColor&,Color,
	bool,bClearDepth,
	float,Depth,
	bool,bClearStencil,
	uint32,Stencil,
	FIntRect,ExcludeRect,
	,
	);

/*
 * This method clears all MRT's to potentially different color values
 * @param ExcludeRect within the viewport in pixels, is only a hint to optimize - if a fast clear can be done this is preferred
 */
DEFINE_RHIMETHOD_8(
	void,RHIClearMRT,
	bool,bClearColor,
	int32,NumClearColors,
	const FLinearColor*,ColorArray,
	bool,bClearDepth,
	float,Depth,
	bool,bClearStencil,
	uint32,Stencil,
	FIntRect,ExcludeRect,
	,
	);

// Blocks the CPU until the GPU catches up and goes idle.
DEFINE_RHIMETHOD_0(
	void,RHIBlockUntilGPUIdle,
	,
	);

// Operations to suspend title rendering and yield control to the system
DEFINE_RHIMETHOD_0(
	void,RHISuspendRendering,
	,
	);
DEFINE_RHIMETHOD_0(
	void,RHIResumeRendering,
	,
	);
DEFINE_RHIMETHOD_0(
	bool,RHIIsRenderingSuspended,
	return,return false;
	);

/**
 *	Retrieve available screen resolutions.
 *
 *	@param	Resolutions			TArray<FScreenResolutionRHI> parameter that will be filled in.
 *	@param	bIgnoreRefreshRate	If true, ignore refresh rates.
 *
 *	@return	bool				true if successfully filled the array
 */
DEFINE_RHIMETHOD_2(
	bool,RHIGetAvailableResolutions,
	FScreenResolutionArray&,Resolutions,
	bool,bIgnoreRefreshRate,
	return,return false;
	);

/**
 * Returns a supported screen resolution that most closely matches input.
 * @param Width - Input: Desired resolution width in pixels. Output: A width that the platform supports.
 * @param Height - Input: Desired resolution height in pixels. Output: A height that the platform supports.
 */
DEFINE_RHIMETHOD_2(
	void,RHIGetSupportedResolution,
	uint32&,Width,
	uint32&,Height,
	,
	);

/**
 * Function that is used to allocate / free space used for virtual texture mip levels.
 * Make sure you also update the visible mip levels.
 * @param Texture - the texture to update, must have been created with TexCreate_Virtual
 * @param FirstMip - the first mip that should be in memory
 */
DEFINE_RHIMETHOD_2(
	void,RHIVirtualTextureSetFirstMipInMemory,
	FTexture2DRHIParamRef,Texture,
	uint32,FirstMip,
	,
	);

/**
 * Function that can be used to update which is the first visible mip to the GPU.
 * @param Texture - the texture to update, must have been created with TexCreate_Virtual
 * @param FirstMip - the first mip that should be visible to the GPU
 */
DEFINE_RHIMETHOD_2(
	void,RHIVirtualTextureSetFirstMipVisible,
	FTexture2DRHIParamRef,Texture,
	uint32,FirstMip,
	,
	);
