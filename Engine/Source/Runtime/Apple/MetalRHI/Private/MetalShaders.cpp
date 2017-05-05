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

static FString METAL_LIB_EXTENSION(TEXT(".metallib"));
static FString METAL_MAP_EXTENSION(TEXT(".metalmap"));

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
void TMetalBaseShader<BaseResourceType, ShaderType>::Init(const TArray<uint8>& InShaderCode, FMetalCodeHeader& Header)
{
	FShaderCodeReader ShaderCode(InShaderCode);

	FMemoryReader Ar(InShaderCode, true);
	
	Ar.SetLimitSize(ShaderCode.GetActualShaderCodeSize());

	// was the shader already compiled offline?
	uint8 OfflineCompiledFlag;
	Ar << OfflineCompiledFlag;
	check(OfflineCompiledFlag == 0 || OfflineCompiledFlag == 1);

	// get the header
	Header = { 0 };
	Ar << Header;

	SourceLen = Header.SourceLen;
	SourceCRC = Header.SourceCRC;

	// remember where the header ended and code (precompiled or source) begins
	int32 CodeOffset = Ar.Tell();
	uint32 BufferSize = ShaderCode.GetActualShaderCodeSize() - CodeOffset;
	const ANSICHAR* SourceCode = (ANSICHAR*)InShaderCode.GetData() + CodeOffset;

	if (!OfflineCompiledFlag)
	{
		UE_LOG(LogMetal, Display, TEXT("Loaded a non-offline compiled shader (will be slower to load)"));
	}
	
	FMetalCompiledShaderKey Key(Header.SourceLen, Header.SourceCRC);

	// Find the existing compiled shader in the cache.
	if (!Header.bFunctionConstants)
	{
		Function = [GetMetalCompiledShaderCache().FindRef(Key) retain];
	}
	if (!Function)
	{
		// Archived shaders should never get in here.
		check(!(Header.CompileFlags & (1 << CFLAG_Archive)) || BufferSize > 0);
		
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
			// Get the function from the library - the function name is "Main" followed by the CRC32 of the source MTLSL as 0-padded hex.
			// This ensures that even if we move to a unified library that the function names will be unique - duplicates will only have one entry in the library.
			NSString* Name = [NSString stringWithFormat:@"Main_%0.8x_%0.8x", Header.SourceLen, Header.SourceCRC];
			Function = [[Library newFunctionWithName:Name] retain];
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
}

template<typename BaseResourceType, int32 ShaderType>
void TMetalBaseShader<BaseResourceType, ShaderType>::Init(const TArray<uint8>& InShaderCode, id<MTLLibrary> InLibrary, FMetalCodeHeader& Header)
{
	check(InLibrary);
	
	FShaderCodeReader ShaderCode(InShaderCode);
	
	FMemoryReader Ar(InShaderCode, true);
	
	Ar.SetLimitSize(ShaderCode.GetActualShaderCodeSize());
	
	// was the shader already compiled offline?
	uint8 OfflineCompiledFlag;
	Ar << OfflineCompiledFlag;
	
	// Should always be offline compiled
	if(OfflineCompiledFlag == 1)
	{
		// get the header
		Header = { 0 };
		Ar << Header;

		SourceLen = Header.SourceLen;
		SourceCRC = Header.SourceCRC;

		// Only archived shaders should be in here.
		UE_CLOG(!(Header.CompileFlags & (1 << CFLAG_Archive)), LogMetal, Warning, TEXT("Loaded a shader from a library that wasn't marked for archiving."));
		{
			// Find the existing compiled shader in the cache.
			FMetalCompiledShaderKey Key(Header.SourceLen, Header.SourceCRC);
			if (!Header.bFunctionConstants)
			{
				Function = [GetMetalCompiledShaderCache().FindRef(Key) retain];
			}
			if (!Function)
			{
				Library = [InLibrary retain];
				
				static NSString* const Offline = @"OFFLINE";
				GlslCodeNSString = [Offline retain];

				if (!Header.bFunctionConstants)
				{
					// Get the function from the library - the function name is "Main" followed by the CRC32 of the source MTLSL as 0-padded hex.
					// This ensures that even if we move to a unified library that the function names will be unique - duplicates will only have one entry in the library.
					NSString* Name = [NSString stringWithFormat:@"Main_%0.8x_%0.8x", Header.SourceLen, Header.SourceCRC];
					Function = [[Library newFunctionWithName:Name] retain];
					check(Function);
					if (Function)
					{
						GetMetalCompiledShaderCache().Add(Key, Function);
						TRACK_OBJECT(STAT_MetalFunctionCount, Function);
					}
					[Library release];
					Library = nil;
				}
			}
			
			Bindings = Header.Bindings;
			UniformBuffersCopyInfo = Header.UniformBuffersCopyInfo;
			SideTableBinding = Header.SideTable;
		}
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
: Pipeline(nil)
, NumThreadsX(0)
, NumThreadsY(0)
, NumThreadsZ(0)
{
	FMetalCodeHeader Header = {0};
	Init(InCode, Header);
	
	NumThreadsX = FMath::Max((int32)Header.NumThreadsX, 1);
	NumThreadsY = FMath::Max((int32)Header.NumThreadsY, 1);
	NumThreadsZ = FMath::Max((int32)Header.NumThreadsZ, 1);
	
	NSError* Error;
	
	id<MTLComputePipelineState> Kernel = nil;
	MTLComputePipelineReflection* Reflection = nil;
	
#if METAL_DEBUG_OPTIONS
	if (GetMetalDeviceContext().GetCommandQueue().GetRuntimeDebuggingLevel() >= EMetalDebugLevelFastValidation)
	{
		MTLAutoreleasedComputePipelineReflection ComputeReflection;
		Kernel = [[GetMetalDeviceContext().GetDevice() newComputePipelineStateWithFunction:Function options:MTLPipelineOptionArgumentInfo reflection:&ComputeReflection error:&Error] autorelease];
		Reflection = ComputeReflection;
	}
	else
#endif
	{
		Kernel = [[GetMetalDeviceContext().GetDevice() newComputePipelineStateWithFunction:Function error:&Error] autorelease];
	}
	
	if (Kernel == nil)
	{
        NSLog(@"Failed to create kernel: %@", Error);
        NSLog(@"*********** Error\n%@", GlslCodeNSString);
        
        UE_LOG(LogRHI, Fatal, TEXT("Failed to create compute kernel: %s"), *FString([Error description]));
	}
	
	Pipeline = [FMetalShaderPipeline new];
	Pipeline.ComputePipelineState = Kernel;
#if METAL_DEBUG_OPTIONS
    Pipeline.ComputePipelineReflection = Reflection;
	Pipeline.ComputeSource = GlslCodeNSString;
#endif
	METAL_DEBUG_OPTION(FMemory::Memzero(Pipeline->ResourceMask, sizeof(Pipeline->ResourceMask)));
	TRACK_OBJECT(STAT_MetalComputePipelineStateCount, Pipeline);
}

FMetalComputeShader::FMetalComputeShader(const TArray<uint8>& InCode, id<MTLLibrary> InLibrary)
: Pipeline(nil)
, NumThreadsX(0)
, NumThreadsY(0)
, NumThreadsZ(0)
{
	FMetalCodeHeader Header = {0};
	Init(InCode, InLibrary, Header);
	
	NumThreadsX = FMath::Max((int32)Header.NumThreadsX, 1);
	NumThreadsY = FMath::Max((int32)Header.NumThreadsY, 1);
	NumThreadsZ = FMath::Max((int32)Header.NumThreadsZ, 1);
	
	if (Function)
	{
		NSError* Error;
		
		id<MTLComputePipelineState> Kernel = nil;
		MTLComputePipelineReflection* Reflection = nil;
		
	#if METAL_DEBUG_OPTIONS
		if (GetMetalDeviceContext().GetCommandQueue().GetRuntimeDebuggingLevel() >= EMetalDebugLevelFastValidation)
		{
			MTLAutoreleasedComputePipelineReflection ComputeReflection;
			Kernel = [[GetMetalDeviceContext().GetDevice() newComputePipelineStateWithFunction:Function options:MTLPipelineOptionArgumentInfo reflection:&ComputeReflection error:&Error] autorelease];
			Reflection = ComputeReflection;
		}
		else
	#endif
		{
			Kernel = [[GetMetalDeviceContext().GetDevice() newComputePipelineStateWithFunction:Function error:&Error] autorelease];
		}
		
		if (Kernel == nil)
		{
			NSLog(@"Failed to create kernel: %@", Error);
			NSLog(@"*********** Error\n%@", GlslCodeNSString);
			
			UE_LOG(LogRHI, Fatal, TEXT("Failed to create compute kernel: %s"), *FString([Error description]));
		}
		
		Pipeline = [FMetalShaderPipeline new];
		Pipeline.ComputePipelineState = Kernel;
#if METAL_DEBUG_OPTIONS
        Pipeline.ComputePipelineReflection = Reflection;
		Pipeline.ComputeSource = GlslCodeNSString;
#endif
		METAL_DEBUG_OPTION(FMemory::Memzero(Pipeline->ResourceMask, sizeof(Pipeline->ResourceMask)));
		TRACK_OBJECT(STAT_MetalComputePipelineStateCount, Pipeline);
	}
}

FMetalComputeShader::~FMetalComputeShader()
{
	if (Pipeline)
	{
		UNTRACK_OBJECT(STAT_MetalComputePipelineStateCount, Pipeline);
	}
	[Pipeline release];
	Pipeline = nil;
}

FMetalVertexShader::FMetalVertexShader(const TArray<uint8>& InCode)
{
	FMetalCodeHeader Header = {0};
	Init(InCode, Header);
	
	TessellationOutputAttribs = Header.TessellationOutputAttribs;
	TessellationPatchCountBuffer = Header.TessellationPatchCountBuffer;
	TessellationIndexBuffer = Header.TessellationIndexBuffer;
	TessellationHSOutBuffer = Header.TessellationHSOutBuffer;
	TessellationHSTFOutBuffer = Header.TessellationHSTFOutBuffer;
	TessellationControlPointOutBuffer = Header.TessellationControlPointOutBuffer;
	TessellationControlPointIndexBuffer = Header.TessellationControlPointIndexBuffer;
	TessellationOutputControlPoints = Header.TessellationOutputControlPoints;
	TessellationDomain = Header.TessellationDomain;
	TessellationInputControlPoints = Header.TessellationInputControlPoints;
	TessellationMaxTessFactor = Header.TessellationMaxTessFactor;
	TessellationPatchesPerThreadGroup = Header.TessellationPatchesPerThreadGroup;
}

FMetalVertexShader::FMetalVertexShader(const TArray<uint8>& InCode, id<MTLLibrary> InLibrary)
{
	FMetalCodeHeader Header = {0};
	Init(InCode, InLibrary, Header);
	
	TessellationOutputAttribs = Header.TessellationOutputAttribs;
	TessellationPatchCountBuffer = Header.TessellationPatchCountBuffer;
	TessellationIndexBuffer = Header.TessellationIndexBuffer;
	TessellationHSOutBuffer = Header.TessellationHSOutBuffer;
	TessellationHSTFOutBuffer = Header.TessellationHSTFOutBuffer;
	TessellationControlPointOutBuffer = Header.TessellationControlPointOutBuffer;
	TessellationControlPointIndexBuffer = Header.TessellationControlPointIndexBuffer;
	TessellationOutputControlPoints = Header.TessellationOutputControlPoints;
	TessellationDomain = Header.TessellationDomain;
	TessellationInputControlPoints = Header.TessellationInputControlPoints;
	TessellationMaxTessFactor = Header.TessellationMaxTessFactor;
	TessellationPatchesPerThreadGroup = Header.TessellationPatchesPerThreadGroup;
}

FMetalPixelShader::FMetalPixelShader(const TArray<uint8>& InCode)
{
	FMetalCodeHeader Header = {0};
	Init(InCode, Header);
}

FMetalPixelShader::FMetalPixelShader(const TArray<uint8>& InCode, id<MTLLibrary> InLibrary)
{
	FMetalCodeHeader Header = {0};
	Init(InCode, InLibrary, Header);
}

FMetalHullShader::FMetalHullShader(const TArray<uint8>& InCode)
{
	FMetalCodeHeader Header = {0};
	Init(InCode, Header);
}

FMetalHullShader::FMetalHullShader(const TArray<uint8>& InCode, id<MTLLibrary> InLibrary)
{
	FMetalCodeHeader Header = {0};
	Init(InCode, InLibrary, Header);
}

FMetalDomainShader::FMetalDomainShader(const TArray<uint8>& InCode)
{
	FMetalCodeHeader Header = {0};
	Init(InCode, Header);
	
	// for VSHS
	TessellationHSOutBuffer = Header.TessellationHSOutBuffer;
	TessellationControlPointOutBuffer = Header.TessellationControlPointOutBuffer;
	
	switch (Header.TessellationOutputWinding)
	{
		// NOTE: cw and ccw are flipped
		case EMetalOutputWindingMode::Clockwise:		TessellationOutputWinding = MTLWindingCounterClockwise; break;
		case EMetalOutputWindingMode::CounterClockwise:	TessellationOutputWinding = MTLWindingClockwise; break;
		default: check(0);
	}
	
	switch (Header.TessellationPartitioning)
	{
		case EMetalPartitionMode::Pow2:				TessellationPartitioning = MTLTessellationPartitionModePow2; break;
		case EMetalPartitionMode::Integer:			TessellationPartitioning = MTLTessellationPartitionModeInteger; break;
		case EMetalPartitionMode::FractionalOdd:	TessellationPartitioning = MTLTessellationPartitionModeFractionalOdd; break;
		case EMetalPartitionMode::FractionalEven:	TessellationPartitioning = MTLTessellationPartitionModeFractionalEven; break;
		default: check(0);
	}
}

FMetalDomainShader::FMetalDomainShader(const TArray<uint8>& InCode, id<MTLLibrary> InLibrary)
{
	FMetalCodeHeader Header = {0};
	Init(InCode, InLibrary, Header);
	
	// for VSHS
	TessellationHSOutBuffer = Header.TessellationHSOutBuffer;
	TessellationControlPointOutBuffer = Header.TessellationControlPointOutBuffer;
	
	switch (Header.TessellationOutputWinding)
	{
		// NOTE: cw and ccw are flipped
		case EMetalOutputWindingMode::Clockwise:		TessellationOutputWinding = MTLWindingCounterClockwise; break;
		case EMetalOutputWindingMode::CounterClockwise:	TessellationOutputWinding = MTLWindingClockwise; break;
		default: check(0);
	}
	
	switch (Header.TessellationPartitioning)
	{
		case EMetalPartitionMode::Pow2:				TessellationPartitioning = MTLTessellationPartitionModePow2; break;
		case EMetalPartitionMode::Integer:			TessellationPartitioning = MTLTessellationPartitionModeInteger; break;
		case EMetalPartitionMode::FractionalOdd:	TessellationPartitioning = MTLTessellationPartitionModeFractionalOdd; break;
		case EMetalPartitionMode::FractionalEven:	TessellationPartitioning = MTLTessellationPartitionModeFractionalEven; break;
		default: check(0);
	}
}

FVertexShaderRHIRef FMetalDynamicRHI::RHICreateVertexShader(const TArray<uint8>& Code)
{
    @autoreleasepool {
	FMetalVertexShader* Shader = new FMetalVertexShader(Code);
	return Shader;
	}
}

FVertexShaderRHIRef FMetalDynamicRHI::RHICreateVertexShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	@autoreleasepool {

	checkSlow(Library && Library->IsNativeLibrary() && IsMetalPlatform(Library->GetPlatform()) && Library->GetPlatform() <= GMaxRHIShaderPlatform);
	
	FMetalShaderLibrary* MetalLibrary = ResourceCast(Library);
	return MetalLibrary->CreateVertexShader(Hash);
	
	}
}

FPixelShaderRHIRef FMetalDynamicRHI::RHICreatePixelShader(const TArray<uint8>& Code)
{
	@autoreleasepool {
	FMetalPixelShader* Shader = new FMetalPixelShader(Code);
	return Shader;
	}
}

FPixelShaderRHIRef FMetalDynamicRHI::RHICreatePixelShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	@autoreleasepool {
	
	checkSlow(Library && Library->IsNativeLibrary() && IsMetalPlatform(Library->GetPlatform()) && Library->GetPlatform() <= GMaxRHIShaderPlatform);
	
	FMetalShaderLibrary* MetalLibrary = ResourceCast(Library);
	return MetalLibrary->CreatePixelShader(Hash);
	
	}
}

FHullShaderRHIRef FMetalDynamicRHI::RHICreateHullShader(const TArray<uint8>& Code) 
{
	@autoreleasepool {
	FMetalHullShader* Shader = new FMetalHullShader(Code);
	return Shader;
	}
}

FHullShaderRHIRef FMetalDynamicRHI::RHICreateHullShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	@autoreleasepool {
	
	checkSlow(Library && Library->IsNativeLibrary() && IsMetalPlatform(Library->GetPlatform()) && Library->GetPlatform() <= GMaxRHIShaderPlatform);
	
	FMetalShaderLibrary* MetalLibrary = ResourceCast(Library);
	return MetalLibrary->CreateHullShader(Hash);
	
	}
}

