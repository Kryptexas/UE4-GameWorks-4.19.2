// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalShaders.cpp: Metal shader RHI implementation.
=============================================================================*/

#include "MetalRHIPrivate.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "MetalShaderResources.h"
#include "MetalResources.h"
#include "ShaderCache.h"
#include "MetalProfiler.h"
#include "MetalCommandBuffer.h"
#include "Serialization/MemoryReader.h"
#include "Misc/FileHelper.h"

#define SHADERCOMPILERCOMMON_API
#	include "Developer/ShaderCompilerCommon/Public/ShaderCompilerCommon.h"
#undef SHADERCOMPILERCOMMON_API

/** Set to 1 to enable shader debugging (makes the driver save the shader source) */
#define DEBUG_METAL_SHADERS (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)

class FMetalCompiledShaderKey
{
public:
	FMetalCompiledShaderKey(
		uint32 InCodeSize,
		uint32 InCodeCRC
		)
		: CodeSize(InCodeSize)
		, CodeCRC(InCodeCRC)
	{}

	friend bool operator ==(const FMetalCompiledShaderKey& A, const FMetalCompiledShaderKey& B)
	{
		return A.CodeSize == B.CodeSize && A.CodeCRC == B.CodeCRC;
	}

	friend uint32 GetTypeHash(const FMetalCompiledShaderKey &Key)
	{
		return HashCombine(GetTypeHash(Key.CodeSize), GetTypeHash(Key.CodeCRC));
	}

private:
	GLenum TypeEnum;
	uint32 CodeSize;
	uint32 CodeCRC;
};

struct FMetalCompiledShaderCache
{
public:
	FMetalCompiledShaderCache()
	{
		int Err = pthread_rwlock_init(&Lock, nullptr);
		checkf(Err == 0, TEXT("pthread_rwlock_init failed with error: %d"), Err);
	}
	
	~FMetalCompiledShaderCache()
	{
		int Err = pthread_rwlock_destroy(&Lock);
		checkf(Err == 0, TEXT("pthread_rwlock_destroy failed with error: %d"), Err);
		for (TPair<FMetalCompiledShaderKey, id<MTLFunction>> Pair : Cache)
		{
			[Pair.Value release];
		}
	}
	
	id<MTLFunction> FindRef(FMetalCompiledShaderKey Key)
	{
		int Err = pthread_rwlock_rdlock(&Lock);
		checkf(Err == 0, TEXT("pthread_rwlock_rdlock failed with error: %d"), Err);
		id<MTLFunction> Func = Cache.FindRef(Key);
		Err = pthread_rwlock_unlock(&Lock);
		checkf(Err == 0, TEXT("pthread_rwlock_unlock failed with error: %d"), Err);
		return Func;
	}
	
	void Add(FMetalCompiledShaderKey Key, id<MTLFunction> Function)
	{
		int Err = pthread_rwlock_wrlock(&Lock);
		checkf(Err == 0, TEXT("pthread_rwlock_wrlock failed with error: %d"), Err);
		Cache.Add(Key, Function);
		Err = pthread_rwlock_unlock(&Lock);
		checkf(Err == 0, TEXT("pthread_rwlock_unlock failed with error: %d"), Err);
	}
	
private:
	pthread_rwlock_t Lock;
	TMap<FMetalCompiledShaderKey, id<MTLFunction>> Cache;
};

static FMetalCompiledShaderCache& GetMetalCompiledShaderCache()
{
	static FMetalCompiledShaderCache CompiledShaderCache;
	return CompiledShaderCache;
}

