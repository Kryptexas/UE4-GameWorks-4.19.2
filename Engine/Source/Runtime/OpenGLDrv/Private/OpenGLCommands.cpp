// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OpenGLCommands.cpp: OpenGL RHI commands implementation.
=============================================================================*/

#include "OpenGLDrvPrivate.h"
#include "OpenGLState.h"

#define DECLARE_ISBOUNDSHADER(ShaderType) inline void ValidateBoundShader(TRefCountPtr<FOpenGLBoundShaderState> InBoundShaderState, F##ShaderType##RHIParamRef ShaderType##RHI) \
{ \
	DYNAMIC_CAST_OPENGLRESOURCE(ShaderType,ShaderType); \
	ensureMsgf(InBoundShaderState && ShaderType == InBoundShaderState->ShaderType, TEXT("Parameters are being set for a %s which is not currently bound"), TEXT(#ShaderType)); \
}

DECLARE_ISBOUNDSHADER(VertexShader)
DECLARE_ISBOUNDSHADER(PixelShader)
DECLARE_ISBOUNDSHADER(GeometryShader)
DECLARE_ISBOUNDSHADER(HullShader)
DECLARE_ISBOUNDSHADER(DomainShader)

#if DO_CHECK
	#define VALIDATE_BOUND_SHADER(s) ValidateBoundShader(PendingState.BoundShaderState, s)
#else
	#define VALIDATE_BOUND_SHADER(s)
#endif

namespace OpenGLConsoleVariables
{
#if PLATFORM_WINDOWS
	int32 bUseMapBuffer = 0;
#else
	int32 bUseMapBuffer = 1;
#endif
	static FAutoConsoleVariableRef CVarUseMapBuffer(
		TEXT("OpenGL.UseMapBuffer"),
		bUseMapBuffer,
		TEXT("If true, use glMapBuffer otherwise use glBufferSubdata.")
		);


	int32 bSkipCompute = 0;
	static FAutoConsoleVariableRef CVarSkipCompute(
		TEXT("OpenGL.SkipCompute"),
		bSkipCompute,
		TEXT("If true, don't issue dispatch work.")
		);
};

TGlobalResource<FVector4VertexDeclaration> GOpenGLVector4VertexDeclaration;

#if PLATFORM_64BITS
#define INDEX_TO_VOID(Index) (void*)((uint64)(Index))
#else
#define INDEX_TO_VOID(Index) (void*)((uint32)(Index))
#endif

enum EClearType
{
	CT_None				= 0x0,
	CT_Depth			= 0x1,
	CT_Stencil			= 0x2,
	CT_Color			= 0x4,
	CT_DepthStencil		= CT_Depth | CT_Stencil,
};

struct FPendingSamplerDataValue
{
	GLenum	Enum;
	GLint	Value;
};

struct FVertexBufferPair
{
	FOpenGLVertexBuffer*				Source;
	TRefCountPtr<FOpenGLVertexBuffer>	Dest;
};
static TArray<FVertexBufferPair> ZeroStrideExpandedBuffersList;


static int FindVertexBuffer(FOpenGLVertexBuffer* Source)
{
	for (int32 Index = 0; Index < ZeroStrideExpandedBuffersList.Num(); ++Index)
	{
		if (ZeroStrideExpandedBuffersList[Index].Source == Source)
		{
			return Index;
		}
	}
	return -1;
}

static FOpenGLVertexBuffer* FindExpandedZeroStrideBuffer(FOpenGLVertexBuffer* ZeroStrideVertexBuffer, uint32 Stride, uint32 NumVertices, const FOpenGLVertexElement& VertexElement)
{
	uint32 Size = NumVertices * Stride;
	int32 FoundExpandedVBIndex = FindVertexBuffer(ZeroStrideVertexBuffer);
	if (FoundExpandedVBIndex != -1)
	{
		// Check if the current size is big enough
		FOpenGLVertexBuffer* ExpandedVB = ZeroStrideExpandedBuffersList[FoundExpandedVBIndex].Dest;
		if (Size <= ExpandedVB->GetSize())
		{
			return ExpandedVB;
		}
	}
	else
	{
		FVertexBufferPair NewPair;
		NewPair.Source = ZeroStrideVertexBuffer;
		NewPair.Dest = NULL;
		FoundExpandedVBIndex = ZeroStrideExpandedBuffersList.Num();
		ZeroStrideExpandedBuffersList.Add(NewPair);
	}

	int32 VertexTypeSize = 0;
	switch( VertexElement.Type )
	{
	case GL_FLOAT:
	case GL_UNSIGNED_INT:
	case GL_INT:
		VertexTypeSize = 4;
		break;
	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
	case GL_HALF_FLOAT:
		VertexTypeSize = 2;
		break;
	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
		VertexTypeSize = 1;
		break;
	case GL_DOUBLE:
		VertexTypeSize = 8;
		break;
	default:
		check(0);
		break;
	}

	const int32 VertexElementSize = ( VertexElement.Size == GL_BGRA ) ? 4 : VertexElement.Size;
	const int32 SizeToFill = VertexElementSize * VertexTypeSize;
	void* RESTRICT SourceData = ZeroStrideVertexBuffer->GetZeroStrideBuffer();
	check(SourceData);
	TRefCountPtr<FOpenGLVertexBuffer> ExpandedVB = new FOpenGLVertexBuffer(0, Size, BUF_Static, NULL);
	uint8* RESTRICT Data = ExpandedVB->Lock(0, Size, false, true);

	switch (SizeToFill)
	{
	case 4:
		{
			uint32 Source = *(uint32*)SourceData;
			uint32* RESTRICT Dest = (uint32*)Data;
			for (uint32 Index = 0; Index < Size / sizeof(uint32); ++Index)
			{
				*Dest++ = Source;
			}
		}
		break;
	case 8:
		{
			uint64 Source = *(uint64*)SourceData;
			uint64* RESTRICT Dest = (uint64*)Data;
			for (uint32 Index = 0; Index < Size / sizeof(uint64); ++Index)
			{
				*Dest++ = Source;
			}
		}
		break;
	case 16:
		{
			uint64 SourceA = *(uint64*)SourceData;
			uint64 SourceB = *((uint64*)SourceData + 1);
			uint64* RESTRICT Dest = (uint64*)Data;
			for (uint32 Index = 0; Index < Size / (2 * sizeof(uint64)); ++Index)
			{
				*Dest++ = SourceA;
				*Dest++ = SourceB;
			}
		}
		break;
	default:
		check(0);
	}

	ExpandedVB->Unlock();

	ZeroStrideExpandedBuffersList[FoundExpandedVBIndex].Dest = ExpandedVB;

	return ExpandedVB;
}

static FORCEINLINE GLint ModifyFilterByMips(GLint Filter, bool bHasMips)
{
	if (!bHasMips)
	{
		switch (Filter)
		{
			case GL_LINEAR_MIPMAP_NEAREST:
			case GL_LINEAR_MIPMAP_LINEAR:
				return GL_LINEAR;

			case GL_NEAREST_MIPMAP_NEAREST:
			case GL_NEAREST_MIPMAP_LINEAR:
				return GL_NEAREST;

			default:
				break;
		}
	}

	return Filter;
}



// Vertex state.
void FOpenGLDynamicRHI::RHISetStreamSource(uint32 StreamIndex,FVertexBufferRHIParamRef VertexBufferRHI,uint32 Stride,uint32 Offset)
{
	DYNAMIC_CAST_OPENGLRESOURCE(VertexBuffer,VertexBuffer);
	PendingState.Streams[StreamIndex].VertexBuffer = VertexBuffer;
	PendingState.Streams[StreamIndex].Stride = Stride;
	PendingState.Streams[StreamIndex].Offset = Offset;
}

void FOpenGLDynamicRHI::RHISetStreamOutTargets(uint32 NumTargets, const FVertexBufferRHIParamRef* VertexBuffers, const uint32* Offsets)
{
	check(0);
}

// Rasterizer state.
void FOpenGLDynamicRHI::RHISetRasterizerState(FRasterizerStateRHIParamRef NewStateRHI)
{
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(RasterizerState,NewState);
	PendingState.RasterizerState = NewState->Data;
}

void FOpenGLDynamicRHI::UpdateRasterizerStateInOpenGLContext( FOpenGLContextState& ContextState )
{
	if (FOpenGL::SupportsPolygonMode() && ContextState.RasterizerState.FillMode != PendingState.RasterizerState.FillMode)
	{
		FOpenGL::PolygonMode(GL_FRONT_AND_BACK, PendingState.RasterizerState.FillMode);
		ContextState.RasterizerState.FillMode = PendingState.RasterizerState.FillMode;
	}

	if (ContextState.RasterizerState.CullMode != PendingState.RasterizerState.CullMode)
	{
		if (PendingState.RasterizerState.CullMode != GL_NONE)
		{
			// Only call glEnable if needed
			if (ContextState.RasterizerState.CullMode == GL_NONE)
			{
				glEnable(GL_CULL_FACE);
			}
			glCullFace(PendingState.RasterizerState.CullMode);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}
		ContextState.RasterizerState.CullMode = PendingState.RasterizerState.CullMode;
	}

	// Convert our platform independent depth bias into an OpenGL depth bias.
	const float BiasScale = float((1<<24)-1);	// Warning: this assumes depth bits == 24, and won't be correct with 32.
	float DepthBias = PendingState.RasterizerState.DepthBias * BiasScale;
	if (ContextState.RasterizerState.DepthBias != PendingState.RasterizerState.DepthBias
		|| ContextState.RasterizerState.SlopeScaleDepthBias != PendingState.RasterizerState.SlopeScaleDepthBias)
	{
		if ((DepthBias == 0.0f) && (PendingState.RasterizerState.SlopeScaleDepthBias == 0.0f))
		{
			// If we're here, both previous 2 'if' conditions are true, and this implies that cached state was not all zeroes, so we need to glDisable.
			glDisable(GL_POLYGON_OFFSET_FILL);
			if ( FOpenGL::SupportsPolygonMode() )
			{
				glDisable(GL_POLYGON_OFFSET_LINE);
				glDisable(GL_POLYGON_OFFSET_POINT);
			}
		}
		else
		{
			if (ContextState.RasterizerState.DepthBias == 0.0f && ContextState.RasterizerState.SlopeScaleDepthBias == 0.0f)
			{
				glEnable(GL_POLYGON_OFFSET_FILL);
				if ( FOpenGL::SupportsPolygonMode() )
				{
					glEnable(GL_POLYGON_OFFSET_LINE);
					glEnable(GL_POLYGON_OFFSET_POINT);
				}
			}
			glPolygonOffset(PendingState.RasterizerState.SlopeScaleDepthBias, DepthBias);
		}

		ContextState.RasterizerState.DepthBias = PendingState.RasterizerState.DepthBias;
		ContextState.RasterizerState.SlopeScaleDepthBias = PendingState.RasterizerState.SlopeScaleDepthBias;
	}
}

void FOpenGLDynamicRHI::UpdateViewportInOpenGLContext( FOpenGLContextState& ContextState )
{
	if (ContextState.Viewport != PendingState.Viewport)
	{
		//@todo the viewport defined by glViewport does not clip, unlike the viewport in d3d
		// Set the scissor rect to the viewport unless it is explicitly set smaller to emulate d3d.
		glViewport(
			PendingState.Viewport.Min.X,
			PendingState.Viewport.Min.Y,
			PendingState.Viewport.Max.X - PendingState.Viewport.Min.X,
			PendingState.Viewport.Max.Y - PendingState.Viewport.Min.Y);

		ContextState.Viewport = PendingState.Viewport;
	}

	if (ContextState.DepthMinZ != PendingState.DepthMinZ || ContextState.DepthMaxZ != PendingState.DepthMaxZ)
	{
		FOpenGL::DepthRange(PendingState.DepthMinZ, PendingState.DepthMaxZ);
		ContextState.DepthMinZ = PendingState.DepthMinZ;
		ContextState.DepthMaxZ = PendingState.DepthMaxZ;
	}
}

void FOpenGLDynamicRHI::RHISetViewport(uint32 MinX,uint32 MinY,float MinZ,uint32 MaxX,uint32 MaxY,float MaxZ)
{
	PendingState.Viewport.Min.X = MinX;
	PendingState.Viewport.Min.Y = MinY;
	PendingState.Viewport.Max.X = MaxX;
	PendingState.Viewport.Max.Y = MaxY;
	PendingState.DepthMinZ = MinZ;
	PendingState.DepthMaxZ = MaxZ;
}

void FOpenGLDynamicRHI::RHISetScissorRect(bool bEnable,uint32 MinX,uint32 MinY,uint32 MaxX,uint32 MaxY)
{
	PendingState.bScissorEnabled = bEnable;
	PendingState.Scissor.Min.X = MinX;
	PendingState.Scissor.Min.Y = MinY;
	PendingState.Scissor.Max.X = MaxX;
	PendingState.Scissor.Max.Y = MaxY;
}

inline void FOpenGLDynamicRHI::UpdateScissorRectInOpenGLContext( FOpenGLContextState& ContextState )
{
	VERIFY_GL_SCOPE();
	if (ContextState.bScissorEnabled != PendingState.bScissorEnabled)
	{
		if (PendingState.bScissorEnabled)
		{
			glEnable(GL_SCISSOR_TEST);
		}
		else
		{
			glDisable(GL_SCISSOR_TEST);
		}
		ContextState.bScissorEnabled = PendingState.bScissorEnabled;
	}

	if( PendingState.bScissorEnabled &&
		ContextState.Scissor != PendingState.Scissor )
	{
		glScissor(PendingState.Scissor.Min.X, PendingState.Scissor.Min.Y, PendingState.Scissor.Max.X - PendingState.Scissor.Min.X, PendingState.Scissor.Max.Y - PendingState.Scissor.Min.Y);
		ContextState.Scissor = PendingState.Scissor;
	}
}

/**
* Set bound shader state. This will set the vertex decl/shader, and pixel shader
* @param BoundShaderState - state resource
*/
void FOpenGLDynamicRHI::RHISetBoundShaderState( FBoundShaderStateRHIParamRef BoundShaderStateRHI)
{
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(BoundShaderState,BoundShaderState);
	PendingState.BoundShaderState = BoundShaderState;

	// Prevent transient bound shader states from being recreated for each use by keeping a history of the most recently used bound shader states.
	// The history keeps them alive, and the bound shader state cache allows them to be reused if needed.
	BoundShaderStateHistory.Add(BoundShaderState);
}

void FOpenGLDynamicRHI::RHISetUAVParameter(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 UAVIndex,FUnorderedAccessViewRHIParamRef UnorderedAccessViewRHI)
{
	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);

	VERIFY_GL_SCOPE();
	if(UnorderedAccessViewRHI)
	{
		DYNAMIC_CAST_OPENGLRESOURCE(UnorderedAccessView, UnorderedAccessView);
		InternalSetShaderUAV(FOpenGL::GetFirstComputeUAVUnit() + UAVIndex, UnorderedAccessView->Format , UnorderedAccessView->Resource);
	}
	else
	{
		InternalSetShaderUAV(FOpenGL::GetFirstComputeUAVUnit() + UAVIndex, GL_R32F, 0);
	}
}

void FOpenGLDynamicRHI::RHISetUAVParameter(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 UAVIndex,FUnorderedAccessViewRHIParamRef UAVRHI, uint32 InitialCount )
{
	// TODO: Implement for OpenGL
	check(0);
}

void FOpenGLDynamicRHI::InternalSetShaderTexture(FOpenGLTextureBase* Texture, GLint TextureIndex, GLenum Target, GLuint Resource, int NumMips, int LimitMip)
{
	auto& PendingTextureState = PendingState.Textures[TextureIndex];
	PendingTextureState.Texture = Texture;
	PendingTextureState.Target = Target;
	PendingTextureState.Resource = Resource;
	PendingTextureState.LimitMip = LimitMip;
	PendingTextureState.bHasMips = (NumMips == 0 || NumMips > 1);
	PendingTextureState.NumMips = NumMips;
}

void FOpenGLDynamicRHI::InternalSetSamplerStates(GLint TextureIndex, FOpenGLSamplerState* SamplerState)
{
	PendingState.SamplerStates[TextureIndex] = SamplerState;
}

void FOpenGLDynamicRHI::CachedSetupTextureStage(FOpenGLContextState& ContextState, GLint TextureIndex, GLenum Target, GLuint Resource, GLint LimitMip, GLint NumMips)
{
	VERIFY_GL_SCOPE();
	auto& TextureState = ContextState.Textures[TextureIndex];
	const bool bSameTarget = (TextureState.Target == Target);
	const bool bSameResource = (TextureState.Resource == Resource);

	if( bSameTarget && bSameResource )
	{
		// Nothing changed, no need to update
		return;
	}

	// Something will have to be changed. Switch to the stage in question.
	if( ContextState.ActiveTexture != TextureIndex )
	{
		glActiveTexture( GL_TEXTURE0 + TextureIndex );
		ContextState.ActiveTexture = TextureIndex;
	}

	if (bSameTarget)
	{
		glBindTexture(Target, Resource);
	}
	else
	{
		if(TextureState.Target != GL_NONE)
		{
			// Unbind different texture target on the same stage, to avoid OpenGL keeping its data, and potential driver problems.
			glBindTexture(TextureState.Target, 0);
		}

		if(Target != GL_NONE)
		{
			glBindTexture(Target, Resource);
		}
	}
	
	// Use the texture SRV's LimitMip value to specify the mip available for sampling
	// This requires SupportsTextureBaseLevel & is a fallback for SupportsTextureView
	// which should be preferred.
	if(Target != GL_NONE && Target != GL_TEXTURE_BUFFER && !FOpenGL::SupportsTextureView())
	{
		const bool bSameLimitMip = bSameTarget && bSameResource && TextureState.LimitMip == LimitMip;
		const bool bSameNumMips = bSameTarget && bSameResource && TextureState.NumMips == NumMips;
		
		if(FOpenGL::SupportsTextureBaseLevel() && !bSameLimitMip)
		{
			GLint BaseMip = LimitMip == -1 ? 0 : LimitMip;
			FOpenGL::TexParameter(Target, GL_TEXTURE_BASE_LEVEL, BaseMip);
		}
		TextureState.LimitMip = LimitMip;
		
		if(FOpenGL::SupportsTextureMaxLevel() && !bSameNumMips)
		{
			GLint MaxMip = LimitMip == -1 ? NumMips - 1 : LimitMip;
			FOpenGL::TexParameter(Target, GL_TEXTURE_MAX_LEVEL, MaxMip);
		}
		TextureState.NumMips = NumMips;
	}
	else
	{
		TextureState.LimitMip = 0;
		TextureState.NumMips = 0;
	}

	TextureState.Target = Target;
	TextureState.Resource = Resource;
}

inline void FOpenGLDynamicRHI::ApplyTextureStage(FOpenGLContextState& ContextState, GLint TextureIndex, const FTextureStage& TextureStage, FOpenGLSamplerState* SamplerState)
{
	GLenum Target = TextureStage.Target;
	VERIFY_GL_SCOPE();
	const bool bHasTexture = (TextureStage.Texture != NULL);
	if (!bHasTexture || TextureStage.Texture->SamplerState != SamplerState)
	{
		// Texture must be bound first
		if( ContextState.ActiveTexture != TextureIndex )
		{
			glActiveTexture(GL_TEXTURE0 + TextureIndex);
			ContextState.ActiveTexture = TextureIndex;
		}

		if (FOpenGL::HasSamplerRestrictions() && bHasTexture)
		{
			if (!TextureStage.Texture->IsPowerOfTwo())
			{
				bool bChanged = false;
				if (SamplerState->Data.WrapS != GL_CLAMP_TO_EDGE)
				{
					SamplerState->Data.WrapS = GL_CLAMP_TO_EDGE;
					bChanged = true;
				}
				if (SamplerState->Data.WrapT != GL_CLAMP_TO_EDGE)
				{
					SamplerState->Data.WrapT = GL_CLAMP_TO_EDGE;
					bChanged = true;
				}
				if (bChanged)
				{
					ANSICHAR DebugName[128] = "";
					if (FOpenGL::GetLabelObject(GL_TEXTURE, TextureStage.Resource, sizeof(DebugName), DebugName) != 0)
					{
						UE_LOG(LogRHI, Warning, TEXT("Texture %s (Index %d, Resource %d) has a non-clamp mode; switching to clamp to avoid driver problems"), ANSI_TO_TCHAR(DebugName), TextureIndex, TextureStage.Resource);
					}
					else
					{
						UE_LOG(LogRHI, Warning, TEXT("Texture %d (Resource %d) has a non-clamp mode; switching to clamp to avoid driver problems"), TextureIndex, TextureStage.Resource);
					}
				}
			}
		}

		// Sets parameters of currently bound texture
		FOpenGL::TexParameter(Target, GL_TEXTURE_WRAP_S, SamplerState->Data.WrapS);
		FOpenGL::TexParameter(Target, GL_TEXTURE_WRAP_T, SamplerState->Data.WrapT);
		if( FOpenGL::SupportsTexture3D() )
		{
			FOpenGL::TexParameter(Target, GL_TEXTURE_WRAP_R, SamplerState->Data.WrapR);
		}

		if( FOpenGL::SupportsTextureLODBias() )
		{
			FOpenGL::TexParameter(Target, GL_TEXTURE_LOD_BIAS, SamplerState->Data.LODBias);
		}
		// Make sure we don't set mip filtering on if the texture has no mip levels, as that will cause a crash/black render on ES2.
		FOpenGL::TexParameter(Target, GL_TEXTURE_MIN_FILTER, ModifyFilterByMips(SamplerState->Data.MinFilter, TextureStage.bHasMips));
		FOpenGL::TexParameter(Target, GL_TEXTURE_MAG_FILTER, SamplerState->Data.MagFilter);
		if( FOpenGL::SupportsTextureFilterAnisotropic() )
		{
			FOpenGL::TexParameter(Target, GL_TEXTURE_MAX_ANISOTROPY_EXT, SamplerState->Data.MaxAnisotropy);
		}

		if( FOpenGL::SupportsTextureCompare() )
		{
			FOpenGL::TexParameter(Target, GL_TEXTURE_COMPARE_MODE, SamplerState->Data.CompareMode);
			FOpenGL::TexParameter(Target, GL_TEXTURE_COMPARE_FUNC, SamplerState->Data.CompareFunc);
		}

		if (bHasTexture)
		{
			TextureStage.Texture->SamplerState = SamplerState;
		}
	}
}

template <typename StateType>
void FOpenGLDynamicRHI::SetupTexturesForDraw( FOpenGLContextState& ContextState, const StateType ShaderState, int32 MaxTexturesNeeded )
{
	// Texture must be bound first
	const bool bNeedsSetupSamplerStage = !FOpenGL::SupportsSamplerObjects();

	if(FOpenGL::SupportsSeamlessCubeMap())
	{
		if(ContextState.bSeamlessCubemapEnabled != PendingState.bSeamlessCubemapEnabled)
		{
			if(PendingState.bSeamlessCubemapEnabled)
			{
				glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
			}
			else
			{
				glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
			}
			ContextState.bSeamlessCubemapEnabled = PendingState.bSeamlessCubemapEnabled;
		}
	}

	const int32 MaxProgramTexture = ShaderState->MaxTextureStageUsed();

	for( int32 TextureStageIndex = 0; TextureStageIndex <= MaxProgramTexture; ++TextureStageIndex )
	{
		if (!ShaderState->NeedsTextureStage(TextureStageIndex))
		{
			// Current program doesn't make use of this texture stage. No matter what UE4 wants to have on in,
			// it won't be useful for this draw, so telling OpenGL we don't really need it to give the driver
			// more leeway in memory management, and avoid false alarms about same texture being set on
			// texture stage and in framebuffer.
			CachedSetupTextureStage( ContextState, TextureStageIndex, GL_NONE, 0, -1, 1 );
		}
		else
		{
			const FTextureStage& TextureStage = PendingState.Textures[TextureStageIndex];
			
#if UE_BUILD_DEBUG
			// Use the texture SRV's LimitMip value to specify the mip available for sampling
			// This requires SupportsTextureBaseLevel & is a fallback for SupportsTextureView
			// which should be preferred.
			if(!FOpenGL::SupportsTextureView())
			{
				FTextureStage& CurrenTextureStage = ContextState.Textures[TextureStageIndex];
				const bool bSameTarget = (TextureStage.Target == CurrenTextureStage.Target);
				const bool bSameResource = (TextureStage.Resource == CurrenTextureStage.Resource);
				const bool bSameLimitMip = bSameTarget && bSameResource && CurrenTextureStage.LimitMip == TextureStage.LimitMip;
				const bool bSameNumMips = bSameTarget && bSameResource && CurrenTextureStage.NumMips == TextureStage.NumMips;
				
				// When trying to limit the mip available for sampling (as part of texture SRV)
				// ensure that the texture is bound to only one sampler, or that all samplers
				// share the same restriction.
				if(TextureStage.LimitMip != -1)
				{
					for( int32 TexIndex = 0; TexIndex <= MaxProgramTexture; ++TexIndex )
					{
						if(TexIndex != TextureStageIndex && ShaderState->NeedsTextureStage(TexIndex))
						{
							const FTextureStage& OtherStage = PendingState.Textures[TexIndex];
							const bool bSameResource = OtherStage.Resource == TextureStage.Resource;
							const bool bSameTarget = OtherStage.Target == TextureStage.Target;
							const GLint TextureStageBaseMip = TextureStage.LimitMip == -1 ? 0 : TextureStage.LimitMip;
							const GLint OtherStageBaseMip = OtherStage.LimitMip == -1 ? 0 : OtherStage.LimitMip;
							const bool bSameLimitMip = TextureStageBaseMip == OtherStageBaseMip;
							const GLint TextureStageMaxMip = TextureStage.LimitMip == -1 ? TextureStage.NumMips - 1 : TextureStage.LimitMip;
							const GLint OtherStageMaxMip = OtherStage.LimitMip == -1 ? OtherStage.NumMips - 1 : OtherStage.LimitMip;
							const bool bSameMaxMip = TextureStageMaxMip == OtherStageMaxMip;
							if( bSameTarget && bSameResource && !bSameLimitMip && !bSameMaxMip )
							{
								UE_LOG(LogRHI, Warning, TEXT("Texture SRV fallback requires that each texture SRV be bound with the same mip-range restrictions. Expect rendering errors."));
							}
						}
					}
				}
			}
#endif
			
			CachedSetupTextureStage( ContextState, TextureStageIndex, TextureStage.Target, TextureStage.Resource, TextureStage.LimitMip, TextureStage.NumMips );
			if (bNeedsSetupSamplerStage)
			{
				ApplyTextureStage( ContextState, TextureStageIndex, TextureStage, PendingState.SamplerStates[TextureStageIndex] );
			}
		}
	}

	// For now, continue to clear unused stages
	for( int32 TextureStageIndex = MaxProgramTexture + 1; TextureStageIndex < MaxTexturesNeeded; ++TextureStageIndex )
	{
		CachedSetupTextureStage( ContextState, TextureStageIndex, GL_NONE, 0, -1, 1 );
	}
}

void FOpenGLDynamicRHI::SetupTexturesForDraw( FOpenGLContextState& ContextState )
{
	SetupTexturesForDraw(ContextState, PendingState.BoundShaderState, FOpenGL::GetMaxCombinedTextureImageUnits());
}

void FOpenGLDynamicRHI::InternalSetShaderUAV(GLint UAVIndex, GLenum Format, GLuint Resource)
{
	PendingState.UAVs[UAVIndex].Format = Format;
	PendingState.UAVs[UAVIndex].Resource = Resource;
}

void FOpenGLDynamicRHI::SetupUAVsForDraw( FOpenGLContextState& ContextState, const TRefCountPtr<FOpenGLComputeShader> &ComputeShader, int32 MaxUAVsNeeded )
{
	for( int32 UAVStageIndex = 0; UAVStageIndex < MaxUAVsNeeded; ++UAVStageIndex )
	{
		if (!ComputeShader->NeedsUAVStage(UAVStageIndex))
		{
			CachedSetupUAVStage(ContextState, UAVStageIndex, GL_R32F, 0 );
		}
		else
		{
			CachedSetupUAVStage(ContextState, UAVStageIndex, PendingState.UAVs[UAVStageIndex].Format, PendingState.UAVs[UAVStageIndex].Resource );
		}
	}
	
}


void FOpenGLDynamicRHI::CachedSetupUAVStage( FOpenGLContextState& ContextState, GLint UAVIndex, GLenum Format, GLuint Resource)
{
	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);

	if( ContextState.UAVs[UAVIndex].Format == Format && ContextState.Textures[UAVIndex].Resource == Resource)
	{
		// Nothing's changed, no need to update
		return;
	}

	FOpenGL::BindImageTexture(UAVIndex, Resource, 0, GL_FALSE, 0, GL_READ_WRITE, Format);
	
	ContextState.UAVs[UAVIndex].Format = Format;
	ContextState.UAVs[UAVIndex].Resource = Resource;
}

