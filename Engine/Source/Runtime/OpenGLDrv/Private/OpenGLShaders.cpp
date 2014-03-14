// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OpenGLShaders.cpp: OpenGL shader RHI implementation.
=============================================================================*/

#include "OpenGLDrvPrivate.h"
#include "Shader.h"
#include "GlobalShader.h"

#define CHECK_FOR_GL_SHADERS_TO_REPLACE 0

#if PLATFORM_WINDOWS
#include <mmintrin.h>
#elif PLATFORM_MAC
#include <xmmintrin.h>
#endif

const uint32 SizeOfFloat4 = 16;
const uint32 NumFloatsInFloat4 = 4;

/**
 * Verify that an OpenGL shader has compiled successfully.
 */
static bool VerifyCompiledShader(GLuint Shader, const ANSICHAR* GlslCode)
{
	SCOPE_CYCLE_COUNTER(STAT_OpenGLShaderCompileVerifyTime);

	GLint CompileStatus;
	glGetShaderiv(Shader, GL_COMPILE_STATUS, &CompileStatus);
	if (CompileStatus != GL_TRUE)
	{
		GLint LogLength;
		ANSICHAR DefaultLog[] = "No log";
		ANSICHAR *CompileLog = DefaultLog;
		glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &LogLength);
		if (LogLength > 1)
		{
			CompileLog = (ANSICHAR *)FMemory::Malloc(LogLength);
			glGetShaderInfoLog(Shader, LogLength, NULL, CompileLog);
		}

#if DEBUG_GL_SHADERS
		if (GlslCode)
		{
			UE_LOG(LogRHI,Error,TEXT("Shader:\n%s"),ANSI_TO_TCHAR(GlslCode));
		}
#endif
		UE_LOG(LogRHI,Fatal,TEXT("Failed to compile shader. Compile log:\n%s"), ANSI_TO_TCHAR(CompileLog));

		if (LogLength > 1)
		{
			FMemory::Free(CompileLog);
		}
		return false;
	}
	return true;
}

/**
 * Verify that an OpenGL program has linked successfully.
 */
static bool VerifyLinkedProgram(GLuint Program)
{
	SCOPE_CYCLE_COUNTER(STAT_OpenGLShaderLinkVerifyTime);

	GLint LinkStatus = 0;
	glGetProgramiv(Program, GL_LINK_STATUS, &LinkStatus);
	if (LinkStatus != GL_TRUE)
	{
		GLint LogLength;
		ANSICHAR DefaultLog[] = "No log";
		ANSICHAR *CompileLog = DefaultLog;
		glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &LogLength);
		if (LogLength > 1)
		{
			CompileLog = (ANSICHAR *)FMemory::Malloc(LogLength);
			glGetProgramInfoLog(Program, LogLength, NULL, CompileLog);
		}

		UE_LOG(LogRHI,Error,TEXT("Failed to link program. Compile log:\n%s"),
			ANSI_TO_TCHAR(CompileLog));

		if (LogLength > 1)
		{
			FMemory::Free(CompileLog);
		}
		return false;
	}
	return true;
}

// ============================================================================================================================

class FOpenGLCompiledShaderKey
{
public:
	FOpenGLCompiledShaderKey(
		GLenum InTypeEnum,
		uint32 InCodeSize,
		uint32 InCodeCRC
		)
		: TypeEnum(InTypeEnum)
		, CodeSize(InCodeSize)
		, CodeCRC(InCodeCRC)
	{}

	friend bool operator ==(const FOpenGLCompiledShaderKey& A,const FOpenGLCompiledShaderKey& B)
	{
		return A.TypeEnum == B.TypeEnum && A.CodeSize == B.CodeSize && A.CodeCRC == B.CodeCRC;
	}

	friend uint32 GetTypeHash(const FOpenGLCompiledShaderKey &Key)
	{
		return GetTypeHash(Key.TypeEnum) ^ GetTypeHash(Key.CodeSize) ^ GetTypeHash(Key.CodeCRC);
	}

private:
	GLenum TypeEnum;
	uint32 CodeSize;
	uint32 CodeCRC;
};

typedef TMap<FOpenGLCompiledShaderKey,GLuint> FOpenGLCompiledShaderCache;

static FOpenGLCompiledShaderCache& GetOpenGLCompiledShaderCache()
{
	static FOpenGLCompiledShaderCache CompiledShaderCache;
	return CompiledShaderCache;
}

// ============================================================================================================================


static const TCHAR* ShaderNameFromShaderType(GLenum ShaderType)
{
	switch(ShaderType)
	{
		case GL_VERTEX_SHADER: return TEXT("vertex");
		case GL_FRAGMENT_SHADER: return TEXT("fragment");
		case GL_GEOMETRY_SHADER: return TEXT("geometry");
		case GL_TESS_CONTROL_SHADER: return TEXT("hull");
		case GL_TESS_EVALUATION_SHADER: return TEXT("domain");
		case GL_COMPUTE_SHADER: return TEXT("compute");
		default: return NULL;
	}
}

// ============================================================================================================================


static void ReplaceShaderSubstring(ANSICHAR* ShaderCode, const ANSICHAR* OldString, const ANSICHAR* NewString)
{
	char* Curr = ShaderCode;
	while (Curr != NULL)
	{
		char* Substr = (char*)strstr(Curr, OldString);
		Curr = Substr;
		if (Substr)
		{
			strcpy(Substr, NewString);
			Curr += strlen(NewString);
			for (size_t index = 0; index < strlen(OldString)-strlen(NewString); ++index)
			{
				*Curr = ' ';
				++Curr;
			}
		}
	}
}

/**
 * Compiles an OpenGL shader using the given GLSL microcode.
 * @returns the compiled shader upon success.
 */
template <typename ShaderType>
ShaderType* CompileOpenGLShader(const TArray<uint8>& Code)
{
	SCOPE_CYCLE_COUNTER(STAT_OpenGLShaderCompileTime);
	VERIFY_GL_SCOPE();

	ShaderType* Shader = NULL;
	const GLenum TypeEnum = ShaderType::TypeEnum;
	FMemoryReader Ar(Code, true);
	FOpenGLCodeHeader Header = { 0 };

	Ar << Header;
	// Suppress static code analysis warning about a potential comparison of two constants
	CA_SUPPRESS(6326);
	if (Header.GlslMarker != 0x474c534c
		|| (TypeEnum == GL_VERTEX_SHADER && Header.FrequencyMarker != 0x5653)
		|| (TypeEnum == GL_FRAGMENT_SHADER && Header.FrequencyMarker != 0x5053)
		|| (TypeEnum == GL_GEOMETRY_SHADER && Header.FrequencyMarker != 0x4753)
		|| (TypeEnum == GL_COMPUTE_SHADER && Header.FrequencyMarker != 0x4353 && FOpenGL::SupportsComputeShaders())
		|| (TypeEnum == GL_TESS_CONTROL_SHADER && Header.FrequencyMarker != 0x4853 && FOpenGL::SupportsComputeShaders()) /* hull shader*/
		|| (TypeEnum == GL_TESS_EVALUATION_SHADER && Header.FrequencyMarker != 0x4453 && FOpenGL::SupportsComputeShaders()) /* domain shader*/
		)
	{
		UE_LOG(LogRHI,Fatal,
			TEXT("Corrupt shader bytecode. GlslMarker=0x%08x FrequencyMarker=0x%04x"),
			Header.GlslMarker,
			Header.FrequencyMarker
			);
		return NULL;
	}

	int32 CodeOffset = Ar.Tell();
	const ANSICHAR* GlslCode = (ANSICHAR*)Code.GetTypedData() + CodeOffset;
	GLint GlslCodeLength = Code.Num() - CodeOffset - 1;
	uint32 GlslCodeCRC = FCrc::MemCrc_DEPRECATED(GlslCode,GlslCodeLength);

#if CHECK_FOR_GL_SHADERS_TO_REPLACE
	ANSICHAR* NewCode = 0;
#endif

	FOpenGLCompiledShaderKey Key(TypeEnum,GlslCodeLength,GlslCodeCRC);

	// Find the existing compiled shader in the cache.
	GLuint Resource = GetOpenGLCompiledShaderCache().FindRef(Key);
	if (!Resource)
	{
#if CHECK_FOR_GL_SHADERS_TO_REPLACE
		{
			// 1. Check for specific file
			uint32 CRC = FCrc::MemCrc_DEPRECATED( GlslCode, GlslCodeLength+1 );
			FString PotentialShaderFileName = FString::Printf( TEXT("%s-%d-0x%x.txt"), ShaderNameFromShaderType(TypeEnum), GlslCodeLength, CRC );
			FString PotentialShaderFile = FPaths::ProfilingDir();
			PotentialShaderFile *= PotentialShaderFileName;

			UE_LOG( LogRHI, Log, TEXT("Looking for shader file '%s' for potential replacement."), *PotentialShaderFileName );

			int64 FileSize = IFileManager::Get().FileSize(*PotentialShaderFile);
			if( FileSize > 0 )
			{
				FArchive* Ar = IFileManager::Get().CreateFileReader(*PotentialShaderFile);
				if( Ar != NULL )
				{
					// read in the file
					ANSICHAR* NewCode = (ANSICHAR*)FMemory::Malloc( FileSize+1 );
					Ar->Serialize(NewCode, FileSize);
					delete Ar;
					NewCode[FileSize] = 0;

					GlslCode = NewCode;
					GlslCodeLength = (GLint)(int32)FileSize+1;
					UE_LOG( LogRHI, Log, TEXT("Replacing %s shader with length %d and CRC 0x%x with the one from a file."), ( TypeEnum == GL_VERTEX_SHADER ) ? TEXT("vertex") : ( ( TypeEnum == GL_FRAGMENT_SHADER ) ? TEXT("fragment") : TEXT("geometry")), GlslCodeLength-1, CRC );
				}
			}
		}
#endif

		Resource = glCreateShader(TypeEnum);
#if PLATFORM_ANDROID || PLATFORM_HTML5
		//Here is some code to add in a #define for textureCubeLodEXT

		//#version 100 has to be the first line in the file, so it has to be added before anything else.
		ANSICHAR* VersionString = new ANSICHAR[18];
		memset((void*)VersionString, '\0', 18*sizeof(ANSICHAR));
		if (FOpenGL::UseES30ShadingLanguage())
		{
			strcpy(VersionString, "#version 300 es");
			strcat(VersionString, "\n");
		}
		else 
		{
			strcpy(VersionString, "#version 100");
			strcat(VersionString, "\n");
		}
		
		//This #define fixes compiler errors on Android (which doesnt seem to support textureCubeLodEXT)
		const ANSICHAR* Prologue = NULL; 
		if (FOpenGL::UseES30ShadingLanguage())
		{
			if (TypeEnum == GL_VERTEX_SHADER)
			{
				Prologue = "#define texture2D texture				\n"
					"#define texture2DProj textureProj		\n"
					"#define texture2DLod textureLod			\n"
					"#define texture2DProjLod textureProjLod \n"
					"#define textureCube texture				\n"
					"#define textureCubeLod textureLod		\n"
					"#define textureCubeLodEXT textureLod	\n";

				ReplaceShaderSubstring(const_cast<ANSICHAR*>(GlslCode), "attribute", "in");
				ReplaceShaderSubstring(const_cast<ANSICHAR*>(GlslCode), "varying", "out");
			} 
			else if (TypeEnum == GL_FRAGMENT_SHADER)
			{
				Prologue = "#define texture2D texture				\n"
					"#define texture2DProj textureProj		\n"
					"#define texture2DLod textureLod			\n"
					"#define texture2DProjLod textureProjLod \n"
					"#define textureCube texture				\n"
					"#define textureCubeLod textureLod		\n"
					"#define textureCubeLodEXT textureLod	\n"
					"\n"
					"out vec4 gl_FragColor;";

				ReplaceShaderSubstring(const_cast<ANSICHAR*>(GlslCode), "varying", "in");
			}
		}
		else if(!FOpenGL::SupportsShaderTextureLod())
		{
			Prologue = "#define textureCubeLodEXT(a, b, c) textureCube(a, b) \n";
		}
		else if(!FOpenGL::SupportsTextureCubeLodEXT())
		{
			Prologue = "#define textureCubeLodEXT textureCubeLod \n";
		}
		else 
		{
			Prologue = "";
		}
		//remove "#version 100" line,  if found in existing GlslCode. 
		ANSICHAR* VersionCodePointer = const_cast<ANSICHAR*>(GlslCode);
		VersionCodePointer = strstr(VersionCodePointer, "#version 100");
		if(VersionCodePointer != NULL)
		{
			for(int index=0; index < 12; index++)
			{
				VersionCodePointer[index] = ' ';
			}
		}
		//Assemble the source strings into an array to pass into the compiler.
		const GLchar* ShaderSourceStrings[3] = { VersionString, Prologue, GlslCode };
		const GLint ShaderSourceLen[3] = { (GLint)(strlen(VersionString)), (GLint)(strlen(Prologue)), (GLint)(strlen(GlslCode)) };
		glShaderSource(Resource, 3, (const GLchar**)&ShaderSourceStrings, ShaderSourceLen);		
		glCompileShader(Resource);
#else
		glShaderSource(Resource, 1, (const GLchar**)&GlslCode, &GlslCodeLength);
		glCompileShader(Resource);
#endif
		GetOpenGLCompiledShaderCache().Add(Key,Resource);
	}

	Shader = new ShaderType();
	Shader->Resource = Resource;
	Shader->Bindings = Header.Bindings;
	Shader->BoundUniformBuffers.AddZeroed( Header.Bindings.NumUniformBuffers );
	Shader->UniformBuffersCopyInfo = Header.UniformBuffersCopyInfo;

#if DEBUG_GL_SHADERS
	Shader->GlslCode.Empty(GlslCodeLength + 1);
	Shader->GlslCode.AddUninitialized(GlslCodeLength + 1);
	FMemory::Memcpy(Shader->GlslCode.GetTypedData(), GlslCode, GlslCodeLength + 1);
	Shader->GlslCodeString = (ANSICHAR*)Shader->GlslCode.GetTypedData();
#endif

#if CHECK_FOR_GL_SHADERS_TO_REPLACE
	if( NewCode )
	{
		FMemory::Free( NewCode );
	}
#endif

	return Shader;
}