FDomainShaderRHIRef FMetalDynamicRHI::RHICreateDomainShader(const TArray<uint8>& Code) 
{
	@autoreleasepool {
	FMetalDomainShader* Shader = new FMetalDomainShader(Code);
	return Shader;
	}
}

FDomainShaderRHIRef FMetalDynamicRHI::RHICreateDomainShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	@autoreleasepool {
	
	checkSlow(Library && Library->IsNativeLibrary() && IsMetalPlatform(Library->GetPlatform()) && Library->GetPlatform() <= GMaxRHIShaderPlatform);
	
	FMetalShaderLibrary* MetalLibrary = ResourceCast(Library);
	return MetalLibrary->CreateDomainShader(Hash);
	
	}
}

FGeometryShaderRHIRef FMetalDynamicRHI::RHICreateGeometryShader(const TArray<uint8>& Code) 
{
	@autoreleasepool {
	FMetalGeometryShader* Shader = new FMetalGeometryShader;
	FMetalCodeHeader Header = {0};
	Shader->Init(Code, Header);
	return Shader;
	}
}

FGeometryShaderRHIRef FMetalDynamicRHI::RHICreateGeometryShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	@autoreleasepool {
	
	checkSlow(Library && Library->IsNativeLibrary() && IsMetalPlatform(Library->GetPlatform()) && Library->GetPlatform() <= GMaxRHIShaderPlatform);

	FMetalShaderLibrary* MetalLibrary = ResourceCast(Library);
	return MetalLibrary->CreateGeometryShader(Hash);
	
	}
}