void FOpenGLDynamicRHI::RHISetShaderResourceViewParameter(FPixelShaderRHIParamRef PixelShaderRHI,uint32 TextureIndex,FShaderResourceViewRHIParamRef SRVRHI)
{
	VERIFY_GL_SCOPE();
	VALIDATE_BOUND_SHADER(PixelShaderRHI);
	check(FOpenGL::SupportsResourceView());
	DYNAMIC_CAST_OPENGLRESOURCE(ShaderResourceView,SRV);
	GLuint Resource = 0;
	GLenum Target = GL_TEXTURE_BUFFER;
	int32 LimitMip = -1;
	if( SRV )
	{
		Resource = SRV->Resource;
		Target = SRV->Target;
		LimitMip = SRV->LimitMip;
	}
	InternalSetShaderTexture(NULL, FOpenGL::GetFirstPixelTextureUnit() + TextureIndex, Target, Resource, 0, LimitMip);
	RHISetShaderSampler(PixelShaderRHI,TextureIndex,PointSamplerState);
}

void FOpenGLDynamicRHI::RHISetShaderResourceViewParameter(FVertexShaderRHIParamRef VertexShaderRHI,uint32 TextureIndex,FShaderResourceViewRHIParamRef SRVRHI)
{
	VALIDATE_BOUND_SHADER(VertexShaderRHI);

	VERIFY_GL_SCOPE();
	check(FOpenGL::SupportsResourceView());
	DYNAMIC_CAST_OPENGLRESOURCE(ShaderResourceView,SRV);
	GLuint Resource = 0;
	GLenum Target = GL_TEXTURE_BUFFER;
	int32 LimitMip = -1;
	if( SRV )
	{
		Resource = SRV->Resource;
		Target = SRV->Target;
		LimitMip = SRV->LimitMip;
	}
	InternalSetShaderTexture(NULL, FOpenGL::GetFirstVertexTextureUnit() + TextureIndex, Target, Resource, 0, LimitMip);
	RHISetShaderSampler(VertexShaderRHI,TextureIndex,PointSamplerState);
}

