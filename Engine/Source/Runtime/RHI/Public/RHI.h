// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHI.h: Render Hardware Interface definitions.
=============================================================================*/

#ifndef __RHI_h__
#define __RHI_h__

#include "Core.h"
#include "StaticArray.h"
#include "Runtime/Engine/Public/PixelFormat.h" // for EPixelFormat

// Forward declarations.
class FSceneView;
struct FMeshBatch;

/** RHI Logging. */
RHI_API DECLARE_LOG_CATEGORY_EXTERN(LogRHI,Log,VeryVerbose);

/**
 * RHI configuration settings.
 */

namespace RHIConfig
{
	RHI_API bool ShouldSaveScreenshotAfterProfilingGPU();
	RHI_API bool ShouldShowProfilerAfterProfilingGPU();
	RHI_API float GetGPUHitchThreshold();
}

/**
 * RHI globals.
 */

/** True if the render hardware has been initialized. */
extern RHI_API bool GIsRHIInitialized;

/**
 * RHI capabilities.
 */

/** Maximum number of miplevels in a texture. */
enum { MAX_TEXTURE_MIP_COUNT = 14 };

/** The maximum number of vertex elements which can be used by a vertex declaration. */
enum { MaxVertexElementCount = 16 };

/** The alignment in bytes between elements of array shader parameters. */
enum { ShaderArrayElementAlignBytes = 16 };

/** The number of render-targets that may be simultaneously written to. */
enum { MaxSimultaneousRenderTargets = 8 };

/** The number of UAVs that may be simultaneously bound to a shader. */
enum { MaxSimultaneousUAVs = 8 };


/** The maximum number of mip-maps that a texture can contain. 	*/
extern	RHI_API int32		GMaxTextureMipCount;

/** true if the RHI supports textures that may be bound as both a render target and a shader resource. */
extern RHI_API bool GSupportsRenderDepthTargetableShaderResources;

/**
* true if the our renderer, the driver and the hardware supports the feature.
*/
extern RHI_API bool GSupportsVertexTextureFetch;

/** true if the RHI supports binding depth as a texture when testing against depth */
extern RHI_API bool GSupportsDepthFetchDuringDepthTest;

/** 
 * only set if RHI has the information (after init of the RHI and only if RHI has that information, never changes after that)
 * e.g. "NVIDIA GeForce GTX 670"
 */
extern RHI_API FString GRHIAdapterName;

// 0 means not defined yet, use functions like IsRHIDeviceAMD() to access
extern RHI_API uint32 GRHIVendorId;

// to trigger GPU specific optimizations and fallbacks
RHI_API bool IsRHIDeviceAMD();

// to trigger GPU specific optimizations and fallbacks
RHI_API bool IsRHIDeviceIntel();

// to trigger GPU specific optimizations and fallbacks
RHI_API bool IsRHIDeviceNVIDIA();

/** true if PF_G8 render targets are supported */
extern RHI_API bool GSupportsRenderTargetFormat_PF_G8;

/** true if PF_FloatRGBA render targets are supported */
extern RHI_API bool GSupportsRenderTargetFormat_PF_FloatRGBA;

/** true if mobile framebuffer fetch is supported */
extern RHI_API bool GSupportsShaderFramebufferFetch;

/** true if the GPU supports hidden surface removal in hardware. */
extern RHI_API bool GHardwareHiddenSurfaceRemoval;

/** true if the RHI supports asynchronous creation of texture resources */
extern RHI_API bool GRHISupportsAsyncTextureCreation;

/** Can we handle quad primitives? */
extern RHI_API bool GSupportsQuads;

/** Used to work around a bug with Nvidia cards on Mac: using GL_LayerIndex to specify render target layer doesn't work when rendering to mips. */
extern RHI_API bool GSupportsGSRenderTargetLayerSwitchingToMips;

/** The offset from the upper left corner of a pixel to the position which is used to sample textures for fragments of that pixel. */
extern RHI_API float GPixelCenterOffset;

/** The minimum Z value in clip space for the RHI. */
extern RHI_API float GMinClipZ;

/** The sign to apply to the Y axis of projection matrices. */
extern RHI_API float GProjectionSignY;

/** The maximum size to allow for the shadow depth buffer in the X dimension.  This must be larger or equal to GMaxShadowDepthBufferSizeY. */
extern RHI_API int32 GMaxShadowDepthBufferSizeX;
/** The maximum size to allow for the shadow depth buffer in the Y dimension. */
extern RHI_API int32 GMaxShadowDepthBufferSizeY;

/** The maximum size allowed for 2D textures in both dimensions. */
extern RHI_API int32 GMaxTextureDimensions;
FORCEINLINE uint32 GetMax2DTextureDimension()
{
	return GMaxTextureDimensions;
}

/** The maximum size allowed for cube textures. */
extern RHI_API int32 GMaxCubeTextureDimensions;
FORCEINLINE uint32 GetMaxCubeTextureDimension()
{
	return GMaxCubeTextureDimensions;
}

/** The Maximum number of layers in a 1D or 2D texture array. */
extern RHI_API int32 GMaxTextureArrayLayers;
FORCEINLINE uint32 GetMaxTextureArrayLayers()
{
	return GMaxTextureArrayLayers;
}

/** true if we are running with the NULL RHI */
extern RHI_API bool GUsingNullRHI;

/**
 *	The size to check against for Draw*UP call vertex counts.
 *	If greater than this value, the draw call will not occur.
 */
extern RHI_API int32 GDrawUPVertexCheckCount;
/**
 *	The size to check against for Draw*UP call index counts.
 *	If greater than this value, the draw call will not occur.
 */
extern RHI_API int32 GDrawUPIndexCheckCount;

/** true if the rendering hardware supports vertex instancing. */
extern RHI_API bool GSupportsVertexInstancing;

/** true if the rendering hardware can emulate vertex instancing. */
extern RHI_API bool GSupportsEmulatedVertexInstancing;

/** If false code needs to patch up vertex declaration. */
extern RHI_API bool GVertexElementsCanShareStreamOffset;

/** true for each VET that is supported. One-to-one mapping with EVertexElementType */
extern RHI_API class FVertexElementTypeSupportInfo GVertexElementTypeSupport;

/** When greater than one, indicates that SLI rendering is enabled */
#if PLATFORM_DESKTOP
#define WITH_SLI (1)
extern RHI_API int32 GNumActiveGPUsForRendering;
#else
#define WITH_SLI (0)
#define GNumActiveGPUsForRendering (1)
#endif

/** Whether the next frame should profile the GPU. */
extern RHI_API bool GTriggerGPUProfile;

/** Whether we are profiling GPU hitches. */
extern RHI_API bool GTriggerGPUHitchProfile;

/** True if the RHI supports texture streaming */
extern RHI_API bool GRHISupportsTextureStreaming;
/** Amount of memory allocated by textures. In bytes. */
extern RHI_API volatile int64 GCurrentTextureMemorySize;
/** Amount of memory allocated by rendertargets. In bytes. */
extern RHI_API volatile int64 GCurrentRendertargetMemorySize;
/** Current texture streaming pool size, in bytes. 0 means unlimited. */
extern RHI_API int64 GTexturePoolSize;
/** Whether to read the texture pool size from engine.ini. Can be turned on with -UseTexturePool on the command line. */
extern RHI_API bool GReadTexturePoolSizeFromIni;
/** In percent. If non-zero, the texture pool size is a percentage of GTotalGraphicsMemory. */
extern RHI_API int32 GPoolSizeVRAMPercentage;