/**
 * Helper for constructing strings of the form XXXXX##.
 * @param Str - The string to build.
 * @param Offset - Offset into the string at which to set the number.
 * @param Index - Number to set. Must be in the range [0,100).
 */
static ANSICHAR* SetIndex(ANSICHAR* Str, int32 Offset, int32 Index)
{
	check(Index >= 0 && Index < 100);

	Str += Offset;
	if (Index >= 10)
	{
		*Str++ = '0' + (ANSICHAR)(Index / 10);
	}
	*Str++ = '0' + (ANSICHAR)(Index % 10);
	*Str = '\0';
	return Str;
}

FVertexShaderRHIRef FOpenGLDynamicRHI::RHICreateVertexShader(const TArray<uint8>& Code)
{
	return CompileOpenGLShader<FOpenGLVertexShader>(Code);
}

FPixelShaderRHIRef FOpenGLDynamicRHI::RHICreatePixelShader(const TArray<uint8>& Code)
{
	return CompileOpenGLShader<FOpenGLPixelShader>(Code);
}

FGeometryShaderRHIRef FOpenGLDynamicRHI::RHICreateGeometryShader(const TArray<uint8>& Code)
{
	return CompileOpenGLShader<FOpenGLGeometryShader>(Code);
}

FHullShaderRHIRef FOpenGLDynamicRHI::RHICreateHullShader(const TArray<uint8>& Code)
{
	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	return CompileOpenGLShader<FOpenGLHullShader>(Code);
}

FDomainShaderRHIRef FOpenGLDynamicRHI::RHICreateDomainShader(const TArray<uint8>& Code)
{
	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	return CompileOpenGLShader<FOpenGLDomainShader>(Code);
}

FGeometryShaderRHIRef FOpenGLDynamicRHI::RHICreateGeometryShaderWithStreamOutput(const TArray<uint8>& Code, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream) 
{
	UE_LOG(LogRHI, Fatal,TEXT("OpenGL Render path does not support stream output!"));
	return NULL;
}


static void MarkShaderParameterCachesDirty(FOpenGLShaderParameterCache* ShaderParameters, bool UpdateCompute)
{
	const int32 StageStart = UpdateCompute  ? OGL_SHADER_STAGE_COMPUTE : OGL_SHADER_STAGE_VERTEX;
	const int32 StageEnd = UpdateCompute ? OGL_NUM_SHADER_STAGES : OGL_NUM_NON_COMPUTE_SHADER_STAGES;
	for (int32 Stage = StageStart; Stage < StageEnd; ++Stage)
	{
		ShaderParameters[Stage].MarkAllDirty();
	}
}

void FOpenGLDynamicRHI::BindUniformBufferBase(FOpenGLContextState& ContextState, int32 NumUniformBuffers, const TArray< TRefCountPtr<FRHIUniformBuffer> >& BoundUniformBuffers, uint32 FirstUniformBuffer, bool ForceUpdate)
{
	check(IsInRenderingThread());
	for (int32 BufferIndex = 0; BufferIndex < NumUniformBuffers; ++BufferIndex)
	{
		GLuint Buffer;
		int32 BindIndex = FirstUniformBuffer + BufferIndex;
		if (BoundUniformBuffers[BufferIndex])
		{
			FRHIUniformBuffer* UB = BoundUniformBuffers[BufferIndex];
			Buffer = ((FOpenGLUniformBuffer*)UB)->Resource;
		}
		else
		{
			if (PendingState.ZeroFilledDummyUniformBuffer == 0)
			{
				void* ZeroBuffer = FMemory::Malloc(ZERO_FILLED_DUMMY_UNIFORM_BUFFER_SIZE);
				FMemory::Memzero(ZeroBuffer,ZERO_FILLED_DUMMY_UNIFORM_BUFFER_SIZE);
				FOpenGL::GenBuffers(1, &PendingState.ZeroFilledDummyUniformBuffer);
				check(PendingState.ZeroFilledDummyUniformBuffer != 0);
				CachedBindUniformBuffer(ContextState,PendingState.ZeroFilledDummyUniformBuffer);
				glBufferData(GL_UNIFORM_BUFFER, ZERO_FILLED_DUMMY_UNIFORM_BUFFER_SIZE, ZeroBuffer, GL_STATIC_DRAW);
				FMemory::Free(ZeroBuffer);
				IncrementBufferMemory(GL_UNIFORM_BUFFER, false, ZERO_FILLED_DUMMY_UNIFORM_BUFFER_SIZE);
			}

			Buffer = PendingState.ZeroFilledDummyUniformBuffer;
		}

		if (ForceUpdate || ContextState.UniformBuffers[BindIndex] != Buffer)
		{
			FOpenGL::BindBufferBase(GL_UNIFORM_BUFFER, BindIndex, Buffer);
			ContextState.UniformBuffers[BindIndex] = Buffer;
			ContextState.UniformBufferBound = Buffer;	// yes, calling glBindBufferBase also changes uniform buffer binding.
		}
	}
}

// ============================================================================================================================

class FOpenGLLinkedProgramConfiguration
{
public:

	struct ShaderInfo
	{
		FOpenGLShaderBindings Bindings;
		GLuint Resource;
	}
	Shaders[OGL_NUM_SHADER_STAGES];

	FOpenGLLinkedProgramConfiguration()
	{
		for ( int32 Stage = 0; Stage < OGL_NUM_SHADER_STAGES; Stage++)
		{
			Shaders[Stage].Resource = 0;
		}
	}

	friend bool operator ==(const FOpenGLLinkedProgramConfiguration& A,const FOpenGLLinkedProgramConfiguration& B)
	{
		bool bEqual = true;
		for ( int32 Stage = 0; Stage < OGL_NUM_SHADER_STAGES && bEqual; Stage++)
		{
			bEqual &= A.Shaders[Stage].Resource == B.Shaders[Stage].Resource;
			bEqual &= A.Shaders[Stage].Bindings == B.Shaders[Stage].Bindings;
		}
		return bEqual;
	}

	friend uint32 GetTypeHash(const FOpenGLLinkedProgramConfiguration &Config)
	{
		uint32 Hash = 0;
		for ( int32 Stage = 0; Stage < OGL_NUM_SHADER_STAGES; Stage++)
		{
			Hash ^= GetTypeHash(Config.Shaders[Stage].Bindings);
			Hash ^= Config.Shaders[Stage].Resource;
		}
		return Hash;
	}
};

class FOpenGLLinkedProgram
{
public:
	FOpenGLLinkedProgramConfiguration Config;

	struct FPackedUniformInfo
	{
		GLint	Location;
		uint8	ArrayType;	// OGL_PACKED_ARRAYINDEX_TYPE
		uint8	Index;		// OGL_PACKED_INDEX_TYPE
	};

