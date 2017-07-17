
// AppleARKit
#include "AppleARKitCameraTextureResource.h"
#include "AppleARKit.h"
#include "AppleARKitCameraTexture.h"

// UE4
#include "Containers/ResourceArray.h"
#include "MediaShaders.h"
#include "PipelineStateCache.h"
#include "RHIUtilities.h"
#include "RHIStaticStates.h"

#if ARKIT_SUPPORT

/**
 * Passes a CVMetalTextureRef through to the RHI to wrap in an RHI texture without traversing system memory.
 * @see FAvfTexture2DResourceWrapper & FMetalSurface::FMetalSurface
 */
class FAppleARKitCameraTextureResourceWrapper : public FResourceBulkDataInterface
{
public:
	FAppleARKitCameraTextureResourceWrapper( CFTypeRef InImageBuffer )
	: ImageBuffer( InImageBuffer )
	{
		check( ImageBuffer );
		CFRetain( ImageBuffer );
	}
	
	/**
	 * @return ptr to the resource memory which has been preallocated
	 */
	virtual const void* GetResourceBulkData() const override
	{
		return ImageBuffer;
	}
	
	/**
	 * @return size of resource memory
	 */
	virtual uint32 GetResourceBulkDataSize() const override
	{
		// Returning ~0u is a special hack in the Metal RHI to be able to pass 
		// CVImageBufferRef or CVMetalTextureRef via ImageBuffer bulk data void*
		// @see FAvfTexture2DResourceWrapper in AvfMediaTracks.cpp
		return ImageBuffer ? ~0u : 0;
	}
	
	/**
	 * Free memory after it has been used to initialize RHI resource
	 */
	virtual void Discard() override
	{
		delete this;
	}
	
	virtual ~FAppleARKitCameraTextureResourceWrapper()
	{
		CFRelease( ImageBuffer );
		ImageBuffer = nullptr;
	}
	
	CFTypeRef ImageBuffer;
};

#endif // ARKIT_SUPPORT

class FAppleARKitCameraTextureResourceMem : public FResourceBulkDataInterface
{
public:
	FAppleARKitCameraTextureResourceMem(uint32 InSizeX, uint32 InSizeY)
	  : SizeX( InSizeX ), SizeY( InSizeY )
	{
		BGRAPixels = new uint8[SizeX*SizeY*4];
		FPlatformMemory::Memset( BGRAPixels, 50, SizeX*SizeY*4 );
	}
	
	virtual const void* GetResourceBulkData() const override
	{
		return (void*)BGRAPixels;
	}
	
	virtual uint32 GetResourceBulkDataSize() const override
	{
		return SizeX*SizeY*4;
	}
	
	virtual void Discard() override
	{
		delete this;
	}
	
	virtual ~FAppleARKitCameraTextureResourceMem()
	{
		if ( BGRAPixels )
		{
			delete BGRAPixels;
		}
	}
	
	uint32 SizeX;
	uint32 SizeY;

	uint8* BGRAPixels;
};

FAppleARKitCameraTextureResource::FAppleARKitCameraTextureResource( UAppleARKitCameraTexture* InOwner )
	: Owner( InOwner )
{
#if ARKIT_SUPPORT

	// Get a reference to the shared session
	Session = FAppleARKitModule::Get().GetSession();
	check( Session.IsValid() );

	// Start the shared session
	UE_LOG( LogAppleARKit, Log, TEXT("CameraTextureResource Starting Session %p ..."), &*Session );
	Session->Run();

#endif // ARKIT_SUPPORT
}

uint32 FAppleARKitCameraTextureResource::GetSizeX() const
{
	return SizeX;
}

uint32 FAppleARKitCameraTextureResource::GetSizeY() const
{
	return SizeY;
}

void FAppleARKitCameraTextureResource::InitDynamicRHI()
{
	// Create the sampler state RHI resource.
	FSamplerStateInitializerRHI SamplerStateInitializer
	(
		SF_Point,
		AM_Wrap,
		AM_Wrap,
		AM_Wrap
	);
	SamplerStateRHI = RHICreateSamplerState( SamplerStateInitializer );

	// Create a dummy texture for now
	uint32 CreateFlags = TexCreate_NoTiling | TexCreate_ShaderResource | TexCreate_SRGB;
	FRHIResourceCreateInfo CreateInfo( new FAppleARKitCameraTextureResourceMem( GetSizeX(), GetSizeY() ) );
	TextureRHI = RHICreateTexture2D( GetSizeX(), GetSizeY(), /*Format=*/PF_B8G8R8A8, /*NumMips=*/1, /*NumSamples=*/1, CreateFlags, CreateInfo );
	RHIUpdateTextureReference( Owner->TextureReference.TextureReferenceRHI, TextureRHI );

#if ARKIT_SUPPORT

	// Add to the list of global deferred updates to be updated during scene rendering.
	AddToDeferredUpdateList( /*OnlyUpdateOnce=*/false );

#endif // ARKIT_SUPPORT
}