/** Initialization constructor. */
template<typename BaseResourceType, int32 ShaderType>
TMetalBaseShader<BaseResourceType, ShaderType>::TMetalBaseShader(const TArray<uint8>& InShaderCode)
: Function(nil)
, Library(nil)
, SideTableBinding(-1)
, GlslCodeNSString(nil)
{
	FShaderCodeReader ShaderCode(InShaderCode);

	FMemoryReader Ar(InShaderCode, true);
	
	Ar.SetLimitSize(ShaderCode.GetActualShaderCodeSize());

	// was the shader already compiled offline?
	uint8 OfflineCompiledFlag;
	Ar << OfflineCompiledFlag;
	check(OfflineCompiledFlag == 0 || OfflineCompiledFlag == 1);

	// get the header
	FMetalCodeHeader Header = { 0 };
	Ar << Header;

	// remember where the header ended and code (precompiled or source) begins
	int32 CodeOffset = Ar.Tell();
	const ANSICHAR* SourceCode = (ANSICHAR*)InShaderCode.GetData() + CodeOffset;

	uint32 CodeLength, CodeCRC;
	if (OfflineCompiledFlag)
	{
		CodeLength = ShaderCode.GetActualShaderCodeSize() - CodeOffset;

		// CRC the compiled code
		CodeCRC = FCrc::MemCrc_DEPRECATED(InShaderCode.GetData() + CodeOffset, CodeLength);
	}
	else
	{
		UE_LOG(LogMetal, Display, TEXT("Loaded a non-offline compiled shader (will be slower to load)"));

		CodeLength = ShaderCode.GetActualShaderCodeSize() - CodeOffset - 1;

		// CRC the source
		CodeCRC = FCrc::MemCrc_DEPRECATED(SourceCode, CodeLength);
	}

	FMetalCompiledShaderKey Key(CodeLength, CodeCRC);

	static NSString* const Offline = @"OFFLINE";
	bool bOfflineCompile = (OfflineCompiledFlag > 0);
	
	const ANSICHAR* ShaderSource = ShaderCode.FindOptionalData('c');
	bool const bHasShaderSource = (ShaderSource && FCStringAnsi::Strlen(ShaderSource) > 0);
	if (bOfflineCompile && bHasShaderSource)
	{
		GlslCodeNSString = [NSString stringWithUTF8String:ShaderSource];
		[GlslCodeNSString retain];
	}
	else
	{
		GlslCodeNSString = [Offline retain];
	}

	// Find the existing compiled shader in the cache.
	Function = [GetMetalCompiledShaderCache().FindRef(Key) retain];
	if (!Function)
	{
		if (bOfflineCompile && bHasShaderSource)
		{
			// For debug/dev/test builds we can use the stored code for debugging - but shipping builds shouldn't have this as it is inappropriate.
#if METAL_DEBUG_OPTIONS
			// For iOS/tvOS we must use runtime compilation to make the shaders debuggable, but
			bool bSavedSource = false;
			
#if PLATFORM_MAC
			const ANSICHAR* ShaderPath = ShaderCode.FindOptionalData('p');
			bool const bHasShaderPath = (ShaderPath && FCStringAnsi::Strlen(ShaderPath) > 0);
			
			// on Mac if we have a path for the shader we can access the shader code
			if (bHasShaderPath)
			{
				FString ShaderPathString(ShaderPath);
				
				if (IFileManager::Get().MakeDirectory(*FPaths::GetPath(ShaderPathString), true))
				{
					bSavedSource = FFileHelper::SaveStringToFile(FString(GlslCodeNSString), *ShaderPathString);
				}
				
				static bool bAttemptedAuth = false;
				if (!bSavedSource && !bAttemptedAuth)
				{
					bAttemptedAuth = true;
					
					if (IFileManager::Get().MakeDirectory(*FPaths::GetPath(ShaderPathString), true))
					{
						bSavedSource = FFileHelper::SaveStringToFile(FString(GlslCodeNSString), *ShaderPathString);
					}
					
					if (!bSavedSource)
					{
						FPlatformMisc::MessageBoxExt(EAppMsgType::Ok,
													*NSLOCTEXT("MetalRHI", "ShaderDebugAuthFail", "Could not access directory required for debugging optimised Metal shaders. Falling back to slower runtime compilation of shaders for debugging.").ToString(), TEXT("Error"));
					}
				}
			}
#endif
			// Switch the compile mode so we get debuggable shaders even if we failed to save - if we didn't want
			// shader debugging we wouldn't have included the code...
			bOfflineCompile = bSavedSource;
#endif
		}
		else
		{
			GlslCodeNSString = [Offline retain];
		}

		if (bOfflineCompile)
		{
			// allow GCD to copy the data into its own buffer
			//		dispatch_data_t GCDBuffer = dispatch_data_create(InShaderCode.GetTypedData() + CodeOffset, ShaderCode.GetActualShaderCodeSize() - CodeOffset, nil, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
			uint32 BufferSize = ShaderCode.GetActualShaderCodeSize() - CodeOffset;
			void* Buffer = FMemory::Malloc( BufferSize );
			FMemory::Memcpy( Buffer, InShaderCode.GetData() + CodeOffset, BufferSize );
			dispatch_data_t GCDBuffer = dispatch_data_create(Buffer, BufferSize, dispatch_get_main_queue(), ^(void) { FMemory::Free(Buffer); } );

			// load up the already compiled shader
			NSError* Error;
			Library = [GetMetalDeviceContext().GetDevice() newLibraryWithData:GCDBuffer error:&Error];
			if (Library == nil)
			{
				NSLog(@"Failed to create library: %@", Error);
			}
			
			dispatch_release(GCDBuffer);
		}
		else
		{
			NSError* Error;
			NSString* ShaderString = ((OfflineCompiledFlag == 0) ? [NSString stringWithUTF8String:SourceCode] : GlslCodeNSString);

			if(Header.ShaderName.Len())
			{
				ShaderString = [NSString stringWithFormat:@"// %@\n%@", Header.ShaderName.GetNSString(), ShaderString];
			}
			
			MTLCompileOptions *CompileOptions = [[MTLCompileOptions alloc] init];
			CompileOptions.fastMathEnabled = (BOOL)(!(Header.CompileFlags & (1 << CFLAG_NoFastMath)));
			if (GetMetalDeviceContext().SupportsFeature(EMetalFeaturesShaderVersions))
			{
#if PLATFORM_MAC
				CompileOptions.languageVersion = Header.Version == 0 ? MTLLanguageVersion1_1 : (MTLLanguageVersion)((1 << 16) + Header.Version);
#else
				CompileOptions.languageVersion = (MTLLanguageVersion)((1 << 16) + Header.Version);
#endif
			}
#if DEBUG_METAL_SHADERS
			NSDictionary *PreprocessorMacros = @{ @"MTLSL_ENABLE_DEBUG_INFO" : @(1)};
			[CompileOptions setPreprocessorMacros : PreprocessorMacros];
#endif
			Library = [GetMetalDeviceContext().GetDevice() newLibraryWithSource:ShaderString options : CompileOptions error : &Error];
			[CompileOptions release];

			if (Library == nil)
			{
				NSLog(@"Failed to create library: %@", Error);
				NSLog(@"*********** Error\n%@", ShaderString);
			}
			else if (Error != nil)
			{
				// Warning...
                NSLog(@"%@\n", Error);
                NSLog(@"*********** Error\n%@", ShaderString);
			}

			GlslCodeNSString = ShaderString;
			[GlslCodeNSString retain];
		}

		if (!Header.bFunctionConstants)
		{
			// assume there's only one function called 'Main', and use that to get the function from the library
			Function = [[Library newFunctionWithName:@"Main"] retain];
			check(Function);
			GetMetalCompiledShaderCache().Add(Key, Function);
			[Library release];
			Library = nil;
			TRACK_OBJECT(STAT_MetalFunctionCount, Function);
		}
	}

	Bindings = Header.Bindings;
	UniformBuffersCopyInfo = Header.UniformBuffersCopyInfo;
	SideTableBinding = Header.SideTable;

	//@todo: Find better way...
	if (ShaderType == SF_Compute)
	{
		auto* ComputeShader = (FMetalComputeShader*)this;
		ComputeShader->NumThreadsX = FMath::Max((int32)Header.NumThreadsX, 1);
		ComputeShader->NumThreadsY = FMath::Max((int32)Header.NumThreadsY, 1);
		ComputeShader->NumThreadsZ = FMath::Max((int32)Header.NumThreadsZ, 1);
	}
}