	// Holds information needed per stage regarding packed uniform globals and uniform buffers
	struct FStagePackedUniformInfo
	{
		// Packed Uniform Arrays (regular globals); array elements per precision/type
		TArray<FPackedUniformInfo>			PackedUniformInfos;

		// Packed Uniform Buffers; outer array is per Uniform Buffer; inner array is per precision/type
		TArray<TArray<FPackedUniformInfo>>	PackedUniformBufferInfos;

		// Holds the unique ID of the last uniform buffer uploaded to the program; since we don't reuse uniform buffers
		// (can't modify existing ones), we use this as a check for dirty/need to mem copy on Mobile
		TArray<uint32>						LastEmulatedUniformBufferSet;
	};
	FStagePackedUniformInfo	StagePackedUniformInfo[OGL_NUM_SHADER_STAGES];

	GLuint		Program;
	TBitArray<>	TextureStageNeeds;
	TBitArray<>	UAVStageNeeds;
	int32		MaxTextureStage;

	FOpenGLLinkedProgram()
	: Program(0), MaxTextureStage(-1)
	{
		TextureStageNeeds.Init( false, FOpenGL::GetMaxCombinedTextureImageUnits() );
		UAVStageNeeds.Init( false, OGL_MAX_COMPUTE_STAGE_UAV_UNITS );
	}

	~FOpenGLLinkedProgram()
	{
		check(Program);
		glDeleteProgram(Program);
	}

	void ConfigureShaderStage( int Stage, uint32 FirstUniformBuffer );

	// Make sure GlobalArrays (created from shader reflection) matches our info (from the cross compiler)
	static inline void SortPackedUniformInfos(const TArray<FPackedUniformInfo>& ReflectedUniformInfos, const TArray<FOpenGLPackedArrayInfo>& PackedGlobalArrays, TArray<FPackedUniformInfo>& OutPackedUniformInfos)
	{
		check(OutPackedUniformInfos.Num() == 0);
		OutPackedUniformInfos.AddUninitialized(PackedGlobalArrays.Num());
		for (int32 Index = 0; Index < PackedGlobalArrays.Num(); ++Index)
		{
			auto& PackedArray = PackedGlobalArrays[Index];
			int32 FoundIndex = -1;

			// Find this Global Array in the reflection list
			for (int32 FindIndex = 0; FindIndex < ReflectedUniformInfos.Num(); ++FindIndex)
			{
				auto& ReflectedInfo = ReflectedUniformInfos[FindIndex];
				if (ReflectedInfo.ArrayType == PackedArray.TypeName)
				{
					FoundIndex = FindIndex;
					OutPackedUniformInfos[Index] = ReflectedInfo;
					break;
				}
			}

			if (FoundIndex == -1)
			{
				OutPackedUniformInfos[Index].Location = -1;
				OutPackedUniformInfos[Index].ArrayType = -1;
				OutPackedUniformInfos[Index].Index = -1;
			}
		}
	}
};

typedef TMap<FOpenGLLinkedProgramConfiguration,FOpenGLLinkedProgram*> FOpenGLProgramsForReuse;

static FOpenGLProgramsForReuse& GetOpenGLProgramsCache()
{
	static FOpenGLProgramsForReuse ProgramsCache;
	return ProgramsCache;
}

// This short queue preceding released programs cache is here because usually the programs are requested again
// very shortly after they're released, so looking through recently released programs first provides tangible
// performance improvement.

#define LAST_RELEASED_PROGRAMS_CACHE_COUNT 10

static FOpenGLLinkedProgram* StaticLastReleasedPrograms[LAST_RELEASED_PROGRAMS_CACHE_COUNT] = { 0 };
static int32 StaticLastReleasedProgramsIndex = 0;

// ============================================================================================================================

static int32 CountSetBits(const TBitArray<>& Array)
{
	int32 Result = 0;
	for (TBitArray<>::FConstIterator BitIt(Array); BitIt; ++BitIt)
	{
		Result += BitIt.GetValue();
	}
	return Result;
}

void FOpenGLLinkedProgram::ConfigureShaderStage( int Stage, uint32 FirstUniformBuffer )
{
	static const ANSICHAR StagePrefix[OGL_NUM_SHADER_STAGES] = { 'v', 'p', 'g', 'h', 'd', 'c'};
	static const GLint FirstTextureUnit[OGL_NUM_SHADER_STAGES] =
	{
		FOpenGL::GetFirstVertexTextureUnit(),
		FOpenGL::GetFirstPixelTextureUnit(),
		FOpenGL::GetFirstGeometryTextureUnit(),
		FOpenGL::GetFirstHullTextureUnit(),
		FOpenGL::GetFirstDomainTextureUnit(),
		FOpenGL::GetFirstComputeTextureUnit()
	};
	static const GLint FirstUAVUnit[OGL_NUM_SHADER_STAGES] =
	{
		OGL_UAV_NOT_SUPPORTED_FOR_GRAPHICS_UNIT,
		OGL_UAV_NOT_SUPPORTED_FOR_GRAPHICS_UNIT,
		OGL_UAV_NOT_SUPPORTED_FOR_GRAPHICS_UNIT,
		OGL_UAV_NOT_SUPPORTED_FOR_GRAPHICS_UNIT,
		OGL_UAV_NOT_SUPPORTED_FOR_GRAPHICS_UNIT,
		FOpenGL::GetFirstComputeUAVUnit()
	};
	ANSICHAR NameBuffer[10] = {0};

	// verify that only CS uses UAVs
	check((Stage != OGL_SHADER_STAGE_COMPUTE) ? (CountSetBits(UAVStageNeeds) == 0) : true);

	SCOPE_CYCLE_COUNTER(STAT_OpenGLShaderBindParameterTime);
	VERIFY_GL_SCOPE();

	NameBuffer[0] = StagePrefix[Stage];

	// Bind Global uniform arrays (vu_h, pu_i, etc)
	{
		NameBuffer[1] = 'u';
		NameBuffer[2] = '_';
		NameBuffer[3] = 0;
		NameBuffer[4] = 0;

		TArray<FPackedUniformInfo> PackedUniformInfos;
		for (uint8 Index = 0; Index < OGL_PACKED_TYPEINDEX_MAX; ++Index)
		{
			uint8 ArrayIndexType = GLPackedTypeIndexToTypeName(Index);
			NameBuffer[3] = ArrayIndexType;
			GLint Location = glGetUniformLocation(Program, NameBuffer);
			if ((int32)Location != -1)
			{
				FPackedUniformInfo Info = {Location, ArrayIndexType, Index};
				PackedUniformInfos.Add(Info);
			}
		}

		SortPackedUniformInfos(PackedUniformInfos, Config.Shaders[Stage].Bindings.PackedGlobalArrays, StagePackedUniformInfo[Stage].PackedUniformInfos);
	}

	// Bind uniform buffer packed arrays (vc0_h, pc2_i, etc)
	{
		NameBuffer[1] = 'c';
		NameBuffer[2] = 0;
		NameBuffer[3] = 0;
		NameBuffer[4] = 0;
		NameBuffer[5] = 0;
		NameBuffer[6] = 0;
		for (uint8 UB = 0; UB < Config.Shaders[Stage].Bindings.NumUniformBuffers; ++UB)
		{
			TArray<FPackedUniformInfo> PackedBuffers;
			ANSICHAR* Str = SetIndex(NameBuffer, 2, UB);
			*Str++ = '_';
			Str[1] = 0;
			for (uint8 Index = 0; Index < OGL_PACKED_TYPEINDEX_MAX; ++Index)
			{
				uint8 ArrayIndexType = GLPackedTypeIndexToTypeName(Index);
				Str[0] = ArrayIndexType;
				GLint Location = glGetUniformLocation(Program, NameBuffer);
				if ((int32)Location != -1)
				{
					FPackedUniformInfo Info = {Location, ArrayIndexType, Index};
					PackedBuffers.Add(Info);
				}
			}

			StagePackedUniformInfo[Stage].PackedUniformBufferInfos.Add(PackedBuffers);
		}
	}

	// Reserve and setup Space for Emulated Uniform Buffers
	StagePackedUniformInfo[Stage].LastEmulatedUniformBufferSet.Empty(Config.Shaders[Stage].Bindings.NumUniformBuffers);
	StagePackedUniformInfo[Stage].LastEmulatedUniformBufferSet.AddZeroed(Config.Shaders[Stage].Bindings.NumUniformBuffers);

	// Bind samplers.
	NameBuffer[1] = 's';
	NameBuffer[2] = 0;
	NameBuffer[3] = 0;
	NameBuffer[4] = 0;
	int32 LastFoundIndex = -1;
	for (int32 SamplerIndex = 0; SamplerIndex < Config.Shaders[Stage].Bindings.NumSamplers; ++SamplerIndex)
	{
		SetIndex(NameBuffer, 2, SamplerIndex);
		GLint Location = glGetUniformLocation(Program, NameBuffer);
		if (Location == -1)
		{
			if (LastFoundIndex != -1)
			{
				// It may be an array of samplers. Get the initial element location, if available, and count from it.
				SetIndex(NameBuffer, 2, LastFoundIndex);
				int32 OffsetOfArraySpecifier = (LastFoundIndex>9)?4:3;
				int32 ArrayIndex = SamplerIndex-LastFoundIndex;
				NameBuffer[OffsetOfArraySpecifier] = '[';
				ANSICHAR* EndBracket = SetIndex(NameBuffer, OffsetOfArraySpecifier+1, ArrayIndex);
				*EndBracket++ = ']';
				*EndBracket = 0;
				Location = glGetUniformLocation(Program, NameBuffer);
			}
		}
		else
		{
			LastFoundIndex = SamplerIndex;
		}

		if (Location != -1)
		{
			glUniform1i(Location, FirstTextureUnit[Stage] + SamplerIndex);
			TextureStageNeeds[ FirstTextureUnit[Stage] + SamplerIndex ] = true;
			MaxTextureStage = FMath::Max( MaxTextureStage, FirstTextureUnit[Stage] + SamplerIndex);
		}
	}

	// Bind UAVs/images.
	NameBuffer[1] = 'i';
	NameBuffer[2] = 0;
	NameBuffer[3] = 0;
	NameBuffer[4] = 0;
	int32 LastFoundUAVIndex = -1;
	for (int32 UAVIndex = 0; UAVIndex < Config.Shaders[Stage].Bindings.NumUAVs; ++UAVIndex)
	{
		SetIndex(NameBuffer, 2, UAVIndex);
		GLint Location = glGetUniformLocation(Program, NameBuffer);
		if (Location == -1)
		{
			if (LastFoundUAVIndex != -1)
			{
				// It may be an array of UAVs. Get the initial element location, if available, and count from it.
				SetIndex(NameBuffer, 2, LastFoundUAVIndex);
				int32 OffsetOfArraySpecifier = (LastFoundUAVIndex>9)?4:3;
				int32 ArrayIndex = UAVIndex-LastFoundUAVIndex;
				NameBuffer[OffsetOfArraySpecifier] = '[';
				ANSICHAR* EndBracket = SetIndex(NameBuffer, OffsetOfArraySpecifier+1, ArrayIndex);
				*EndBracket++ = ']';
				*EndBracket = '\0';
				Location = glGetUniformLocation(Program, NameBuffer);
			}
		}
		else
		{
			LastFoundUAVIndex = UAVIndex;
		}

		if (Location != -1)
		{
			glUniform1i(Location, FirstUAVUnit[Stage] + UAVIndex);
			UAVStageNeeds[ FirstUAVUnit[Stage] + UAVIndex ] = true;
		}
	}

	// Bind uniform buffers.
	if (FOpenGL::SupportsUniformBuffers())
	{
		NameBuffer[1] = 'b';
		NameBuffer[2] = 0;
		NameBuffer[3] = 0;
		NameBuffer[4] = 0;
		for (int32 BufferIndex = 0; BufferIndex < Config.Shaders[Stage].Bindings.NumUniformBuffers; ++BufferIndex)
		{
			SetIndex(NameBuffer, 2, BufferIndex);
			GLint Location = FOpenGL::GetUniformBlockIndex(Program, NameBuffer);
			if (Location >= 0)
			{
				FOpenGL::UniformBlockBinding(Program, Location, FirstUniformBuffer + BufferIndex);
			}
		}
	}
}