void FOpenGLDynamicRHI::RHISetShaderResourceViewParameter(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 TextureIndex,FShaderResourceViewRHIParamRef SRVRHI)
{
	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	check(FOpenGL::SupportsResourceView());
	DYNAMIC_CAST_OPENGLRESOURCE(ShaderResourceView,SRV);
	GLuint Resource = 0;
	GLenum Target = GL_TEXTURE_BUFFER;
	int32 LimitMip = -1;
	if( SRV )
	{
		Resource = SRV->Resource;
		Target = SRV->Target;
		LimitMip = SRV->LimitMip;
	}
	InternalSetShaderTexture(NULL, FOpenGL::GetFirstComputeTextureUnit() + TextureIndex, Target, Resource, 0, LimitMip);
	RHISetShaderSampler(ComputeShaderRHI,TextureIndex,PointSamplerState);
}

void FOpenGLDynamicRHI::RHISetShaderResourceViewParameter(FHullShaderRHIParamRef HullShaderRHI,uint32 TextureIndex,FShaderResourceViewRHIParamRef SRVRHI)
{
	VALIDATE_BOUND_SHADER(HullShaderRHI);

	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	check(FOpenGL::SupportsResourceView());
	DYNAMIC_CAST_OPENGLRESOURCE(ShaderResourceView,SRV);
	GLuint Resource = 0;
	GLenum Target = GL_TEXTURE_BUFFER;
	int32 LimitMip = -1;
	if( SRV )
	{
		Resource = SRV->Resource;
		Target = SRV->Target;
		LimitMip = SRV->LimitMip;
	}
	InternalSetShaderTexture(NULL, FOpenGL::GetFirstHullTextureUnit() + TextureIndex, Target, Resource, 0, LimitMip);
}

void FOpenGLDynamicRHI::RHISetShaderResourceViewParameter(FDomainShaderRHIParamRef DomainShaderRHI,uint32 TextureIndex,FShaderResourceViewRHIParamRef SRVRHI)
{
	VALIDATE_BOUND_SHADER(DomainShaderRHI);

	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	check(FOpenGL::SupportsResourceView());
	DYNAMIC_CAST_OPENGLRESOURCE(ShaderResourceView,SRV);
	GLuint Resource = 0;
	GLenum Target = GL_TEXTURE_BUFFER;
	int32 LimitMip = -1;
	if( SRV )
	{
		Resource = SRV->Resource;
		Target = SRV->Target;
		LimitMip = SRV->LimitMip;
	}
	InternalSetShaderTexture(NULL, FOpenGL::GetFirstDomainTextureUnit() + TextureIndex, Target, Resource, 0, LimitMip);
}

void FOpenGLDynamicRHI::RHISetShaderResourceViewParameter(FGeometryShaderRHIParamRef GeometryShaderRHI,uint32 TextureIndex,FShaderResourceViewRHIParamRef SRVRHI)
{
	VALIDATE_BOUND_SHADER(GeometryShaderRHI);

	VERIFY_GL_SCOPE();
	check(FOpenGL::SupportsResourceView());
	DYNAMIC_CAST_OPENGLRESOURCE(ShaderResourceView,SRV);
	GLuint Resource = 0;
	GLenum Target = GL_TEXTURE_BUFFER;
	int32 LimitMip = -1;
	if( SRV )
	{
		Resource = SRV->Resource;
		Target = SRV->Target;
		LimitMip = SRV->LimitMip;
	}
	InternalSetShaderTexture(NULL, FOpenGL::GetFirstGeometryTextureUnit() + TextureIndex, Target, Resource, 0, LimitMip);
	RHISetShaderSampler(GeometryShaderRHI,TextureIndex,PointSamplerState);
}

void FOpenGLDynamicRHI::RHISetShaderTexture(FVertexShaderRHIParamRef VertexShaderRHI,uint32 TextureIndex,FTextureRHIParamRef NewTextureRHI)
{
	VALIDATE_BOUND_SHADER(VertexShaderRHI);

	VERIFY_GL_SCOPE();
	FOpenGLTextureBase* NewTexture = GetOpenGLTextureFromRHITexture(NewTextureRHI);
	InternalSetShaderTexture(NewTexture, FOpenGL::GetFirstVertexTextureUnit() + TextureIndex, NewTexture->Target, NewTexture->Resource, NewTextureRHI->GetNumMips(), -1);
}

void FOpenGLDynamicRHI::RHISetShaderTexture(FHullShaderRHIParamRef HullShaderRHI,uint32 TextureIndex,FTextureRHIParamRef NewTextureRHI)
{
	VALIDATE_BOUND_SHADER(HullShaderRHI);
	
	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	FOpenGLTextureBase* NewTexture = GetOpenGLTextureFromRHITexture(NewTextureRHI);
	InternalSetShaderTexture(NewTexture, FOpenGL::GetFirstHullTextureUnit() + TextureIndex, NewTexture->Target, NewTexture->Resource, NewTextureRHI->GetNumMips(), -1);
}

void FOpenGLDynamicRHI::RHISetShaderTexture(FDomainShaderRHIParamRef DomainShaderRHI,uint32 TextureIndex,FTextureRHIParamRef NewTextureRHI)
{
	VALIDATE_BOUND_SHADER(DomainShaderRHI);

	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	FOpenGLTextureBase* NewTexture = GetOpenGLTextureFromRHITexture(NewTextureRHI);
	InternalSetShaderTexture(NewTexture, FOpenGL::GetFirstDomainTextureUnit() + TextureIndex, NewTexture->Target, NewTexture->Resource, NewTextureRHI->GetNumMips(), -1);
}

void FOpenGLDynamicRHI::RHISetShaderTexture(FGeometryShaderRHIParamRef GeometryShaderRHI,uint32 TextureIndex,FTextureRHIParamRef NewTextureRHI)
{
	VALIDATE_BOUND_SHADER(GeometryShaderRHI);

	VERIFY_GL_SCOPE();
	FOpenGLTextureBase* NewTexture = GetOpenGLTextureFromRHITexture(NewTextureRHI);
	InternalSetShaderTexture(NewTexture, FOpenGL::GetFirstGeometryTextureUnit() + TextureIndex, NewTexture->Target, NewTexture->Resource, NewTextureRHI->GetNumMips(), -1);
}

void FOpenGLDynamicRHI::RHISetShaderTexture(FPixelShaderRHIParamRef PixelShaderRHI,uint32 TextureIndex,FTextureRHIParamRef NewTextureRHI)
{
	VALIDATE_BOUND_SHADER(PixelShaderRHI);

	VERIFY_GL_SCOPE();
	FOpenGLTextureBase* NewTexture = GetOpenGLTextureFromRHITexture(NewTextureRHI);
	InternalSetShaderTexture(NewTexture, FOpenGL::GetFirstPixelTextureUnit() + TextureIndex, NewTexture->Target, NewTexture->Resource, NewTextureRHI->GetNumMips(), -1);
}

void FOpenGLDynamicRHI::RHISetShaderSampler(FVertexShaderRHIParamRef VertexShaderRHI,uint32 SamplerIndex,FSamplerStateRHIParamRef NewStateRHI)
{
	VALIDATE_BOUND_SHADER(VertexShaderRHI);

	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(SamplerState,NewState);
	if (FOpenGL::SupportsSamplerObjects())
	{
		FOpenGL::BindSampler(FOpenGL::GetFirstVertexTextureUnit() + SamplerIndex, NewState->Resource);
	}
	else
	{
		InternalSetSamplerStates(FOpenGL::GetFirstVertexTextureUnit() + SamplerIndex, NewState);
	}
}

void FOpenGLDynamicRHI::RHISetShaderSampler(FHullShaderRHIParamRef HullShaderRHI,uint32 SamplerIndex,FSamplerStateRHIParamRef NewStateRHI)
{
	VALIDATE_BOUND_SHADER(HullShaderRHI);

	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(SamplerState,NewState);
	if (FOpenGL::SupportsSamplerObjects())
	{
		FOpenGL::BindSampler(FOpenGL::GetFirstHullTextureUnit() + SamplerIndex, NewState->Resource);
	}
	else
	{
		InternalSetSamplerStates(FOpenGL::GetFirstHullTextureUnit() + SamplerIndex, NewState);
	}
}
void FOpenGLDynamicRHI::RHISetShaderSampler(FDomainShaderRHIParamRef DomainShaderRHI,uint32 SamplerIndex,FSamplerStateRHIParamRef NewStateRHI)
{
	VALIDATE_BOUND_SHADER(DomainShaderRHI);

	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(SamplerState,NewState);
	if (FOpenGL::SupportsSamplerObjects())
	{
		FOpenGL::BindSampler(FOpenGL::GetFirstDomainTextureUnit() + SamplerIndex, NewState->Resource);
	}
	else
	{
		InternalSetSamplerStates(FOpenGL::GetFirstDomainTextureUnit() + SamplerIndex, NewState);
	}
}

void FOpenGLDynamicRHI::RHISetShaderSampler(FGeometryShaderRHIParamRef GeometryShaderRHI,uint32 SamplerIndex,FSamplerStateRHIParamRef NewStateRHI)
{
	VALIDATE_BOUND_SHADER(GeometryShaderRHI);

	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(SamplerState,NewState);
	if (FOpenGL::SupportsSamplerObjects())
	{
		FOpenGL::BindSampler(FOpenGL::GetFirstGeometryTextureUnit() + SamplerIndex, NewState->Resource);
	}
	else
	{
		InternalSetSamplerStates(FOpenGL::GetFirstGeometryTextureUnit() + SamplerIndex, NewState);
	}
}

void FOpenGLDynamicRHI::RHISetShaderTexture(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 TextureIndex,FTextureRHIParamRef NewTextureRHI)
{
	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	FOpenGLTextureBase* NewTexture = GetOpenGLTextureFromRHITexture(NewTextureRHI);
	InternalSetShaderTexture(NewTexture, FOpenGL::GetFirstComputeTextureUnit() + TextureIndex, NewTexture->Target, NewTexture->Resource, NewTextureRHI->GetNumMips(), -1);
}

void FOpenGLDynamicRHI::RHISetShaderSampler(FPixelShaderRHIParamRef PixelShaderRHI,uint32 SamplerIndex,FSamplerStateRHIParamRef NewStateRHI)
{
	VALIDATE_BOUND_SHADER(PixelShaderRHI);

	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(SamplerState,NewState);
	if ( FOpenGL::SupportsSamplerObjects() )
	{
		FOpenGL::BindSampler(FOpenGL::GetFirstPixelTextureUnit() + SamplerIndex, NewState->Resource);
	}
	else
	{
		InternalSetSamplerStates(FOpenGL::GetFirstPixelTextureUnit() + SamplerIndex, NewState);
	}
}

void FOpenGLDynamicRHI::RHISetShaderUniformBuffer(FVertexShaderRHIParamRef VertexShaderRHI,uint32 BufferIndex,FUniformBufferRHIParamRef BufferRHI)
{
	VALIDATE_BOUND_SHADER(VertexShaderRHI);

	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(VertexShader,VertexShader);
	VertexShader->BoundUniformBuffers[BufferIndex] = BufferRHI;
}

void FOpenGLDynamicRHI::RHISetShaderUniformBuffer(FHullShaderRHIParamRef HullShaderRHI,uint32 BufferIndex,FUniformBufferRHIParamRef BufferRHI)
{
	VALIDATE_BOUND_SHADER(HullShaderRHI);

	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(HullShader,HullShader);
	HullShader->BoundUniformBuffers[BufferIndex] = BufferRHI;
}

void FOpenGLDynamicRHI::RHISetShaderUniformBuffer(FDomainShaderRHIParamRef DomainShaderRHI,uint32 BufferIndex,FUniformBufferRHIParamRef BufferRHI)
{
	VALIDATE_BOUND_SHADER(DomainShaderRHI);

	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(DomainShader,DomainShader);
	DomainShader->BoundUniformBuffers[BufferIndex] = BufferRHI;
}

void FOpenGLDynamicRHI::RHISetShaderUniformBuffer(FGeometryShaderRHIParamRef GeometryShaderRHI,uint32 BufferIndex,FUniformBufferRHIParamRef BufferRHI)
{
	VALIDATE_BOUND_SHADER(GeometryShaderRHI);

	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(GeometryShader,GeometryShader);
	GeometryShader->BoundUniformBuffers[BufferIndex] = BufferRHI;
}

void FOpenGLDynamicRHI::RHISetShaderSampler(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 SamplerIndex,FSamplerStateRHIParamRef NewStateRHI)
{
	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(SamplerState,NewState);
	FOpenGL::BindSampler(FOpenGL::GetFirstComputeTextureUnit() + SamplerIndex, NewState->Resource);

}

void FOpenGLDynamicRHI::RHISetShaderUniformBuffer(FPixelShaderRHIParamRef PixelShaderRHI,uint32 BufferIndex,FUniformBufferRHIParamRef BufferRHI)
{
	VALIDATE_BOUND_SHADER(PixelShaderRHI);

	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(PixelShader,PixelShader);
	PixelShader->BoundUniformBuffers[BufferIndex] = BufferRHI;
}

void FOpenGLDynamicRHI::RHISetShaderUniformBuffer(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 BufferIndex,FUniformBufferRHIParamRef BufferRHI)
{
	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(ComputeShader, ComputeShader);
	ComputeShader->BoundUniformBuffers[BufferIndex] = BufferRHI;
}

void FOpenGLDynamicRHI::RHISetShaderParameter(FVertexShaderRHIParamRef VertexShaderRHI,uint32 BufferIndex,uint32 BaseIndex,uint32 NumBytes,const void* NewValue)
{
	VALIDATE_BOUND_SHADER(VertexShaderRHI);

	VERIFY_GL_SCOPE();
	PendingState.ShaderParameters[OGL_SHADER_STAGE_VERTEX].Set(BufferIndex, BaseIndex, NumBytes, NewValue);
}

void FOpenGLDynamicRHI::RHISetShaderParameter(FPixelShaderRHIParamRef PixelShaderRHI,uint32 BufferIndex,uint32 BaseIndex,uint32 NumBytes,const void* NewValue)
{
	VALIDATE_BOUND_SHADER(PixelShaderRHI);

	VERIFY_GL_SCOPE();
	PendingState.ShaderParameters[OGL_SHADER_STAGE_PIXEL].Set(BufferIndex, BaseIndex, NumBytes, NewValue);
}

void FOpenGLDynamicRHI::RHISetShaderParameter(FHullShaderRHIParamRef HullShaderRHI,uint32 BufferIndex,uint32 BaseIndex,uint32 NumBytes,const void* NewValue)
{
	VALIDATE_BOUND_SHADER(HullShaderRHI);

	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	PendingState.ShaderParameters[OGL_SHADER_STAGE_HULL].Set(BufferIndex, BaseIndex, NumBytes, NewValue);
}

void FOpenGLDynamicRHI::RHISetShaderParameter(FDomainShaderRHIParamRef DomainShaderRHI,uint32 BufferIndex,uint32 BaseIndex,uint32 NumBytes,const void* NewValue)
{
	VALIDATE_BOUND_SHADER(DomainShaderRHI);

	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	VERIFY_GL_SCOPE();
	PendingState.ShaderParameters[OGL_SHADER_STAGE_DOMAIN].Set(BufferIndex, BaseIndex, NumBytes, NewValue);
}

void FOpenGLDynamicRHI::RHISetShaderParameter(FGeometryShaderRHIParamRef GeometryShaderRHI,uint32 BufferIndex,uint32 BaseIndex,uint32 NumBytes,const void* NewValue)
{
	VALIDATE_BOUND_SHADER(GeometryShaderRHI);

	VERIFY_GL_SCOPE();
	PendingState.ShaderParameters[OGL_SHADER_STAGE_GEOMETRY].Set(BufferIndex, BaseIndex, NumBytes, NewValue);
}