/** Destructor */
template<typename BaseResourceType, int32 ShaderType>
TMetalBaseShader<BaseResourceType, ShaderType>::~TMetalBaseShader()
{
	UNTRACK_OBJECT(STAT_MetalFunctionCount, Function);
	[Function release];
	[Library release];
	[GlslCodeNSString release];
}

FMetalComputeShader::FMetalComputeShader(const TArray<uint8>& InCode)
	: TMetalBaseShader<FRHIComputeShader, SF_Compute>(InCode)
	, Reflection(nil)
//	, NumThreadsX(0)
//	, NumThreadsY(0)
//	, NumThreadsZ(0)
{
	NSError* Error;
	
#if METAL_DEBUG_OPTIONS
	if (GetMetalDeviceContext().GetCommandQueue().GetRuntimeDebuggingLevel() >= EMetalDebugLevelValidation)
	{
		MTLAutoreleasedComputePipelineReflection ComputeReflection;
		Kernel = [GetMetalDeviceContext().GetDevice() newComputePipelineStateWithFunction:Function options:MTLPipelineOptionArgumentInfo reflection:&ComputeReflection error:&Error];
		Reflection = [ComputeReflection retain];
	}
	else
#endif
	{
		Kernel = [GetMetalDeviceContext().GetDevice() newComputePipelineStateWithFunction:Function error:&Error];
	}
	
	if (Kernel == nil)
	{
        NSLog(@"Failed to create kernel: %@", Error);
        NSLog(@"*********** Error\n%@", GlslCodeNSString);
        
        UE_LOG(LogRHI, Fatal, TEXT("Failed to create compute kernel: %s"), *FString([Error description]));
	}
	TRACK_OBJECT(STAT_MetalComputePipelineStateCount, Kernel);
}

FMetalComputeShader::~FMetalComputeShader()
{
	UNTRACK_OBJECT(STAT_MetalComputePipelineStateCount, Kernel);
	[Reflection release];
}

FMetalVertexShader::FMetalVertexShader(const TArray<uint8>& InCode)
	: TMetalBaseShader<FRHIVertexShader, SF_Vertex>(InCode)
{
}

FMetalDomainShader::FMetalDomainShader(const TArray<uint8>& InCode)
	: TMetalBaseShader<FRHIDomainShader, SF_Domain>(InCode)
{
}