#if ENABLE_UNIFORM_BUFFER_LAYOUT_VERIFICATION

#define ENABLE_UNIFORM_BUFFER_LAYOUT_NAME_MANGLING_CL1862097 1
/*
	As of CL 1862097 uniform buffer names are mangled to avoid collisions between variables referenced 
	in different shaders of the same program

	layout(std140) uniform _vb0
	{
	#define View View_vb0
	anon_struct_0000 View;
	};

	layout(std140) uniform _vb1
	{
	#define Primitive Primitive_vb1
	anon_struct_0001 Primitive;
	};
*/
	

struct UniformData
{
	UniformData(uint32 InOffset, uint32 InArrayElements)
		: Offset(InOffset)
		, ArrayElements(InArrayElements)
	{
	}
	uint32 Offset;
	uint32 ArrayElements;

	bool operator == (const UniformData& RHS) const
	{
		return	Offset == RHS.Offset &&	ArrayElements == RHS.ArrayElements;
	}
	bool operator != (const UniformData& RHS) const
	{
		return	!(*this == RHS);
	}
};
#if ENABLE_UNIFORM_BUFFER_LAYOUT_NAME_MANGLING_CL1862097
static void VerifyUniformLayout(const FString& BlockName, const TCHAR* UniformName, const UniformData& GLSLUniform)
#else
static void VerifyUniformLayout(const TCHAR* UniformName, const UniformData& GLSLUniform)
#endif //#if ENABLE_UNIFORM_BUFFER_LAYOUT_NAME_MANGLING_CL1862097
{
	static TMap<FString, UniformData> Uniforms;

	if(!Uniforms.Num())
	{
		for (TLinkedList<FUniformBufferStruct*>::TIterator StructIt(FUniformBufferStruct::GetStructList()); StructIt; StructIt.Next())
		{
#if ENABLE_UNIFORM_BUFFER_LAYOUT_DUMP
			UE_LOG(LogRHI, Log, TEXT("UniformBufferStruct %s %s %d"),
				StructIt->GetStructTypeName(),
				StructIt->GetShaderVariableName(),
				StructIt->GetSize()
				);
#endif  // #if ENABLE_UNIFORM_BUFFER_LAYOUT_DUMP
			const TArray<FUniformBufferStruct::FMember>& StructMembers = StructIt->GetMembers();
			for(int32 MemberIndex = 0;MemberIndex < StructMembers.Num();++MemberIndex)
			{
				const FUniformBufferStruct::FMember& Member = StructMembers[MemberIndex];

				FString BaseTypeName;
				switch(Member.GetBaseType())
				{
					case UBMT_STRUCT:  BaseTypeName = TEXT("struct"); 
					case UBMT_BOOL:    BaseTypeName = TEXT("bool"); break;
					case UBMT_INT32:   BaseTypeName = TEXT("int"); break;
					case UBMT_UINT32:  BaseTypeName = TEXT("uint"); break;
					case UBMT_FLOAT32: BaseTypeName = TEXT("float"); break;
					default:           UE_LOG(LogShaders, Fatal,TEXT("Unrecognized uniform buffer struct member base type."));
				};
#if ENABLE_UNIFORM_BUFFER_LAYOUT_DUMP
				UE_LOG(LogRHI, Log, TEXT("  +%d %s%dx%d %s[%d]"),
					Member.GetOffset(),
					*BaseTypeName,
					Member.GetNumRows(),
					Member.GetNumColumns(),
					Member.GetName(),
					Member.GetNumElements()
					);
#endif // #if ENABLE_UNIFORM_BUFFER_LAYOUT_DUMP
				FString CompositeName = FString(StructIt->GetShaderVariableName()) + TEXT(".") + Member.GetName();

				// GLSL returns array members with a "[0]" suffix
				if(Member.GetNumElements())
				{
					CompositeName += TEXT("[0]");
				}

				check(!Uniforms.Contains(CompositeName));
				Uniforms.Add(CompositeName, UniformData(Member.GetOffset(), Member.GetNumElements()));
			}
		}
	}

#if ENABLE_UNIFORM_BUFFER_LAYOUT_NAME_MANGLING_CL1862097
	/* unmangle the uniform name by stripping the block name from it
	
	layout(std140) uniform _vb0
	{
	#define View View_vb0
		anon_struct_0000 View;
	};
	*/
	FString RequestedUniformName = FString(UniformName).Replace(*(BlockName + TEXT(".")), TEXT("."));
	if(RequestedUniformName.StartsWith(TEXT(".")))
	{
		RequestedUniformName = RequestedUniformName.RightChop(1);
	}
#else
	FString RequestedUniformName = UniformName;
#endif

	const UniformData* FoundUniform = Uniforms.Find(RequestedUniformName);

	// MaterialTemplate uniform buffer does not have an entry in the FUniformBufferStructs list, so skipping it here
	if(!(RequestedUniformName.StartsWith("Material.") || RequestedUniformName.StartsWith("MaterialCollection")))
	{
		if(!FoundUniform || (*FoundUniform != GLSLUniform))
		{
			UE_LOG(LogRHI, Fatal, TEXT("uniform buffer member %s in the GLSL source doesn't match it's declaration in it's FUniformBufferStruct"), *RequestedUniformName);
		}
	}
}

static void VerifyUniformBufferLayouts(GLuint Program)
{
	GLint NumBlocks = 0;
	glGetProgramiv(Program, GL_ACTIVE_UNIFORM_BLOCKS, &NumBlocks);

#if ENABLE_UNIFORM_BUFFER_LAYOUT_DUMP
	UE_LOG(LogRHI, Log, TEXT("program %d has %d uniform blocks"), Program, NumBlocks);
#endif  // #if ENABLE_UNIFORM_BUFFER_LAYOUT_DUMP

	for(GLint BlockIndex = 0; BlockIndex < NumBlocks; ++BlockIndex)
	{
		const GLsizei BufferSize = 256;
		char Buffer[BufferSize] = {0};
		GLsizei Length = 0;

		GLint ActiveUniforms = 0;
		GLint BlockBytes = 0;

		glGetActiveUniformBlockiv(Program, BlockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &ActiveUniforms);
		glGetActiveUniformBlockiv(Program, BlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &BlockBytes);
		glGetActiveUniformBlockName(Program, BlockIndex, BufferSize, &Length, Buffer);

#if ENABLE_UNIFORM_BUFFER_LAYOUT_NAME_MANGLING_CL1862097
		FString BlockName(Buffer);
#endif // #if ENABLE_UNIFORM_BUFFER_LAYOUT_NAME_MANGLING_CL1862097

		FString ReferencedBy;
		{
			GLint ReferencedByVS = 0;
			GLint ReferencedByPS = 0;
			GLint ReferencedByGS = 0;
			GLint ReferencedByHS = 0;
			GLint ReferencedByDS = 0;
			GLint ReferencedByCS = 0;

			glGetActiveUniformBlockiv(Program, BlockIndex, GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER, &ReferencedByVS);
			glGetActiveUniformBlockiv(Program, BlockIndex, GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER, &ReferencedByPS);
			glGetActiveUniformBlockiv(Program, BlockIndex, GL_UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER, &ReferencedByGS);
			if(GRHIFeatureLevel >= ERHIFeatureLevel::SM5)
			{
				glGetActiveUniformBlockiv(Program, BlockIndex, GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_CONTROL_SHADER, &ReferencedByHS);
				glGetActiveUniformBlockiv(Program, BlockIndex, GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_EVALUATION_SHADER, &ReferencedByDS);
				glGetActiveUniformBlockiv(Program, BlockIndex, GL_UNIFORM_BLOCK_REFERENCED_BY_COMPUTE_SHADER, &ReferencedByCS);
			}

			if(ReferencedByVS) {ReferencedBy += TEXT("V");}
			if(ReferencedByHS) {ReferencedBy += TEXT("H");}
			if(ReferencedByDS) {ReferencedBy += TEXT("D");}
			if(ReferencedByGS) {ReferencedBy += TEXT("G");}
			if(ReferencedByPS) {ReferencedBy += TEXT("P");}
			if(ReferencedByCS) {ReferencedBy += TEXT("C");}
		}
#if ENABLE_UNIFORM_BUFFER_LAYOUT_DUMP
		UE_LOG(LogRHI, Log, TEXT("  [%d] uniform block (%s) = %s, %d active uniforms, %d bytes {"),
			BlockIndex, 
			*ReferencedBy,
			ANSI_TO_TCHAR(Buffer),
			ActiveUniforms, 
			BlockBytes
			); 
#endif // #if ENABLE_UNIFORM_BUFFER_LAYOUT_DUMP
		if(ActiveUniforms)
		{
			// the other TArrays copy construct this to get the proper array size
			TArray<GLint> ActiveUniformIndices;
			ActiveUniformIndices.Init(ActiveUniforms);

			glGetActiveUniformBlockiv(Program, BlockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, ActiveUniformIndices.GetTypedData());
			
			TArray<GLint> ActiveUniformOffsets(ActiveUniformIndices);
			glGetActiveUniformsiv(Program, ActiveUniforms, reinterpret_cast<const GLuint*>(ActiveUniformIndices.GetData()), GL_UNIFORM_OFFSET, ActiveUniformOffsets.GetData());

			TArray<GLint> ActiveUniformSizes(ActiveUniformIndices);
			glGetActiveUniformsiv(Program, ActiveUniforms, reinterpret_cast<const GLuint*>(ActiveUniformIndices.GetData()), GL_UNIFORM_SIZE, ActiveUniformSizes.GetData());

			TArray<GLint> ActiveUniformTypes(ActiveUniformIndices);
			glGetActiveUniformsiv(Program, ActiveUniforms, reinterpret_cast<const GLuint*>(ActiveUniformIndices.GetData()), GL_UNIFORM_TYPE, ActiveUniformTypes.GetData());

			TArray<GLint> ActiveUniformArrayStrides(ActiveUniformIndices);
			glGetActiveUniformsiv(Program, ActiveUniforms, reinterpret_cast<const GLuint*>(ActiveUniformIndices.GetData()), GL_UNIFORM_ARRAY_STRIDE, ActiveUniformArrayStrides.GetData());

			extern const TCHAR* GetGLUniformTypeString( GLint UniformType );

			for(GLint i = 0; i < ActiveUniformIndices.Num(); ++i)
			{
				const GLint UniformIndex = ActiveUniformIndices[i];
				GLsizei Size = 0;
				GLenum Type = 0;
				glGetActiveUniform(Program, UniformIndex , BufferSize, &Length, &Size, &Type, Buffer);

#if ENABLE_UNIFORM_BUFFER_LAYOUT_DUMP				
				UE_LOG(LogRHI, Log, TEXT("    [%d] +%d %s %s %d elements %d array stride"),
					UniformIndex,
					ActiveUniformOffsets[i],
					GetGLUniformTypeString(ActiveUniformTypes[i]),
					ANSI_TO_TCHAR(Buffer),
					ActiveUniformSizes[i],
					ActiveUniformArrayStrides[i]
				); 
#endif // #if ENABLE_UNIFORM_BUFFER_LAYOUT_DUMP
		
				const UniformData GLSLUniform
				(
					ActiveUniformOffsets[i], 
					ActiveUniformArrayStrides[i] > 0 ? ActiveUniformSizes[i] : 0 // GLSL has 1 as array size for non-array uniforms, but FUniformBufferStruct assumes 0
				);
#if ENABLE_UNIFORM_BUFFER_LAYOUT_NAME_MANGLING_CL1862097
				VerifyUniformLayout(BlockName, ANSI_TO_TCHAR(Buffer), GLSLUniform);
#else
				VerifyUniformLayout(ANSI_TO_TCHAR(Buffer), GLSLUniform);
#endif
			}
		}
	}
}
#endif  // #if ENABLE_UNIFORM_BUFFER_LAYOUT_VERIFICATION

