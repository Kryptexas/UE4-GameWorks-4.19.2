// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FSlateRHIResourceManager;
class FSlateRHIRenderingPolicy;


#define USE_MAX_DRAWBUFFERS 1

#if USE_MAX_DRAWBUFFERS
// Number of draw buffers that can be active at any given time
const uint32 NumDrawBuffers = 3;
#endif


/** Resource for crash buffer editor capturing */
class FSlateCrashReportResource : public FRenderResource
{
public:
	FSlateCrashReportResource(FIntRect InVirtualScreen)
		: ReadbackBufferIndex(0)
		, ElementListIndex(0)
		, VirtualScreen(InVirtualScreen) {}

	/** FRenderResource Interface.  Called when render resources need to be initialized */
	virtual void InitDynamicRHI();

	/** FRenderResource Interface.  Called when render resources need to be released */
	virtual void ReleaseDynamicRHI();
	
	/** Changes the target readback buffer */
	void SwapTargetReadbackBuffer() {ReadbackBufferIndex = (ReadbackBufferIndex + 1) % 2;}
	
	/** Gets the next element list, ping ponging between the element lists */
	FSlateWindowElementList* GetNextElementList();

	/** Accessors */
	FIntRect GetVirtualScreen() const {return VirtualScreen;}
	FTexture2DRHIRef GetBuffer() const {return CrashReportBuffer;}
	FTexture2DRHIRef GetReadbackBuffer() const {return ReadbackBuffer[ReadbackBufferIndex];}

private:
	/** The crash report buffer we push out to a movie capture */
	FTexture2DRHIRef CrashReportBuffer;

	/** The readback buffers for copying back to the CPU from the GPU */
	FTexture2DRHIRef ReadbackBuffer[2];
	/** We ping pong between the buffers in case the GPU is a frame behind (GSystemSettings.bAllowOneFrameThreadLag) */
	int32 ReadbackBufferIndex;
	
	/** Window Element Lists for keypress buffer drawing */
	FSlateWindowElementList ElementList[2];
	/** We ping pong between the element lists, which is on the main thread, so it needs a different index than ReadbackBufferIndex */
	int32 ElementListIndex;

	/** The size of the virtual screen, used to calculate the buffer size */
	FIntRect VirtualScreen;
};


/** A Slate rendering implementation for Unreal engine */
class FSlateRHIRenderer : public FSlateRenderer
{
private:

	/** An RHI Representation of a viewport with cached width and height for detecting resizes */
	struct FViewportInfo : public FRenderResource
	{
		/** The projection matrix used in the viewport */
		FMatrix ProjectionMatrix;	
		/** The viewport rendering handle */
		FViewportRHIRef ViewportRHI;
		/** The depth buffer texture if any */
		FTexture2DRHIRef DepthStencil;
		/** The OS Window handle (for recreating the viewport) */
		void* OSWindow;
		/** The actual width of the viewport */
		uint32 Width;
		/** The actual height of the viewport */
		uint32 Height;
		/** The desired width of the viewport */
		uint32 DesiredWidth;
		/** The desired height of the viewport */
		uint32 DesiredHeight;
		/** Whether or not the viewport requires a stencil test */
		bool bRequiresStencilTest;
		/** Whether or not the viewport is in fullscreen */
		bool bFullscreen;
	
		/** FRenderResource interface */
		virtual void InitRHI();
		virtual void ReleaseRHI();

		FViewportInfo()
			:	OSWindow(NULL), 
				Width(0),
				Height(0),
				DesiredWidth(0),
				DesiredHeight(0),
				bRequiresStencilTest(false),
				bFullscreen(false)
		{

		}

		~FViewportInfo()
		{
			DepthStencil.SafeRelease();
		}

		void ConditionallyUpdateDepthBuffer(bool bInRequiresStencilTest);
		void RecreateDepthBuffer_RenderThread();
	};
public:
	/** 
	 * Constructor. Initializes all rendering resources
	 * 
	 * @param InStyle	A style used when rendering elements.  The renderer loads textures from information in this style
	 */
	FSlateRHIRenderer();
	~FSlateRHIRenderer();