/** Some simple runtime stats, reset on every call to RHIBeginFrame. */
extern RHI_API int32 GNumDrawCallsRHI;
extern RHI_API int32 GNumPrimitivesDrawnRHI;

/** Called once per frame only from within an RHI. */
extern RHI_API void RHIPrivateBeginFrame();

/** @warning: update *LegacyShaderPlatform* when the below changes */
enum EShaderPlatform
{
	SP_PCD3D_SM5		= 0,
	SP_OPENGL_SM4		= 1,
	SP_PS4				= 2,
	/** Used when running in Feature Level ES2 in OpenGL. */
	SP_OPENGL_PCES2		= 3,
	SP_XBOXONE			= 4,
	SP_PCD3D_SM4		= 5,
	SP_OPENGL_SM5		= 6,
	/** Used when running in Feature Level ES2 in D3D11. */
	SP_PCD3D_ES2		= 7,
	SP_OPENGL_ES2		= 8,
	SP_OPENGL_ES2_WEBGL = 9, 
	SP_OPENGL_ES2_IOS	= 10,
	
	SP_NumPlatforms		= 11,
	SP_NumBits			= 4,
};
checkAtCompileTime(SP_NumPlatforms <= (1 << SP_NumBits), SP_NumBits_TooSmall);

enum ERenderQueryType
{
	// e.g. WaitForFrameEventCompletion()
	RQT_Undefined,
	// Result is the number of samples that are not culled (divide by MSAACount to get pixels)
	RQT_Occlusion,
	// Result is time in micro seconds = 1/1000 ms = 1/1000000 sec
	RQT_AbsoluteTime,
};

inline bool IsPCPlatform(const EShaderPlatform Platform)
{
	return Platform == SP_PCD3D_SM5 || Platform == SP_PCD3D_SM4 || Platform == SP_PCD3D_ES2 || Platform ==  SP_OPENGL_SM4 || Platform == SP_OPENGL_SM5 || Platform == SP_OPENGL_PCES2;
}

/** Whether the shader platform corresponds to the ES2 feature level. */
inline bool IsES2Platform(const EShaderPlatform Platform)
{
	return Platform == SP_PCD3D_ES2 || Platform == SP_OPENGL_PCES2 || Platform == SP_OPENGL_ES2 || Platform == SP_OPENGL_ES2_WEBGL || Platform == SP_OPENGL_ES2_IOS; 
}

inline bool IsOpenGLPlatform(const EShaderPlatform Platform)
{
	return Platform == SP_OPENGL_SM4 || Platform == SP_OPENGL_SM5 || Platform == SP_OPENGL_PCES2 || Platform == SP_OPENGL_ES2 || Platform == SP_OPENGL_ES2_WEBGL || Platform == SP_OPENGL_ES2_IOS;
}

inline bool IsConsolePlatform(const EShaderPlatform Platform)
{
	return Platform == SP_PS4 || Platform == SP_XBOXONE;
}

RHI_API FName LegacyShaderPlatformToShaderFormat(EShaderPlatform Platform);
RHI_API EShaderPlatform ShaderFormatToLegacyShaderPlatform(FName ShaderFormat);

/**
 * Adjusts a projection matrix to output in the correct clip space for the
 * current RHI. Unreal projection matrices follow certain conventions and
 * need to be patched for some RHIs. All projection matrices should be adjusted
 * before being used for rendering!
 */
inline FMatrix AdjustProjectionMatrixForRHI(const FMatrix& InProjectionMatrix)
{
	FScaleMatrix ClipSpaceFixScale(FVector(1.0f, GProjectionSignY, 1.0f - GMinClipZ));
	FTranslationMatrix ClipSpaceFixTranslate(FVector(0.0f, 0.0f, GMinClipZ));	
	return InProjectionMatrix * ClipSpaceFixScale * ClipSpaceFixTranslate;
}

/** Current shader platform. */
extern RHI_API EShaderPlatform GRHIShaderPlatform;

/**
 * The RHI's feature level indicates what level of support can be relied upon.
 * Note: these are named after graphics API's like ES2 but a feature level can be used with a different API (eg ERHIFeatureLevel::ES2 on D3D11)
 * As long as the graphics API supports all the features of the feature level (eg no ERHIFeatureLevel::SM5 on OpenGL ES2)
 */
namespace ERHIFeatureLevel
{
	enum Type
	{
		/** Feature level defined by the core capabilities of OpenGL ES2. */
		ES2,
		/** Feature level defined by the capabilities of DX9 Shader Model 3. */
		SM3,
		/** Feature level defined by the capabilities of DX10 Shader Model 4. */
		SM4,
		/** Feature level defined by the capabilities of DX11 Shader Model 5. */
		SM5,
		Num
	};
};

/** Finds a corresponding ERHIFeatureLevel::Type given an FName, or returns false if one could not be found. */
extern RHI_API bool GetFeatureLevelFromName(FName Name, ERHIFeatureLevel::Type& OutFeatureLevel);

/** Creates a string for the given feature level. */
extern RHI_API void GetFeatureLevelName(ERHIFeatureLevel::Type InFeatureLevel, FString& OutName);

/** Creates an FName for the given feature level. */
extern RHI_API void GetFeatureLevelName(ERHIFeatureLevel::Type InFeatureLevel, FName& OutName);

/** The current feature level supported by the RHI. */
extern RHI_API ERHIFeatureLevel::Type GRHIFeatureLevel;

/** Table for finding out which shader platform corresponds to a given feature level for this RHI. */
extern RHI_API EShaderPlatform GShaderPlatformForFeatureLevel[ERHIFeatureLevel::Num];

inline ERHIFeatureLevel::Type GetMaxSupportedFeatureLevel(EShaderPlatform InShaderPlatform)
{
	switch(InShaderPlatform)
	{
	case SP_PCD3D_SM5:
	case SP_OPENGL_SM5:
	case SP_PS4:
	case SP_XBOXONE:
		return ERHIFeatureLevel::SM5;
	case SP_PCD3D_SM4:
	case SP_OPENGL_SM4:
		return ERHIFeatureLevel::SM4;
	case SP_PCD3D_ES2:
	case SP_OPENGL_PCES2:
	case SP_OPENGL_ES2:
	case SP_OPENGL_ES2_WEBGL:
	case  SP_OPENGL_ES2_IOS:
		return ERHIFeatureLevel::ES2;
	default:
		check(0);
		return ERHIFeatureLevel::Num;
	}	
}

/** Returns true if the feature level is supported by the shader platform. */
inline bool IsFeatureLevelSupported(EShaderPlatform InShaderPlatform, ERHIFeatureLevel::Type InFeatureLevel)
{
	switch(InShaderPlatform)
	{
	case SP_PCD3D_SM5:
		return InFeatureLevel <= ERHIFeatureLevel::SM5;
	case SP_PCD3D_SM4:
		return InFeatureLevel <= ERHIFeatureLevel::SM4;
	case SP_PCD3D_ES2:
		return InFeatureLevel <= ERHIFeatureLevel::ES2;
	case SP_OPENGL_PCES2:
		return InFeatureLevel <= ERHIFeatureLevel::ES2;
	case SP_OPENGL_ES2:
		return InFeatureLevel <= ERHIFeatureLevel::ES2;
	case SP_OPENGL_ES2_WEBGL: 
		return InFeatureLevel <= ERHIFeatureLevel::ES2;
	case SP_OPENGL_ES2_IOS: 
		return InFeatureLevel <= ERHIFeatureLevel::ES2;
	case SP_OPENGL_SM4:
		return InFeatureLevel <= ERHIFeatureLevel::SM4;
	case SP_OPENGL_SM5:
		return InFeatureLevel <= ERHIFeatureLevel::SM5;
	case SP_PS4:
		return InFeatureLevel <= ERHIFeatureLevel::SM5;
	case SP_XBOXONE:
		return InFeatureLevel <= ERHIFeatureLevel::SM5;
	default:
		return false;
	}	
}