/**
 * Link vertex and pixel shaders in to an OpenGL program.
 */
static FOpenGLLinkedProgram* LinkProgram( const FOpenGLLinkedProgramConfiguration& Config)
{
	ANSICHAR Buf[32] = {0};

	SCOPE_CYCLE_COUNTER(STAT_OpenGLShaderLinkTime);
	VERIFY_GL_SCOPE();

	GLuint Program = glCreateProgram();

	// ensure that compute shaders are always alone
	check( (Config.Shaders[OGL_SHADER_STAGE_VERTEX].Resource == 0) != (Config.Shaders[OGL_SHADER_STAGE_COMPUTE].Resource == 0));
	check( (Config.Shaders[OGL_SHADER_STAGE_PIXEL].Resource == 0) != (Config.Shaders[OGL_SHADER_STAGE_COMPUTE].Resource == 0));

	if (Config.Shaders[OGL_SHADER_STAGE_VERTEX].Resource)
	{
		glAttachShader(Program, Config.Shaders[OGL_SHADER_STAGE_VERTEX].Resource);
	}
	if (Config.Shaders[OGL_SHADER_STAGE_PIXEL].Resource)
	{
		glAttachShader(Program, Config.Shaders[OGL_SHADER_STAGE_PIXEL].Resource);
	}
	if (Config.Shaders[OGL_SHADER_STAGE_GEOMETRY].Resource)
	{
		glAttachShader(Program, Config.Shaders[OGL_SHADER_STAGE_GEOMETRY].Resource);
	}
	if (Config.Shaders[OGL_SHADER_STAGE_HULL].Resource)
	{
		glAttachShader(Program, Config.Shaders[OGL_SHADER_STAGE_HULL].Resource);
	}
	if (Config.Shaders[OGL_SHADER_STAGE_DOMAIN].Resource)
	{
		glAttachShader(Program, Config.Shaders[OGL_SHADER_STAGE_DOMAIN].Resource);
	}
	if (Config.Shaders[OGL_SHADER_STAGE_COMPUTE].Resource)
	{
		glAttachShader(Program, Config.Shaders[OGL_SHADER_STAGE_COMPUTE].Resource);
	}
	
	// Bind attribute indices.
	if (Config.Shaders[OGL_SHADER_STAGE_VERTEX].Resource)
	{
		uint32 Mask = Config.Shaders[OGL_SHADER_STAGE_VERTEX].Bindings.InOutMask;
		uint32 Index = 0;
		FCStringAnsi::Strcpy(Buf, "in_ATTRIBUTE");
		while (Mask)
		{
			if (Mask & 0x1)
			{
				if (Index < 10)
				{
					Buf[12] = '0' + Index;
					Buf[13] = 0;
				}
				else
				{
					Buf[12] = '1';
					Buf[13] = '0' + (Index % 10);
					Buf[14] = 0;
				}
			glBindAttribLocation(Program, Index, Buf);
			}
			Index++;
			Mask >>= 1;
		}
	}

	// Bind frag data locations.
	if (Config.Shaders[OGL_SHADER_STAGE_PIXEL].Resource)
	{
		uint32 Mask = (Config.Shaders[OGL_SHADER_STAGE_PIXEL].Bindings.InOutMask) & 0x7fff; // mask out the depth bit
		uint32 Index = 0;
		FCStringAnsi::Strcpy(Buf, "out_Target");
		while (Mask)
		{
			if (Mask & 0x1)
			{
				if (Index < 10)
				{
					Buf[10] = '0' + Index;
					Buf[11] = 0;
				}
				else
				{
					Buf[10] = '1';
					Buf[11] = '0' + (Index % 10);
					Buf[12] = 0;
				}
				FOpenGL::BindFragDataLocation(Program, Index, Buf);
			}
			Index++;
			Mask >>= 1;
		}
	}

	// Link.
	glLinkProgram(Program);
	if (!VerifyLinkedProgram(Program))
	{
		return NULL;
	}

	FOpenGLLinkedProgram* LinkedProgram = new FOpenGLLinkedProgram;
	LinkedProgram->Config = Config;
	LinkedProgram->Program = Program;

	glUseProgram(Program);

	if (Config.Shaders[OGL_SHADER_STAGE_VERTEX].Resource)
	{
		LinkedProgram->ConfigureShaderStage(
			OGL_SHADER_STAGE_VERTEX,
			OGL_FIRST_UNIFORM_BUFFER
			);
		check(LinkedProgram->StagePackedUniformInfo[OGL_SHADER_STAGE_VERTEX].PackedUniformInfos.Num() <= Config.Shaders[OGL_SHADER_STAGE_VERTEX].Bindings.PackedGlobalArrays.Num());
	}

	if (Config.Shaders[OGL_SHADER_STAGE_PIXEL].Resource)
	{
		LinkedProgram->ConfigureShaderStage(
			OGL_SHADER_STAGE_PIXEL,
			OGL_FIRST_UNIFORM_BUFFER +
			Config.Shaders[OGL_SHADER_STAGE_VERTEX].Bindings.NumUniformBuffers
			);
		check(LinkedProgram->StagePackedUniformInfo[OGL_SHADER_STAGE_PIXEL].PackedUniformInfos.Num() <= Config.Shaders[OGL_SHADER_STAGE_PIXEL].Bindings.PackedGlobalArrays.Num());
	}

	if (Config.Shaders[OGL_SHADER_STAGE_GEOMETRY].Resource)
	{
		LinkedProgram->ConfigureShaderStage(
			OGL_SHADER_STAGE_GEOMETRY,
			OGL_FIRST_UNIFORM_BUFFER +
			Config.Shaders[OGL_SHADER_STAGE_VERTEX].Bindings.NumUniformBuffers +
			Config.Shaders[OGL_SHADER_STAGE_PIXEL].Bindings.NumUniformBuffers
			);
		check(LinkedProgram->StagePackedUniformInfo[OGL_SHADER_STAGE_GEOMETRY].PackedUniformInfos.Num() <= Config.Shaders[OGL_SHADER_STAGE_GEOMETRY].Bindings.PackedGlobalArrays.Num());
	}

	if (Config.Shaders[OGL_SHADER_STAGE_HULL].Resource)
	{
		LinkedProgram->ConfigureShaderStage(
			OGL_SHADER_STAGE_HULL,
			OGL_FIRST_UNIFORM_BUFFER +
			Config.Shaders[OGL_SHADER_STAGE_VERTEX].Bindings.NumUniformBuffers +
			Config.Shaders[OGL_SHADER_STAGE_PIXEL].Bindings.NumUniformBuffers +
			Config.Shaders[OGL_SHADER_STAGE_GEOMETRY].Bindings.NumUniformBuffers
		);
	}

	if (Config.Shaders[OGL_SHADER_STAGE_DOMAIN].Resource)
	{
		LinkedProgram->ConfigureShaderStage(
			OGL_SHADER_STAGE_DOMAIN,
			OGL_FIRST_UNIFORM_BUFFER +
			Config.Shaders[OGL_SHADER_STAGE_VERTEX].Bindings.NumUniformBuffers +
			Config.Shaders[OGL_SHADER_STAGE_PIXEL].Bindings.NumUniformBuffers +
			Config.Shaders[OGL_SHADER_STAGE_GEOMETRY].Bindings.NumUniformBuffers +
			Config.Shaders[OGL_SHADER_STAGE_HULL].Bindings.NumUniformBuffers
		);
	}

	if (Config.Shaders[OGL_SHADER_STAGE_COMPUTE].Resource)
	{
		LinkedProgram->ConfigureShaderStage(
			OGL_SHADER_STAGE_COMPUTE,
			OGL_FIRST_UNIFORM_BUFFER
			);
		check(LinkedProgram->StagePackedUniformInfo[OGL_SHADER_STAGE_COMPUTE].PackedUniformInfos.Num() <= Config.Shaders[OGL_SHADER_STAGE_COMPUTE].Bindings.PackedGlobalArrays.Num());
	}
#if ENABLE_UNIFORM_BUFFER_LAYOUT_VERIFICATION	
	VerifyUniformBufferLayouts(Program);
#endif // #if ENABLE_UNIFORM_BUFFER_LAYOUT_VERIFICATION
	return LinkedProgram;
}