void FOpenGLDynamicRHI::RHISetShaderParameter(FComputeShaderRHIParamRef ComputeShaderRHI,uint32 BufferIndex,uint32 BaseIndex,uint32 NumBytes,const void* NewValue)
{ 
	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);
	PendingState.ShaderParameters[OGL_SHADER_STAGE_COMPUTE].Set(BufferIndex, BaseIndex, NumBytes, NewValue);
}

void FOpenGLDynamicRHI::RHISetDepthStencilState(FDepthStencilStateRHIParamRef NewStateRHI,uint32 StencilRef)
{
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(DepthStencilState,NewState);
	PendingState.DepthStencilState = NewState->Data;
	PendingState.StencilRef = StencilRef;
}

void FOpenGLDynamicRHI::UpdateDepthStencilStateInOpenGLContext( FOpenGLContextState& ContextState )
{
	if (ContextState.DepthStencilState.bZEnable != PendingState.DepthStencilState.bZEnable)
	{
		if (PendingState.DepthStencilState.bZEnable)
		{
			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}
		ContextState.DepthStencilState.bZEnable = PendingState.DepthStencilState.bZEnable;
	}

	if (ContextState.DepthStencilState.bZWriteEnable != PendingState.DepthStencilState.bZWriteEnable)
	{
		glDepthMask(PendingState.DepthStencilState.bZWriteEnable);
		ContextState.DepthStencilState.bZWriteEnable = PendingState.DepthStencilState.bZWriteEnable;
	}

	if (PendingState.DepthStencilState.bZEnable)
	{
		if (ContextState.DepthStencilState.ZFunc != PendingState.DepthStencilState.ZFunc)
		{
			glDepthFunc(PendingState.DepthStencilState.ZFunc);
			ContextState.DepthStencilState.ZFunc = PendingState.DepthStencilState.ZFunc;
		}
	}

	if (ContextState.DepthStencilState.bStencilEnable != PendingState.DepthStencilState.bStencilEnable)
	{
		if (PendingState.DepthStencilState.bStencilEnable)
		{
			glEnable(GL_STENCIL_TEST);
		}
		else
		{
			glDisable(GL_STENCIL_TEST);
		}
		ContextState.DepthStencilState.bStencilEnable = PendingState.DepthStencilState.bStencilEnable;
	}

	// If only two-sided <-> one-sided stencil mode changes, and nothing else, we need to call full set of functions
	// to ensure all drivers handle this correctly - some of them might keep those states in different variables.
	if (ContextState.DepthStencilState.bTwoSidedStencilMode != PendingState.DepthStencilState.bTwoSidedStencilMode)
	{
		// Invalidate cache to enforce update of part of stencil state that needs to be set with different functions, when needed next
		// Values below are all invalid, but they'll never be used, only compared to new values to be set.
		ContextState.DepthStencilState.StencilFunc = 0xFFFF;
		ContextState.DepthStencilState.StencilFail = 0xFFFF;
		ContextState.DepthStencilState.StencilZFail = 0xFFFF;
		ContextState.DepthStencilState.StencilPass = 0xFFFF;
		ContextState.DepthStencilState.CCWStencilFunc = 0xFFFF;
		ContextState.DepthStencilState.CCWStencilFail = 0xFFFF;
		ContextState.DepthStencilState.CCWStencilZFail = 0xFFFF;
		ContextState.DepthStencilState.CCWStencilPass = 0xFFFF;
		ContextState.DepthStencilState.StencilReadMask = 0xFFFF;

		ContextState.DepthStencilState.bTwoSidedStencilMode = PendingState.DepthStencilState.bTwoSidedStencilMode;
	}

	if (PendingState.DepthStencilState.bStencilEnable)
	{
		if (PendingState.DepthStencilState.bTwoSidedStencilMode)
		{
			if (ContextState.DepthStencilState.StencilFunc != PendingState.DepthStencilState.StencilFunc
				|| ContextState.StencilRef != PendingState.StencilRef
				|| ContextState.DepthStencilState.StencilReadMask != PendingState.DepthStencilState.StencilReadMask)
			{
				glStencilFuncSeparate(GL_BACK, PendingState.DepthStencilState.StencilFunc, PendingState.StencilRef, PendingState.DepthStencilState.StencilReadMask);
				ContextState.DepthStencilState.StencilFunc = PendingState.DepthStencilState.StencilFunc;
			}

			if (ContextState.DepthStencilState.StencilFail != PendingState.DepthStencilState.StencilFail
				|| ContextState.DepthStencilState.StencilZFail != PendingState.DepthStencilState.StencilZFail
				|| ContextState.DepthStencilState.StencilPass != PendingState.DepthStencilState.StencilPass)
			{
				glStencilOpSeparate(GL_BACK, PendingState.DepthStencilState.StencilFail, PendingState.DepthStencilState.StencilZFail, PendingState.DepthStencilState.StencilPass);
				ContextState.DepthStencilState.StencilFail = PendingState.DepthStencilState.StencilFail;
				ContextState.DepthStencilState.StencilZFail = PendingState.DepthStencilState.StencilZFail;
				ContextState.DepthStencilState.StencilPass = PendingState.DepthStencilState.StencilPass;
			}

			if (ContextState.DepthStencilState.CCWStencilFunc != PendingState.DepthStencilState.CCWStencilFunc
				|| ContextState.StencilRef != PendingState.StencilRef
				|| ContextState.DepthStencilState.StencilReadMask != PendingState.DepthStencilState.StencilReadMask)
			{
				glStencilFuncSeparate(GL_FRONT, PendingState.DepthStencilState.CCWStencilFunc, PendingState.StencilRef, PendingState.DepthStencilState.StencilReadMask);
				ContextState.DepthStencilState.CCWStencilFunc = PendingState.DepthStencilState.CCWStencilFunc;
			}

			if (ContextState.DepthStencilState.CCWStencilFail != PendingState.DepthStencilState.CCWStencilFail
				|| ContextState.DepthStencilState.CCWStencilZFail != PendingState.DepthStencilState.CCWStencilZFail
				|| ContextState.DepthStencilState.CCWStencilPass != PendingState.DepthStencilState.CCWStencilPass)
			{
				glStencilOpSeparate(GL_FRONT, PendingState.DepthStencilState.CCWStencilFail, PendingState.DepthStencilState.CCWStencilZFail, PendingState.DepthStencilState.CCWStencilPass);
				ContextState.DepthStencilState.CCWStencilFail = PendingState.DepthStencilState.CCWStencilFail;
				ContextState.DepthStencilState.CCWStencilZFail = PendingState.DepthStencilState.CCWStencilZFail;
				ContextState.DepthStencilState.CCWStencilPass = PendingState.DepthStencilState.CCWStencilPass;
			}

			ContextState.DepthStencilState.StencilReadMask = PendingState.DepthStencilState.StencilReadMask;
			ContextState.StencilRef = PendingState.StencilRef;
		}
		else
		{
			if (ContextState.DepthStencilState.StencilFunc != PendingState.DepthStencilState.StencilFunc
				|| ContextState.StencilRef != PendingState.StencilRef
				|| ContextState.DepthStencilState.StencilReadMask != PendingState.DepthStencilState.StencilReadMask)
			{
				glStencilFunc(PendingState.DepthStencilState.StencilFunc, PendingState.StencilRef, PendingState.DepthStencilState.StencilReadMask);
				ContextState.DepthStencilState.StencilFunc = PendingState.DepthStencilState.StencilFunc;
				ContextState.DepthStencilState.StencilReadMask = PendingState.DepthStencilState.StencilReadMask;
				ContextState.StencilRef = PendingState.StencilRef;
			}

			if (ContextState.DepthStencilState.StencilFail != PendingState.DepthStencilState.StencilFail
				|| ContextState.DepthStencilState.StencilZFail != PendingState.DepthStencilState.StencilZFail
				|| ContextState.DepthStencilState.StencilPass != PendingState.DepthStencilState.StencilPass)
			{
				glStencilOp(PendingState.DepthStencilState.StencilFail, PendingState.DepthStencilState.StencilZFail, PendingState.DepthStencilState.StencilPass);
				ContextState.DepthStencilState.StencilFail = PendingState.DepthStencilState.StencilFail;
				ContextState.DepthStencilState.StencilZFail = PendingState.DepthStencilState.StencilZFail;
				ContextState.DepthStencilState.StencilPass = PendingState.DepthStencilState.StencilPass;
			}
		}

		if (ContextState.DepthStencilState.StencilWriteMask != PendingState.DepthStencilState.StencilWriteMask)
		{
			glStencilMask(PendingState.DepthStencilState.StencilWriteMask);
			ContextState.DepthStencilState.StencilWriteMask = PendingState.DepthStencilState.StencilWriteMask;
		}
	}
}

void FOpenGLDynamicRHI::SetPendingBlendStateForActiveRenderTargets( FOpenGLContextState& ContextState )
{
	VERIFY_GL_SCOPE();

	bool bABlendWasSet = false;

	//
	// Need to expand setting for glBlendFunction and glBlendEquation

	const uint32 NumRenderTargets = FOpenGL::SupportsMultipleRenderTargets() ? MaxSimultaneousRenderTargets : 1;

	for (uint32 RenderTargetIndex = 0;RenderTargetIndex < NumRenderTargets; ++RenderTargetIndex)
	{
		if(PendingState.RenderTargets[RenderTargetIndex] == 0)
		{
			// Even if on this stage blend states are incompatible with other stages, we can disregard it, as no render target is assigned to it.
			continue;
		}

		const FOpenGLBlendStateData::FRenderTarget& RenderTargetBlendState = PendingState.BlendState.RenderTargets[RenderTargetIndex];
		FOpenGLBlendStateData::FRenderTarget& CachedRenderTargetBlendState = ContextState.BlendState.RenderTargets[RenderTargetIndex];

		if (CachedRenderTargetBlendState.bAlphaBlendEnable != RenderTargetBlendState.bAlphaBlendEnable)
		{
			if (RenderTargetBlendState.bAlphaBlendEnable)
			{
				FOpenGL::EnableIndexed(GL_BLEND,RenderTargetIndex);
			}
			else
			{
				FOpenGL::DisableIndexed(GL_BLEND,RenderTargetIndex);
			}
			CachedRenderTargetBlendState.bAlphaBlendEnable = RenderTargetBlendState.bAlphaBlendEnable;
		}

		if (RenderTargetBlendState.bAlphaBlendEnable)
		{
			if ( FOpenGL::SupportsSeparateAlphaBlend() )
			{
				// Set current blend per stage
				if (RenderTargetBlendState.bSeparateAlphaBlendEnable)
				{
					if (CachedRenderTargetBlendState.ColorSourceBlendFactor != RenderTargetBlendState.ColorSourceBlendFactor
						|| CachedRenderTargetBlendState.ColorDestBlendFactor != RenderTargetBlendState.ColorDestBlendFactor
						|| CachedRenderTargetBlendState.AlphaSourceBlendFactor != RenderTargetBlendState.AlphaSourceBlendFactor
						|| CachedRenderTargetBlendState.AlphaDestBlendFactor != RenderTargetBlendState.AlphaDestBlendFactor)
					{
						FOpenGL::BlendFuncSeparatei(
							RenderTargetIndex, 
							RenderTargetBlendState.ColorSourceBlendFactor, RenderTargetBlendState.ColorDestBlendFactor,
							RenderTargetBlendState.AlphaSourceBlendFactor, RenderTargetBlendState.AlphaDestBlendFactor
							);
					}

					if (CachedRenderTargetBlendState.ColorBlendOperation != RenderTargetBlendState.ColorBlendOperation
						|| CachedRenderTargetBlendState.AlphaBlendOperation != RenderTargetBlendState.AlphaBlendOperation)
					{
						FOpenGL::BlendEquationSeparatei(
							RenderTargetIndex, 
							RenderTargetBlendState.ColorBlendOperation,
							RenderTargetBlendState.AlphaBlendOperation
							);
					}
					
#if PLATFORM_MAC	// Flush the separate blend state changes through GL or on Intel cards under OS X the state change may be silently ignored.
					// We may wish to consider using GL_APPLE_flush_render for this workaround if it is still faster to call glFlushRenderAPPLE than plain glFlush.
					if (CachedRenderTargetBlendState.ColorSourceBlendFactor != RenderTargetBlendState.ColorSourceBlendFactor
						|| CachedRenderTargetBlendState.ColorDestBlendFactor != RenderTargetBlendState.ColorDestBlendFactor
						|| CachedRenderTargetBlendState.AlphaSourceBlendFactor != RenderTargetBlendState.AlphaSourceBlendFactor
						|| CachedRenderTargetBlendState.AlphaDestBlendFactor != RenderTargetBlendState.AlphaDestBlendFactor
						|| CachedRenderTargetBlendState.ColorBlendOperation != RenderTargetBlendState.ColorBlendOperation
						|| CachedRenderTargetBlendState.AlphaBlendOperation != RenderTargetBlendState.AlphaBlendOperation)
					{
						glFlush();
					}
#endif
				}
				else
				{
					if (CachedRenderTargetBlendState.ColorSourceBlendFactor != RenderTargetBlendState.ColorSourceBlendFactor
						|| CachedRenderTargetBlendState.ColorDestBlendFactor != RenderTargetBlendState.ColorDestBlendFactor
						|| CachedRenderTargetBlendState.AlphaSourceBlendFactor != RenderTargetBlendState.ColorSourceBlendFactor
						|| CachedRenderTargetBlendState.AlphaDestBlendFactor != RenderTargetBlendState.ColorDestBlendFactor)
					{
						FOpenGL::BlendFunci(RenderTargetIndex, RenderTargetBlendState.ColorSourceBlendFactor, RenderTargetBlendState.ColorDestBlendFactor);
					}

					if (CachedRenderTargetBlendState.ColorBlendOperation != RenderTargetBlendState.ColorBlendOperation)
					{
						FOpenGL::BlendEquationi(RenderTargetIndex, RenderTargetBlendState.ColorBlendOperation);
					}
				}

				CachedRenderTargetBlendState.bSeparateAlphaBlendEnable = RenderTargetBlendState.bSeparateAlphaBlendEnable;
				CachedRenderTargetBlendState.ColorBlendOperation = RenderTargetBlendState.ColorBlendOperation;
				CachedRenderTargetBlendState.ColorSourceBlendFactor = RenderTargetBlendState.ColorSourceBlendFactor;
				CachedRenderTargetBlendState.ColorDestBlendFactor = RenderTargetBlendState.ColorDestBlendFactor;
				if( RenderTargetBlendState.bSeparateAlphaBlendEnable )
				{ 
					CachedRenderTargetBlendState.AlphaSourceBlendFactor = RenderTargetBlendState.AlphaSourceBlendFactor;
					CachedRenderTargetBlendState.AlphaDestBlendFactor = RenderTargetBlendState.AlphaDestBlendFactor;
				}
				else
				{
					CachedRenderTargetBlendState.AlphaSourceBlendFactor = RenderTargetBlendState.ColorSourceBlendFactor;
					CachedRenderTargetBlendState.AlphaDestBlendFactor = RenderTargetBlendState.ColorDestBlendFactor;
				}
			}
			else
			{
				if( bABlendWasSet )
				{
					// Detect the case of subsequent render target needing different blend setup than one already set in this call.
					if( CachedRenderTargetBlendState.bSeparateAlphaBlendEnable != RenderTargetBlendState.bSeparateAlphaBlendEnable
						|| CachedRenderTargetBlendState.ColorBlendOperation != RenderTargetBlendState.ColorBlendOperation
						|| CachedRenderTargetBlendState.ColorSourceBlendFactor != RenderTargetBlendState.ColorSourceBlendFactor
						|| CachedRenderTargetBlendState.ColorDestBlendFactor != RenderTargetBlendState.ColorDestBlendFactor
						|| ( RenderTargetBlendState.bSeparateAlphaBlendEnable && 
							( CachedRenderTargetBlendState.AlphaSourceBlendFactor != RenderTargetBlendState.AlphaSourceBlendFactor
							|| CachedRenderTargetBlendState.AlphaDestBlendFactor != RenderTargetBlendState.AlphaDestBlendFactor
							)
							)
						)
						UE_LOG(LogRHI, Fatal, TEXT("OpenGL state on draw requires setting different blend operation or factors to different render targets. This is not supported on Mac OS X!"));
				}
				else
				{
					// Set current blend to all stages
					if (RenderTargetBlendState.bSeparateAlphaBlendEnable)
					{
						if (CachedRenderTargetBlendState.ColorSourceBlendFactor != RenderTargetBlendState.ColorSourceBlendFactor
							|| CachedRenderTargetBlendState.ColorDestBlendFactor != RenderTargetBlendState.ColorDestBlendFactor
							|| CachedRenderTargetBlendState.AlphaSourceBlendFactor != RenderTargetBlendState.AlphaSourceBlendFactor
							|| CachedRenderTargetBlendState.AlphaDestBlendFactor != RenderTargetBlendState.AlphaDestBlendFactor)
						{
							glBlendFuncSeparate(
								RenderTargetBlendState.ColorSourceBlendFactor, RenderTargetBlendState.ColorDestBlendFactor,
								RenderTargetBlendState.AlphaSourceBlendFactor, RenderTargetBlendState.AlphaDestBlendFactor
								);
						}

						if (CachedRenderTargetBlendState.ColorBlendOperation != RenderTargetBlendState.ColorBlendOperation
							|| CachedRenderTargetBlendState.AlphaBlendOperation != RenderTargetBlendState.AlphaBlendOperation)
						{
							glBlendEquationSeparate(
								RenderTargetBlendState.ColorBlendOperation,
								RenderTargetBlendState.AlphaBlendOperation
								);
						}
					}
					else
					{
						if (CachedRenderTargetBlendState.ColorSourceBlendFactor != RenderTargetBlendState.ColorSourceBlendFactor
							|| CachedRenderTargetBlendState.ColorDestBlendFactor != RenderTargetBlendState.ColorDestBlendFactor
							|| CachedRenderTargetBlendState.AlphaSourceBlendFactor != RenderTargetBlendState.ColorSourceBlendFactor
							|| CachedRenderTargetBlendState.AlphaDestBlendFactor != RenderTargetBlendState.ColorDestBlendFactor)
						{
							glBlendFunc(RenderTargetBlendState.ColorSourceBlendFactor, RenderTargetBlendState.ColorDestBlendFactor);
						}

						if (CachedRenderTargetBlendState.ColorBlendOperation != RenderTargetBlendState.ColorBlendOperation)
						{
							glBlendEquation(RenderTargetBlendState.ColorBlendOperation);
						}
					}

					// Set cached values of all stages to what they were set by global calls, common to all stages
					for(uint32 RenderTargetIndex2 = 0; RenderTargetIndex2 < MaxSimultaneousRenderTargets; ++RenderTargetIndex2 )
					{
					FOpenGLBlendStateData::FRenderTarget& CachedRenderTargetBlendState2 = ContextState.BlendState.RenderTargets[RenderTargetIndex2];
						CachedRenderTargetBlendState2.bSeparateAlphaBlendEnable = RenderTargetBlendState.bSeparateAlphaBlendEnable;
						CachedRenderTargetBlendState2.ColorBlendOperation = RenderTargetBlendState.ColorBlendOperation;
						CachedRenderTargetBlendState2.ColorSourceBlendFactor = RenderTargetBlendState.ColorSourceBlendFactor;
						CachedRenderTargetBlendState2.ColorDestBlendFactor = RenderTargetBlendState.ColorDestBlendFactor;
						if( RenderTargetBlendState.bSeparateAlphaBlendEnable )
						{ 
							CachedRenderTargetBlendState2.AlphaSourceBlendFactor = RenderTargetBlendState.AlphaSourceBlendFactor;
							CachedRenderTargetBlendState2.AlphaDestBlendFactor = RenderTargetBlendState.AlphaDestBlendFactor;
						}
						else
						{
							CachedRenderTargetBlendState2.AlphaSourceBlendFactor = RenderTargetBlendState.ColorSourceBlendFactor;
							CachedRenderTargetBlendState2.AlphaDestBlendFactor = RenderTargetBlendState.ColorDestBlendFactor;
						}
					}

					bABlendWasSet = true;
				}
			}
		}

		CachedRenderTargetBlendState.bSeparateAlphaBlendEnable = RenderTargetBlendState.bSeparateAlphaBlendEnable;

		if(CachedRenderTargetBlendState.ColorWriteMaskR != RenderTargetBlendState.ColorWriteMaskR
			|| CachedRenderTargetBlendState.ColorWriteMaskG != RenderTargetBlendState.ColorWriteMaskG
			|| CachedRenderTargetBlendState.ColorWriteMaskB != RenderTargetBlendState.ColorWriteMaskB
			|| CachedRenderTargetBlendState.ColorWriteMaskA != RenderTargetBlendState.ColorWriteMaskA)
		{
			FOpenGL::ColorMaskIndexed(
				RenderTargetIndex,
				RenderTargetBlendState.ColorWriteMaskR,
				RenderTargetBlendState.ColorWriteMaskG,
				RenderTargetBlendState.ColorWriteMaskB,
				RenderTargetBlendState.ColorWriteMaskA
				);

			CachedRenderTargetBlendState.ColorWriteMaskR = RenderTargetBlendState.ColorWriteMaskR;
			CachedRenderTargetBlendState.ColorWriteMaskG = RenderTargetBlendState.ColorWriteMaskG;
			CachedRenderTargetBlendState.ColorWriteMaskB = RenderTargetBlendState.ColorWriteMaskB;
			CachedRenderTargetBlendState.ColorWriteMaskA = RenderTargetBlendState.ColorWriteMaskA;
		}
	}
}

