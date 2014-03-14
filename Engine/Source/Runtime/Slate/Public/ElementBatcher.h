// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#ifndef __SLATEELEMENTBATCHER_H__
#define __SLATEELEMENTBATCHER_H__


class FSlateShaderResource;

/**
 * A class which batches Slate elements for rendering
 */
class SLATE_API FSlateElementBatcher
{
public:
	FSlateElementBatcher( TSharedRef<FSlateRenderingPolicy> InRenderingPolicy );
	~FSlateElementBatcher();

	/** 
	 * Batches elements to be rendered 
	 * 
	 * @param DrawElements	The elements to batch
	 */
	void AddElements( const TArray<FSlateDrawElement>& DrawElements );

	/**
	 * Populates the window element list with batched data for use in rendering
	 *
	 * @param WindowElementList		The window element list to update
	 * @param bRequiresStencilTest	Will be set to true if stencil testing should be enabled when drawing this viewport (currently for antialiased lines)
	 */
	void FillBatchBuffers(FSlateWindowElementList& WindowElementList, bool& bRequiresStencilTest);

	/**
	 * Returns true if the elements in this batche require vsync.
	 */
	bool RequiresVsync() const { return bRequiresVsync; }

	/** 
	 * Resets all stored data accumulated during the batching process
	 */
	void ResetBatches();

	void ResetStats();

private:

	/** 
	 * Creates vertices necessary to draw a Quad element 
	 *
	 * @param Position			The top left screen space position of the element
	 * @param Size				The size of the element
	 * @param Scale				The amount to scale the element by
	 * @param InClippingRect	The clipping rectangle for the element
	 * @param Layer				The layer to draw this element in
	 * @param DrawEffects		DrawEffects to apply
	 * @param Color				The color of the quad
	 */
	void AddQuadElement( const FVector2D& Position, const FVector2D& Size, float Scale, const FSlateRect& InClippingRect, uint32 Layer, FColor Color = FColor(255,255,255));

	/** 
	 * Creates vertices necessary to draw a 3x3 element
	 *
	 * @param Position			The top left screen space position of the element
	 * @param Size				The size of the element
	 * @param Scale				The amount to scale the element by
	 * @param InPayload			The data payload for this element
	 * @param InClippingRect	The clipping rectangle for the element
	 * @param DrawEffects		DrawEffects to apply
	 * @param Layer				The layer to draw this element in
	 */
	void AddBoxElement( const FVector2D& Position, const FVector2D& Size, float Scale, const FSlateDataPayload& InPayload, const FSlateRect& InClippingRect, ESlateDrawEffect::Type DrawEffects, uint32 Layer );

	/** 
	 * Creates vertices necessary to draw a string (one quad per character)
	 *
	 * @param Position			The top left screen space position of the element
	 * @param Size				The size of the element
	 * @param Scale				The amount to scale the element by
	 * @param InPayload			The data payload for this element
	 * @param InClippingRect	The clipping rectangle for the element
	 * @param DrawEffects		DrawEffects to apply
	 * @param Layer				The layer to draw this element in
	 */
	void AddTextElement( const FVector2D& Position, const FVector2D& Size, float Scale, const FSlateDataPayload& InPayload, const FSlateRect& InClippingRect, ESlateDrawEffect::Type DrawEffects, uint32 Layer );

	/** 
	 * Creates vertices necessary to draw a gradient box (horizontal or vertical)
	 *
	 * @param Position			The top left screen space position of the element
	 * @param Size				The size of the element
	 * @param Scale				The amount to scale the element by
	 * @param InPayload			The data payload for this element
	 * @param InClippingRect	The clipping rectangle for the element
	 * @param DrawEffects		DrawEffects to apply
	 * @param Layer				The layer to draw this element in
	 */
	void AddGradientElement( const FVector2D& Position, const FVector2D& Size, float Scale, const FSlateDataPayload& InPayload, const FSlateRect& InClippingRect, ESlateDrawEffect::Type DrawEffects, uint32 Layer );

	/** 
	 * Creates vertices necessary to draw a spline (bezier curve)
	 *
	 * @param Position			The top left screen space position of the element
	 * @param Scale				The amount to scale the element by
	 * @param InPayload			The data payload for this element
	 * @param InClippingRect	The clipping rectangle for the element
	 * @param DrawEffects		DrawEffects to apply
	 * @param Layer				The layer to draw this element in
	 */
	void AddSplineElement( const FVector2D& Position, float Scale, const FSlateDataPayload& InPayload, const FSlateRect& InClippingRect, ESlateDrawEffect::Type DrawEffects, uint32 Layer );