FComputeShaderRHIRef FOpenGLDynamicRHI::RHICreateComputeShader(const TArray<uint8>& Code)
{
	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	
	FOpenGLComputeShader* ComputeShader = CompileOpenGLShader<FOpenGLComputeShader>(Code);

	check( ComputeShader != 0);

	FOpenGLLinkedProgramConfiguration Config;

	Config.Shaders[OGL_SHADER_STAGE_COMPUTE].Resource = ComputeShader->Resource;
	Config.Shaders[OGL_SHADER_STAGE_COMPUTE].Bindings = ComputeShader->Bindings;

	ComputeShader->LinkedProgram = LinkProgram( Config);

	check( ComputeShader->LinkedProgram != 0);
	return ComputeShader;
}

// ============================================================================================================================

FBoundShaderStateRHIRef FOpenGLDynamicRHI::RHICreateBoundShaderState(
	FVertexDeclarationRHIParamRef VertexDeclarationRHI, 
	FVertexShaderRHIParamRef VertexShaderRHI, 
	FHullShaderRHIParamRef HullShaderRHI,
	FDomainShaderRHIParamRef DomainShaderRHI, 
	FPixelShaderRHIParamRef PixelShaderRHI, 
	FGeometryShaderRHIParamRef GeometryShaderRHI
	)
{ 
	VERIFY_GL_SCOPE();

	SCOPE_CYCLE_COUNTER(STAT_OpenGLCreateBoundShaderStateTime);

	if(!PixelShaderRHI)
	{
		// use special null pixel shader when PixelShader was set to NULL
		PixelShaderRHI = TShaderMapRef<FNULLPS>(GetGlobalShaderMap())->GetPixelShader();
	}

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
	else
	{
		check(VertexDeclarationRHI);

		DYNAMIC_CAST_OPENGLRESOURCE(VertexDeclaration,VertexDeclaration);
		DYNAMIC_CAST_OPENGLRESOURCE(VertexShader,VertexShader);
		DYNAMIC_CAST_OPENGLRESOURCE(PixelShader,PixelShader);
		DYNAMIC_CAST_OPENGLRESOURCE(HullShader,HullShader);
		DYNAMIC_CAST_OPENGLRESOURCE(DomainShader,DomainShader);
		DYNAMIC_CAST_OPENGLRESOURCE(GeometryShader,GeometryShader);

		FOpenGLLinkedProgramConfiguration Config;

		// Fill-in the configuration
		Config.Shaders[OGL_SHADER_STAGE_VERTEX].Bindings = VertexShader->Bindings;
		Config.Shaders[OGL_SHADER_STAGE_VERTEX].Resource = VertexShader->Resource;
		Config.Shaders[OGL_SHADER_STAGE_PIXEL].Bindings = PixelShader->Bindings;
		Config.Shaders[OGL_SHADER_STAGE_PIXEL].Resource = PixelShader->Resource;
		if (GeometryShader)
		{
			Config.Shaders[OGL_SHADER_STAGE_GEOMETRY].Bindings = GeometryShader->Bindings;
			Config.Shaders[OGL_SHADER_STAGE_GEOMETRY].Resource = GeometryShader->Resource;
		}

		if ( FOpenGL::SupportsComputeShaders() )
		{
			if ( HullShader)
			{
				Config.Shaders[OGL_SHADER_STAGE_HULL].Bindings = HullShader->Bindings;
				Config.Shaders[OGL_SHADER_STAGE_HULL].Resource = HullShader->Resource;
			}
			if (FOpenGL::SupportsComputeShaders() && DomainShader)
			{
				Config.Shaders[OGL_SHADER_STAGE_DOMAIN].Bindings = DomainShader->Bindings;
				Config.Shaders[OGL_SHADER_STAGE_DOMAIN].Resource = DomainShader->Resource;
			}
		}

		// Check if we already have such a program in released programs cache. Use it, if we do.
		FOpenGLLinkedProgram* LinkedProgram = 0;

		int32 Index = StaticLastReleasedProgramsIndex;
		for( int CacheIndex = 0; CacheIndex < LAST_RELEASED_PROGRAMS_CACHE_COUNT; ++CacheIndex )
		{
			FOpenGLLinkedProgram* Prog = StaticLastReleasedPrograms[Index];
			if( Prog && Prog->Config == Config )
			{
				StaticLastReleasedPrograms[Index] = 0;
				LinkedProgram = Prog;
				break;
			}
			Index = (Index == LAST_RELEASED_PROGRAMS_CACHE_COUNT-1) ? 0 : Index+1;
		}

		if (!LinkedProgram)
		{
			FOpenGLLinkedProgram** CachedProgram = GetOpenGLProgramsCache().Find( Config);

			if (CachedProgram)
			{
				LinkedProgram = *CachedProgram;
			}
			else
			{
				const ANSICHAR* GlslCode = NULL;
				if (!VertexShader->bSuccessfullyCompiled)
				{
#if DEBUG_GL_SHADERS
					GlslCode = VertexShader->GlslCodeString;
#endif
					VertexShader->bSuccessfullyCompiled = VerifyCompiledShader(VertexShader->Resource, GlslCode);
				}
				if (!PixelShader->bSuccessfullyCompiled)
				{
#if DEBUG_GL_SHADERS
					GlslCode = PixelShader->GlslCodeString;
#endif
					PixelShader->bSuccessfullyCompiled = VerifyCompiledShader(PixelShader->Resource, GlslCode);
				}
				if (GeometryShader && !GeometryShader->bSuccessfullyCompiled)
				{
#if DEBUG_GL_SHADERS
					GlslCode = GeometryShader->GlslCodeString;
#endif
					GeometryShader->bSuccessfullyCompiled = VerifyCompiledShader(GeometryShader->Resource, GlslCode);
				}
				if ( FOpenGL::SupportsComputeShaders() )
				{
					if (HullShader && !HullShader->bSuccessfullyCompiled)
					{
#if DEBUG_GL_SHADERS
						GlslCode = HullShader->GlslCodeString;
#endif
						HullShader->bSuccessfullyCompiled = VerifyCompiledShader(HullShader->Resource, GlslCode);
					}
					if (DomainShader && !DomainShader->bSuccessfullyCompiled)
					{
#if DEBUG_GL_SHADERS
						GlslCode = DomainShader->GlslCodeString;
#endif
						DomainShader->bSuccessfullyCompiled = VerifyCompiledShader(DomainShader->Resource, GlslCode);
					}
				}

				// Make sure we have OpenGL context set up, and invalidate the parameters cache and current program (as we'll link a new one soon)
				GetContextStateForCurrentContext().Program = -1;
				MarkShaderParameterCachesDirty(PendingState.ShaderParameters, false);

				// Link program, using the data provided in config
				LinkedProgram = LinkProgram(Config);

				// Add this program to the cache
				GetOpenGLProgramsCache().Add(Config,LinkedProgram);

				if (LinkedProgram == NULL)
				{
#if DEBUG_GL_SHADERS
					if (VertexShader->bSuccessfullyCompiled)
					{
						UE_LOG(LogRHI,Error,TEXT("Vertex Shader:\n%s"),ANSI_TO_TCHAR(VertexShader->GlslCode.GetTypedData()));
					}
					if (PixelShader->bSuccessfullyCompiled)
					{
						UE_LOG(LogRHI,Error,TEXT("Pixel Shader:\n%s"),ANSI_TO_TCHAR(PixelShader->GlslCode.GetTypedData()));
					}
					if (GeometryShader && GeometryShader->bSuccessfullyCompiled)
					{
						UE_LOG(LogRHI,Error,TEXT("Geometry Shader:\n%s"),ANSI_TO_TCHAR(GeometryShader->GlslCode.GetTypedData()));
					}
					if ( FOpenGL::SupportsComputeShaders() )
					{
						if (HullShader && HullShader->bSuccessfullyCompiled)
						{
							UE_LOG(LogRHI,Error,TEXT("Hull Shader:\n%s"),ANSI_TO_TCHAR(HullShader->GlslCode.GetTypedData()));
						}
						if (DomainShader && DomainShader->bSuccessfullyCompiled)
						{
							UE_LOG(LogRHI,Error,TEXT("Domain Shader:\n%s"),ANSI_TO_TCHAR(DomainShader->GlslCode.GetTypedData()));
						}
					}
#endif //DEBUG_GL_SHADERS
					check(LinkedProgram);
				}
			}
		}

		FOpenGLBoundShaderState* BoundShaderState = new FOpenGLBoundShaderState(
			LinkedProgram,
			VertexDeclarationRHI,
			VertexShaderRHI,
			PixelShaderRHI,
			GeometryShaderRHI,
			HullShaderRHI,
			DomainShaderRHI
			);

		return BoundShaderState;
	}
}

void DestroyShadersAndPrograms()
{
	FOpenGLProgramsForReuse& ProgramCache = GetOpenGLProgramsCache();

	for( TMap<FOpenGLLinkedProgramConfiguration,FOpenGLLinkedProgram*>::TIterator It( ProgramCache ); It; ++It )
	{
		delete It.Value();
	}
	ProgramCache.Empty();
	
	StaticLastReleasedProgramsIndex = 0;

	FOpenGLCompiledShaderCache& ShaderCache = GetOpenGLCompiledShaderCache();
	for( TMap<FOpenGLCompiledShaderKey,GLuint>::TIterator It( ShaderCache ); It; ++It )
	{
		glDeleteShader(It.Value());
	}
	ShaderCache.Empty();
}