FVertexShaderRHIRef FMetalDynamicRHI::RHICreateVertexShader(const TArray<uint8>& Code)
{
	FMetalVertexShader* Shader = new FMetalVertexShader(Code);
	FShaderCodeReader ShaderCode(Code);
	FMemoryReader Ar(Code, true);
	Ar.SetLimitSize(ShaderCode.GetActualShaderCodeSize());

	// was the shader already compiled offline?
	uint8 OfflineCompiledFlag;
	Ar << OfflineCompiledFlag;

	// get the header
	FMetalCodeHeader Header = { 0 };
	Ar << Header;

	// for VSHS
	Shader->TessellationOutputAttribs = Header.TessellationOutputAttribs;
	Shader->TessellationPatchCountBuffer = Header.TessellationPatchCountBuffer;
	Shader->TessellationIndexBuffer = Header.TessellationIndexBuffer;
	Shader->TessellationHSOutBuffer = Header.TessellationHSOutBuffer;
	Shader->TessellationHSTFOutBuffer = Header.TessellationHSTFOutBuffer;
	Shader->TessellationControlPointOutBuffer = Header.TessellationControlPointOutBuffer;
	Shader->TessellationControlPointIndexBuffer = Header.TessellationControlPointIndexBuffer;
	Shader->TessellationOutputControlPoints = Header.TessellationOutputControlPoints;
	Shader->TessellationDomain = Header.TessellationDomain;
	Shader->TessellationInputControlPoints = Header.TessellationInputControlPoints;
	Shader->TessellationMaxTessFactor = Header.TessellationMaxTessFactor;
	Shader->TessellationPatchesPerThreadGroup = Header.TessellationPatchesPerThreadGroup;
	return Shader;
}

FPixelShaderRHIRef FMetalDynamicRHI::RHICreatePixelShader(const TArray<uint8>& Code)
{
	FMetalPixelShader* Shader = new FMetalPixelShader(Code);
	return Shader;
}

FHullShaderRHIRef FMetalDynamicRHI::RHICreateHullShader(const TArray<uint8>& Code) 
{ 
	FMetalHullShader* Shader = new FMetalHullShader(Code);
	return Shader;
}

FDomainShaderRHIRef FMetalDynamicRHI::RHICreateDomainShader(const TArray<uint8>& Code) 
{ 
	FMetalDomainShader* Shader = new FMetalDomainShader(Code);
	FShaderCodeReader ShaderCode(Code);
	FMemoryReader Ar(Code, true);
	Ar.SetLimitSize(ShaderCode.GetActualShaderCodeSize());

	// was the shader already compiled offline?
	uint8 OfflineCompiledFlag;
	Ar << OfflineCompiledFlag;

	// get the header
	FMetalCodeHeader Header = { 0 };
	Ar << Header;
	
	Shader->TessellationHSOutBuffer = Header.TessellationHSOutBuffer;
	Shader->TessellationControlPointOutBuffer = Header.TessellationControlPointOutBuffer;

	switch (Header.TessellationOutputWinding)
	{
		// NOTE: cw and ccw are flipped
		case EMetalOutputWindingMode::Clockwise:		Shader->TessellationOutputWinding = MTLWindingCounterClockwise; break;
		case EMetalOutputWindingMode::CounterClockwise:	Shader->TessellationOutputWinding = MTLWindingClockwise; break;
		default: check(0);
	}
	
	switch (Header.TessellationPartitioning)
	{
		case EMetalPartitionMode::Pow2:				Shader->TessellationPartitioning = MTLTessellationPartitionModePow2; break;
		case EMetalPartitionMode::Integer:			Shader->TessellationPartitioning = MTLTessellationPartitionModeInteger; break;
		case EMetalPartitionMode::FractionalOdd:	Shader->TessellationPartitioning = MTLTessellationPartitionModeFractionalOdd; break;
		case EMetalPartitionMode::FractionalEven:	Shader->TessellationPartitioning = MTLTessellationPartitionModeFractionalEven; break;
		default: check(0);
	}
	return Shader;
}

FGeometryShaderRHIRef FMetalDynamicRHI::RHICreateGeometryShader(const TArray<uint8>& Code) 
{ 
	FMetalGeometryShader* Shader = new FMetalGeometryShader(Code);
	return Shader;
}

FGeometryShaderRHIRef FMetalDynamicRHI::RHICreateGeometryShaderWithStreamOutput(const TArray<uint8>& Code, const FStreamOutElementList& ElementList,
	uint32 NumStrides, const uint32* Strides, int32 RasterizedStream)
{
	checkf(0, TEXT("Not supported yet"));
	return NULL;
}

FComputeShaderRHIRef FMetalDynamicRHI::RHICreateComputeShader(const TArray<uint8>& Code) 
{ 
	FMetalComputeShader* Shader = new FMetalComputeShader(Code);
	
	// @todo WARNING: We have to hash here because of the way we immediately link and don't afford the cache a chance to set the OutputHash from ShaderCore.
	if (FShaderCache::GetShaderCache())
	{
		FSHAHash Hash;
		FSHA1::HashBuffer(Code.GetData(), Code.Num(), Hash.Hash);
		Shader->SetHash(Hash);
	}
	
	return Shader;
}