	/** FSlateRenderer interface */
	virtual void Initialize() OVERRIDE;
	virtual void Destroy() OVERRIDE;
	virtual FSlateDrawBuffer& GetDrawBuffer() OVERRIDE;
	virtual void OnWindowDestroyed( const TSharedRef<SWindow>& InWindow ) OVERRIDE;
	virtual void RequestResize( const TSharedPtr<SWindow>& Window, uint32 NewWidth, uint32 NewHeight ) OVERRIDE;
	virtual void CreateViewport( const TSharedRef<SWindow> Window ) OVERRIDE;
	virtual void UpdateFullscreenState( const TSharedRef<SWindow> Window, uint32 OverrideResX, uint32 OverrideResY ) OVERRIDE;
	virtual void DrawWindows( FSlateDrawBuffer& InWindowDrawBuffer ) OVERRIDE;
	virtual void DrawWindows() OVERRIDE;
	virtual void FlushCommands() const OVERRIDE;
	virtual void Sync() const OVERRIDE;
	virtual void ReleaseDynamicResource( const FSlateBrush& InBrush ) OVERRIDE;
	virtual FIntPoint GenerateDynamicImageResource(const FName InTextureName) OVERRIDE;
	virtual bool GenerateDynamicImageResource( FName ResourceName, uint32 Width, uint32 Height, const TArray< uint8 >& Bytes ) OVERRIDE;
	virtual void* GetViewportResource( const SWindow& Window ) OVERRIDE;
	virtual void SetColorVisionDeficiencyType( uint32 Type ) OVERRIDE;

	/** Draws windows from a FSlateDrawBuffer on the render thread */
	void DrawWindow_RenderThread( const FSlateRHIRenderer::FViewportInfo& ViewportInfo, const FSlateWindowElementList& WindowElementList, bool bLockToVsync );


	/**
	 * Copies all slate windows out to a buffer at half resolution with debug information
	 * like the mouse cursor and any keypresses.
	 */
	virtual void CopyWindowsToDrawBuffer(const TArray<FString>& KeypressBuffer);
	
	/** Allows and disallows access to the crash tracker buffer data on the CPU */
	virtual void MapCrashTrackerBuffer(void** OutImageData, int32* OutWidth, int32* OutHeight) OVERRIDE;
	virtual void UnmapCrashTrackerBuffer() OVERRIDE;

	/**
	 * Reloads texture resources from disk                   
	 */
	virtual void ReloadTextureResources() OVERRIDE;


	virtual void LoadStyleResources( const ISlateStyle& Style ) OVERRIDE;

	/**
	 * Creates a window with an atlas visualizer inside it
	 */
	virtual void DisplayTextureAtlases();

	/**
	 * Returns the viewport RHI reference for the provided window
	 *
	 * @param Window	The window to get the RHI viewport from 
	 */


	/** Returns whether shaders that Slate depends on have been compiled. */
	virtual bool AreShadersInitialized() const;

	/** 
	 * Removes references to FViewportRHI's.  
	 * This has to be done explicitly instead of using the FRenderResource mechanism because FViewportRHI's are managed by the game thread.
	 * This is needed before destroying the RHI device. 
	 */
	virtual void InvalidateAllViewports();

private:
	/** Loads all known textures from Slate styles */
	void LoadUsedTextures();

	/**
	 * Resizes the viewport for a window if needed
	 * 
	 * @param ViewportInfo	The viewport to resize
	 * @param Width			The width that we should size to
	 * @param Height		The height that we shoudl size to
	 * @param bFullscreen	If we should be in fullscreen
	 */
	void ConditionalResizeViewport( FViewportInfo* ViewportInfo, uint32 Width, uint32 Height, bool bFullscreen );
	
	/** 
	 * Creates necessary resources to render a window and sends draw commands to the rendering thread
	 *
	 * @param WindowDrawBuffer	The buffer containing elements to draw 
	 */
	void DrawWindows_Private( FSlateDrawBuffer& InWindowDrawBuffer );

private:
	/** A mapping of SWindows to their RHI implementation */
	TMap< const SWindow*, FViewportInfo*> WindowToViewportInfo;

	/** View matrix used by all windows */
	FMatrix ViewMatrix;

	/** Keep a pointer around for when we have deferred drawing happening */
	FSlateDrawBuffer* EnqueuedWindowDrawBuffer;

#if USE_MAX_DRAWBUFFERS
	/** Double buffered draw buffers so that the rendering thread can be rendering windows while the game thread is setting up for next frame */
	FSlateDrawBuffer DrawBuffers[NumDrawBuffers];

	/** The draw buffer which is currently free for use by the game thread */
	uint8 FreeBufferIndex;
#endif

	/** Element batcher which renders draw elements */
	TSharedPtr<FSlateElementBatcher> ElementBatcher;

	/** Texture manager for accessing textures on the game thread */
	TSharedPtr<FSlateRHIResourceManager> ResourceManager;

	/** Drawing policy */
	TSharedPtr<FSlateRHIRenderingPolicy> RenderingPolicy;

	/** The resource which allows us to capture the editor to a buffer */
	FSlateCrashReportResource* CrashTrackerResource;
};