inline bool RHISupportsTessellation(const EShaderPlatform Platform)
{
	if (IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5))
	{
		return ((Platform == SP_PCD3D_SM5) || (Platform == SP_XBOXONE));
	}
	return false;
}

inline uint32 GetFeatureLevelMaxTextureSamplers(ERHIFeatureLevel::Type FeatureLevel)
{
	if (FeatureLevel == ERHIFeatureLevel::ES2)
	{
		return 8;
	}
	else
	{
		return 16;
	}
}

inline int32 GetFeatureLevelMaxNumberOfBones(ERHIFeatureLevel::Type FeatureLevel)
{
	switch (FeatureLevel)
	{
	case ERHIFeatureLevel::ES2:
	case ERHIFeatureLevel::SM3:
		return 75;
	case ERHIFeatureLevel::SM4:
	case ERHIFeatureLevel::SM5:
		return 256;
	default:
		check(0);
	}

	return 0;
}

//
// Common RHI definitions.
//

enum ESamplerFilter
{
	SF_Point,
	SF_Bilinear,
	SF_Trilinear,
	SF_AnisotropicPoint,
	SF_AnisotropicLinear,
};

enum ESamplerAddressMode
{
	AM_Wrap,
	AM_Clamp,
	AM_Mirror,
	/** Not supported on all platforms */
	AM_Border
};

enum ESamplerCompareFunction
{
	SCF_Never,
	SCF_Less
};

enum ERasterizerFillMode
{
	FM_Point,
	FM_Wireframe,
	FM_Solid
};

enum ERasterizerCullMode
{
	CM_None,
	CM_CW,
	CM_CCW
};

enum EColorWriteMask
{
	CW_RED   = 0x01,
	CW_GREEN = 0x02,
	CW_BLUE  = 0x04,
	CW_ALPHA = 0x08,

	CW_NONE  = 0,
	CW_RGB   = CW_RED | CW_GREEN | CW_BLUE,
	CW_RGBA  = CW_RED | CW_GREEN | CW_BLUE | CW_ALPHA,
	CW_RG    = CW_RED | CW_GREEN,
	CW_BA    = CW_BLUE | CW_ALPHA,
};

enum ECompareFunction
{
	CF_Less,
	CF_LessEqual,
	CF_Greater,
	CF_GreaterEqual,
	CF_Equal,
	CF_NotEqual,
	CF_Never,
	CF_Always
};

enum EStencilOp
{
	SO_Keep,
	SO_Zero,
	SO_Replace,
	SO_SaturatedIncrement,
	SO_SaturatedDecrement,
	SO_Invert,
	SO_Increment,
	SO_Decrement
};

enum EBlendOperation
{
	BO_Add,
	BO_Subtract,
	BO_Min,
	BO_Max,
    BO_ReverseSubtract,
};

enum EBlendFactor
{
	BF_Zero,
	BF_One,
	BF_SourceColor,
	BF_InverseSourceColor,
	BF_SourceAlpha,
	BF_InverseSourceAlpha,
	BF_DestAlpha,
	BF_InverseDestAlpha,
	BF_DestColor,
	BF_InverseDestColor,
	BF_ConstantBlendFactor,
	BF_InverseConstantBlendFactor
};

enum EVertexElementType
{
	VET_None,
	VET_Float1,
	VET_Float2,
	VET_Float3,
	VET_Float4,
	VET_PackedNormal,	// FPackedNormal
	VET_UByte4,
	VET_UByte4N,
	VET_Color,
	VET_Short2,
	VET_Short4,
	VET_Short2N,		// 16 bit word normalized to (value/32767.0,value/32767.0,0,0,1)
	VET_Half2,			// 16 bit float using 1 bit sign, 5 bit exponent, 10 bit mantissa 
	VET_MAX
};

enum ECubeFace
{
	CubeFace_PosX=0,
	CubeFace_NegX,
	CubeFace_PosY,
	CubeFace_NegY,
	CubeFace_PosZ,
	CubeFace_NegZ,
	CubeFace_MAX
};

enum EUniformBufferUsage
{
	// the uniform buffer is temporary, used for a single draw call then discarded
	UniformBuffer_SingleUse = 0,
	// the uniform buffer is used for multiple draw calls, possibly across multiple frames
	UniformBuffer_MultiUse,
};

enum EResourceLockMode
{
	RLM_ReadOnly,
	RLM_WriteOnly,
	RLM_Num
};

inline FArchive& operator <<(FArchive& Ar, EResourceLockMode& LockMode)
{
	uint32 Temp = LockMode;
	Ar << Temp;
	LockMode = (EResourceLockMode)Temp;
	return Ar;
}

/** limited to 8 types in FReadSurfaceDataFlags */
enum ERangeCompressionMode
{
	// 0 .. 1
	RCM_UNorm,
	// -1 .. 1
	RCM_SNorm,
	// 0 .. 1 unless there are smaller values than 0 or bigger values than 1, then the range is extended to the minimum or the maximum of the values
	RCM_MinMaxNorm,
	// minimum .. maximum (each channel independent)
	RCM_MinMax,
};

/** to customize the RHIReadSurfaceData() output */
class FReadSurfaceDataFlags
{
public:
	// @param InCompressionMode defines the value input range that is mapped to output range
	// @param InCubeFace defined which cubemap side is used, only required for cubemap content, then it needs to be a valid side
	FReadSurfaceDataFlags(ERangeCompressionMode InCompressionMode = RCM_UNorm, ECubeFace InCubeFace = CubeFace_MAX) 
		:CubeFace(InCubeFace), CompressionMode(InCompressionMode), bLinearToGamma(true), MaxDepthRange(16000.0f), bOutputStencil(false), MipLevel(0)
	{
	}

	ECubeFace GetCubeFace() const
	{
		checkSlow(CubeFace <= CubeFace_NegZ);
		return CubeFace;
	}

	ERangeCompressionMode GetCompressionMode() const
	{
		return CompressionMode;
	}

	void SetLinearToGamma(bool Value)
	{
		bLinearToGamma = Value;
	}

	bool GetLinearToGamma() const
	{
		return bLinearToGamma;
	}

	void SetOutputStencil(bool Value)
	{
		bOutputStencil = Value;
	}

	bool GetOutputStencil() const
	{
		return bOutputStencil;
	}

	void SetMip(uint8 InMipLevel)
	{
		MipLevel = InMipLevel;
	}

	uint8 GetMip() const
	{
		return MipLevel;
	}	

	void SetMaxDepthRange(float Value)
	{
		MaxDepthRange = Value;
	}

	float ComputeNormalizedDepth(float DeviceZ) const
	{
		return abs(ConvertFromDeviceZ(DeviceZ) / MaxDepthRange);
	}

private:

	// @return SceneDepth
	float ConvertFromDeviceZ(float DeviceZ) const
	{
		DeviceZ = FMath::Min(DeviceZ, 1 - Z_PRECISION);

		// for depth to linear conversion
		const FVector2D InvDeviceZToWorldZ(0.1f, 0.1f);

		return 1.0f / (DeviceZ * InvDeviceZToWorldZ.X - InvDeviceZToWorldZ.Y);
	}