FGeometryShaderRHIRef FMetalDynamicRHI::RHICreateGeometryShaderWithStreamOutput(const TArray<uint8>& Code, const FStreamOutElementList& ElementList,
	uint32 NumStrides, const uint32* Strides, int32 RasterizedStream)
{
	checkf(0, TEXT("Not supported yet"));
	return NULL;
}

FGeometryShaderRHIRef FMetalDynamicRHI::RHICreateGeometryShaderWithStreamOutput(const FStreamOutElementList& ElementList,
	uint32 NumStrides, const uint32* Strides, int32 RasterizedStream, FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	@autoreleasepool {
	
	checkSlow(Library && Library->IsNativeLibrary() && IsMetalPlatform(Library->GetPlatform()) && Library->GetPlatform() <= GMaxRHIShaderPlatform);
	
	FMetalShaderLibrary* MetalLibrary = ResourceCast(Library);
	return MetalLibrary->CreateGeometryShaderWithStreamOutput(Hash, ElementList, NumStrides, Strides, RasterizedStream);
	
	}
}

FComputeShaderRHIRef FMetalDynamicRHI::RHICreateComputeShader(const TArray<uint8>& Code) 
{
	@autoreleasepool {
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
}

FComputeShaderRHIRef FMetalDynamicRHI::RHICreateComputeShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash) 
{ 
	@autoreleasepool {
	
	checkSlow(Library && Library->IsNativeLibrary() && IsMetalPlatform(Library->GetPlatform()) && Library->GetPlatform() <= GMaxRHIShaderPlatform);

	FMetalShaderLibrary* MetalLibrary = ResourceCast(Library);
	FComputeShaderRHIRef Shader = MetalLibrary->CreateComputeShader(Hash);
	
	if(Shader.IsValid() && FShaderCache::GetShaderCache())
	{
		// @todo WARNING: We have to hash here because of the way we immediately link and don't afford the cache a chance to set the OutputHash from ShaderCore.
		Shader->SetHash(Hash);
	}
	
	return Shader;
	
	}
}