FMetalBoundShaderState::FMetalBoundShaderState(
			FVertexDeclarationRHIParamRef InVertexDeclarationRHI,
			FVertexShaderRHIParamRef InVertexShaderRHI,
			FPixelShaderRHIParamRef InPixelShaderRHI,
			FHullShaderRHIParamRef InHullShaderRHI,
			FDomainShaderRHIParamRef InDomainShaderRHI,
			FGeometryShaderRHIParamRef InGeometryShaderRHI)
	:	CacheLink(InVertexDeclarationRHI,InVertexShaderRHI,InPixelShaderRHI,InHullShaderRHI,InDomainShaderRHI,InGeometryShaderRHI,this)
{
	FMetalVertexDeclaration* InVertexDeclaration = FMetalDynamicRHI::ResourceCast(InVertexDeclarationRHI);
	FMetalVertexShader* InVertexShader = FMetalDynamicRHI::ResourceCast(InVertexShaderRHI);
	FMetalPixelShader* InPixelShader = FMetalDynamicRHI::ResourceCast(InPixelShaderRHI);
	FMetalHullShader* InHullShader = FMetalDynamicRHI::ResourceCast(InHullShaderRHI);
	FMetalDomainShader* InDomainShader = FMetalDynamicRHI::ResourceCast(InDomainShaderRHI);
	FMetalGeometryShader* InGeometryShader = FMetalDynamicRHI::ResourceCast(InGeometryShaderRHI);

	// cache everything
	VertexDeclaration = InVertexDeclaration;
	VertexShader = InVertexShader;
	PixelShader = InPixelShader;
	HullShader = InHullShader;
	DomainShader = InDomainShader;
	GeometryShader = InGeometryShader;

#if 0
	if (GFrameCounter > 3)
	{
		NSLog(@"===============================================================");
		NSLog(@"Creating a BSS at runtime frame %lld... this may hitch! [this = %p]", GFrameCounter, this);
		NSLog(@"Vertex declaration:");
		FVertexDeclarationElementList& Elements = VertexDeclaration->Elements;
		for (int32 i = 0; i < Elements.Num(); i++)
		{
			FVertexElement& Elem = Elements[i];
			NSLog(@"   Elem %d: attr: %d, stream: %d, type: %d, stride: %d, offset: %d", i, Elem.AttributeIndex, Elem.StreamIndex, (uint32)Elem.Type, Elem.Stride, Elem.Offset);
		}
		
		NSLog(@"\nVertexShader:");
		NSLog(@"%@", VertexShader ? VertexShader->GlslCodeNSString : @"NONE");
		NSLog(@"\nPixelShader:");
		NSLog(@"%@", PixelShader ? PixelShader->GlslCodeNSString : @"NONE");
		NSLog(@"===============================================================");
	}
#endif
	
	AddRef();
	
#if METAL_SUPPORTS_PARALLEL_RHI_EXECUTE
	CacheLink.AddToCache();
#endif
	
	int Err = pthread_rwlock_init(&PipelineMutex, nullptr);
	checkf(Err == 0, TEXT("pthread_rwlock_init failed with errno: %d"), Err);
}

FMetalBoundShaderState::~FMetalBoundShaderState()
{
#if METAL_SUPPORTS_PARALLEL_RHI_EXECUTE
	CacheLink.RemoveFromCache();
#endif
	
	int Err = pthread_rwlock_destroy(&PipelineMutex);
	checkf(Err == 0, TEXT("pthread_rwlock_destroy failed with errno: %d"), Err);
}

