// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_TextureSetProxy.h"
#include "OculusHMDPrivateRHI.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS_OPENGL
#include "OculusHMD_CustomPresent.h"

#if PLATFORM_WINDOWS
#ifndef WINDOWS_PLATFORM_TYPES_GUARD
#include "AllowWindowsPlatformTypes.h"
#endif
#endif

namespace OculusHMD
{
//-------------------------------------------------------------------------------------------------
// FOpenGLTextureSetProxy
//-------------------------------------------------------------------------------------------------

class FOpenGLTextureSetProxy : public FTextureSetProxy
{
public:
	FOpenGLTextureSetProxy(FTextureRHIRef InTexture, const TArray<ovrpTextureHandle>& InTextures)
		: FTextureSetProxy(InTexture, InTextures.Num()) 
	{
		for (int32 TextureIndex = 0; TextureIndex < InTextures.Num(); ++TextureIndex)
		{
			SwapChainElement el;
			el.Texture = (GLuint)InTextures[TextureIndex];
			SwapChainElements.Push(el);
		}

		ExecuteOnRHIThread([&]()
		{
			AliasResources_RHIThread();
		});
	}

	virtual ~FOpenGLTextureSetProxy()
	{
		if (InRenderThread())
		{
			ExecuteOnRHIThread([&]()
			{
				SwapChainElements.Empty(0);
			});
		}
		else
		{
			SwapChainElements.Empty(0);
		}

		RHITexture = nullptr;
	}

protected:
	virtual void AliasResources_RHIThread() override
	{
		CheckInRHIThread();

		FOpenGLTexture2D* GLTS = static_cast<FOpenGLTexture2D*>(RHITexture->GetTexture2D());
		FOpenGLTextureCube* GLTSCube = static_cast<FOpenGLTextureCube*>(RHITexture->GetTextureCube());

		if(GLTS)
			GLTS->Resource = SwapChainElements[SwapChainIndex_RHIThread].Texture;

		if(GLTSCube)
			GLTSCube->Resource = SwapChainElements[SwapChainIndex_RHIThread].Texture;
	}

	struct SwapChainElement
	{
		GLuint Texture;
	};

	TArray<SwapChainElement> SwapChainElements;
};


//-------------------------------------------------------------------------------------------------
// APIs
//-------------------------------------------------------------------------------------------------

FTextureSetProxyPtr CreateTextureSetProxy_OpenGL(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, uint32 InArraySize, bool bIsCubemap, const TArray<ovrpTextureHandle>& InTextures)
{
	CheckInRenderThread();

	if (!InTextures.Num() || !(GLuint)InTextures[0])
	{
		return nullptr;
	}

	uint32 TexCreateFlags = TexCreate_ShaderResource | TexCreate_RenderTargetable;

	FOpenGLDynamicRHI* GLRHI = (FOpenGLDynamicRHI*)GDynamicRHI;

	TexCreateFlags |= TexCreate_SRGB;

	FTextureRHIRef TexRef;

	if (bIsCubemap)
	{
		TexRef = new FOpenGLTextureCube(
			GLRHI,
			0,
			GL_TEXTURE_CUBE_MAP,
			GL_NONE,
			InSizeX,
			InSizeY,
			0,
			InNumMips,
			InNumSamples,
			InNumSamplesTileMem,
			InArraySize,
			InFormat,
			false,
			false,
			TexCreateFlags,
			nullptr,
			FClearValueBinding());
	}
	else
	{
		TexRef = new FOpenGLTexture2D(
			GLRHI,
			0,
			InArraySize > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D,
			GL_NONE,
			InSizeX,
			InSizeY,
			0,
			InNumMips,
			InNumSamples,
			InNumSamplesTileMem,
			InArraySize,
			InFormat,
			false,
			false,
			TexCreateFlags,
			nullptr,
			FClearValueBinding());
	}
	
	OpenGLTextureAllocated(TexRef, TexCreateFlags);

	if (TexRef)
	{
		return MakeShareable(new FOpenGLTextureSetProxy(TexRef, InTextures));
	}

	return nullptr;
}


} // namespace OculusHMD

#if PLATFORM_WINDOWS
#undef WINDOWS_PLATFORM_TYPES_GUARD
#endif

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS_OPENGL