void FOpenGLDynamicRHI::BindPendingShaderState( FOpenGLContextState& ContextState )
{
	VERIFY_GL_SCOPE();

	bool ForceUniformBindingUpdate = false;

	GLuint PendingProgram = PendingState.BoundShaderState->LinkedProgram->Program;
	if (ContextState.Program != PendingProgram)
	{
		glUseProgram(PendingProgram);
		ContextState.Program = PendingProgram;
		MarkShaderParameterCachesDirty(PendingState.ShaderParameters, false);
		//Disable the forced rebinding to reduce driver overhead
#if 0
		ForceUniformBindingUpdate = true;
#endif
	}

	if (!IsES2Platform(GRHIShaderPlatform))
	{
		BindUniformBufferBase(ContextState, PendingState.BoundShaderState->VertexShader->Bindings.NumUniformBuffers,
			PendingState.BoundShaderState->VertexShader->BoundUniformBuffers, OGL_FIRST_UNIFORM_BUFFER, ForceUniformBindingUpdate);

		BindUniformBufferBase(ContextState, PendingState.BoundShaderState->PixelShader->Bindings.NumUniformBuffers,
			PendingState.BoundShaderState->PixelShader->BoundUniformBuffers,
			OGL_FIRST_UNIFORM_BUFFER + PendingState.BoundShaderState->VertexShader->Bindings.NumUniformBuffers,
			ForceUniformBindingUpdate);

		if (PendingState.BoundShaderState->GeometryShader)
		{
			BindUniformBufferBase(ContextState,
				PendingState.BoundShaderState->GeometryShader->Bindings.NumUniformBuffers,
				PendingState.BoundShaderState->GeometryShader->BoundUniformBuffers,
				OGL_FIRST_UNIFORM_BUFFER + PendingState.BoundShaderState->VertexShader->Bindings.NumUniformBuffers +
				PendingState.BoundShaderState->PixelShader->Bindings.NumUniformBuffers,
				ForceUniformBindingUpdate);
		}

		if (PendingState.BoundShaderState->HullShader)
		{
			BindUniformBufferBase(ContextState,
				PendingState.BoundShaderState->HullShader->Bindings.NumUniformBuffers,
				PendingState.BoundShaderState->HullShader->BoundUniformBuffers,
				OGL_FIRST_UNIFORM_BUFFER + PendingState.BoundShaderState->VertexShader->Bindings.NumUniformBuffers +
				PendingState.BoundShaderState->PixelShader->Bindings.NumUniformBuffers +
				PendingState.BoundShaderState->GeometryShader->Bindings.NumUniformBuffers,
				ForceUniformBindingUpdate);
		}

		if (PendingState.BoundShaderState->DomainShader)
		{
			BindUniformBufferBase(ContextState,
				PendingState.BoundShaderState->DomainShader->Bindings.NumUniformBuffers,
				PendingState.BoundShaderState->DomainShader->BoundUniformBuffers,
				OGL_FIRST_UNIFORM_BUFFER + PendingState.BoundShaderState->VertexShader->Bindings.NumUniformBuffers +
				PendingState.BoundShaderState->PixelShader->Bindings.NumUniformBuffers +
				PendingState.BoundShaderState->GeometryShader->Bindings.NumUniformBuffers +
				PendingState.BoundShaderState->HullShader->Bindings.NumUniformBuffers,
				ForceUniformBindingUpdate);
		}
	}
}

FOpenGLBoundShaderState::FOpenGLBoundShaderState(
	FOpenGLLinkedProgram* InLinkedProgram,
	FVertexDeclarationRHIParamRef InVertexDeclarationRHI,
	FVertexShaderRHIParamRef InVertexShaderRHI,
	FPixelShaderRHIParamRef InPixelShaderRHI,
	FGeometryShaderRHIParamRef InGeometryShaderRHI,
	FHullShaderRHIParamRef InHullShaderRHI,
	FDomainShaderRHIParamRef InDomainShaderRHI
	)
	:	CacheLink(InVertexDeclarationRHI, InVertexShaderRHI, InPixelShaderRHI,
		InHullShaderRHI, InDomainShaderRHI,	InGeometryShaderRHI, this)
{
	DYNAMIC_CAST_OPENGLRESOURCE(VertexDeclaration,InVertexDeclaration);
	DYNAMIC_CAST_OPENGLRESOURCE(VertexShader,InVertexShader);
	DYNAMIC_CAST_OPENGLRESOURCE(PixelShader,InPixelShader);
	DYNAMIC_CAST_OPENGLRESOURCE(HullShader,InHullShader);
	DYNAMIC_CAST_OPENGLRESOURCE(DomainShader,InDomainShader);
	DYNAMIC_CAST_OPENGLRESOURCE(GeometryShader,InGeometryShader);

	VertexDeclaration = InVertexDeclaration;
	VertexShader = InVertexShader;
	PixelShader = InPixelShader;
	GeometryShader = InGeometryShader;

	HullShader = InHullShader;
	DomainShader = InDomainShader;

	LinkedProgram = InLinkedProgram;
}

FOpenGLBoundShaderState::~FOpenGLBoundShaderState()
{
	check(LinkedProgram);
	FOpenGLLinkedProgram* Prog = StaticLastReleasedPrograms[StaticLastReleasedProgramsIndex];
	StaticLastReleasedPrograms[StaticLastReleasedProgramsIndex++] = LinkedProgram;
	if (StaticLastReleasedProgramsIndex == LAST_RELEASED_PROGRAMS_CACHE_COUNT)
	{
		StaticLastReleasedProgramsIndex = 0;
	}
	OnProgramDeletion(LinkedProgram->Program);
}

bool FOpenGLBoundShaderState::NeedsTextureStage(int32 TextureStageIndex)
{
	return LinkedProgram->TextureStageNeeds[TextureStageIndex];
}

int32 FOpenGLBoundShaderState::MaxTextureStageUsed()
{
	return LinkedProgram->MaxTextureStage;
}

bool FOpenGLComputeShader::NeedsTextureStage(int32 TextureStageIndex)
{
	return LinkedProgram->TextureStageNeeds[TextureStageIndex];
}

int32 FOpenGLComputeShader::MaxTextureStageUsed()
{
	return LinkedProgram->MaxTextureStage;
}

bool FOpenGLComputeShader::NeedsUAVStage(int32 UAVStageIndex)
{
	return LinkedProgram->UAVStageNeeds[UAVStageIndex];
}

void FOpenGLDynamicRHI::BindPendingComputeShaderState(FOpenGLContextState& ContextState, FComputeShaderRHIParamRef ComputeShaderRHI)
{
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(ComputeShader,ComputeShader);
	bool ForceUniformBindingUpdate = false;

	GLuint PendingProgram = ComputeShader->LinkedProgram->Program;
	if (ContextState.Program != PendingProgram)
	{
		glUseProgram(PendingProgram);
		ContextState.Program = PendingProgram;
		MarkShaderParameterCachesDirty(PendingState.ShaderParameters, true);
		ForceUniformBindingUpdate = true;
	}

	if (!IsES2Platform(GRHIShaderPlatform))
	{
		BindUniformBufferBase(ContextState,
			ComputeShader->Bindings.NumUniformBuffers,
			ComputeShader->BoundUniformBuffers,
			OGL_FIRST_UNIFORM_BUFFER, ForceUniformBindingUpdate);
	}
}

/** Constructor. */
FOpenGLShaderParameterCache::FOpenGLShaderParameterCache() :
	GlobalUniformArraySize(-1)
{
	for (int32 ArrayIndex = 0; ArrayIndex < OGL_PACKED_TYPEINDEX_MAX; ++ArrayIndex)
	{
		PackedGlobalUniformDirty[ArrayIndex].LowVector = 0;
		PackedGlobalUniformDirty[ArrayIndex].HighVector = MAX_uint32;
	}
}

void FOpenGLShaderParameterCache::InitializeResources(int32 UniformArraySize)
{
	check(GlobalUniformArraySize == -1);

	PackedGlobalUniforms[0] = (uint8*)FMemory::Malloc(UniformArraySize * OGL_PACKED_TYPEINDEX_MAX);
	PackedUniformsScratch[0] = (uint8*)FMemory::Malloc(UniformArraySize * OGL_PACKED_TYPEINDEX_MAX);

	FMemory::Memzero(PackedGlobalUniforms[0], UniformArraySize * OGL_PACKED_TYPEINDEX_MAX);
	FMemory::Memzero(PackedUniformsScratch[0], UniformArraySize * OGL_PACKED_TYPEINDEX_MAX);
	for (int32 ArrayIndex = 1; ArrayIndex < OGL_PACKED_TYPEINDEX_MAX; ++ArrayIndex)
	{
		PackedGlobalUniforms[ArrayIndex] = PackedGlobalUniforms[ArrayIndex - 1] + UniformArraySize;
		PackedUniformsScratch[ArrayIndex] = PackedUniformsScratch[ArrayIndex - 1] + UniformArraySize;
	}
	GlobalUniformArraySize = UniformArraySize;

	for (int32 ArrayIndex = 0; ArrayIndex < OGL_PACKED_TYPEINDEX_MAX; ++ArrayIndex)
	{
		PackedGlobalUniformDirty[ArrayIndex].LowVector = 0;
		PackedGlobalUniformDirty[ArrayIndex].HighVector = UniformArraySize;
	}
}

/** Destructor. */
FOpenGLShaderParameterCache::~FOpenGLShaderParameterCache()
{
	if (GlobalUniformArraySize > 0)
	{
		FMemory::Free(PackedUniformsScratch[0]);
		FMemory::Free(PackedGlobalUniforms[0]);
	}

	FMemory::MemZero(PackedUniformsScratch);
	FMemory::MemZero(PackedGlobalUniforms);

	GlobalUniformArraySize = -1;
}

/**
 * Marks all uniform arrays as dirty.
 */
void FOpenGLShaderParameterCache::MarkAllDirty()
{
	for (int32 ArrayIndex = 0; ArrayIndex < OGL_PACKED_TYPEINDEX_MAX; ++ArrayIndex)
	{
		PackedGlobalUniformDirty[ArrayIndex].LowVector = 0;
		PackedGlobalUniformDirty[ArrayIndex].HighVector = GlobalUniformArraySize;
	}
}

/**
 * Set parameter values.
 */