FMetalShaderPipeline* FMetalBoundShaderState::PrepareToDraw(FMetalHashedVertexDescriptor const& VertexDesc, const FMetalRenderPipelineDesc& RenderPipelineDesc)
{
	SCOPE_CYCLE_COUNTER(STAT_MetalBoundShaderPrepareDrawTime);
	
	// generate a key for the current statez
	FMetalRenderPipelineHash PipelineHash = RenderPipelineDesc.GetHash();
	
	if(GUseRHIThread)
	{
		SCOPE_CYCLE_COUNTER(STAT_MetalBoundShaderLockTime);
		int Err = pthread_rwlock_rdlock(&PipelineMutex);
		checkf(Err == 0, TEXT("pthread_rwlock_rdlock failed with errno: %d"), Err);
	}
	
	// have we made a matching state object yet?
    FMetalShaderPipeline* PipelineStatePack = nil;
	TMap<FMetalHashedVertexDescriptor, FMetalShaderPipeline*>* Dict = PipelineStates.Find(PipelineHash);
	if (Dict)
	{
		PipelineStatePack = Dict->FindRef(VertexDesc);
	}
	
	if(GUseRHIThread)
	{
		SCOPE_CYCLE_COUNTER(STAT_MetalBoundShaderLockTime);
		int Err = pthread_rwlock_unlock(&PipelineMutex);
		checkf(Err == 0, TEXT("pthread_rwlock_unlock failed with errno: %d"), Err);
	}
	
	// make one if not
	if (PipelineStatePack == nil)
	{
		PipelineStatePack = RenderPipelineDesc.CreatePipelineStateForBoundShaderState(this, VertexDesc);
		check(PipelineStatePack);
		
		if(GUseRHIThread)
		{
			SCOPE_CYCLE_COUNTER(STAT_MetalBoundShaderLockTime);
			int Err = pthread_rwlock_wrlock(&PipelineMutex);
			checkf(Err == 0, TEXT("pthread_rwlock_wrlock failed with errno: %d"), Err);
		}
		
		Dict = PipelineStates.Find(PipelineHash);
		FMetalShaderPipeline* ExistingPipeline = Dict ? Dict->FindRef(VertexDesc) : nil;
		if (!ExistingPipeline)
		{
			if (Dict)
			{
				Dict->Add(VertexDesc, PipelineStatePack);
			}
			else
			{
				TMap<FMetalHashedVertexDescriptor, FMetalShaderPipeline*>& InnerDict = PipelineStates.Add(PipelineHash);
				InnerDict.Add(VertexDesc, PipelineStatePack);
			}
		}
		else
		{
			PipelineStatePack = ExistingPipeline;
		}
		
		if(GUseRHIThread)
		{
			SCOPE_CYCLE_COUNTER(STAT_MetalBoundShaderLockTime);
			int Err = pthread_rwlock_unlock(&PipelineMutex);
			checkf(Err == 0, TEXT("pthread_rwlock_unlock failed with errno: %d"), Err);
		}
		
#if METAL_DEBUG_OPTIONS
		if (GFrameCounter > 3)
		{
			NSLog(@"Created a hitchy pipeline state for hash %llx%llx%llx (this = %p)", (uint64)PipelineHash.RasterBits, (uint64)(PipelineHash.TargetBits), (uint64)VertexDesc.VertexDescHash, this);
		}
#endif
	}

	return PipelineStatePack;
}

FBoundShaderStateRHIRef FMetalDynamicRHI::RHICreateBoundShaderState(
	FVertexDeclarationRHIParamRef VertexDeclarationRHI, 
	FVertexShaderRHIParamRef VertexShaderRHI, 
	FHullShaderRHIParamRef HullShaderRHI, 
	FDomainShaderRHIParamRef DomainShaderRHI, 
	FPixelShaderRHIParamRef PixelShaderRHI,
	FGeometryShaderRHIParamRef GeometryShaderRHI
	)
{
#if METAL_SUPPORTS_PARALLEL_RHI_EXECUTE
	// Check for an existing bound shader state which matches the parameters
	FBoundShaderStateRHIRef CachedBoundShaderStateLink = GetCachedBoundShaderState_Threadsafe(
		VertexDeclarationRHI,
		VertexShaderRHI,
		PixelShaderRHI,
		HullShaderRHI,
		DomainShaderRHI,
		GeometryShaderRHI
		);

	if(CachedBoundShaderStateLink.GetReference())
	{
		// If we've already created a bound shader state with these parameters, reuse it.
		return CachedBoundShaderStateLink;
	}
#else
	// Check for an existing bound shader state which matches the parameters
	FCachedBoundShaderStateLink* CachedBoundShaderStateLink = GetCachedBoundShaderState(
		VertexDeclarationRHI,
		VertexShaderRHI,
		PixelShaderRHI,
		HullShaderRHI,
		DomainShaderRHI,
		GeometryShaderRHI
		);

	if(CachedBoundShaderStateLink)
	{
		// If we've already created a bound shader state with these parameters, reuse it.
		return CachedBoundShaderStateLink->BoundShaderState;
	}
#endif
	else
	{
		SCOPE_CYCLE_COUNTER(STAT_MetalBoundShaderStateTime);
//		FBoundShaderStateKey Key(VertexDeclarationRHI,VertexShaderRHI,PixelShaderRHI,HullShaderRHI,DomainShaderRHI,GeometryShaderRHI);
//		NSLog(@"BSS Key = %x", GetTypeHash(Key));
		FMetalBoundShaderState* State = new FMetalBoundShaderState(VertexDeclarationRHI,VertexShaderRHI,PixelShaderRHI,HullShaderRHI,DomainShaderRHI,GeometryShaderRHI);
		
		FShaderCache::LogBoundShaderState(GMaxRHIShaderPlatform, VertexDeclarationRHI, VertexShaderRHI, State->PixelShader, HullShaderRHI, DomainShaderRHI, GeometryShaderRHI, State);
		
		return State;
	}
}