FMetalShaderLibrary::FMetalShaderLibrary(EShaderPlatform InPlatform, id<MTLLibrary> InLibrary, FMetalShaderMap const& InMap)
: FRHIShaderLibrary(InPlatform)
, Library(InLibrary)
, Map(InMap)
{
	
}

FMetalShaderLibrary::~FMetalShaderLibrary()
{
	
}

FPixelShaderRHIRef FMetalShaderLibrary::CreatePixelShader(const FSHAHash& Hash)
{
	TPair<uint8, TArray<uint8>>* Code = Map.HashMap.Find(Hash);
	if (Code)
	{
		FMetalPixelShader* Shader = new FMetalPixelShader(Code->Value, Library);
		if (Shader->Library || Shader->Function)
		{
			return Shader;
		}
		else
		{
			delete Shader;
		}
	}
	
	return FPixelShaderRHIRef();
}

FVertexShaderRHIRef FMetalShaderLibrary::CreateVertexShader(const FSHAHash& Hash)
{
	TPair<uint8, TArray<uint8>>* Code = Map.HashMap.Find(Hash);
	if (Code)
	{
		FMetalVertexShader* Shader = new FMetalVertexShader(Code->Value, Library);
		if (Shader->Library || Shader->Function)
		{
			return Shader;
		}
		else
		{
			delete Shader;
		}
	}
	return FVertexShaderRHIRef();
}