	ECubeFace CubeFace;
	ERangeCompressionMode CompressionMode;
	bool bLinearToGamma;	
	float MaxDepthRange;
	bool bOutputStencil;
	uint8 MipLevel;
};

/** Info for supporting the vertex element types */
class FVertexElementTypeSupportInfo
{
public:
	FVertexElementTypeSupportInfo() { for(int32 i=0; i<VET_MAX; i++) ElementCaps[i]=true; }
	FORCEINLINE bool IsSupported(EVertexElementType ElementType) { return ElementCaps[ElementType]; }
	FORCEINLINE void SetSupported(EVertexElementType ElementType,bool bIsSupported) { ElementCaps[ElementType]=bIsSupported; }
private:
	/** cap bit set for each VET. One-to-one mapping based on EVertexElementType */
	bool ElementCaps[VET_MAX];
};

struct FVertexElement
{
	uint8 StreamIndex;
	uint8 Offset;
	TEnumAsByte<EVertexElementType> Type;
	uint8 AttributeIndex;
	bool bUseInstanceIndex;

	FVertexElement() {}
	FVertexElement(uint8 InStreamIndex,uint8 InOffset,EVertexElementType InType,uint8 InAttributeIndex,bool bInUseInstanceIndex = false):
		StreamIndex(InStreamIndex),
		Offset(InOffset),
		Type(InType),
		AttributeIndex(InAttributeIndex),
		bUseInstanceIndex(bInUseInstanceIndex)
	{}
	/**
	* Suppress the compiler generated assignment operator so that padding won't be copied.
	* This is necessary to get expected results for code that zeros, assigns and then CRC's the whole struct.
	*/
	void operator=(const FVertexElement& Other)
	{
		StreamIndex = Other.StreamIndex;
		Offset = Other.Offset;
		Type = Other.Type;
		AttributeIndex = Other.AttributeIndex;
		bUseInstanceIndex = Other.bUseInstanceIndex;
	}

	friend FArchive& operator<<(FArchive& Ar,FVertexElement& Element)
	{
		Ar << Element.StreamIndex;
		Ar << Element.Offset;
		Ar << Element.Type;
		Ar << Element.AttributeIndex;
		Ar << Element.bUseInstanceIndex;
		return Ar;
	}
};

typedef TArray<FVertexElement,TFixedAllocator<MaxVertexElementCount> > FVertexDeclarationElementList;

/** RHI representation of a single stream out element. */
struct FStreamOutElement
{
	/** Index of the output stream from the geometry shader. */
	uint32 Stream;

	/** Semantic name of the output element as defined in the geometry shader.  This should not contain the semantic number. */
	const ANSICHAR* SemanticName;

	/** Semantic index of the output element as defined in the geometry shader.  For example "TEXCOORD5" in the shader would give a SemanticIndex of 5. */
	uint32 SemanticIndex;

	/** Start component index of the shader output element to stream out. */
	uint8 StartComponent;

	/** Number of components of the shader output element to stream out. */
	uint8 ComponentCount;

	/** Stream output target slot, corresponding to the streams set by RHISetStreamOutTargets. */
	uint8 OutputSlot;

	FStreamOutElement() {}
	FStreamOutElement(uint32 InStream, const ANSICHAR* InSemanticName, uint32 InSemanticIndex, uint8 InComponentCount, uint8 InOutputSlot) :
		Stream(InStream),
		SemanticName(InSemanticName),
		SemanticIndex(InSemanticIndex),
		StartComponent(0),
		ComponentCount(InComponentCount),
		OutputSlot(InOutputSlot)
	{}
};

typedef TArray<FStreamOutElement,TFixedAllocator<MaxVertexElementCount> > FStreamOutElementList;

struct FSamplerStateInitializerRHI
{
	FSamplerStateInitializerRHI() {}
	FSamplerStateInitializerRHI(
		ESamplerFilter InFilter,
		ESamplerAddressMode InAddressU = AM_Wrap,
		ESamplerAddressMode InAddressV = AM_Wrap,
		ESamplerAddressMode InAddressW = AM_Wrap,
		int32 InMipBias = 0,
		int32 InMaxAnisotropy = 0,
		float InMinMipLevel = 0,
		float InMaxMipLevel = FLT_MAX,
		uint32 InBorderColor = 0,
		/** Only supported in D3D11 */
		ESamplerCompareFunction InSamplerComparisonFunction = SCF_Never
		)
	:	Filter(InFilter)
	,	AddressU(InAddressU)
	,	AddressV(InAddressV)
	,	AddressW(InAddressW)
	,	MipBias(InMipBias)
	,	MinMipLevel(InMinMipLevel)
	,	MaxMipLevel(InMaxMipLevel)
	,	MaxAnisotropy(InMaxAnisotropy)
	,	BorderColor(InBorderColor)
	,	SamplerComparisonFunction(InSamplerComparisonFunction)
	{
	}
	TEnumAsByte<ESamplerFilter> Filter;
	TEnumAsByte<ESamplerAddressMode> AddressU;
	TEnumAsByte<ESamplerAddressMode> AddressV;
	TEnumAsByte<ESamplerAddressMode> AddressW;
	int32 MipBias;
	/** Smallest mip map level that will be used, where 0 is the highest resolution mip level. */
	float MinMipLevel;
	/** Largest mip map level that will be used, where 0 is the highest resolution mip level. */
	float MaxMipLevel;
	int32 MaxAnisotropy;
	uint32 BorderColor;
	TEnumAsByte<ESamplerCompareFunction> SamplerComparisonFunction;

	friend FArchive& operator<<(FArchive& Ar,FSamplerStateInitializerRHI& SamplerStateInitializer)
	{
		Ar << SamplerStateInitializer.Filter;
		Ar << SamplerStateInitializer.AddressU;
		Ar << SamplerStateInitializer.AddressV;
		Ar << SamplerStateInitializer.AddressW;
		Ar << SamplerStateInitializer.MipBias;
		Ar << SamplerStateInitializer.MinMipLevel;
		Ar << SamplerStateInitializer.MaxMipLevel;
		Ar << SamplerStateInitializer.MaxAnisotropy;
		Ar << SamplerStateInitializer.BorderColor;
		Ar << SamplerStateInitializer.SamplerComparisonFunction;
		return Ar;
	}
};
struct FRasterizerStateInitializerRHI
{
	TEnumAsByte<ERasterizerFillMode> FillMode;
	TEnumAsByte<ERasterizerCullMode> CullMode;
	float DepthBias;
	float SlopeScaleDepthBias;
	bool bAllowMSAA;
	bool bEnableLineAA;
	
	friend FArchive& operator<<(FArchive& Ar,FRasterizerStateInitializerRHI& RasterizerStateInitializer)
	{
		Ar << RasterizerStateInitializer.FillMode;
		Ar << RasterizerStateInitializer.CullMode;
		Ar << RasterizerStateInitializer.DepthBias;
		Ar << RasterizerStateInitializer.SlopeScaleDepthBias;
		Ar << RasterizerStateInitializer.bAllowMSAA;
		Ar << RasterizerStateInitializer.bEnableLineAA;
		return Ar;
	}
};
struct FDepthStencilStateInitializerRHI
{
	bool bEnableDepthWrite;
	TEnumAsByte<ECompareFunction> DepthTest;