void FOpenGLShaderParameterCache::Set(uint32 BufferIndexName, uint32 ByteOffset, uint32 NumBytes, const void* NewValues)
{
	uint32 BufferIndex = GLPackedTypeNameToTypeIndex(BufferIndexName);
	check(GlobalUniformArraySize != -1);
	check(BufferIndex < OGL_PACKED_TYPEINDEX_MAX);
	check(ByteOffset + NumBytes <= (uint32)GlobalUniformArraySize);
	PackedGlobalUniformDirty[BufferIndex].LowVector = FMath::Min(PackedGlobalUniformDirty[BufferIndex].LowVector, ByteOffset / SizeOfFloat4);
	PackedGlobalUniformDirty[BufferIndex].HighVector = FMath::Max(PackedGlobalUniformDirty[BufferIndex].HighVector, (ByteOffset + NumBytes + SizeOfFloat4 - 1) / SizeOfFloat4);
	FMemory::Memcpy(PackedGlobalUniforms[BufferIndex] + ByteOffset, NewValues, NumBytes);
}

/**
 * Commit shader parameters to the currently bound program.
 * @param ParameterTable - Information on the bound uniform arrays for the program.
 */
void FOpenGLShaderParameterCache::CommitPackedGlobals(const FOpenGLLinkedProgram* LinkedProgram, int32 Stage)
{
	SCOPE_CYCLE_COUNTER(STAT_OpenGLUniformCommitTime);
	VERIFY_GL_SCOPE();
	const uint32 BytesPerRegister = 16;

	/**
	 * Note that this always uploads the entire uniform array when it is dirty.
	 * The arrays are marked dirty either when the bound shader state changes or
	 * a value in the array is modified. OpenGL actually caches uniforms per-
	 * program. If we shadowed those per-program uniforms we could avoid calling
	 * glUniform4?v for values that have not changed since the last invocation
	 * of the program. 
	 *
	 * It's unclear whether the driver does the same thing and whether there is
	 * a performance benefit. Even if there is, this type of caching makes any
	 * multithreading vastly more difficult, so for now uniforms are not cached
	 * per-program.
	 */
	const TArray<FOpenGLLinkedProgram::FPackedUniformInfo>& PackedUniforms = LinkedProgram->StagePackedUniformInfo[Stage].PackedUniformInfos;
	const TArray<FOpenGLPackedArrayInfo>& PackedArrays = LinkedProgram->Config.Shaders[Stage].Bindings.PackedGlobalArrays;
	for (int32 PackedUniform = 0; PackedUniform < PackedUniforms.Num(); ++PackedUniform)
	{
		const FOpenGLLinkedProgram::FPackedUniformInfo& UniformInfo = PackedUniforms[PackedUniform];
		const int32 ArrayIndex = UniformInfo.Index;
		const int32 NumVectors = PackedArrays[PackedUniform].Size / BytesPerRegister;
		GLint Location = UniformInfo.Location;
		const void* UniformData = PackedGlobalUniforms[ArrayIndex];
		if (PackedGlobalUniformDirty[ArrayIndex].HighVector > PackedGlobalUniformDirty[ArrayIndex].LowVector)
		{
			const uint32 StartVector = PackedGlobalUniformDirty[ArrayIndex].LowVector;
			uint32 NumDirtyVectors = PackedGlobalUniformDirty[ArrayIndex].HighVector - StartVector;
			NumDirtyVectors = FMath::Min(NumDirtyVectors, NumVectors - StartVector);
			check(NumDirtyVectors);
			UniformData = (uint8*)UniformData + PackedGlobalUniformDirty[ArrayIndex].LowVector * SizeOfFloat4;
			Location += StartVector;
			switch (UniformInfo.Index)
			{
			case OGL_PACKED_TYPEINDEX_HIGHP:
			case OGL_PACKED_TYPEINDEX_MEDIUMP:
			case OGL_PACKED_TYPEINDEX_LOWP:
				glUniform4fv(Location, NumDirtyVectors, (GLfloat*)UniformData);
				break;

			case OGL_PACKED_TYPEINDEX_INT:
				glUniform4iv(Location, NumDirtyVectors, (GLint*)UniformData);
				break;

			case OGL_PACKED_TYPEINDEX_UINT:
				FOpenGL::Uniform4uiv(Location, NumDirtyVectors, (GLuint*)UniformData);
				break;
			}

			PackedGlobalUniformDirty[ArrayIndex].LowVector = GlobalUniformArraySize / SizeOfFloat4;
			PackedGlobalUniformDirty[ArrayIndex].HighVector = 0;
		}
	}
}

void FOpenGLShaderParameterCache::CommitPackedUniformBuffers(FOpenGLLinkedProgram* LinkedProgram, int32 Stage, const TArray< TRefCountPtr<FRHIUniformBuffer> >& RHIUniformBuffers, const TArray<FOpenGLUniformBufferCopyInfo>& UniformBuffersCopyInfo)
{
	SCOPE_CYCLE_COUNTER(STAT_OpenGLConstantBufferUpdateTime);
	VERIFY_GL_SCOPE();

	// Uniform Buffers are split into precision/type; the list of RHI UBs is traversed and if a new one was set, its
	// contents are copied per precision/type into corresponding scratch buffers which are then uploaded to the program
	const FOpenGLShaderBindings& Bindings = LinkedProgram->Config.Shaders[Stage].Bindings;
	check(Bindings.NumUniformBuffers == RHIUniformBuffers.Num());

	if (Bindings.bFlattenUB)
	{
		int32 LastInfoIndex = 0;
		for (int32 BufferIndex = 0; BufferIndex < RHIUniformBuffers.Num(); ++BufferIndex)
		{
			const FRHIUniformBuffer* RHIUniformBuffer = RHIUniformBuffers[BufferIndex];
			check(RHIUniformBuffer);
			FOpenGLEUniformBuffer* EmulatedUniformBuffer = (FOpenGLEUniformBuffer*)RHIUniformBuffer;
			const uint32* RESTRICT SourceData = EmulatedUniformBuffer->Buffer->Data.GetTypedData();
			for (int32 InfoIndex = LastInfoIndex; InfoIndex < UniformBuffersCopyInfo.Num(); ++InfoIndex)
			{
				const FOpenGLUniformBufferCopyInfo& Info = UniformBuffersCopyInfo[InfoIndex];
				if (Info.SourceUBIndex == BufferIndex)
				{
					float* RESTRICT ScratchMem = (float*)PackedGlobalUniforms[Info.DestUBTypeIndex];
					ScratchMem += Info.DestOffsetInFloats;
					FMemory::Memcpy(ScratchMem, SourceData + Info.SourceOffsetInFloats, Info.SizeInFloats * sizeof(float));
					PackedGlobalUniformDirty[Info.DestUBTypeIndex].LowVector = FMath::Min(PackedGlobalUniformDirty[Info.DestUBTypeIndex].LowVector, uint32(Info.DestOffsetInFloats / NumFloatsInFloat4));
					PackedGlobalUniformDirty[Info.DestUBTypeIndex].HighVector = FMath::Max(PackedGlobalUniformDirty[Info.DestUBTypeIndex].HighVector, uint32((Info.DestOffsetInFloats + Info.SizeInFloats + NumFloatsInFloat4 - 1) / NumFloatsInFloat4));
				}
				else
				{
					LastInfoIndex = InfoIndex;
					break;
				}
			}
		}
	}
	else
	{
		const auto& PackedUniformBufferInfos = LinkedProgram->StagePackedUniformInfo[Stage].PackedUniformBufferInfos;
		int32 LastCopyInfoIndex = 0;
		auto& EmulatedUniformBufferSet = LinkedProgram->StagePackedUniformInfo[Stage].LastEmulatedUniformBufferSet;
		for (int32 BufferIndex = 0; BufferIndex < RHIUniformBuffers.Num(); ++BufferIndex)
		{
			const FRHIUniformBuffer* RHIUniformBuffer = RHIUniformBuffers[BufferIndex];
			check(RHIUniformBuffer);
			FOpenGLEUniformBuffer* EmulatedUniformBuffer = (FOpenGLEUniformBuffer*)RHIUniformBuffer;
			if (EmulatedUniformBufferSet[BufferIndex] != EmulatedUniformBuffer->UniqueID)
			{
				EmulatedUniformBufferSet[BufferIndex] = EmulatedUniformBuffer->UniqueID;

				// Go through the list of copy commands and perform the appropriate copy into the scratch buffer
				for (int32 InfoIndex = LastCopyInfoIndex; InfoIndex < UniformBuffersCopyInfo.Num(); ++InfoIndex)
				{
					const FOpenGLUniformBufferCopyInfo& Info = UniformBuffersCopyInfo[InfoIndex];
					if (Info.SourceUBIndex == BufferIndex)
					{
						const uint32* RESTRICT SourceData = EmulatedUniformBuffer->Buffer->Data.GetTypedData();
						SourceData += Info.SourceOffsetInFloats;
						float* RESTRICT ScratchMem = (float*)PackedUniformsScratch[Info.DestUBTypeIndex];
						ScratchMem += Info.DestOffsetInFloats;
						FMemory::Memcpy(ScratchMem, SourceData, Info.SizeInFloats * sizeof(float));
					}
					else if (Info.SourceUBIndex > BufferIndex)
					{
						// Done finding current copies
						LastCopyInfoIndex = InfoIndex;
						break;
					}

					// keep going since we could have skipped this loop when skipping cached UBs...
				}

				// Upload the split buffers to the program
				const auto& UniformBufferUploadInfoList = PackedUniformBufferInfos[BufferIndex];
				auto& UBInfo = Bindings.PackedUniformBuffers[BufferIndex];
				for (int32 InfoIndex = 0; InfoIndex < UniformBufferUploadInfoList.Num(); ++InfoIndex)
				{
					const auto& UniformInfo = UniformBufferUploadInfoList[InfoIndex];
					const void* RESTRICT UniformData = PackedUniformsScratch[UniformInfo.Index];
					int32 NumVectors = UBInfo[InfoIndex].Size / SizeOfFloat4;
					check(UniformInfo.ArrayType == UBInfo[InfoIndex].TypeName);
					switch (UniformInfo.Index)
					{
					case OGL_PACKED_TYPEINDEX_HIGHP:
					case OGL_PACKED_TYPEINDEX_MEDIUMP:
					case OGL_PACKED_TYPEINDEX_LOWP:
						glUniform4fv(UniformInfo.Location, NumVectors, (GLfloat*)UniformData);
						break;

					case OGL_PACKED_TYPEINDEX_INT:
						glUniform4iv(UniformInfo.Location, NumVectors, (GLint*)UniformData);
						break;

					case OGL_PACKED_TYPEINDEX_UINT:
						FOpenGL::Uniform4uiv(UniformInfo.Location, NumVectors, (GLuint*)UniformData);
						break;
					}
				}
			}
		}
	}
}