FHullShaderRHIRef FMetalShaderLibrary::CreateHullShader(const FSHAHash& Hash)
{
	TPair<uint8, TArray<uint8>>* Code = Map.HashMap.Find(Hash);
	if (Code)
	{
		FMetalHullShader* Shader = new FMetalHullShader(Code->Value, Library);
		if(Shader->Library || Shader->Function)
		{
			return Shader;
		}
		else
		{
			delete Shader;
		}
	}
	return FHullShaderRHIRef();
}

FDomainShaderRHIRef FMetalShaderLibrary::CreateDomainShader(const FSHAHash& Hash)
{
	TPair<uint8, TArray<uint8>>* Code = Map.HashMap.Find(Hash);
	if (Code)
	{
		FMetalDomainShader* Shader = new FMetalDomainShader(Code->Value, Library);
		if (Shader->Library || Shader->Function)
		{
			return Shader;
		}
		else
		{
			delete Shader;
		}
	}
	return FDomainShaderRHIRef();
}

FGeometryShaderRHIRef FMetalShaderLibrary::CreateGeometryShader(const FSHAHash& Hash)
{
	checkf(0, TEXT("Not supported yet"));
	return NULL;
}

FGeometryShaderRHIRef FMetalShaderLibrary::CreateGeometryShaderWithStreamOutput(const FSHAHash& Hash, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream)
{
	checkf(0, TEXT("Not supported yet"));
	return NULL;
}