	bool bEnableFrontFaceStencil;
	TEnumAsByte<ECompareFunction> FrontFaceStencilTest;
	TEnumAsByte<EStencilOp> FrontFaceStencilFailStencilOp;
	TEnumAsByte<EStencilOp> FrontFaceDepthFailStencilOp;
	TEnumAsByte<EStencilOp> FrontFacePassStencilOp;
	bool bEnableBackFaceStencil;
	TEnumAsByte<ECompareFunction> BackFaceStencilTest;
	TEnumAsByte<EStencilOp> BackFaceStencilFailStencilOp;
	TEnumAsByte<EStencilOp> BackFaceDepthFailStencilOp;
	TEnumAsByte<EStencilOp> BackFacePassStencilOp;
	uint8 StencilReadMask;
	uint8 StencilWriteMask;

	FDepthStencilStateInitializerRHI(
		bool bInEnableDepthWrite = true,
		ECompareFunction InDepthTest = CF_LessEqual,
		bool bInEnableFrontFaceStencil = false,
		ECompareFunction InFrontFaceStencilTest = CF_Always,
		EStencilOp InFrontFaceStencilFailStencilOp = SO_Keep,
		EStencilOp InFrontFaceDepthFailStencilOp = SO_Keep,
		EStencilOp InFrontFacePassStencilOp = SO_Keep,
		bool bInEnableBackFaceStencil = false,
		ECompareFunction InBackFaceStencilTest = CF_Always,
		EStencilOp InBackFaceStencilFailStencilOp = SO_Keep,
		EStencilOp InBackFaceDepthFailStencilOp = SO_Keep,
		EStencilOp InBackFacePassStencilOp = SO_Keep,
		uint8 InStencilReadMask = 0xFF,
		uint8 InStencilWriteMask = 0xFF
		)
	: bEnableDepthWrite(bInEnableDepthWrite)
	, DepthTest(InDepthTest)
	, bEnableFrontFaceStencil(bInEnableFrontFaceStencil)
	, FrontFaceStencilTest(InFrontFaceStencilTest)
	, FrontFaceStencilFailStencilOp(InFrontFaceStencilFailStencilOp)
	, FrontFaceDepthFailStencilOp(InFrontFaceDepthFailStencilOp)
	, FrontFacePassStencilOp(InFrontFacePassStencilOp)
	, bEnableBackFaceStencil(bInEnableBackFaceStencil)
	, BackFaceStencilTest(InBackFaceStencilTest)
	, BackFaceStencilFailStencilOp(InBackFaceStencilFailStencilOp)
	, BackFaceDepthFailStencilOp(InBackFaceDepthFailStencilOp)
	, BackFacePassStencilOp(InBackFacePassStencilOp)
	, StencilReadMask(InStencilReadMask)
	, StencilWriteMask(InStencilWriteMask)
	{}
	
	friend FArchive& operator<<(FArchive& Ar,FDepthStencilStateInitializerRHI& DepthStencilStateInitializer)
	{
		Ar << DepthStencilStateInitializer.bEnableDepthWrite;
		Ar << DepthStencilStateInitializer.DepthTest;
		Ar << DepthStencilStateInitializer.bEnableFrontFaceStencil;
		Ar << DepthStencilStateInitializer.FrontFaceStencilTest;
		Ar << DepthStencilStateInitializer.FrontFaceStencilFailStencilOp;
		Ar << DepthStencilStateInitializer.FrontFaceDepthFailStencilOp;
		Ar << DepthStencilStateInitializer.FrontFacePassStencilOp;
		Ar << DepthStencilStateInitializer.bEnableBackFaceStencil;
		Ar << DepthStencilStateInitializer.BackFaceStencilTest;
		Ar << DepthStencilStateInitializer.BackFaceStencilFailStencilOp;
		Ar << DepthStencilStateInitializer.BackFaceDepthFailStencilOp;
		Ar << DepthStencilStateInitializer.BackFacePassStencilOp;
		Ar << DepthStencilStateInitializer.StencilReadMask;
		Ar << DepthStencilStateInitializer.StencilWriteMask;
		return Ar;
	}
};
class FBlendStateInitializerRHI
{
public:

	struct FRenderTarget
	{
		TEnumAsByte<EBlendOperation> ColorBlendOp;
		TEnumAsByte<EBlendFactor> ColorSrcBlend;
		TEnumAsByte<EBlendFactor> ColorDestBlend;
		TEnumAsByte<EBlendOperation> AlphaBlendOp;
		TEnumAsByte<EBlendFactor> AlphaSrcBlend;
		TEnumAsByte<EBlendFactor> AlphaDestBlend;
		TEnumAsByte<EColorWriteMask> ColorWriteMask;
		
		FRenderTarget(
			EBlendOperation InColorBlendOp = BO_Add,
			EBlendFactor InColorSrcBlend = BF_One,
			EBlendFactor InColorDestBlend = BF_Zero,
			EBlendOperation InAlphaBlendOp = BO_Add,
			EBlendFactor InAlphaSrcBlend = BF_One,
			EBlendFactor InAlphaDestBlend = BF_Zero,
			EColorWriteMask InColorWriteMask = CW_RGBA
			)
		: ColorBlendOp(InColorBlendOp)
		, ColorSrcBlend(InColorSrcBlend)
		, ColorDestBlend(InColorDestBlend)
		, AlphaBlendOp(InAlphaBlendOp)
		, AlphaSrcBlend(InAlphaSrcBlend)
		, AlphaDestBlend(InAlphaDestBlend)
		, ColorWriteMask(InColorWriteMask)
		{}
		
		friend FArchive& operator<<(FArchive& Ar,FRenderTarget& RenderTarget)
		{
			Ar << RenderTarget.ColorBlendOp;
			Ar << RenderTarget.ColorSrcBlend;
			Ar << RenderTarget.ColorDestBlend;
			Ar << RenderTarget.AlphaBlendOp;
			Ar << RenderTarget.AlphaSrcBlend;
			Ar << RenderTarget.AlphaDestBlend;
			Ar << RenderTarget.ColorWriteMask;
			return Ar;
		}
	};

	FBlendStateInitializerRHI() {}

	FBlendStateInitializerRHI(const FRenderTarget& InRenderTargetBlendState)
	:	bUseIndependentRenderTargetBlendStates(false)
	{
		RenderTargets[0] = InRenderTargetBlendState;
	}

	template<uint32 NumRenderTargets>
	FBlendStateInitializerRHI(const TStaticArray<FRenderTarget,NumRenderTargets>& InRenderTargetBlendStates)
	:	bUseIndependentRenderTargetBlendStates(NumRenderTargets > 1)
	{
		checkAtCompileTime(NumRenderTargets <= MaxSimultaneousRenderTargets,TooManyRenderTargetBlendStates);

		for(uint32 RenderTargetIndex = 0;RenderTargetIndex < NumRenderTargets;++RenderTargetIndex)
		{
			RenderTargets[RenderTargetIndex] = InRenderTargetBlendStates[RenderTargetIndex];
		}
	}

	TStaticArray<FRenderTarget,MaxSimultaneousRenderTargets> RenderTargets;
	bool bUseIndependentRenderTargetBlendStates;
	
	friend FArchive& operator<<(FArchive& Ar,FBlendStateInitializerRHI& BlendStateInitializer)
	{
		Ar << BlendStateInitializer.RenderTargets;
		Ar << BlendStateInitializer.bUseIndependentRenderTargetBlendStates;
		return Ar;
	}
};

