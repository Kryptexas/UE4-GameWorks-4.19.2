// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

class FSlateOpenGLTextureCache;

class FSlateOpenGLRenderingPolicy : public FSlateRenderingPolicy
{
public:
	FSlateOpenGLRenderingPolicy( TSharedPtr<FSlateFontCache>& InFontCache, TSharedPtr<FSlateOpenGLTextureManager>& InTextureManager );
	~FSlateOpenGLRenderingPolicy();

	/**
	 * Updates vertex and index buffers used in drawing
	 *
	 * @param InVertices	The vertices to copy to the vertex buffer
	 * @param InIndices		The indices to copy to the index buffer
	 */
	void UpdateBuffers( const FSlateWindowElementList& InElementList );
	
	/**
	 * Draws Slate elements
	 *
	 * @param ViewProjectionMatrix	The view projection matrix to pass to the vertex shader
	 * @param RenderBatches			A list of batches that should be rendered.
	 */
	void DrawElements( const FMatrix& ViewProjectionMatrix, const TArray<FSlateRenderBatch>& RenderBatches );

	
	/**
	 * Returns a slate texture that represents a viewport                   
	 */
	FSlateShaderResource* GetViewportResource( const ISlateViewport* InViewportInterface ) { return NULL; }
	
	/**
	 * Returns a slate texture resource
	 *
	 * @param	The name of the texture to return
 	 */
	FSlateShaderResourceProxy* GetTextureResource( const FSlateBrush& Brush );

	/**
	 * Returns the font cache used when the OpenGL rendering policy is active
 	 */
	TSharedPtr<FSlateFontCache>& GetFontCache() { return FontCache; }

	/** 
	 * Initializes resources if needed
	 */
	void ConditionalInitializeResources();

	/** 
	 * Releases rendering resources
	 */
	void ReleaseResources();
private:
	/** Vertex shader used for all elements */
	FSlateOpenGLVS VertexShader;
	/** Pixel shader used for all elements */
	FSlateOpenGLPS	PixelShader;
	/** Shader program for all elements */
	FSlateOpenGLElementProgram ElementProgram;
	/** Vertex buffer containing all the vertices of every element */
	FSlateOpenGLVertexBuffer VertexBuffer;
	/** Index buffer for accessing vertices of elements */
	FSlateOpenGLIndexBuffer IndexBuffer;
	/** A default white texture to use if no other texture can be found */
	FSlateOpenGLTexture* WhiteTexture;
	/** The font cache for accessing text rendering data */
	TSharedPtr<FSlateFontCache> FontCache;
	/** Texture manager for accessing OpenGL textures */
	TSharedPtr<FSlateOpenGLTextureManager> TextureManager;
	/** True if the rendering policy has been initialized */
	bool bIsInitialized;
};