FComputeShaderRHIRef FMetalShaderLibrary::CreateComputeShader(const FSHAHash& Hash)
{
	TPair<uint8, TArray<uint8>>* Code = Map.HashMap.Find(Hash);
	if (Code)
	{
		FMetalComputeShader* Shader = new FMetalComputeShader(Code->Value, Library);
		if (Shader->Library || Shader->Function)
		{
			return Shader;
		}
		else
		{
			delete Shader;
		}
	}
	return FComputeShaderRHIRef();
}

//
//Library Iterator
//
FRHIShaderLibrary::FShaderLibraryEntry FMetalShaderLibrary::FMetalShaderLibraryIterator::operator*() const
{
	FShaderLibraryEntry Entry;
	
	Entry.Hash = IteratorImpl->Key;
	Entry.Frequency = (EShaderFrequency)IteratorImpl->Value.Key;
	Entry.Platform = GetLibrary()->GetPlatform();
	
	return Entry;
}

#if !PLATFORM_MAC
static FString ConvertToIOSPath(const FString& Filename, bool bForWrite)
{
	FString Result = Filename;
	if (Result.Contains(TEXT("/OnDemandResources/")))
	{
		return Result;
	}
	
	Result.ReplaceInline(TEXT("../"), TEXT(""));
	Result.ReplaceInline(TEXT(".."), TEXT(""));
	Result.ReplaceInline(FPlatformProcess::BaseDir(), TEXT(""));
	
	if(bForWrite)
	{
		static FString WritePathBase = FString([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]) + TEXT("/");
		return WritePathBase + Result;
	}
	else
	{
		// if filehostip exists in the command line, cook on the fly read path should be used
		FString Value;
		// Cache this value as the command line doesn't change...
		static bool bHasHostIP = FParse::Value(FCommandLine::Get(), TEXT("filehostip"), Value) || FParse::Value(FCommandLine::Get(), TEXT("streaminghostip"), Value);
		static bool bIsIterative = FParse::Value(FCommandLine::Get(), TEXT("iterative"), Value);
		if (bHasHostIP)
		{
			static FString ReadPathBase = FString([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]) + TEXT("/");
			return ReadPathBase + Result;
		}
		else if (bIsIterative)
		{
			static FString ReadPathBase = FString([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]) + TEXT("/");
			return ReadPathBase + Result.ToLower();
		}
		else
		{
			static FString ReadPathBase = FString([[NSBundle mainBundle] bundlePath]) + TEXT("/cookeddata/");
			return ReadPathBase + Result.ToLower();
		}
	}
	
	return Result;
}
#endif