enum EPrimitiveType
{
	PT_TriangleList,
	PT_TriangleStrip,
	PT_LineList,
	PT_QuadList,
	PT_PointList,
	PT_1_ControlPointPatchList,
	PT_2_ControlPointPatchList,
	PT_3_ControlPointPatchList,
	PT_4_ControlPointPatchList,
	PT_5_ControlPointPatchList,
	PT_6_ControlPointPatchList,
	PT_7_ControlPointPatchList,
	PT_8_ControlPointPatchList,
	PT_9_ControlPointPatchList,
	PT_10_ControlPointPatchList,
	PT_11_ControlPointPatchList,
	PT_12_ControlPointPatchList,
	PT_13_ControlPointPatchList,
	PT_14_ControlPointPatchList,
	PT_15_ControlPointPatchList,
	PT_16_ControlPointPatchList,
	PT_17_ControlPointPatchList,
	PT_18_ControlPointPatchList,
	PT_19_ControlPointPatchList,
	PT_20_ControlPointPatchList,
	PT_21_ControlPointPatchList,
	PT_22_ControlPointPatchList,
	PT_23_ControlPointPatchList,
	PT_24_ControlPointPatchList,
	PT_25_ControlPointPatchList,
	PT_26_ControlPointPatchList,
	PT_27_ControlPointPatchList,
	PT_28_ControlPointPatchList,
	PT_29_ControlPointPatchList,
	PT_30_ControlPointPatchList,
	PT_31_ControlPointPatchList,
	PT_32_ControlPointPatchList,
	PT_Num,
	PT_NumBits = 6
};

checkAtCompileTime( PT_Num <= (1 << 8), EPrimitiveType_DoesntFitInAByte );
checkAtCompileTime( PT_Num <= (1 << PT_NumBits), PT_NumBits_TooSmall );

/**
 *	Resource usage flags - for vertex and index buffers.
 */
enum EBufferUsageFlags
{
	// Mutually exclusive write-frequency flags
	BUF_Static            = 0x0001, // The buffer will be written to once.
	BUF_Dynamic           = 0x0002, // The buffer will be written to occasionally.
	BUF_Volatile          = 0x0004, // The buffer will be written to frequently.

	// Mutually exclusive bind flags.
	BUF_UnorderedAccess   = 0x0008, // Allows an unordered access view to be created for the buffer.

	/** Create a byte address buffer, which is basically a structured buffer with a uint32 type. */
	BUF_ByteAddressBuffer = 0x0020,
	/** Create a structured buffer with an atomic UAV counter. */
	BUF_UAVCounter        = 0x0040,
	/** Create a buffer that can be bound as a stream output target. */
	BUF_StreamOutput      = 0x0080,
	/** Create a buffer which contains the arguments used by DispatchIndirect or DrawIndirect. */
	BUF_DrawIndirect      = 0x0100,
	/** 
	 * Create a buffer that can be bound as a shader resource. 
	 * This is only needed for buffer types which wouldn't ordinarily be used as a shader resource, like a vertex buffer.
	 */
	BUF_ShaderResource    = 0x0200,

	/**
	 * Request that this buffer is directly CPU accessible
	 * (@todo josh: this is probably temporary and will go away in a few months)
	 */
	BUF_KeepCPUAccessible = 0x0400,

	/**
	 * Provide information that this buffer will contain only one vertex, which should be delivered to every primitive drawn.
	 * This is necessary for OpenGL implementations, which need to handle this case very differently (and can't handle GL_HALF_FLOAT in such vertices at all).
	 */
	BUF_ZeroStride        = 0x0800,

	/** Buffer should go in fast vram (hint only) */
	BUF_FastVRAM          = 0x1000,

	// Helper bit-masks
	BUF_AnyDynamic      = (BUF_Dynamic|BUF_Volatile),
};

/**
 *	Screen Resolution
 */
struct FScreenResolutionRHI
{
	uint32	Width;
	uint32	Height;
	uint32	RefreshRate;
};

/**
 *	Viewport bounds structure to set multiple view ports for the geometry shader
 *  (needs to be 1:1 to the D3D11 structure)
 */
struct FViewportBounds
{
	float	TopLeftX;
	float	TopLeftY;
	float	Width;
	float	Height;
	float	MinDepth;
	float	MaxDepth;

	FViewportBounds() {}

	FViewportBounds(float InTopLeftX, float InTopLeftY, float InWidth, float InHeight, float InMinDepth = 0.0f, float InMaxDepth = 1.0f)
		:TopLeftX(InTopLeftX), TopLeftY(InTopLeftY), Width(InWidth), Height(InHeight), MinDepth(InMinDepth), MaxDepth(InMaxDepth)
	{
	}

	friend FArchive& operator<<(FArchive& Ar,FViewportBounds& ViewportBounds)
	{
		Ar << ViewportBounds.TopLeftX;
		Ar << ViewportBounds.TopLeftY;
		Ar << ViewportBounds.Width;
		Ar << ViewportBounds.Height;
		Ar << ViewportBounds.MinDepth;
		Ar << ViewportBounds.MaxDepth;
		return Ar;
	}
};


typedef TArray<FScreenResolutionRHI>	FScreenResolutionArray;

#define ENUM_RHI_RESOURCE_TYPES(EnumerationMacro) \
	EnumerationMacro(SamplerState,None) \
	EnumerationMacro(RasterizerState,None) \
	EnumerationMacro(DepthStencilState,None) \
	EnumerationMacro(BlendState,None) \
	EnumerationMacro(VertexDeclaration,None) \
	EnumerationMacro(VertexShader,None) \
	EnumerationMacro(HullShader,None) \
	EnumerationMacro(DomainShader,None) \
	EnumerationMacro(PixelShader,None) \
	EnumerationMacro(GeometryShader,None) \
	EnumerationMacro(ComputeShader,None) \
	EnumerationMacro(BoundShaderState,None) \
	EnumerationMacro(UniformBuffer,None) \
	EnumerationMacro(IndexBuffer,None) \
	EnumerationMacro(VertexBuffer,None) \
	EnumerationMacro(StructuredBuffer,None) \
	EnumerationMacro(Texture,None) \
	EnumerationMacro(Texture2D,Texture) \
	EnumerationMacro(Texture2DArray,Texture) \
	EnumerationMacro(Texture3D,Texture) \
	EnumerationMacro(TextureCube,Texture) \
	EnumerationMacro(RenderQuery,None) \
	EnumerationMacro(Viewport,None) \
	EnumerationMacro(UnorderedAccessView,None) \
	EnumerationMacro(ShaderResourceView,None)

/** An enumeration of the different RHI reference types. */
enum ERHIResourceType
{
	RRT_None,

#define DECLARE_RESOURCETYPE_ENUM(Type,ParentType) RRT_##Type,
	ENUM_RHI_RESOURCE_TYPES(DECLARE_RESOURCETYPE_ENUM)
#undef DECLARE_RESOURCETYPE_ENUM

	RRT_Num
};

/** Flags used for texture creation */
enum ETextureCreateFlags
{
	TexCreate_None					= 0,

	// Texture can be used as a render target
	TexCreate_RenderTargetable		= 1<<0,
	// Texture can be used as a resolve target
	TexCreate_ResolveTargetable		= 1<<1,
	// Texture can be used as a depth-stencil target.
	TexCreate_DepthStencilTargetable= 1<<2,
	// Texture can be used as a shader resource.
	TexCreate_ShaderResource		= 1<<3,
	