FMetalShaderParameterCache::FMetalShaderParameterCache() :
	GlobalUniformArraySize(-1)
{
	for (int32 ArrayIndex = 0; ArrayIndex < CrossCompiler::PACKED_TYPEINDEX_MAX; ++ArrayIndex)
	{
		PackedGlobalUniformDirty[ArrayIndex].LowVector = 0;
		PackedGlobalUniformDirty[ArrayIndex].HighVector = 0;
	}
}

void FMetalShaderParameterCache::InitializeResources(int32 UniformArraySize)
{
	check(GlobalUniformArraySize == -1);
	
	PackedGlobalUniforms[0] = (uint8*)FMemory::Malloc(UniformArraySize * CrossCompiler::PACKED_TYPEINDEX_MAX);
	PackedUniformsScratch[0] = (uint8*)FMemory::Malloc(UniformArraySize * CrossCompiler::PACKED_TYPEINDEX_MAX);
	
	FMemory::Memzero(PackedGlobalUniforms[0], UniformArraySize * CrossCompiler::PACKED_TYPEINDEX_MAX);
	FMemory::Memzero(PackedUniformsScratch[0], UniformArraySize * CrossCompiler::PACKED_TYPEINDEX_MAX);
	for (int32 ArrayIndex = 1; ArrayIndex < CrossCompiler::PACKED_TYPEINDEX_MAX; ++ArrayIndex)
	{
		PackedGlobalUniforms[ArrayIndex] = PackedGlobalUniforms[ArrayIndex - 1] + UniformArraySize;
		PackedUniformsScratch[ArrayIndex] = PackedUniformsScratch[ArrayIndex - 1] + UniformArraySize;
	}
	GlobalUniformArraySize = UniformArraySize;
	
	for (int32 ArrayIndex = 0; ArrayIndex < CrossCompiler::PACKED_TYPEINDEX_MAX; ++ArrayIndex)
	{
		PackedGlobalUniformDirty[ArrayIndex].LowVector = 0;
		PackedGlobalUniformDirty[ArrayIndex].HighVector = 0;
	}
}

/** Destructor. */
FMetalShaderParameterCache::~FMetalShaderParameterCache()
{
	if (GlobalUniformArraySize > 0)
	{
		FMemory::Free(PackedUniformsScratch[0]);
		FMemory::Free(PackedGlobalUniforms[0]);
	}
	
	FMemory::Memzero(PackedUniformsScratch);
	FMemory::Memzero(PackedGlobalUniforms);
	
	GlobalUniformArraySize = -1;
}

/**
 * Marks all uniform arrays as dirty.
 */
void FMetalShaderParameterCache::MarkAllDirty()
{
	for (int32 ArrayIndex = 0; ArrayIndex < CrossCompiler::PACKED_TYPEINDEX_MAX; ++ArrayIndex)
	{
		PackedGlobalUniformDirty[ArrayIndex].LowVector = 0;
		PackedGlobalUniformDirty[ArrayIndex].HighVector = 0;//UniformArraySize / 16;
	}
}

const int SizeOfFloat4 = 4 * sizeof(float);

/**
 * Set parameter values.
 */
void FMetalShaderParameterCache::Set(uint32 BufferIndexName, uint32 ByteOffset, uint32 NumBytes, const void* NewValues)
{
	uint32 BufferIndex = CrossCompiler::PackedTypeNameToTypeIndex(BufferIndexName);
	check(GlobalUniformArraySize != -1);
	check(BufferIndex < CrossCompiler::PACKED_TYPEINDEX_MAX);
	check(ByteOffset + NumBytes <= (uint32)GlobalUniformArraySize);
	PackedGlobalUniformDirty[BufferIndex].LowVector = FMath::Min(PackedGlobalUniformDirty[BufferIndex].LowVector, ByteOffset / SizeOfFloat4);
	PackedGlobalUniformDirty[BufferIndex].HighVector = FMath::Max(PackedGlobalUniformDirty[BufferIndex].HighVector, (ByteOffset + NumBytes + SizeOfFloat4 - 1) / SizeOfFloat4);
	FMemory::Memcpy(PackedGlobalUniforms[BufferIndex] + ByteOffset, NewValues, NumBytes);
}

