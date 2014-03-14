// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OpenGLShaderResources.h: OpenGL shader resource RHI definitions.
=============================================================================*/

#pragma once

/**
 * OpenGL shader stages.
 */
enum
{
	OGL_SHADER_STAGE_VERTEX = 0,
	OGL_SHADER_STAGE_PIXEL,
	OGL_SHADER_STAGE_GEOMETRY,
	OGL_SHADER_STAGE_HULL,
	OGL_SHADER_STAGE_DOMAIN,
	OGL_NUM_NON_COMPUTE_SHADER_STAGES,
	OGL_SHADER_STAGE_COMPUTE = OGL_NUM_NON_COMPUTE_SHADER_STAGES,
	OGL_NUM_SHADER_STAGES
};

/**
 * Shader related constants.
 */
enum
{
	OGL_PACKED_TYPENAME_HIGHP	= 'h',	// Make sure these enums match hlslcc
	OGL_PACKED_TYPENAME_MEDIUMP	= 'm',
	OGL_PACKED_TYPENAME_LOWP	= 'l',
	OGL_PACKED_TYPENAME_INT		= 'i',
	OGL_PACKED_TYPENAME_UINT	= 'u',
	OGL_PACKED_TYPENAME_SAMPLER	= 's',
	OGL_PACKED_TYPENAME_IMAGE	= 'g',

	OGL_PACKED_TYPEINDEX_HIGHP		= 0,
	OGL_PACKED_TYPEINDEX_MEDIUMP	= 1,
	OGL_PACKED_TYPEINDEX_LOWP		= 2,
	OGL_PACKED_TYPEINDEX_INT		= 3,
	OGL_PACKED_TYPEINDEX_UINT		= 4,
	OGL_PACKED_TYPEINDEX_MAX		= 5,

	OGL_MAX_UNIFORM_BUFFER_BINDINGS = 12,	// @todo-mobile: Remove me
	OGL_FIRST_UNIFORM_BUFFER = 0,			// @todo-mobile: Remove me
	OGL_MAX_COMPUTE_STAGE_UAV_UNITS = 8,	// @todo-mobile: Remove me
	OGL_UAV_NOT_SUPPORTED_FOR_GRAPHICS_UNIT = -1, // for now, only CS supports UAVs/ images
};

static FORCEINLINE uint8 GLPackedTypeIndexToTypeName(uint8 ArrayType)
{
	switch (ArrayType)
	{
		case OGL_PACKED_TYPEINDEX_HIGHP:	return OGL_PACKED_TYPENAME_HIGHP;
		case OGL_PACKED_TYPEINDEX_MEDIUMP:	return OGL_PACKED_TYPENAME_MEDIUMP;
		case OGL_PACKED_TYPEINDEX_LOWP:		return OGL_PACKED_TYPENAME_LOWP;
		case OGL_PACKED_TYPEINDEX_INT:		return OGL_PACKED_TYPENAME_INT;
		case OGL_PACKED_TYPEINDEX_UINT:		return OGL_PACKED_TYPENAME_UINT;
	}
	check(0);
	return 0;
}

static FORCEINLINE uint8 GLPackedTypeNameToTypeIndex(uint8 ArrayName)
{
	switch (ArrayName)
	{
		case OGL_PACKED_TYPENAME_HIGHP:		return OGL_PACKED_TYPEINDEX_HIGHP;
		case OGL_PACKED_TYPENAME_MEDIUMP:	return OGL_PACKED_TYPEINDEX_MEDIUMP;
		case OGL_PACKED_TYPENAME_LOWP:		return OGL_PACKED_TYPEINDEX_LOWP;
		case OGL_PACKED_TYPENAME_INT:		return OGL_PACKED_TYPEINDEX_INT;
		case OGL_PACKED_TYPENAME_UINT:		return OGL_PACKED_TYPEINDEX_UINT;
	}
	check(0);
	return 0;
}

struct FOpenGLPackedArrayInfo
{
	uint16	Size;		// Bytes
	uint8	TypeName;	// OGL_PACKED_TYPENAME
	uint8	TypeIndex;	// OGL_PACKED_TYPE
};

inline FArchive& operator<<(FArchive& Ar, FOpenGLPackedArrayInfo& Info)
{
	Ar << Info.Size;
	Ar << Info.TypeName;
	Ar << Info.TypeIndex;
	return Ar;
}

/**
 * Shader binding information.
 */
struct FOpenGLShaderBindings
{
	TArray<TArray<FOpenGLPackedArrayInfo>>	PackedUniformBuffers;
	TArray<FOpenGLPackedArrayInfo>			PackedGlobalArrays;

	uint16	InOutMask;
	uint8	NumSamplers;
	uint8	NumUniformBuffers;
	uint8	NumUAVs;
	bool	bFlattenUB;