	// Texture is encoded in sRGB gamma space
	TexCreate_SRGB					= 1<<4,
	// Texture will be created without a packed miptail
	TexCreate_NoMipTail				= 1<<5,
	// Texture will be created with an un-tiled format
	TexCreate_NoTiling				= 1<<6,
	// Texture that may be updated every frame
	TexCreate_Dynamic				= 1<<8,
	// Allow silent texture creation failure
	// @warning:	When you update this, you must update FTextureAllocations::FindTextureType() in Core/Private/UObject/TextureAllocations.cpp
	TexCreate_AllowFailure			= 1<<9,
	// Disable automatic defragmentation if the initial texture memory allocation fails.
	// @warning:	When you update this, you must update FTextureAllocations::FindTextureType() in Core/Private/UObject/TextureAllocations.cpp
	TexCreate_DisableAutoDefrag		= 1<<10,
	// Create the texture with automatic -1..1 biasing
	TexCreate_BiasNormalMap			= 1<<11,
	// Create the texture with the flag that allows mip generation later, only applicable to D3D11
	TexCreate_GenerateMipCapable	= 1<<12,
	// UnorderedAccessView (DX11 only)
	// Warning: Causes additional synchronization between draw calls when using a render target allocated with this flag, use sparingly
	// See: GCNPerformanceTweets.pdf Tip 37
	TexCreate_UAV					= 1<<16,
	// Render target texture that will be displayed on screen (back buffer)
	TexCreate_Presentable			= 1<<17,
	// Texture data is accessible by the CPU
	TexCreate_CPUReadback			= 1<<18,
	// Texture was processed offline (via a texture conversion process for the current platform)
	TexCreate_OfflineProcessed		= 1<<19,
	// Texture needs to go in fast VRAM if available (HINT only)
	TexCreate_FastVRAM				= 1<<20,
	// by default the texture is not showing up in the list - this is to reduce clutter, using the FULL option this can be ignored
	TexCreate_HideInVisualizeTexture= 1<<21,
	// Texture should be created in virtual memory, with no physical memory allocation made
	// You must make further calls to RHIVirtualTextureSetFirstMipInMemory to allocate physical memory
	// and RHIVirtualTextureSetFirstMipVisible to map the first mip visible to the GPU
	TexCreate_Virtual				= 1<<22,
	// Creates a RenderTargetView for each array slice of the texture
	// Warning: if this was specified when the resource was created, you can't use SV_RenderTargetArrayIndex to route to other slices!
	TexCreate_TargetArraySlicesIndependently	= 1<<23,
};

// Forward-declaration.
struct FResolveParams;

/**
 * Async texture reallocation status, returned by RHIGetReallocateTexture2DStatus().
 */
enum ETextureReallocationStatus
{
	TexRealloc_Succeeded = 0,
	TexRealloc_Failed,
	TexRealloc_InProgress,
};

struct FResolveRect
{
	int32 X1;
	int32 Y1;
	int32 X2;
	int32 Y2;
	// e.g. for a a full 256 x 256 area starting at (0, 0) it would be 
	// the values would be 0, 0, 256, 256
	FResolveRect(int32 InX1=-1, int32 InY1=-1, int32 InX2=-1, int32 InY2=-1)
	:	X1(InX1)
	,	Y1(InY1)
	,	X2(InX2)
	,	Y2(InY2)
	{}

	bool IsValid() const
	{
		return X1 >= 0 && Y1 >= 0 && X2 - X1 > 0 && Y2 - Y1 > 0;
	}

	friend FArchive& operator<<(FArchive& Ar,FResolveRect& ResolveRect)
	{
		Ar << ResolveRect.X1;
		Ar << ResolveRect.Y1;
		Ar << ResolveRect.X2;
		Ar << ResolveRect.Y2;
		return Ar;
	}
};

struct FResolveParams
{
	/** used to specify face when resolving to a cube map texture */
	ECubeFace CubeFace;
	/** resolve RECT bounded by [X1,Y1]..[X2,Y2]. Or -1 for fullscreen */
	FResolveRect Rect;
	/** The mip index to resolve in both source and dest. */
	int32 MipIndex;
	/** Array index to resolve in the source. */
	int32 SourceArrayIndex;
	/** Array index to resolve in the dest. */
	int32 DestArrayIndex;

	/** constructor */
	FResolveParams(
		const FResolveRect& InRect = FResolveRect(), 
		ECubeFace InCubeFace = CubeFace_PosX,
		int32 InMipIndex = 0,
		int32 InSourceArrayIndex = 0,
		int32 InDestArrayIndex = 0)
		:	CubeFace(InCubeFace)
		,	Rect(InRect)
		,	MipIndex(InMipIndex)
		,	SourceArrayIndex(InSourceArrayIndex)
		,	DestArrayIndex(InDestArrayIndex)
	{}

	friend FArchive& operator<<(FArchive& Ar,FResolveParams& ResolveParams)
	{
		uint8 CubeFaceInt = ResolveParams.CubeFace;
		Ar << CubeFaceInt;
		if(Ar.IsLoading())
		{
			ResolveParams.CubeFace = (ECubeFace)ResolveParams.CubeFace;
		}
		Ar << ResolveParams.Rect;
		Ar << ResolveParams.MipIndex;
		Ar << ResolveParams.SourceArrayIndex;
		Ar << ResolveParams.DestArrayIndex;
		return Ar;
	}
};

/** specifies an update region for a texture */
struct FUpdateTextureRegion2D
{
	/** offset in texture */
	int32 DestX;
	int32 DestY;
	
	/** offset in source image data */
	int32 SrcX;
	int32 SrcY;
	
	/** size of region to copy */
	int32 Width;
	int32 Height;

	FUpdateTextureRegion2D()
	{}

	FUpdateTextureRegion2D(int32 InDestX, int32 InDestY, int32 InSrcX, int32 InSrcY, int32 InWidth, int32 InHeight)
	:	DestX(InDestX)
	,	DestY(InDestY)
	,	SrcX(InSrcX)
	,	SrcY(InSrcY)
	,	Width(InWidth)
	,	Height(InHeight)
	{}

	friend FArchive& operator<<(FArchive& Ar,FUpdateTextureRegion2D& Region)
	{
		Ar << Region.DestX;
		Ar << Region.DestY;
		Ar << Region.SrcX;
		Ar << Region.SrcY;
		Ar << Region.Width;
		Ar << Region.Height;
		return Ar;
	}
};

/** specifies an update region for a texture */
struct FUpdateTextureRegion3D
{
	/** offset in texture */
	int32 DestX;
	int32 DestY;
	int32 DestZ;

	/** offset in source image data */
	int32 SrcX;
	int32 SrcY;
	int32 SrcZ;

	/** size of region to copy */
	int32 Width;
	int32 Height;
	int32 Depth;

	FUpdateTextureRegion3D()
	{}

	FUpdateTextureRegion3D(int32 InDestX, int32 InDestY, int32 InDestZ, int32 InSrcX, int32 InSrcY, int32 InSrcZ, int32 InWidth, int32 InHeight, int32 InDepth)
	:	DestX(InDestX)
	,	DestY(InDestY)
	,	DestZ(InDestZ)
	,	SrcX(InSrcX)
	,	SrcY(InSrcY)
	,	SrcZ(InSrcZ)
	,	Width(InWidth)
	,	Height(InHeight)
	,	Depth(InDepth)
	{}

	friend FArchive& operator<<(FArchive& Ar,FUpdateTextureRegion3D& Region)
	{
		Ar << Region.DestX;
		Ar << Region.DestY;
		Ar << Region.DestZ;
		Ar << Region.SrcX;
		Ar << Region.SrcY;
		Ar << Region.SrcZ;
		Ar << Region.Width;
		Ar << Region.Height;
		Ar << Region.Depth;
		return Ar;
	}
};