void FAppleARKitCameraTextureResource::ReleaseDynamicRHI()
{
#if ARKIT_SUPPORT

	// Cease updating
	RemoveFromDeferredUpdateList();

#endif // ARKIT_SUPPORT

	RHIUpdateTextureReference( Owner->TextureReference.TextureReferenceRHI, FTextureRHIParamRef() );
	FTextureResource::ReleaseRHI();
	TextureRHI.SafeRelease();
}

class FConvertVertexDecl : public FRenderResource
{
public:
    
    FVertexDeclarationRHIRef VertexDeclarationRHI;
    
    // Destructor.
    virtual ~FConvertVertexDecl() {}
    
    virtual void InitRHI() override
    {
        FVertexDeclarationElementList Elements;
        uint16 Stride = sizeof(FMediaElementVertex);
        Elements.Add(FVertexElement(0, STRUCT_OFFSET(FMediaElementVertex, Position), VET_Float4, 0, Stride));
        Elements.Add(FVertexElement(0, STRUCT_OFFSET(FMediaElementVertex, TextureCoordinate), VET_Float2, 1, Stride));
        VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
    }
    
    virtual void ReleaseRHI() override
    {
        VertexDeclarationRHI.SafeRelease();
    }
};

// instance of it
TGlobalResource< FConvertVertexDecl > GConvertVertexDecl;