void FOpenGLDynamicRHI::RHISetBlendState(FBlendStateRHIParamRef NewStateRHI,const FLinearColor& BlendFactor)
{
	DYNAMIC_CAST_OPENGLRESOURCE(BlendState,NewState);
	FMemory::Memcpy(&PendingState.BlendState,&(NewState->Data),sizeof(FOpenGLBlendStateData));
}

void FOpenGLDynamicRHI::RHISetRenderTargets(
	uint32 NumSimultaneousRenderTargets,
	const FRHIRenderTargetView* NewRenderTargetsRHI,
	FTextureRHIParamRef NewDepthStencilTargetRHI,
	uint32 NumUAVs,
	const FUnorderedAccessViewRHIParamRef* UAVs
	)
{
	VERIFY_GL_SCOPE();
	check(NumSimultaneousRenderTargets <= MaxSimultaneousRenderTargets);
	check(NumUAVs == 0);

	FMemory::Memset(PendingState.RenderTargets,0,sizeof(PendingState.RenderTargets));
	FMemory::Memset(PendingState.RenderTargetMipmapLevels,0,sizeof(PendingState.RenderTargetMipmapLevels));
	FMemory::Memset(PendingState.RenderTargetArrayIndex,0,sizeof(PendingState.RenderTargetArrayIndex));
	PendingState.FirstNonzeroRenderTarget = -1;

	for( int32 RenderTargetIndex = NumSimultaneousRenderTargets - 1; RenderTargetIndex >= 0; --RenderTargetIndex )
	{
		PendingState.RenderTargets[RenderTargetIndex] = GetOpenGLTextureFromRHITexture(NewRenderTargetsRHI[RenderTargetIndex].Texture);
		PendingState.RenderTargetMipmapLevels[RenderTargetIndex] = NewRenderTargetsRHI[RenderTargetIndex].MipIndex;
		PendingState.RenderTargetArrayIndex[RenderTargetIndex] = NewRenderTargetsRHI[RenderTargetIndex].ArraySliceIndex;

		if( PendingState.RenderTargets[RenderTargetIndex] )
		{
			PendingState.FirstNonzeroRenderTarget = (int32)RenderTargetIndex;
		}
	}

	FOpenGLTextureBase* NewDepthStencilRT = GetOpenGLTextureFromRHITexture(NewDepthStencilTargetRHI);

	if (IsES2Platform(GRHIShaderPlatform))
	{
		// @todo-mobile

		FOpenGLContextState& ContextState = GetContextStateForCurrentContext();
		GLuint NewColorRT = PendingState.RenderTargets[0] ? PendingState.RenderTargets[0]->Resource : 0;
		// If the color buffer did not change and we are disabling depth, do not switch depth and assume 
		// the high level will disable depth test/write (so we can avoid a logical buffer store);
		// if both are set to nothing, then it's an endframe so we don't want to switch either...
		if (NewDepthStencilRT == NULL && PendingState.DepthStencil != NULL)
		{
#if PLATFORM_ANDROID
			//color RT being 0 means backbuffer is being used. Hence taking only comparison with previous RT into consideration. Fixes black screen issue.
			if ( ContextState.LastES2ColorRT == NewColorRT)
#else
			if (NewColorRT == 0 || ContextState.LastES2ColorRT == NewColorRT)
#endif
			{
				return;
			}
			else
			{
				ContextState.LastES2ColorRT = NewColorRT;
				ContextState.LastES2DepthRT = NewDepthStencilRT ? NewDepthStencilRT->Resource : 0;
			}
		}
		else
		{
			ContextState.LastES2ColorRT = NewColorRT;
			ContextState.LastES2DepthRT = NewDepthStencilRT ? NewDepthStencilRT->Resource : 0;
		}
	}
	PendingState.DepthStencil = NewDepthStencilRT;

	if (PendingState.FirstNonzeroRenderTarget == -1 && !PendingState.DepthStencil)
	{
		// Special case - invalid setup, but sometimes performed by the engine

		PendingState.Framebuffer = 0;
		PendingState.bFramebufferSetupInvalid = true;
		return;
	}

	PendingState.Framebuffer = GetOpenGLFramebuffer(NumSimultaneousRenderTargets, PendingState.RenderTargets, PendingState.RenderTargetArrayIndex, PendingState.RenderTargetMipmapLevels, PendingState.DepthStencil);
	PendingState.bFramebufferSetupInvalid = false;

	if (PendingState.FirstNonzeroRenderTarget != -1)
	{
		// Set viewport size to new render target size. (I see D3D11 doesn't set it in case of depth buffers with no render targets, so doing the same here).
		PendingState.Viewport.Min.X = 0;
		PendingState.Viewport.Min.Y = 0;

		uint32 Width = 0;
		uint32 Height = 0;

		FOpenGLTexture2D* NewRenderTarget2D = (FOpenGLTexture2D*)NewRenderTargetsRHI[PendingState.FirstNonzeroRenderTarget].Texture->GetTexture2D();
		if(NewRenderTarget2D)
		{
			Width = NewRenderTarget2D->GetSizeX();
			Height = NewRenderTarget2D->GetSizeY();
		}
		else
		{
			FOpenGLTextureCube* NewRenderTargetCube = (FOpenGLTextureCube*)NewRenderTargetsRHI[PendingState.FirstNonzeroRenderTarget].Texture->GetTextureCube();
			if(NewRenderTargetCube)
			{
				Width = NewRenderTargetCube->GetSize();
				Height = NewRenderTargetCube->GetSize();
			}
			else
			{
				FOpenGLTexture3D* NewRenderTarget3D = (FOpenGLTexture3D*)NewRenderTargetsRHI[PendingState.FirstNonzeroRenderTarget].Texture->GetTexture3D();
				if(NewRenderTarget3D)
				{
					Width = NewRenderTarget3D->GetSizeX();
					Height = NewRenderTarget3D->GetSizeY();
				}
				else
				{
					FOpenGLTexture2DArray* NewRenderTarget2DArray = (FOpenGLTexture2DArray*)NewRenderTargetsRHI[PendingState.FirstNonzeroRenderTarget].Texture->GetTexture2DArray();
					if(NewRenderTarget3D)
					{
						Width = NewRenderTarget2DArray->GetSizeX();
						Height = NewRenderTarget2DArray->GetSizeY();
					}
					else
					{
						check(0);
					}
				}
			}
		}

		{
			uint32 MipIndex = NewRenderTargetsRHI[PendingState.FirstNonzeroRenderTarget].MipIndex;
			Width = FMath::Max<uint32>(1,(Width >> MipIndex));
			Height = FMath::Max<uint32>(1,(Height >> MipIndex));
		}

		PendingState.Viewport.Max.X = PendingState.RenderTargetWidth = Width;
		PendingState.Viewport.Max.Y = PendingState.RenderTargetHeight = Height;
	}
}

void FOpenGLDynamicRHI::RHIDiscardRenderTargets(bool Depth, bool Stencil, uint32 ColorBitMask)
{
	if(FOpenGL::SupportsDiscardFrameBuffer())
	{
		// 8 Color + Depth + Stencil = 10
		GLenum Attachments[10];
		uint32 I=0;
		if(Depth) 
		{
			Attachments[I] = GL_DEPTH_ATTACHMENT;
			I++;
		}
		if(Stencil) 
		{
			Attachments[I] = GL_STENCIL_ATTACHMENT;
			I++;
		}

		ColorBitMask &= (1 << 8) - 1;
		uint32 J = 0;
		while (ColorBitMask)
		{
			if(ColorBitMask & 1)
			{
				Attachments[I] = GL_COLOR_ATTACHMENT0 + J;
				I++;
			}

			ColorBitMask >>= 1;
			++J;
		}
		FOpenGL::DiscardFramebufferEXT(GL_FRAMEBUFFER, I, Attachments);
	}
}

// Primitive drawing.

void FOpenGLDynamicRHI::EnableVertexElementCached(
	FOpenGLContextState& ContextState,
	const FOpenGLVertexElement &VertexElement,
	GLsizei Stride,
	void *Pointer,
	GLuint Buffer)
{
	VERIFY_GL_SCOPE();

	GLuint AttributeIndex = VertexElement.AttributeIndex;
	FOpenGLCachedAttr &Attr = ContextState.VertexAttrs[AttributeIndex];

	if (!Attr.bEnabled)
	{
		glEnableVertexAttribArray(AttributeIndex);
		Attr.bEnabled = true;
	}

	if (
		(Attr.Pointer != Pointer) ||
		(Attr.Buffer != Buffer) ||
		(Attr.Size != VertexElement.Size) ||
		(Attr.Divisor != VertexElement.Divisor) ||
		(Attr.Type != VertexElement.Type) ||
		(Attr.bNormalized != VertexElement.bNormalized) ||
		(Attr.Stride != Stride))
	{
		CachedBindArrayBuffer(ContextState, Buffer);
		if( !VertexElement.bShouldConvertToFloat )
		{
			FOpenGL::VertexAttribIPointer(
				AttributeIndex,
				VertexElement.Size,
				VertexElement.Type,
				Stride,
				Pointer
				);
		}
		else
		{
			FOpenGL::VertexAttribPointer(
				AttributeIndex,
				VertexElement.Size,
				VertexElement.Type,
				VertexElement.bNormalized,
				Stride,
				Pointer
				);
		}
		FOpenGL::VertexAttribDivisor(AttributeIndex, VertexElement.Divisor);

		Attr.Pointer = Pointer;
		Attr.Buffer = Buffer;
		Attr.Size = VertexElement.Size;
		Attr.Divisor = VertexElement.Divisor;
		Attr.Type = VertexElement.Type;
		Attr.bNormalized = VertexElement.bNormalized;
		Attr.Stride = Stride;
	}
}