FRHIShaderLibraryRef FMetalDynamicRHI::RHICreateShaderLibrary(EShaderPlatform Platform, FString FolderPath)
{
	@autoreleasepool {
	FRHIShaderLibraryRef Result = nullptr;
	
	FName PlatformName = LegacyShaderPlatformToShaderFormat(Platform);
	
	FMetalShaderMap Map;
	FString BinaryShaderFile = FolderPath / PlatformName.GetPlainNameString() + METAL_MAP_EXTENSION;
	FArchive* BinaryShaderAr = IFileManager::Get().CreateFileReader(*BinaryShaderFile);
	if( BinaryShaderAr != NULL )
	{
		*BinaryShaderAr << Map;
		BinaryShaderAr->Flush();
		delete BinaryShaderAr;
		
		// Would be good to check the language version of the library with the archive format here.
		if (Map.Format == PlatformName.GetPlainNameString())
		{
			FString MetalLibraryFilePath = FolderPath / PlatformName.GetPlainNameString() + METAL_LIB_EXTENSION;
			MetalLibraryFilePath = FPaths::ConvertRelativePathToFull(MetalLibraryFilePath);
#if !PLATFORM_MAC
			MetalLibraryFilePath = ConvertToIOSPath(MetalLibraryFilePath.ToLower(), false);
#endif
			
			NSError* Error;
			id<MTLLibrary> Library = [GetMetalDeviceContext().GetDevice() newLibraryWithFile:MetalLibraryFilePath.GetNSString() error:&Error];
			if (Library != nil)
			{
				if (Map.HashMap.Num() != Library.functionNames.count)
				{
					UE_LOG(LogMetal, Error, TEXT("Mistmatch between map (%d) & library (%d) shader count"), Map.HashMap.Num(), Library.functionNames.count);
				}
				
				Result = new FMetalShaderLibrary(Platform, Library, Map);
			}
			else
			{
				UE_LOG(LogMetal, Display, TEXT("Failed to create library: %s"), *FString(Error.description));
			}
		}
		else
		{
			UE_LOG(LogMetal, Display, TEXT("Wrong shader platform wanted: %s, got: %s"), *PlatformName.GetPlainNameString(), *Map.Format);
		}
	}
	else
	{
		UE_LOG(LogMetal, Display, TEXT("No .metalmap file found for %s!"), *PlatformName.GetPlainNameString());
	}
	
	return Result;
	}
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
	FMetalVertexDeclaration* InVertexDeclaration = ResourceCast(InVertexDeclarationRHI);
	FMetalVertexShader* InVertexShader = ResourceCast(InVertexShaderRHI);
	FMetalPixelShader* InPixelShader = ResourceCast(InPixelShaderRHI);
	FMetalHullShader* InHullShader = ResourceCast(InHullShaderRHI);
	FMetalDomainShader* InDomainShader = ResourceCast(InDomainShaderRHI);
	FMetalGeometryShader* InGeometryShader = ResourceCast(InGeometryShaderRHI);

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
	@autoreleasepool {
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
		
		FShaderCache::LogBoundShaderState(ImmediateContext.Context->GetCurrentState().GetShaderCacheStateObject(), GMaxRHIShaderPlatform, VertexDeclarationRHI, VertexShaderRHI, State->PixelShader, HullShaderRHI, DomainShaderRHI, GeometryShaderRHI, State);
		
		return State;
	}
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

void FMetalShaderParameterCache::CommitPackedUniformBuffers(FMetalStateCache* Cache, TRefCountPtr<FMetalBoundShaderState> BoundShaderState, FMetalComputeShader* ComputeShader, int32 Stage, const TArray< TRefCountPtr<FRHIUniformBuffer> >& RHIUniformBuffers, const TArray<CrossCompiler::FUniformBufferCopyInfo>& UniformBuffersCopyInfo)
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

	if (!Bindings.bHasRegularUniformBuffers && !FShaderCache::IsPredrawCall(Cache->GetShaderCacheStateObject()))
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