	FOpenGLShaderBindings() :
		InOutMask(0),
		NumSamplers(0),
		NumUniformBuffers(0),
		NumUAVs(0),
		bFlattenUB(false)
	{
	}

	friend bool operator==( const FOpenGLShaderBindings &A, const FOpenGLShaderBindings &B)
	{
		bool bEqual = true;

		bEqual &= A.InOutMask == B.InOutMask;
		bEqual &= A.NumSamplers == B.NumSamplers;
		bEqual &= A.NumUniformBuffers == B.NumUniformBuffers;
		bEqual &= A.NumUAVs == B.NumUAVs;
		bEqual &= A.bFlattenUB == B.bFlattenUB;
		bEqual &= A.PackedGlobalArrays.Num() == B.PackedGlobalArrays.Num();
		bEqual &= A.PackedUniformBuffers.Num() == B.PackedUniformBuffers.Num();

		if ( !bEqual )
		{
			return bEqual;
		}

		bEqual &= FMemory::Memcmp(A.PackedGlobalArrays.GetTypedData(),B.PackedGlobalArrays.GetTypedData(),A.PackedGlobalArrays.GetTypeSize()*A.PackedGlobalArrays.Num()) == 0; 

		for (int32 Item = 0; Item < A.PackedUniformBuffers.Num(); Item++)
		{
			const TArray<FOpenGLPackedArrayInfo> &ArrayA = A.PackedUniformBuffers[Item];
			const TArray<FOpenGLPackedArrayInfo> &ArrayB = B.PackedUniformBuffers[Item];

			bEqual &= FMemory::Memcmp(ArrayA.GetTypedData(),ArrayB.GetTypedData(),ArrayA.GetTypeSize()*ArrayA.Num()) == 0;
		}

		return bEqual;
	}

	friend uint32 GetTypeHash(const FOpenGLShaderBindings &Binding)
	{
		uint32 Hash = 0;
		Hash = Binding.InOutMask;
		Hash |= Binding.NumSamplers << 16;
		Hash |= Binding.NumUniformBuffers << 24;
		Hash ^= Binding.NumUAVs;
		Hash ^= Binding.bFlattenUB << 8;
		Hash ^= FCrc::MemCrc_DEPRECATED( Binding.PackedGlobalArrays.GetTypedData(), Binding.PackedGlobalArrays.GetTypeSize()*Binding.PackedGlobalArrays.Num());

		for (int32 Item = 0; Item < Binding.PackedUniformBuffers.Num(); Item++)
		{
			const TArray<FOpenGLPackedArrayInfo> &Array = Binding.PackedUniformBuffers[Item];
			Hash ^= FCrc::MemCrc_DEPRECATED( Array.GetTypedData(), Array.GetTypeSize()* Array.Num());
		}
		return Hash;
	}
};

inline FArchive& operator<<(FArchive& Ar, FOpenGLShaderBindings& Bindings)
{
	Ar << Bindings.PackedUniformBuffers;
	Ar << Bindings.PackedGlobalArrays;
	Ar << Bindings.InOutMask;
	Ar << Bindings.NumSamplers;
	Ar << Bindings.NumUniformBuffers;
	Ar << Bindings.NumUAVs;
	Ar << Bindings.bFlattenUB;
	return Ar;
}

// Information for copying members from uniform buffers to packed
struct FOpenGLUniformBufferCopyInfo
{
	uint16 SourceOffsetInFloats;
	uint8 SourceUBIndex;
	uint8 DestUBIndex;
	uint8 DestUBTypeName;
	uint8 DestUBTypeIndex;
	uint16 DestOffsetInFloats;
	uint16 SizeInFloats;
};

inline FArchive& operator<<(FArchive& Ar, FOpenGLUniformBufferCopyInfo& Info)
{
	Ar << Info.SourceOffsetInFloats;
	Ar << Info.SourceUBIndex;
	Ar << Info.DestUBIndex;
	Ar << Info.DestUBTypeName;
	if (Ar.IsLoading())
	{
		Info.DestUBTypeIndex = GLPackedTypeNameToTypeIndex(Info.DestUBTypeName);
	}
	Ar << Info.DestOffsetInFloats;
	Ar << Info.SizeInFloats;
	return Ar;
}

/**
 * Code header information.
 */
struct FOpenGLCodeHeader
{
	uint32 GlslMarker;
	uint16 FrequencyMarker;
	FOpenGLShaderBindings Bindings;
	TArray<FOpenGLUniformBufferCopyInfo> UniformBuffersCopyInfo;
};