void FOpenGLDynamicRHI::EnableVertexElementCachedZeroStride(FOpenGLContextState& ContextState, const FOpenGLVertexElement& VertexElement, uint32 NumVertices, FOpenGLVertexBuffer* ZeroStrideVertexBuffer)
{
	VERIFY_GL_SCOPE();

	GLuint AttributeIndex = VertexElement.AttributeIndex;
	FOpenGLCachedAttr &Attr = ContextState.VertexAttrs[AttributeIndex];
	uint32 Stride = ZeroStrideVertexBuffer->GetSize();

	FOpenGLVertexBuffer* ExpandedVertexBuffer = FindExpandedZeroStrideBuffer(ZeroStrideVertexBuffer, Stride, NumVertices, VertexElement);
	EnableVertexElementCached(ContextState, VertexElement, Stride, 0, ExpandedVertexBuffer->Resource);
}

void FOpenGLDynamicRHI::FreeZeroStrideBuffers()
{
	// Forces releasing references to expanded zero stride vertex buffers
	ZeroStrideExpandedBuffersList.Empty();
}

void FOpenGLDynamicRHI::SetupVertexArrays(FOpenGLContextState& ContextState, uint32 BaseVertexIndex, FOpenGLRHIState::FOpenGLStream* Streams, uint32 NumStreams, uint32 MaxVertices)
{
	VERIFY_GL_SCOPE();
	bool UsedAttributes[NUM_OPENGL_VERTEX_STREAMS] = { 0 };

	check(IsValidRef(PendingState.BoundShaderState));
	check(IsValidRef(PendingState.BoundShaderState->VertexShader));
	FOpenGLVertexDeclaration* VertexDeclaration = PendingState.BoundShaderState->VertexDeclaration;
	uint32 AttributeMask = PendingState.BoundShaderState->VertexShader->Bindings.InOutMask;
	for (int32 ElementIndex = 0; ElementIndex < VertexDeclaration->VertexElements.Num(); ElementIndex++)
	{
		FOpenGLVertexElement& VertexElement = VertexDeclaration->VertexElements[ElementIndex];

		if ( VertexElement.StreamIndex < NumStreams)
		{
			FOpenGLRHIState::FOpenGLStream* Stream = &Streams[VertexElement.StreamIndex];
			uint32 Stride = Stream->Stride;

			uint32 AttributeBit = (1 << VertexElement.AttributeIndex);
			if ((AttributeBit & AttributeMask) == AttributeBit)
			{
				if( Stream->VertexBuffer->GetUsage() & BUF_ZeroStride )
				{
					check(Stride == 0);
					check(Stream->Offset == 0);
					check(VertexElement.Offset == 0);
					check(Stream->VertexBuffer->GetZeroStrideBuffer());
					EnableVertexElementCachedZeroStride(
						ContextState,
						VertexElement,
						MaxVertices,
						Stream->VertexBuffer
						);
				}
				else
				{
					check( Stride > 0 );
					EnableVertexElementCached(
						ContextState,
						VertexElement,
						Stride,
						INDEX_TO_VOID(BaseVertexIndex * Stride + Stream->Offset + VertexElement.Offset),
						Stream->VertexBuffer->Resource
						);
				}
				UsedAttributes[VertexElement.AttributeIndex] = true;
			}
		}
		else
		{
			//workaround attributes with no streams
			uint32 AttributeBit = (1 << VertexElement.AttributeIndex);
			if ((AttributeBit & AttributeMask) == AttributeBit)
			{
				VERIFY_GL_SCOPE();

				GLuint AttributeIndex = VertexElement.AttributeIndex;
				FOpenGLCachedAttr &Attr = ContextState.VertexAttrs[AttributeIndex];

				if (Attr.bEnabled)
				{
					glDisableVertexAttribArray(AttributeIndex);
					Attr.bEnabled = false;
				}

				float data[4] = { 0.0f};

				glVertexAttrib4fv( AttributeIndex, data );
			}
		}
	}

	// Disable remaining vertex arrays
	for (GLuint AttribIndex = 0; AttribIndex < NUM_OPENGL_VERTEX_STREAMS; AttribIndex++)
	{
		if (UsedAttributes[AttribIndex] == false && ContextState.VertexAttrs[AttribIndex].bEnabled)
		{
			glDisableVertexAttribArray(AttribIndex);
			ContextState.VertexAttrs[AttribIndex].bEnabled = false;
		}
	}
}

// Used by default on ES2 for immediate mode rendering.
void FOpenGLDynamicRHI::SetupVertexArraysUP(FOpenGLContextState& ContextState, void* Buffer, uint32 Stride)
{
	VERIFY_GL_SCOPE();
	bool UsedAttributes[NUM_OPENGL_VERTEX_STREAMS] = { 0 };

	check(IsValidRef(PendingState.BoundShaderState));
	check(IsValidRef(PendingState.BoundShaderState->VertexShader));
	FOpenGLVertexDeclaration* VertexDeclaration = PendingState.BoundShaderState->VertexDeclaration;
	uint32 AttributeMask = PendingState.BoundShaderState->VertexShader->Bindings.InOutMask;
	for (int32 ElementIndex = 0; ElementIndex < VertexDeclaration->VertexElements.Num(); ElementIndex++)
	{
		FOpenGLVertexElement &VertexElement = VertexDeclaration->VertexElements[ElementIndex];
		check(VertexElement.StreamIndex < 1);

		uint32 AttributeBit = (1 << VertexElement.AttributeIndex);
		if ((AttributeBit & AttributeMask) == AttributeBit)
		{
			check( Stride > 0 );
			EnableVertexElementCached(
				ContextState,
				VertexElement,
				Stride,
				(void*)(((char*)Buffer) + VertexElement.Offset),
				0
				);
			UsedAttributes[VertexElement.AttributeIndex] = true;
		}
	}

	// Disable remaining vertex arrays
	for (GLuint AttribIndex = 0; AttribIndex < NUM_OPENGL_VERTEX_STREAMS; AttribIndex++)
	{
		if (UsedAttributes[AttribIndex] == false && ContextState.VertexAttrs[AttribIndex].bEnabled)
		{
			glDisableVertexAttribArray(AttribIndex);
			ContextState.VertexAttrs[AttribIndex].bEnabled = false;
		}
	}
}

void FOpenGLDynamicRHI::OnProgramDeletion( GLint ProgramResource )
{
	if( SharedContextState.Program == ProgramResource )
	{
		SharedContextState.Program = -1;
	}

	if( RenderingContextState.Program == ProgramResource )
	{
		RenderingContextState.Program = -1;
	}
}

void FOpenGLDynamicRHI::OnVertexBufferDeletion( GLuint VertexBufferResource )
{
	if (SharedContextState.ArrayBufferBound == VertexBufferResource)
	{
		SharedContextState.ArrayBufferBound = -1;	// will force refresh
	}

	if (RenderingContextState.ArrayBufferBound == VertexBufferResource)
	{
		RenderingContextState.ArrayBufferBound = -1;	// will force refresh
	}

	for (GLuint AttribIndex = 0; AttribIndex < NUM_OPENGL_VERTEX_STREAMS; AttribIndex++)
	{
		if( SharedContextState.VertexAttrs[AttribIndex].Buffer == VertexBufferResource )
		{
			SharedContextState.VertexAttrs[AttribIndex].Pointer = FOpenGLCachedAttr_Invalid;	// that'll enforce state update on next cache test
		}

		if( RenderingContextState.VertexAttrs[AttribIndex].Buffer == VertexBufferResource )
		{
			RenderingContextState.VertexAttrs[AttribIndex].Pointer = FOpenGLCachedAttr_Invalid;	// that'll enforce state update on next cache test
		}
	}
}

void FOpenGLDynamicRHI::OnIndexBufferDeletion( GLuint IndexBufferResource )
{
	if (SharedContextState.ElementArrayBufferBound == IndexBufferResource)
	{
		SharedContextState.ElementArrayBufferBound = -1;	// will force refresh
	}

	if (RenderingContextState.ElementArrayBufferBound == IndexBufferResource)
	{
		RenderingContextState.ElementArrayBufferBound = -1;	// will force refresh
	}
}

void FOpenGLDynamicRHI::OnPixelBufferDeletion( GLuint PixelBufferResource )
{
	if (SharedContextState.PixelUnpackBufferBound == PixelBufferResource)
	{
		SharedContextState.PixelUnpackBufferBound = -1;	// will force refresh
	}

	if (RenderingContextState.PixelUnpackBufferBound == PixelBufferResource)
	{
		RenderingContextState.PixelUnpackBufferBound = -1;	// will force refresh
	}
}

extern void AddNewlyFreedBufferToUniformBufferPool( GLuint Buffer, uint32 BufferSize, bool bStreamDraw );

void FOpenGLDynamicRHI::OnUniformBufferDeletion( GLuint UniformBufferResource, uint32 AllocatedSize, bool bStreamDraw )
{
	if (SharedContextState.UniformBufferBound == UniformBufferResource)
	{
		SharedContextState.UniformBufferBound = -1;	// will force refresh
	}

	if (RenderingContextState.UniformBufferBound == UniformBufferResource)
	{
		RenderingContextState.UniformBufferBound = -1;	// will force refresh
	}

	for (GLuint UniformBufferIndex = 0; UniformBufferIndex < OGL_NUM_SHADER_STAGES*OGL_MAX_UNIFORM_BUFFER_BINDINGS; UniformBufferIndex++)
	{
		if( SharedContextState.UniformBuffers[UniformBufferIndex] == UniformBufferResource )
		{
			SharedContextState.UniformBuffers[UniformBufferIndex] = FOpenGLCachedUniformBuffer_Invalid;	// that'll enforce state update on next cache test
		}

		if( RenderingContextState.UniformBuffers[UniformBufferIndex] == UniformBufferResource )
		{
			RenderingContextState.UniformBuffers[UniformBufferIndex] = FOpenGLCachedUniformBuffer_Invalid;	// that'll enforce state update on next cache test
		}
	}

	AddNewlyFreedBufferToUniformBufferPool( UniformBufferResource, AllocatedSize, bStreamDraw );
}

void FOpenGLDynamicRHI::CommitNonComputeShaderConstants()
{
	VERIFY_GL_SCOPE();

	FOpenGLLinkedProgram* LinkedProgram = PendingState.BoundShaderState->LinkedProgram;
	if (IsES2Platform(GRHIShaderPlatform))
	{
		PendingState.ShaderParameters[OGL_SHADER_STAGE_VERTEX].CommitPackedUniformBuffers(LinkedProgram, OGL_SHADER_STAGE_VERTEX, PendingState.BoundShaderState->VertexShader->BoundUniformBuffers, PendingState.BoundShaderState->VertexShader->UniformBuffersCopyInfo);
	}
	PendingState.ShaderParameters[OGL_SHADER_STAGE_VERTEX].CommitPackedGlobals(LinkedProgram, OGL_SHADER_STAGE_VERTEX);

	if (IsES2Platform(GRHIShaderPlatform))
	{
		PendingState.ShaderParameters[OGL_SHADER_STAGE_PIXEL].CommitPackedUniformBuffers(LinkedProgram, OGL_SHADER_STAGE_PIXEL, PendingState.BoundShaderState->PixelShader->BoundUniformBuffers, PendingState.BoundShaderState->PixelShader->UniformBuffersCopyInfo);
	}
	PendingState.ShaderParameters[OGL_SHADER_STAGE_PIXEL].CommitPackedGlobals(LinkedProgram, OGL_SHADER_STAGE_PIXEL);

	if (PendingState.BoundShaderState->GeometryShader)
	{
		if (IsES2Platform(GRHIShaderPlatform))
		{
			PendingState.ShaderParameters[OGL_SHADER_STAGE_GEOMETRY].CommitPackedUniformBuffers(LinkedProgram, OGL_SHADER_STAGE_GEOMETRY, PendingState.BoundShaderState->GeometryShader->BoundUniformBuffers, PendingState.BoundShaderState->GeometryShader->UniformBuffersCopyInfo);
		}
		PendingState.ShaderParameters[OGL_SHADER_STAGE_GEOMETRY].CommitPackedGlobals(LinkedProgram, OGL_SHADER_STAGE_GEOMETRY);
	}
}


void FOpenGLDynamicRHI::CommitComputeShaderConstants(FComputeShaderRHIParamRef ComputeShaderRHI)
{
	VERIFY_GL_SCOPE();
	check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);

	DYNAMIC_CAST_OPENGLRESOURCE(ComputeShader,ComputeShader);
	const int32 Stage = OGL_SHADER_STAGE_COMPUTE;

	FOpenGLShaderParameterCache& StageShaderParameters = PendingState.ShaderParameters[OGL_SHADER_STAGE_COMPUTE];
	StageShaderParameters.CommitPackedGlobals(ComputeShader->LinkedProgram, OGL_SHADER_STAGE_COMPUTE);
}


void FOpenGLDynamicRHI::RHIDrawPrimitive(uint32 PrimitiveType,uint32 BaseVertexIndex,uint32 NumPrimitives,uint32 NumInstances)
{
	VERIFY_GL_SCOPE();

	RHI_DRAW_CALL_STATS(PrimitiveType,NumPrimitives*NumInstances);

	FOpenGLContextState& ContextState = GetContextStateForCurrentContext();
	BindPendingFramebuffer(ContextState);
	SetPendingBlendStateForActiveRenderTargets(ContextState);
	UpdateViewportInOpenGLContext(ContextState);
	UpdateScissorRectInOpenGLContext(ContextState);
	UpdateRasterizerStateInOpenGLContext(ContextState);
	UpdateDepthStencilStateInOpenGLContext(ContextState);
	BindPendingShaderState(ContextState);
	SetupTexturesForDraw(ContextState);
	CommitNonComputeShaderConstants();
	CachedBindElementArrayBuffer(ContextState,0);
	uint32 VertexCount = RHIGetVertexCountForPrimitiveCount(NumPrimitives,PrimitiveType);
	SetupVertexArrays(ContextState, BaseVertexIndex, PendingState.Streams, VertexCount, NUM_OPENGL_VERTEX_STREAMS);

	GLenum DrawMode = GL_TRIANGLES;
	GLsizei NumElements = 0;
	GLint PatchSize = 0;
	FindPrimitiveType(PrimitiveType, NumPrimitives, DrawMode, NumElements, PatchSize);

	if (FOpenGL::SupportsTessellation() && DrawMode == GL_PATCHES )
	{
		FOpenGL::PatchParameteri(GL_PATCH_VERTICES, PatchSize);
	}

	GPUProfilingData.RegisterGPUWork(NumPrimitives * NumInstances, VertexCount * NumInstances);
	if (NumInstances == 1)
	{
		glDrawArrays(DrawMode, 0, NumElements);
		REPORT_GL_DRAW_ARRAYS_EVENT_FOR_FRAME_DUMP( DrawMode, 0, NumElements );
	}
	else
	{
		check( FOpenGL::SupportsInstancing() );
		FOpenGL::DrawArraysInstanced(DrawMode, 0, NumElements, NumInstances);
		REPORT_GL_DRAW_ARRAYS_INSTANCED_EVENT_FOR_FRAME_DUMP( DrawMode, 0, NumElements, NumInstances );
	}
}

void FOpenGLDynamicRHI::RHIDrawPrimitiveIndirect(uint32 PrimitiveType,FVertexBufferRHIParamRef ArgumentBufferRHI,uint32 ArgumentOffset)
{
	UE_LOG(LogRHI, Fatal,TEXT("OpenGL RHI does not yet support indirect draw calls."));

	GPUProfilingData.RegisterGPUWork(0);

}