	/** 
	 * Creates vertices necessary to draw a series of attached line segments
	 *
	 * @param Position			The top left screen space position of the element
	 * @param Size				The size of the element (not used)
	 * @param Scale				The amount to scale the element by (not used)
	 * @param InPayload			The data payload for this element
	 * @param InClippingRect	The clipping rectangle for the element
	 * @param DrawEffects		DrawEffects to apply
	 * @param Layer				The layer to draw this element in
	 */
	void AddLineElement( const FVector2D& Position, const FVector2D& Size, float Scale, const FSlateDataPayload& InPayload, const FSlateRect& InClippingRect, ESlateDrawEffect::Type DrawEffects, uint32 Layer );
	
	/** 
	 * Creates vertices necessary to draw a viewport (just a textured quad)
	 *
	 * @param Position			The top left screen space position of the element
	 * @param Scale				The amount to scale the element by
	 * @param Size				The size of the element
	 * @param InPayload			The data payload for this element
	 * @param InClippingRect	The clipping rectangle for the element
	 * @param Layer				The layer to draw this element in
	 */
	void AddViewportElement( const FVector2D& Position, const FVector2D& Size, float Scale, const FSlateDataPayload& InPayload, const FSlateRect& InClippingRect, ESlateDrawEffect::Type DrawEffects, uint32 Layer );

	/** 
	 * Creates vertices necessary to draw a border element
	 *
	 * @param Position		The top left screen space position of the element
	 * @param Size			The size of the element
	 * @param Scale			The amount to scale the element by
	 * @param InPayload		The data payload for this element
	 * @param DrawEffects	DrawEffects to apply
	 * @param Layer			The layer to draw this element in
	 */
	void AddBorderElement( const FVector2D& Position, const FVector2D& Size, float Scale, const FSlateDataPayload& InPayload, const FSlateRect& InClippingRect, ESlateDrawEffect::Type DrawEffects, uint32 Layer );

	/**
	 * Batches a custom slate drawing element
	 *
	 * @param Position		The top left screen space position of the element
	 * @param Size			The size of the element
	 * @param Scale			The amount to scale the element by
	 * @param InPayload		The data payload for this element
	 * @param DrawEffects	DrawEffects to apply
	 * @param Layer			The layer to draw this element in
	 */
	void AddCustomElement( const FVector2D& Position, const FVector2D& Size, float Scale, const FSlateDataPayload& InPayload, const FSlateRect& InClippingRect, ESlateDrawEffect::Type DrawEffects, uint32 Layer );


	/** 
	 * Finds an batch for an element based on the passed in parameters
	 * Elements with common parameters and layers will batched together.
	 *
	 * @param Layer			The layer where this element should be drawn (signifies draw order)
	 * @param ShaderParams	The shader params for this element
	 * @param InTexture		The texture to use in the batch
	 * @param PrimitiveType	The primitive type( triangles, lines ) to use when drawing the batch
	 * @param ShaderType	The shader to use when rendering this batch
	 * @param DrawFlags		Any optional draw flags for this batch
	 */	
	FSlateElementBatch& FindBatchForElement( uint32 Layer, 
											 const FShaderParams& ShaderParams, 
											 const FSlateShaderResource* InTexture, 
											 ESlateDrawPrimitive::Type PrimitiveType, 
											 ESlateShader::Type ShaderType, 
											 ESlateDrawEffect::Type DrawEffects, 
											 ESlateBatchDrawFlag::Type DrawFlags = ESlateBatchDrawFlag::None);

	void AddBasicVertices( TArray<FSlateVertex>& OutVertices, FSlateElementBatch& ElementBatch, const TArray<FSlateVertex>& VertexBatch );

	void AddIndices( TArray<SlateIndex>& OutIndices, FSlateElementBatch& ElementBatch, const TArray<SlateIndex>& IndexBatch );

private:
	FSlateImageBrush SplineBrush;
	/** Element batch maps sorted by layer */
	TMap< uint32, TSet<FSlateElementBatch> > LayerToElementBatches;
	/** Array of vertex lists that are currently free (have no elements in them) */
	TArray<uint32> VertexArrayFreeList;
	/** Array of index lists that are currently free (have no elements in them) */
	TArray<uint32> IndexArrayFreeList;
	/** Array of vertex lists for batching vertices.  We use this method for quickly resetting the arrays without deleting memory */
	TArray< TArray<FSlateVertex> > BatchVertexArrays;
	/** Array of vertex lists for batching indices.  We use this method for quickly resetting the arrays without deleting memory */
	TArray< TArray<SlateIndex> > BatchIndexArrays;

	TSharedRef<FSlateRenderingPolicy> RenderingPolicy;

	/** true if any element in the batch requires vsync. */
	bool bRequiresVsync;

	uint32 NumVertices;
	uint32 NumBatches;
	uint32 NumLayers;
	uint32 TotalVertexMemory;
	uint32 RequiredVertexMemory;
	uint32 TotalIndexMemory;
	uint32 RequiredIndexMemory;
};

#endif //__SLATEELEMENTBATCHER_H__