void FMetalShaderParameterCache::CommitPackedGlobals(FMetalStateCache* Cache, FMetalCommandEncoder* Encoder, EShaderFrequency Frequency, const FMetalShaderBindings& Bindings)
{
	// copy the current uniform buffer into the ring buffer to submit
	for (int32 Index = 0; Index < Bindings.PackedGlobalArrays.Num(); ++Index)
	{
		int32 UniformBufferIndex = Bindings.PackedGlobalArrays[Index].TypeIndex;
 
		// is there any data that needs to be copied?
		if (PackedGlobalUniformDirty[UniformBufferIndex].HighVector > 0)
		{
			uint32 TotalSize = Bindings.PackedGlobalArrays[Index].Size;
			uint32 SizeToUpload = PackedGlobalUniformDirty[UniformBufferIndex].HighVector * SizeOfFloat4;
			
			//@todo-rco: Temp workaround
			SizeToUpload = TotalSize;
			
			//@todo-rco: Temp workaround
			uint8 const* Bytes = PackedGlobalUniforms[UniformBufferIndex];
			uint32 Size = FMath::Min(TotalSize, SizeToUpload);
			
			uint32 Offset = Encoder->GetRingBuffer()->Allocate(Size, 0);
			id<MTLBuffer> Buffer = Encoder->GetRingBuffer()->Buffer;
			
			FMemory::Memcpy((uint8*)[Buffer contents] + Offset, Bytes, Size);
			
			Cache->SetShaderBuffer(Frequency, Buffer, nil, Offset, Size, UniformBufferIndex);

			// mark as clean
			PackedGlobalUniformDirty[UniformBufferIndex].HighVector = 0;
		}
	}
}

void FMetalShaderParameterCache::CommitPackedUniformBuffers(TRefCountPtr<FMetalBoundShaderState> BoundShaderState, FMetalComputeShader* ComputeShader, int32 Stage, const TArray< TRefCountPtr<FRHIUniformBuffer> >& RHIUniformBuffers, const TArray<CrossCompiler::FUniformBufferCopyInfo>& UniformBuffersCopyInfo)
{
//	SCOPE_CYCLE_COUNTER(STAT_MetalConstantBufferUpdateTime);
	// Uniform Buffers are split into precision/type; the list of RHI UBs is traversed and if a new one was set, its
	// contents are copied per precision/type into corresponding scratch buffers which are then uploaded to the program
	if (Stage == CrossCompiler::SHADER_STAGE_PIXEL && !IsValidRef(BoundShaderState->PixelShader))
	{
		return;
	}

	auto& Bindings = [this, &Stage, &BoundShaderState, ComputeShader]() -> FMetalShaderBindings& {
		switch(Stage) {
			default: check(0);
			case CrossCompiler::SHADER_STAGE_VERTEX: return BoundShaderState->VertexShader->Bindings;
			case CrossCompiler::SHADER_STAGE_PIXEL: return BoundShaderState->PixelShader->Bindings;
			case CrossCompiler::SHADER_STAGE_COMPUTE: return ComputeShader->Bindings;
			case CrossCompiler::SHADER_STAGE_HULL: return BoundShaderState->HullShader->Bindings;
			case CrossCompiler::SHADER_STAGE_DOMAIN: return BoundShaderState->DomainShader->Bindings;
		}
	}();

	if (!Bindings.bHasRegularUniformBuffers && !FShaderCache::IsPredrawCall())
	{
		check(Bindings.NumUniformBuffers <= RHIUniformBuffers.Num());
		int32 LastInfoIndex = 0;
		for (int32 BufferIndex = 0; BufferIndex < Bindings.NumUniformBuffers; ++BufferIndex)
		{
			const FRHIUniformBuffer* RHIUniformBuffer = RHIUniformBuffers[BufferIndex];
			check(RHIUniformBuffer);
			FMetalUniformBuffer* EmulatedUniformBuffer = (FMetalUniformBuffer*)RHIUniformBuffer;
			const uint32* RESTRICT SourceData = (uint32 const*)((uint8 const*)EmulatedUniformBuffer->GetData() + EmulatedUniformBuffer->Offset);
			for (int32 InfoIndex = LastInfoIndex; InfoIndex < UniformBuffersCopyInfo.Num(); ++InfoIndex)
			{
				const CrossCompiler::FUniformBufferCopyInfo& Info = UniformBuffersCopyInfo[InfoIndex];
				if (Info.SourceUBIndex == BufferIndex)
				{
					float* RESTRICT ScratchMem = (float*)PackedGlobalUniforms[Info.DestUBTypeIndex];
					ScratchMem += Info.DestOffsetInFloats;
					FMemory::Memcpy(ScratchMem, SourceData + Info.SourceOffsetInFloats, Info.SizeInFloats * sizeof(float));
					PackedGlobalUniformDirty[Info.DestUBTypeIndex].LowVector = FMath::Min(PackedGlobalUniformDirty[Info.DestUBTypeIndex].LowVector, uint32(Info.DestOffsetInFloats / SizeOfFloat4));
					PackedGlobalUniformDirty[Info.DestUBTypeIndex].HighVector = FMath::Max(PackedGlobalUniformDirty[Info.DestUBTypeIndex].HighVector, uint32(((Info.DestOffsetInFloats + Info.SizeInFloats) * sizeof(float) + SizeOfFloat4 - 1) / SizeOfFloat4));
				}
				else
				{
					LastInfoIndex = InfoIndex;
					break;
				}
			}
		}
	}
}