void FOpenGLDynamicRHI::RHIDrawIndexedIndirect(FIndexBufferRHIParamRef IndexBufferRHI, uint32 PrimitiveType, FStructuredBufferRHIParamRef ArgumentsBufferRHI, int32 DrawArgumentsIndex, uint32 NumInstances)
{
	check(0);
	GPUProfilingData.RegisterGPUWork(1);
}

void FOpenGLDynamicRHI::RHIDrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBufferRHI,uint32 PrimitiveType,int32 BaseVertexIndex,uint32 MinIndex,uint32 NumVertices,uint32 StartIndex,uint32 NumPrimitives,uint32 NumInstances)
{
	VERIFY_GL_SCOPE();

	DYNAMIC_CAST_OPENGLRESOURCE(IndexBuffer,IndexBuffer);

	RHI_DRAW_CALL_STATS(PrimitiveType,NumPrimitives*NumInstances);

	FOpenGLContextState& ContextState = GetContextStateForCurrentContext();
	BindPendingFramebuffer(ContextState);
	SetPendingBlendStateForActiveRenderTargets(ContextState);
	UpdateViewportInOpenGLContext(ContextState);
	UpdateScissorRectInOpenGLContext(ContextState);
	UpdateRasterizerStateInOpenGLContext(ContextState);
	UpdateDepthStencilStateInOpenGLContext(ContextState);
	BindPendingShaderState(ContextState);
	SetupTexturesForDraw(ContextState);
	CommitNonComputeShaderConstants();
	CachedBindElementArrayBuffer(ContextState,IndexBuffer->Resource);
	SetupVertexArrays(ContextState, BaseVertexIndex, PendingState.Streams, NUM_OPENGL_VERTEX_STREAMS, NumVertices + StartIndex);

	GLenum DrawMode = GL_TRIANGLES;
	GLsizei NumElements = 0;
	GLint PatchSize = 0;
	FindPrimitiveType(PrimitiveType, NumPrimitives, DrawMode, NumElements, PatchSize);

	if (FOpenGL::SupportsTessellation() && DrawMode == GL_PATCHES )
	{
		FOpenGL::PatchParameteri(GL_PATCH_VERTICES, PatchSize);
	}

	GLenum IndexType = IndexBuffer->GetStride() == sizeof(uint32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
	StartIndex *= IndexBuffer->GetStride() == sizeof(uint32) ? sizeof(uint32) : sizeof(uint16);

	GPUProfilingData.RegisterGPUWork(NumPrimitives * NumInstances, NumElements * NumInstances);
	if (NumInstances > 1)
	{
		check( FOpenGL::SupportsInstancing() );
		FOpenGL::DrawElementsInstanced(DrawMode, NumElements, IndexType, INDEX_TO_VOID(StartIndex), NumInstances);
		REPORT_GL_DRAW_ELEMENTS_INSTANCED_EVENT_FOR_FRAME_DUMP(DrawMode, NumElements, IndexType, (void *)StartIndex, NumInstances);
	}
	else
	{
		if ( FOpenGL::SupportsDrawIndexOffset() )
		{
			FOpenGL::DrawRangeElements(DrawMode, MinIndex, MinIndex + NumVertices, NumElements, IndexType, INDEX_TO_VOID(StartIndex));
		}
		else
		{
			glDrawElements(DrawMode, NumElements, IndexType, INDEX_TO_VOID(StartIndex));
		}
		REPORT_GL_DRAW_RANGE_ELEMENTS_EVENT_FOR_FRAME_DUMP(DrawMode, MinIndex, MinIndex + NumVertices, NumElements, IndexType, (void *)StartIndex);
	}
}

void FOpenGLDynamicRHI::RHIDrawIndexedPrimitiveIndirect(uint32 PrimitiveType,FIndexBufferRHIParamRef IndexBuffer,FVertexBufferRHIParamRef ArgumentBufferRHI,uint32 ArgumentOffset)
{
	UE_LOG(LogRHI, Fatal,TEXT("OpenGL RHI does not yet support indirect draw calls."));
	
	GPUProfilingData.RegisterGPUWork(0);
}

/**
 * Preallocate memory or get a direct command stream pointer to fill up for immediate rendering . This avoids memcpys below in DrawPrimitiveUP
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param NumPrimitives The number of primitives in the VertexData buffer
 * @param NumVertices The number of vertices to be written
 * @param VertexDataStride Size of each vertex 
 * @param OutVertexData Reference to the allocated vertex memory
 */
void FOpenGLDynamicRHI::RHIBeginDrawPrimitiveUP( uint32 PrimitiveType, uint32 NumPrimitives, uint32 NumVertices, uint32 VertexDataStride, void*& OutVertexData)
{
	VERIFY_GL_SCOPE();
	check(PendingState.NumPrimitives == 0);

	if(FOpenGL::SupportsFastBufferData()) 
	{
		OutVertexData = DynamicVertexBuffers.Lock(NumVertices * VertexDataStride);
	}
	else 
	{
		uint32 BytesVertex = NumVertices * VertexDataStride;
		if(BytesVertex > PendingState.UpVertexBufferBytes) 
		{
			if(PendingState.UpVertexBuffer) 
			{
				FMemory::Free(PendingState.UpVertexBuffer);
			}
			PendingState.UpVertexBuffer = FMemory::Malloc(BytesVertex);
			PendingState.UpVertexBufferBytes = BytesVertex;
		}
		OutVertexData = PendingState.UpVertexBuffer;
		PendingState.UpStride = VertexDataStride;
	}

	PendingState.PrimitiveType = PrimitiveType;
	PendingState.NumPrimitives = NumPrimitives;
	PendingState.NumVertices = NumVertices;
	if(FOpenGL::SupportsFastBufferData())
	{
		PendingState.DynamicVertexStream.VertexBuffer = DynamicVertexBuffers.GetPendingBuffer();
		PendingState.DynamicVertexStream.Offset = DynamicVertexBuffers.GetPendingOffset();
		PendingState.DynamicVertexStream.Stride = VertexDataStride;
	}
	else
	{
		PendingState.DynamicVertexStream.VertexBuffer = 0;
		PendingState.DynamicVertexStream.Offset = 0;
		PendingState.DynamicVertexStream.Stride = VertexDataStride;
	}
}

/**
 * Draw a primitive using the vertex data populated since RHIBeginDrawPrimitiveUP and clean up any memory as needed
 */
void FOpenGLDynamicRHI::RHIEndDrawPrimitiveUP()
{
	VERIFY_GL_SCOPE();
	check(PendingState.NumPrimitives != 0);

	RHI_DRAW_CALL_STATS(PendingState.PrimitiveType,PendingState.NumPrimitives);

	GLenum DrawMode = GL_TRIANGLES;
	GLsizei NumElements = 0;
	GLint PatchSize = 0;
	FindPrimitiveType(PendingState.PrimitiveType, PendingState.NumPrimitives, DrawMode, NumElements, PatchSize);

	if (FOpenGL::SupportsTessellation() && DrawMode == GL_PATCHES )
	{
		FOpenGL::PatchParameteri(GL_PATCH_VERTICES, PatchSize);
	}

	if(FOpenGL::SupportsFastBufferData()) 
	{
		DynamicVertexBuffers.Unlock();
	}

	FOpenGLContextState& ContextState = GetContextStateForCurrentContext();
	BindPendingFramebuffer(ContextState);
	SetPendingBlendStateForActiveRenderTargets(ContextState);
	UpdateViewportInOpenGLContext(ContextState);
	UpdateScissorRectInOpenGLContext(ContextState);
	UpdateRasterizerStateInOpenGLContext(ContextState);
	UpdateDepthStencilStateInOpenGLContext(ContextState);
	BindPendingShaderState(ContextState);
	SetupTexturesForDraw(ContextState);
	CommitNonComputeShaderConstants();
	CachedBindElementArrayBuffer(ContextState,0);
	if(FOpenGL::SupportsFastBufferData()) 
	{
		SetupVertexArrays(ContextState, 0, &PendingState.DynamicVertexStream, 1, PendingState.NumVertices);
	}
	else
	{
		SetupVertexArraysUP(ContextState, PendingState.UpVertexBuffer, PendingState.UpStride);
	}

	GPUProfilingData.RegisterGPUWork(PendingState.NumPrimitives,PendingState.NumVertices);
	glDrawArrays(DrawMode, 0, NumElements);

	PendingState.NumPrimitives = 0;

	REPORT_GL_DRAW_ARRAYS_EVENT_FOR_FRAME_DUMP( DrawMode, 0, NumElements );
}

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
void FOpenGLDynamicRHI::RHIBeginDrawIndexedPrimitiveUP( uint32 PrimitiveType, uint32 NumPrimitives, uint32 NumVertices, uint32 VertexDataStride, void*& OutVertexData, uint32 MinVertexIndex, uint32 NumIndices, uint32 IndexDataStride, void*& OutIndexData)
{
	check(PendingState.NumPrimitives == 0);
	check((sizeof(uint16) == IndexDataStride) || (sizeof(uint32) == IndexDataStride));

	if(FOpenGL::SupportsFastBufferData()) 
	{
		OutVertexData = DynamicVertexBuffers.Lock(NumVertices * VertexDataStride);
		OutIndexData = DynamicIndexBuffers.Lock(NumIndices * IndexDataStride);
	}
	else
	{
		uint32 BytesVertex = NumVertices * VertexDataStride;
		if(BytesVertex > PendingState.UpVertexBufferBytes) 
		{
			if(PendingState.UpVertexBuffer) 
			{
				FMemory::Free(PendingState.UpVertexBuffer);
			}
			PendingState.UpVertexBuffer = FMemory::Malloc(BytesVertex);
			PendingState.UpVertexBufferBytes = BytesVertex;
		}
		OutVertexData = PendingState.UpVertexBuffer;
		PendingState.UpStride = VertexDataStride;
		uint32 BytesIndex = NumIndices * IndexDataStride;
		if(BytesIndex > PendingState.UpIndexBufferBytes)
		{
			if(PendingState.UpIndexBuffer) 
			{
				FMemory::Free(PendingState.UpIndexBuffer);
			}
			PendingState.UpIndexBuffer = FMemory::Malloc(BytesIndex);
			PendingState.UpIndexBufferBytes = BytesIndex;
		}
		OutIndexData = PendingState.UpIndexBuffer;
	}

	PendingState.PrimitiveType = PrimitiveType;
	PendingState.NumPrimitives = NumPrimitives;
	PendingState.MinVertexIndex = MinVertexIndex;
	PendingState.IndexDataStride = IndexDataStride;
	PendingState.NumVertices = NumVertices;
	if(FOpenGL::SupportsFastBufferData()) 
	{
		PendingState.DynamicVertexStream.VertexBuffer = DynamicVertexBuffers.GetPendingBuffer();
		PendingState.DynamicVertexStream.Offset = DynamicVertexBuffers.GetPendingOffset();
		PendingState.DynamicVertexStream.Stride = VertexDataStride;
	}
	else
	{
		PendingState.DynamicVertexStream.VertexBuffer = 0;
		PendingState.DynamicVertexStream.Offset = 0;
		PendingState.DynamicVertexStream.Stride = VertexDataStride;
	}
}

/**
 * Draw a primitive using the vertex and index data populated since RHIBeginDrawIndexedPrimitiveUP and clean up any memory as needed
 */
void FOpenGLDynamicRHI::RHIEndDrawIndexedPrimitiveUP()
{
	VERIFY_GL_SCOPE();
	check(PendingState.NumPrimitives != 0);

	RHI_DRAW_CALL_STATS(PendingState.PrimitiveType,PendingState.NumPrimitives);

	if(FOpenGL::SupportsFastBufferData()) 
	{
		DynamicVertexBuffers.Unlock();
		DynamicIndexBuffers.Unlock();
	}

	FOpenGLContextState& ContextState = GetContextStateForCurrentContext();
	BindPendingFramebuffer(ContextState);
	SetPendingBlendStateForActiveRenderTargets(ContextState);
	UpdateViewportInOpenGLContext(ContextState);
	UpdateScissorRectInOpenGLContext(ContextState);
	UpdateRasterizerStateInOpenGLContext(ContextState);
	UpdateDepthStencilStateInOpenGLContext(ContextState);
	BindPendingShaderState(ContextState);
	SetupTexturesForDraw(ContextState);
	CommitNonComputeShaderConstants();
	if(FOpenGL::SupportsFastBufferData()) 
	{
		CachedBindElementArrayBuffer(ContextState,DynamicIndexBuffers.GetPendingBuffer()->Resource);
		SetupVertexArrays(ContextState, 0, &PendingState.DynamicVertexStream, 1, PendingState.NumVertices);
	}
	else
	{
		CachedBindElementArrayBuffer(ContextState,0);
		SetupVertexArraysUP(ContextState, PendingState.UpVertexBuffer, PendingState.UpStride);
	}

	GLenum DrawMode = GL_TRIANGLES;
	GLsizei NumElements = 0;
	GLint PatchSize = 0;
	FindPrimitiveType(PendingState.PrimitiveType, PendingState.NumPrimitives, DrawMode, NumElements, PatchSize);
	GLenum IndexType = (PendingState.IndexDataStride == sizeof(uint32)) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;

	if (FOpenGL::SupportsTessellation() && DrawMode == GL_PATCHES )
	{
		FOpenGL::PatchParameteri(GL_PATCH_VERTICES, PatchSize);
	}

	GPUProfilingData.RegisterGPUWork(PendingState.NumPrimitives,PendingState.NumVertices);
	if(FOpenGL::SupportsFastBufferData())
	{
		if ( FOpenGL::SupportsDrawIndexOffset() )
		{
			FOpenGL::DrawRangeElements(DrawMode, PendingState.MinVertexIndex, PendingState.MinVertexIndex + PendingState.NumVertices, NumElements, IndexType, INDEX_TO_VOID(DynamicIndexBuffers.GetPendingOffset()));
		}
		else
		{
			check(PendingState.MinVertexIndex == 0);
			glDrawElements(DrawMode, NumElements, IndexType, INDEX_TO_VOID(DynamicIndexBuffers.GetPendingOffset()));
		}
	}
	else
	{
		glDrawElements(DrawMode, NumElements, IndexType, PendingState.UpIndexBuffer);
	}

	PendingState.NumPrimitives = 0;

	REPORT_GL_DRAW_RANGE_ELEMENTS_EVENT_FOR_FRAME_DUMP( DrawMode, PendingState.MinVertexIndex, PendingState.MinVertexIndex + PendingState.NumVertices, NumElements, IndexType, 0 );
}


// Raster operations.
void FOpenGLDynamicRHI::RHIClear(bool bClearColor,const FLinearColor& Color,bool bClearDepth,float Depth,bool bClearStencil,uint32 Stencil, FIntRect ExcludeRect)
{
	FOpenGLDynamicRHI::RHIClearMRT(bClearColor, 1, &Color, bClearDepth, Depth, bClearStencil, Stencil, ExcludeRect);
}

static inline void ClearCurrentDepthStencilWithCurrentScissor( int8 ClearType, float Depth, uint32 Stencil )
{
	switch (ClearType)
	{
	case CT_DepthStencil:	// Clear depth and stencil
		FOpenGL::ClearBufferfi(GL_DEPTH_STENCIL, 0, Depth, Stencil);
		break;

	case CT_Stencil:	// Clear stencil only
		FOpenGL::ClearBufferiv(GL_STENCIL, 0, (const GLint*)&Stencil);
		break;

	case CT_Depth:	// Clear depth only
		FOpenGL::ClearBufferfv(GL_DEPTH, 0, &Depth);
		break;

	default:
		break;	// impossible anyway
	}
}

void FOpenGLDynamicRHI::ClearCurrentFramebufferWithCurrentScissor(FOpenGLContextState& ContextState, int8 ClearType, int32 NumClearColors, const FLinearColor* ClearColorArray, float Depth, uint32 Stencil)
{
	if ( FOpenGL::SupportsMultipleRenderTargets() )
	{
		// Clear color buffers
		if (ClearType & CT_Color)
		{
			for(int32 ColorIndex = 0; ColorIndex < NumClearColors; ++ColorIndex)
			{
				FOpenGL::ClearBufferfv( GL_COLOR, ColorIndex, (const GLfloat*)&ClearColorArray[ColorIndex] );
			}
		}

		if (ClearType & CT_DepthStencil)
		{
			ClearCurrentDepthStencilWithCurrentScissor(ClearType & CT_DepthStencil, Depth, Stencil);
		}
	}
	else
	{
		GLuint Mask = 0;
		if( ClearType & CT_Color && NumClearColors > 0 )
		{
			if (!ContextState.BlendState.RenderTargets[0].ColorWriteMaskR ||
				!ContextState.BlendState.RenderTargets[0].ColorWriteMaskG ||
				!ContextState.BlendState.RenderTargets[0].ColorWriteMaskB ||
				!ContextState.BlendState.RenderTargets[0].ColorWriteMaskA)
			{
				FOpenGL::ColorMaskIndexed(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				ContextState.BlendState.RenderTargets[0].ColorWriteMaskR = 1;
				ContextState.BlendState.RenderTargets[0].ColorWriteMaskG = 1;
				ContextState.BlendState.RenderTargets[0].ColorWriteMaskB = 1;
				ContextState.BlendState.RenderTargets[0].ColorWriteMaskA = 1;
			}

			if (ContextState.ClearColor != ClearColorArray[0])
			{
				glClearColor( ClearColorArray[0].R, ClearColorArray[0].G, ClearColorArray[0].B, ClearColorArray[0].A );
				ContextState.ClearColor = ClearColorArray[0];
			}
			Mask |= GL_COLOR_BUFFER_BIT;
		}
		if ( ClearType & CT_Depth )
		{
			if (!ContextState.DepthStencilState.bZWriteEnable)
			{
				glDepthMask(GL_TRUE);
				ContextState.DepthStencilState.bZWriteEnable = true;
			}
			if (ContextState.ClearDepth != Depth)
			{
				FOpenGL::ClearDepth( Depth );
				ContextState.ClearDepth = Depth;
			}
			Mask |= GL_DEPTH_BUFFER_BIT;
		}
		if ( ClearType & CT_Stencil )
		{
			if (ContextState.DepthStencilState.StencilWriteMask != 0xFFFFFFFF)
			{
				glStencilMask(0xFFFFFFFF);
				ContextState.DepthStencilState.StencilWriteMask = 0xFFFFFFFF;
			}

			if (ContextState.ClearStencil != Stencil)
			{
				glClearStencil( Stencil );
				ContextState.ClearStencil = Stencil;
			}
			Mask |= GL_STENCIL_BUFFER_BIT;
		}

		// do the clear
		glClear( Mask );
	}

	REPORT_GL_CLEAR_EVENT_FOR_FRAME_DUMP( ClearType, NumClearColors, (const float*)ClearColorArray, Depth, Stencil );
}

void FOpenGLDynamicRHI::RHIClearMRT(bool bClearColor,int32 NumClearColors,const FLinearColor* ClearColorArray,bool bClearDepth,float Depth,bool bClearStencil,uint32 Stencil, FIntRect ExcludeRect)
{
	VERIFY_GL_SCOPE();

	check((GRHIFeatureLevel >= ERHIFeatureLevel::SM5 ) || !PendingState.bFramebufferSetupInvalid);

	if (bClearColor)
	{
		// This is copied from DirectX11 code - apparently there's a silent assumption that there can be no valid render target set at index higher than an invalid one.
		int32 NumActiveRenderTargets = 0;
		for (int32 TargetIndex = 0; TargetIndex < MaxSimultaneousRenderTargets; TargetIndex++)
		{
			if (PendingState.RenderTargets[TargetIndex] != 0)
			{
				NumActiveRenderTargets++;
			}
			else
			{
				break;
			}
		}
		
		// Must specify enough clear colors for all active RTs
		check(NumClearColors >= NumActiveRenderTargets);
	}

	// Remember cached scissor state, and set one to cover viewport
	FIntRect PrevScissor = PendingState.Scissor;
	bool bPrevScissorEnabled = PendingState.bScissorEnabled;

	bool bClearAroundExcludeRect = false;
	if( ExcludeRect.Max.X > ExcludeRect.Min.X && ExcludeRect.Max.Y > ExcludeRect.Min.Y )
	{
		// ExcludeRect has some area
		bClearAroundExcludeRect = true;
		if( ExcludeRect.Min.X >= PendingState.Viewport.Max.X
			|| ExcludeRect.Min.Y >= PendingState.Viewport.Max.Y
			|| ExcludeRect.Max.X <= PendingState.Viewport.Min.X
			|| ExcludeRect.Max.Y <= PendingState.Viewport.Min.Y )
		{
			// but it's completely outside of viewport
			bClearAroundExcludeRect = false;
		}
		else if( ExcludeRect.Min.X <= PendingState.Viewport.Min.X
			&& ExcludeRect.Min.Y <= PendingState.Viewport.Min.Y
			&& ExcludeRect.Max.X >= PendingState.Viewport.Max.X
			&& ExcludeRect.Max.Y >= PendingState.Viewport.Max.Y )
		{
			// and it covers all we could clear, so there's nothing to clear
			return;
		}
	}

	bool bScissorChanged = false;
	GPUProfilingData.RegisterGPUWork(0);
	FOpenGLContextState& ContextState = GetContextStateForCurrentContext();
	BindPendingFramebuffer(ContextState);

	if( !bClearAroundExcludeRect )
	{
		if (bPrevScissorEnabled)
		{
			RHISetScissorRect(true,PrevScissor.Min.X, PrevScissor.Min.Y, PrevScissor.Max.X, PrevScissor.Max.Y);
			bScissorChanged = true;
		}
		else if (PendingState.Viewport.Min.X != 0 || PendingState.Viewport.Min.Y != 0 || PendingState.Viewport.Max.X != PendingState.RenderTargetWidth || PendingState.Viewport.Max.Y != PendingState.RenderTargetHeight)
		{
			RHISetScissorRect(true,PendingState.Viewport.Min.X, PendingState.Viewport.Min.Y, PendingState.Viewport.Max.X, PendingState.Viewport.Max.Y);
			bScissorChanged = true;
		}

		// Always update in case there are uncommitted changes to disable scissor
		UpdateScissorRectInOpenGLContext(ContextState);
	}

	int8 ClearType = CT_None;

	// Prepare color buffer masks, if applicable
	if (bClearColor)
	{
		ClearType |= CT_Color;

		for(int32 ColorIndex = 0; ColorIndex < NumClearColors; ++ColorIndex)
		{
			if( !ContextState.BlendState.RenderTargets[ColorIndex].ColorWriteMaskR ||
				!ContextState.BlendState.RenderTargets[ColorIndex].ColorWriteMaskG ||
				!ContextState.BlendState.RenderTargets[ColorIndex].ColorWriteMaskB ||
				!ContextState.BlendState.RenderTargets[ColorIndex].ColorWriteMaskA)
			{
				FOpenGL::ColorMaskIndexed(ColorIndex, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				ContextState.BlendState.RenderTargets[ColorIndex].ColorWriteMaskR = 1;
				ContextState.BlendState.RenderTargets[ColorIndex].ColorWriteMaskG = 1;
				ContextState.BlendState.RenderTargets[ColorIndex].ColorWriteMaskB = 1;
				ContextState.BlendState.RenderTargets[ColorIndex].ColorWriteMaskA = 1;
			}
		}
	}

	// Prepare depth mask, if applicable
	if (bClearDepth && PendingState.DepthStencil)
	{
		ClearType |= CT_Depth;

		if (!ContextState.DepthStencilState.bZWriteEnable)
		{
			glDepthMask(GL_TRUE);
			ContextState.DepthStencilState.bZWriteEnable = true;
		}
	}

	// Prepare stencil mask, if applicable
	if (bClearStencil && PendingState.DepthStencil)
	{
		ClearType |= CT_Stencil;

		if (ContextState.DepthStencilState.StencilWriteMask != 0xFFFFFFFF)
		{
			glStencilMask(0xFFFFFFFF);
			ContextState.DepthStencilState.StencilWriteMask = 0xFFFFFFFF;
		}
	}

	if ( bClearAroundExcludeRect )
	{
		// This requires clearing four rectangles, like this:
		// -----------------------------------------------
		// |            |                  |             |
		// |            |        3         |             |
		// |            |                  |             |
		// |            |------------------|             |
		// |            |XXXXXXXXXXXXXXXXXX|             |
		// |    1       |XXXXXXXXXXXXXXXXXX|     2       |
		// |            |XXXXXXXXXXXXXXXXXX|             |
		// |            |XXXXXXXXXXXXXXXXXX|             |
		// |            |XXXXXXXXXXXXXXXXXX|             |
		// |            |------------------|             |
		// |            |                  |             |
		// |            |         4        |             |
		// |            |                  |             |
		// -----------------------------------------------

		if( ExcludeRect.Min.X > PendingState.Viewport.Min.X )
		{
			// Rectangle 1
			RHISetScissorRect(true,PendingState.Viewport.Min.X, PendingState.Viewport.Min.Y, ExcludeRect.Min.X, PendingState.Viewport.Max.Y);
			UpdateScissorRectInOpenGLContext(ContextState);
			bScissorChanged = true;
			ClearCurrentFramebufferWithCurrentScissor(ContextState, ClearType, NumClearColors, ClearColorArray, Depth, Stencil);
		}

		if( ExcludeRect.Max.X < PendingState.Viewport.Max.X )
		{
			// Rectangle 2
			RHISetScissorRect(true,ExcludeRect.Max.X, PendingState.Viewport.Min.Y, PendingState.Viewport.Max.X, PendingState.Viewport.Max.Y);
			UpdateScissorRectInOpenGLContext(ContextState);
			bScissorChanged = true;
			ClearCurrentFramebufferWithCurrentScissor(ContextState,  ClearType, NumClearColors, ClearColorArray, Depth, Stencil);
		}

		if( ExcludeRect.Max.Y < PendingState.Viewport.Max.Y )
		{
			// Rectangle 3
			RHISetScissorRect(true,ExcludeRect.Min.X, ExcludeRect.Max.Y, ExcludeRect.Max.X, PendingState.Viewport.Max.Y);
			UpdateScissorRectInOpenGLContext(ContextState);
			bScissorChanged = true;
			ClearCurrentFramebufferWithCurrentScissor(ContextState,  ClearType, NumClearColors, ClearColorArray, Depth, Stencil);
		}

		if( PendingState.Viewport.Min.Y < ExcludeRect.Min.Y )
		{
			// Rectangle 4
			RHISetScissorRect(true,ExcludeRect.Min.X, PendingState.Viewport.Min.Y, ExcludeRect.Max.X, ExcludeRect.Min.Y);
			UpdateScissorRectInOpenGLContext(ContextState);
			bScissorChanged = true;
			ClearCurrentFramebufferWithCurrentScissor(ContextState,  ClearType, NumClearColors, ClearColorArray, Depth, Stencil);
		}
	}
	else
	{
		// Just one clear
		ClearCurrentFramebufferWithCurrentScissor(ContextState, ClearType, NumClearColors, ClearColorArray, Depth, Stencil);
	}

	if (bScissorChanged)
	{
		// Change it back
		RHISetScissorRect(bPrevScissorEnabled,PrevScissor.Min.X, PrevScissor.Min.Y, PrevScissor.Max.X, PrevScissor.Max.Y);
	}
}

// Functions to yield and regain rendering control from OpenGL

void FOpenGLDynamicRHI::RHISuspendRendering()
{
	// Not supported
}

void FOpenGLDynamicRHI::RHIResumeRendering()
{
	// Not supported
}

bool FOpenGLDynamicRHI::RHIIsRenderingSuspended()
{
	// Not supported
	return false;
}

// Blocks the CPU until the GPU catches up and goes idle.
void FOpenGLDynamicRHI::RHIBlockUntilGPUIdle()
{
	// Not really supported
}

/*
 * Returns the total GPU time taken to render the last frame. Same metric as FPlatformTime::Cycles().
 */
uint32 FOpenGLDynamicRHI::RHIGetGPUFrameCycles()
{
	return GGPUFrameTime;
}

void FOpenGLDynamicRHI::RHISetComputeShader(FComputeShaderRHIParamRef ComputeShaderRHI)
{
	if (OpenGLConsoleVariables::bSkipCompute)
	{
		return;
	}

	if ( FOpenGL::SupportsComputeShaders() )
	{
		VERIFY_GL_SCOPE();
		check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);

		PendingState.CurrentComputeShader = ComputeShaderRHI;

		DYNAMIC_CAST_OPENGLRESOURCE(ComputeShader,ComputeShader);
		FOpenGLContextState& ContextState = GetContextStateForCurrentContext();		
		BindPendingComputeShaderState(ContextState, ComputeShader);
	}
	else
	{
		UE_LOG(LogRHI,Fatal,TEXT("Platform doesn't support SM5 for OpenGL but set feature level to SM5"));
	}
}

void FOpenGLDynamicRHI::RHIDispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{ 
	if (OpenGLConsoleVariables::bSkipCompute)
	{
		return;
	}

	if ( FOpenGL::SupportsComputeShaders() )
	{
		VERIFY_GL_SCOPE();

		check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5);

		FComputeShaderRHIParamRef ComputeShaderRHI = PendingState.CurrentComputeShader;
		check(ComputeShaderRHI);

		DYNAMIC_CAST_OPENGLRESOURCE(ComputeShader,ComputeShader);

		FOpenGLContextState& ContextState = GetContextStateForCurrentContext();

		GPUProfilingData.RegisterGPUWork(1);		

		SetupTexturesForDraw(ContextState, ComputeShader, FOpenGL::GetMaxComputeTextureImageUnits());

		SetupUAVsForDraw(ContextState, ComputeShader, OGL_MAX_COMPUTE_STAGE_UAV_UNITS);
	
		CommitComputeShaderConstants(ComputeShader);
	
		FOpenGL::MemoryBarrier(GL_ALL_BARRIER_BITS);
	
		FOpenGL::DispatchCompute(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	
		FOpenGL::MemoryBarrier(GL_ALL_BARRIER_BITS);
	}
	else
	{
		UE_LOG(LogRHI,Fatal,TEXT("Platform doesn't support SM5 for OpenGL but set feature level to SM5"));
	}
}

void FOpenGLDynamicRHI::RHIDispatchIndirectComputeShader(FVertexBufferRHIParamRef ArgumentBuffer,uint32 ArgumentOffset)
{
	if ( FOpenGL::SupportsComputeShaders() )
	{
		VERIFY_GL_SCOPE();
		check(GRHIFeatureLevel >= ERHIFeatureLevel::SM5)

		UE_LOG(LogRHI, Fatal,TEXT("%s not implemented yet"),ANSI_TO_TCHAR(__FUNCTION__)); 
		GPUProfilingData.RegisterGPUWork(1);
	}
	else
	{
		UE_LOG(LogRHI,Fatal,TEXT("Platform doesn't support SM5 for OpenGL but set feature level to SM5"));
	}
}


void FOpenGLDynamicRHI::RHISetMultipleViewports(uint32 Count, const FViewportBounds* Data)
{ UE_LOG(LogRHI, Fatal,TEXT("OpenGL Render path does not support multiple Viewports!")); }