inline FArchive& operator<<(FArchive& Ar, FOpenGLCodeHeader& Header)
{
	Ar << Header.GlslMarker;
	Ar << Header.FrequencyMarker;
	Ar << Header.Bindings;
	int32 NumInfos = Header.UniformBuffersCopyInfo.Num();
	Ar << NumInfos;
	if (Ar.IsSaving())
	{
		for (int32 Index = 0; Index < NumInfos; ++Index)
		{
			Ar << Header.UniformBuffersCopyInfo[Index];
		}
	}
	else if (Ar.IsLoading())
	{
		Header.UniformBuffersCopyInfo.Empty(NumInfos);
		for (int32 Index = 0; Index < NumInfos; ++Index)
		{
			FOpenGLUniformBufferCopyInfo Info;
			Ar << Info;
			Header.UniformBuffersCopyInfo.Add(Info);
		}
	}
	return Ar;
}

class FOpenGLLinkedProgram;

/**
 * OpenGL shader resource.
 */
template <typename RHIResourceType, GLenum GLTypeEnum>
class TOpenGLShader : public RHIResourceType
{
public:

	static const GLenum TypeEnum = GLTypeEnum;

	/** The OpenGL resource ID. */
	GLuint Resource;
	/** true if the shader has compiled successfully. */
	bool bSuccessfullyCompiled;
	/** External bindings for this shader. */
	FOpenGLShaderBindings Bindings;
	TArray< TRefCountPtr<FRHIUniformBuffer> > BoundUniformBuffers;
	// List of memory copies from RHIUniformBuffer to packed uniforms
	TArray<FOpenGLUniformBufferCopyInfo> UniformBuffersCopyInfo;

#if DEBUG_GL_SHADERS
	TArray<ANSICHAR> GlslCode;
	const ANSICHAR*  GlslCodeString; // make it easier in VS to see shader code in debug mode; points to begin of GlslCode
#endif

	/** Constructor. */
	TOpenGLShader()
		: Resource(0)
		, bSuccessfullyCompiled(false)
	{
		FMemory::Memzero( &Bindings, sizeof(Bindings) );
	}

	/** Destructor. */
	~TOpenGLShader()
	{
//		if (Resource)
//		{
//			glDeleteShader(Resource);
//		}
	}
};


typedef TOpenGLShader<FRHIVertexShader, GL_VERTEX_SHADER> FOpenGLVertexShader;
typedef TOpenGLShader<FRHIPixelShader, GL_FRAGMENT_SHADER> FOpenGLPixelShader;
typedef TOpenGLShader<FRHIGeometryShader, GL_GEOMETRY_SHADER> FOpenGLGeometryShader;
typedef TOpenGLShader<FRHIHullShader, GL_TESS_CONTROL_SHADER> FOpenGLHullShader;
typedef TOpenGLShader<FRHIDomainShader, GL_TESS_EVALUATION_SHADER> FOpenGLDomainShader;


class FOpenGLComputeShader : public TOpenGLShader<FRHIComputeShader, GL_COMPUTE_SHADER>
{
public:
	FOpenGLComputeShader():
		LinkedProgram(0)
	{

	}

	bool NeedsTextureStage(int32 TextureStageIndex);
	int32 MaxTextureStageUsed();
	bool NeedsUAVStage(int32 UAVStageIndex);
	FOpenGLLinkedProgram* LinkedProgram;
};



/**
 * Caching of OpenGL uniform parameters.
 */
class FOpenGLShaderParameterCache
{
public:
	/** Constructor. */
	FOpenGLShaderParameterCache();

	/** Destructor. */
	~FOpenGLShaderParameterCache();

	void InitializeResources(int32 UniformArraySize);

	/**
	 * Marks all uniform arrays as dirty.
	 */
	void MarkAllDirty();

	/**
	 * Sets values directly into the packed uniform array
	 */
	void Set(uint32 BufferIndex, uint32 ByteOffset, uint32 NumBytes, const void* NewValues);

	/**
	 * Commit shader parameters to the currently bound program.
	 * @param ParameterTable - Information on the bound uniform arrays for the program.
	 */
	void CommitPackedGlobals(const FOpenGLLinkedProgram* LinkedProgram, int32 Stage);

	void CommitPackedUniformBuffers(FOpenGLLinkedProgram* LinkedProgram, int32 Stage, const TArray< TRefCountPtr<FRHIUniformBuffer> >& UniformBuffers, const TArray<FOpenGLUniformBufferCopyInfo>& UniformBuffersCopyInfo);

private:
	/** CPU memory block for storing uniform values. */
	uint8* PackedGlobalUniforms[OGL_PACKED_TYPEINDEX_MAX];
	
	struct FRange
	{
		uint32	LowVector;
		uint32	HighVector;
	};
	/** Dirty ranges for each uniform array. */
	FRange	PackedGlobalUniformDirty[OGL_PACKED_TYPEINDEX_MAX];

	/** Scratch CPU memory block for uploading packed uniforms. */
	uint8* PackedUniformsScratch[OGL_PACKED_TYPEINDEX_MAX];

	int32 GlobalUniformArraySize;
};