struct FRHIDispatchIndirectParameters
{
	uint32 ThreadGroupCountX;
	uint32 ThreadGroupCountY;
	uint32 ThreadGroupCountZ;
};

struct FRHIDrawIndirectParameters
{
	uint32 VertexCountPerInstance;
	uint32 InstanceCount;
	uint32 StartVertexLocation;
	uint32 StartInstanceLocation;
};

struct FRHIDrawIndexedIndirectParameters
{
	uint32 IndexCountPerInstance;
	uint32 InstanceCount;
	uint32 StartIndexLocation;
	int32 BaseVertexLocation;
	uint32 StartInstanceLocation;
};


struct FTextureMemoryStats
{
	// Hardware state (never change after device creation):

	// -1 if unknown, in bytes
    int64 DedicatedVideoMemory;
	// -1 if unknown, in bytes
    int64 DedicatedSystemMemory;
	// -1 if unknown, in bytes
    int64 SharedSystemMemory;
	// Total amount of "graphics memory" that we think we can use for all our graphics resources, in bytes. -1 if unknown.
	int64 TotalGraphicsMemory;

	// Size of allocated memory, in bytes
	int64 AllocatedMemorySize;
	// 0 if streaming pool size limitation is disabled, in bytes
	int64 TexturePoolSize;
	// Upcoming adjustments to allocated memory, in bytes (async reallocations)
	int32 PendingMemoryAdjustment;

	// defaults
	FTextureMemoryStats()
		: DedicatedVideoMemory(-1)
		, DedicatedSystemMemory(-1)
		, SharedSystemMemory(-1)
		, TotalGraphicsMemory(-1)
		, AllocatedMemorySize(0)
		, TexturePoolSize(0)
		, PendingMemoryAdjustment(0)
	{
	}

	bool AreHardwareStatsValid() const
	{
#if !PLATFORM_HTML5
		return DedicatedVideoMemory >= 0 && DedicatedSystemMemory >= 0 && SharedSystemMemory >= 0;
#else 
		return false; 
#endif 
	}

	bool IsUsingLimitedPoolSize() const
	{
		return TexturePoolSize > 0;
	}

	int64 ComputeAvailableMemorySize() const
	{
		return FMath::Max(TexturePoolSize - AllocatedMemorySize, (int64)0);
	}
};

// RHI counter stats.
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("DrawPrimitive calls"),STAT_RHIDrawPrimitiveCalls,STATGROUP_RHI,RHI_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Triangles drawn"),STAT_RHITriangles,STATGROUP_RHI,RHI_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Lines drawn"),STAT_RHILines,STATGROUP_RHI,RHI_API);

#if STATS
	#define RHI_DRAW_CALL_INC() \
		INC_DWORD_STAT(STAT_RHIDrawPrimitiveCalls); \
		GNumDrawCallsRHI++;

	#define RHI_DRAW_CALL_STATS(PrimitiveType,NumPrimitives) \
		RHI_DRAW_CALL_INC(); \
		INC_DWORD_STAT_BY(STAT_RHITriangles,(uint32)(PrimitiveType != PT_LineList ? (NumPrimitives) : 0)); \
		INC_DWORD_STAT_BY(STAT_RHILines,(uint32)(PrimitiveType == PT_LineList ? (NumPrimitives) : 0)); \
		GNumPrimitivesDrawnRHI += NumPrimitives;
#else
	#define RHI_DRAW_CALL_INC()
	#define RHI_DRAW_CALL_STATS(PrimitiveType,NumPrimitives)
#endif

// RHI memory stats.
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Render target memory 2D"),STAT_RenderTargetMemory2D,STATGROUP_RHI,FPlatformMemory::MCR_GPU,RHI_API);
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Render target memory 3D"),STAT_RenderTargetMemory3D,STATGROUP_RHI,FPlatformMemory::MCR_GPU,RHI_API);
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Render target memory Cube"),STAT_RenderTargetMemoryCube,STATGROUP_RHI,FPlatformMemory::MCR_GPU,RHI_API);
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Texture memory 2D"),STAT_TextureMemory2D,STATGROUP_RHI,FPlatformMemory::MCR_GPU,RHI_API);
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Texture memory 3D"),STAT_TextureMemory3D,STATGROUP_RHI,FPlatformMemory::MCR_GPU,RHI_API);
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Texture memory Cube"),STAT_TextureMemoryCube,STATGROUP_RHI,FPlatformMemory::MCR_GPU,RHI_API);
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Uniform buffer memory"),STAT_UniformBufferMemory,STATGROUP_RHI,FPlatformMemory::MCR_GPU,RHI_API);
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Index buffer memory"),STAT_IndexBufferMemory,STATGROUP_RHI,FPlatformMemory::MCR_GPU,RHI_API);
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Vertex buffer memory"),STAT_VertexBufferMemory,STATGROUP_RHI,FPlatformMemory::MCR_GPU,RHI_API);
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Structured buffer memory"),STAT_StructuredBufferMemory,STATGROUP_RHI,FPlatformMemory::MCR_GPU,RHI_API);
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Pixel buffer memory"),STAT_PixelBufferMemory,STATGROUP_RHI,FPlatformMemory::MCR_GPU,RHI_API);

// RHI base resource types.
#include "RHIResources.h"

//
// Platform-specific RHI types.
//
#if PLATFORM_USES_DYNAMIC_RHI
	// Use dynamically bound RHIs on PCs and when using the null RHI.
	#define USE_DYNAMIC_RHI 1
	#include "DynamicRHI.h"
#else
	// Fall back to the null RHI
	#define USE_DYNAMIC_RHI 1
	#include "DynamicRHI.h"
#endif

#if !USE_DYNAMIC_RHI
	// Define the statically bound RHI methods with the RHI name prefix.
	#define DEFINE_RHIMETHOD(Type,Name,ParameterTypesAndNames,ParameterNames,ReturnStatement,NullImplementation) extern Type Name ParameterTypesAndNames
	#include "RHIMethods.h"
	#undef DEFINE_RHIMETHOD
#endif

/** Initializes the RHI. */
extern RHI_API void RHIInit(bool bHasEditorToken);

/** Shuts down the RHI. */
extern RHI_API void RHIExit();


// the following helper macros allow to safely convert shader types without much code clutter
#define GETSAFERHISHADER_PIXEL(Shader) (Shader ? Shader->GetPixelShader() : (FPixelShaderRHIParamRef)FPixelShaderRHIRef())
#define GETSAFERHISHADER_VERTEX(Shader) (Shader ? Shader->GetVertexShader() : (FVertexShaderRHIParamRef)FVertexShaderRHIRef())
#define GETSAFERHISHADER_HULL(Shader) (Shader ? Shader->GetHullShader() : FHullShaderRHIRef())
#define GETSAFERHISHADER_DOMAIN(Shader) (Shader ? Shader->GetDomainShader() : FDomainShaderRHIRef())
#define GETSAFERHISHADER_GEOMETRY(Shader) (Shader ? Shader->GetGeometryShader() : FGeometryShaderRHIRef())
#define GETSAFERHISHADER_COMPUTE(Shader) (Shader ? Shader->GetComputeShader() : FComputeShaderRHIRef())

// RHI utility functions that depend on the RHI definitions.
#include "RHIUtilities.h"

#endif // __RHI_h__