void FAppleARKitCameraTextureResource::UpdateDeferredResource( FRHICommandListImmediate& RHICmdList, bool bClearRenderTarget/*=true*/ )
{	
#if ARKIT_SUPPORT

	// Make sure we're using Metal
	check( IsMetalPlatform( GMaxRHIShaderPlatform ) );

	// Check the session is running
	if ( !Session.IsValid() || !Session->IsRunning() )
	{
		UE_LOG( LogAppleARKit, Warning, TEXT("Camera texture UpdateDeferredResource called without active session") );
		return;
	}

	// Get last frame
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > Frame = Session->GetCurrentFrame_RenderThread();
	if ( Frame.IsValid() && LastUpdateTimestamp != Frame->Timestamp && Frame->CapturedYImage && Frame->CapturedCbCrImage )
	{
		// Update SizeX & Y
        SizeX = FMath::Max(Frame->CapturedYImageWidth, Frame->CapturedCbCrImageWidth);
		SizeY = FMath::Max(Frame->CapturedYImageHeight, Frame->CapturedCbCrImageHeight);
        
		// Create a special bulk data wrapper to pass the CVMetalTextureRef direct to RHI
		FRHIResourceCreateInfo CreateInfo;
		CreateInfo.BulkData = new FAppleARKitCameraTextureResourceWrapper( Frame->CapturedYImage );
		CreateInfo.ResourceArray = nullptr;
        
        // pull the Y and CbCr textures out of the captured image planes (format is fake here, it will get the format from the FAppleARKitCameraTextureResourceWrapper)
        uint32 CreateFlags = TexCreate_Dynamic | TexCreate_NoTiling | TexCreate_ShaderResource;
        FTexture2DRHIRef YTexture = RHICreateTexture2D( Frame->CapturedYImageWidth, Frame->CapturedYImageHeight, /*Format=*/PF_B8G8R8A8, /*NumMips=*/1, /*NumSamples=*/1, CreateFlags, CreateInfo );
        
        CreateInfo.BulkData = new FAppleARKitCameraTextureResourceWrapper( Frame->CapturedCbCrImage );
        FTexture2DRHIRef CbCrTexture = RHICreateTexture2D( Frame->CapturedCbCrImageWidth, Frame->CapturedCbCrImageHeight, /*Format=*/PF_B8G8R8A8, /*NumMips=*/1, /*NumSamples=*/1, CreateFlags, CreateInfo );
        
        // This dynamically created texture is the render target that we will combine the above two textures to
        CreateFlags = TexCreate_Dynamic | TexCreate_NoTiling | TexCreate_ShaderResource | TexCreate_SRGB | TexCreate_RenderTargetable;
        CreateInfo.BulkData = nullptr;
        TextureRHI = RHICreateTexture2D( GetSizeX(), GetSizeY(), /*Format=*/PF_B8G8R8A8, /*NumMips=*/1, /*NumSamples=*/1, CreateFlags, CreateInfo );
        
        // Update references
        TextureRHI->SetName( Owner->GetFName() );
        RHIUpdateTextureReference( Owner->TextureReference.TextureReferenceRHI, TextureRHI );
        
        
        // run a shader to combine the two diffrently sized Y and CbCr textures into a single RGBA texture that we can render with using a material.
        // this is a temporary solution and it would be better to directly access 2 textures in a material, with a special material node
        // that combines the two (although if the texture was used many times over, it would be better to keep this pre-process step)
        {
            //SCOPED_DRAW_EVENT( RHICmdList, ARTextureConvert );
                
            auto ShaderMap = GetGlobalShaderMap( GMaxRHIFeatureLevel );
            
            // Set the new shaders
            TShaderMapRef<FMediaShadersVS> VertexShader( ShaderMap );
            TShaderMapRef<FYCbCrConvertPS_4x4Matrix> PixelShader( ShaderMap );
            
            FTextureRHIParamRef BoundTextureTarget = TextureRHI;
            SetRenderTargets(RHICmdList, 1, &BoundTextureTarget, nullptr, ESimpleRenderTargetMode::EExistingColorAndDepth, FExclusiveDepthStencil::DepthNop_StencilNop);
            
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
            
            GraphicsPSOInit.BlendState = TStaticBlendStateWriteMask<CW_RGBA, CW_NONE, CW_NONE, CW_NONE, CW_NONE, CW_NONE, CW_NONE, CW_NONE>::GetRHI();
            GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
            GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
            
            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GConvertVertexDecl.VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
            GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
            
            SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
            
            // set the textures, and true for sRGB
			const FMatrix YuvToSrgbARKitConversionMatrix = FMatrix(
			   FPlane(1.0000f, 0.0000f, 1.4020f, -0.7010f),
			   FPlane(1.0000f, -0.3441f, -0.7141f, 0.5291f),
			   FPlane(1.0000f, 1.7720f, 0.0000f, -0.8860f),
			   FPlane(0.0000f, 0.0000f, 0.0000f, 1.0000f)
			   );
            PixelShader->SetParameters( RHICmdList, YTexture->GetTexture2D(), CbCrTexture->GetTexture2D(), YuvToSrgbARKitConversionMatrix, true );
            
            // Draw a fullscreen quad
            FMediaElementVertex Vertices[4];
            Vertices[0].Position.Set( -1.0f, 1.0f, 1.0f, 1.0f );
            Vertices[0].TextureCoordinate.Set( 0.0f, 0.0f );
            
            Vertices[1].Position.Set( 1.0f, 1.0f, 1.0f, 1.0f );
            Vertices[1].TextureCoordinate.Set( 1.0f, 0.0f );
            
            Vertices[2].Position.Set( -1.0f, -1.0f, 1.0f, 1.0f );
            Vertices[2].TextureCoordinate.Set( 0.0f, 1.0f );
            
            Vertices[3].Position.Set( 1.0f, -1.0f, 1.0f, 1.0f );
            Vertices[3].TextureCoordinate.Set( 1.0f, 1.0f );
            
            // set viewport to RT size
            RHICmdList.SetViewport( 0, 0, 0.0f, GetSizeX(), GetSizeY(), 1.0f );
            
            DrawPrimitiveUP( RHICmdList, PT_TriangleStrip, 2, Vertices, sizeof( Vertices[0] ) );
            RHICmdList.CopyToResolveTarget( TextureRHI, TextureRHI, true, FResolveParams() );
        }
        
        // Release the frames CVMetalTextureRef reference now that we wont access it from there
		// again.  Moved this lower just for debugging.
        CFRelease(Frame->CapturedYImage);
        CFRelease(Frame->CapturedCbCrImage);
        Frame->CapturedYImage = nullptr;
        Frame->CapturedCbCrImage = nullptr;
        
		// Yay we did it!
		LastUpdateTimestamp = Frame->Timestamp;
	}

#endif // ARKIT_SUPPORT
}
