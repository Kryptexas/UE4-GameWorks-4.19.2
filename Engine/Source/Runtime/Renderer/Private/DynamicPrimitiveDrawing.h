// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DynamicPrimitiveDrawing.h: Dynamic primitive drawing definitions.
=============================================================================*/

#ifndef __DYNAMICPRIMITIVEDRAWING_H__
#define __DYNAMICPRIMITIVEDRAWING_H__

/**
 * An implementation of the dynamic primitive definition interface to draw the elements passed to it on a given RHI command interface.
 */
template<typename DrawingPolicyFactoryType>
class TDynamicPrimitiveDrawer : public FPrimitiveDrawInterface
{
public:

	/**
	* Init constructor
	*
	* @param InView - view region being rendered
	* @param InDrawingContext - contex for the given draw policy type
	* @param InPreFog - rendering is occurring before fog
	* @param bInIsHitTesting - rendering is occurring during hit testing
	* @param bInIsVelocityRendering - rendering is occurring during velocity pass
	*/
	TDynamicPrimitiveDrawer(
		const FViewInfo* InView,
		const typename DrawingPolicyFactoryType::ContextType& InDrawingContext,
		bool InPreFog,
		bool bInIsHitTesting = false,
		bool bInIsVelocityRendering = false,
		bool bInEditorCompositeDepthTest = false,
		bool bInIsSelectionOutlineRendering = false
		):
		FPrimitiveDrawInterface(InView),
		View(InView),
		DrawingContext(InDrawingContext),
		PrimitiveSceneProxy(NULL),
		bPreFog(InPreFog),
		bDirty(false),
		bIsHitTesting(bInIsHitTesting),
		bIsVelocityRendering(bInIsVelocityRendering),
		bEditorCompositeDepthTest(bInEditorCompositeDepthTest),
		bIsSelectionOutlineRendering(bInIsSelectionOutlineRendering)
	{}

	~TDynamicPrimitiveDrawer();

	void SetPrimitive(const FPrimitiveSceneProxy* NewPrimitiveSceneProxy);

	// FPrimitiveDrawInterface interface.
	virtual bool IsHitTesting() OVERRIDE;
	virtual void SetHitProxy(HHitProxy* HitProxy) OVERRIDE;
	virtual bool IsRenderingSelectionOutline() const OVERRIDE { return bIsSelectionOutlineRendering; }
	virtual void RegisterDynamicResource(FDynamicPrimitiveResource* DynamicResource) OVERRIDE;
	virtual bool IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy) const OVERRIDE;
	virtual int32 DrawMesh(const FMeshBatch& Mesh) OVERRIDE;
	virtual void AddReserveLines(uint8 DepthPriorityGroup, int32 NumLines, bool bDepthBiased = false) OVERRIDE;
	virtual void DrawSprite(
		const FVector& Position,
		float SizeX,
		float SizeY,
		const FTexture* Sprite,
		const FLinearColor& Color,
		uint8 DepthPriorityGroup,
		float U,
		float UL,
		float V,
		float VL,
		uint8 BlendMode = SE_BLEND_Masked
		) OVERRIDE;
	virtual void DrawLine(
		const FVector& Start,
		const FVector& End,
		const FLinearColor& Color,
		uint8 DepthPriorityGroup,
		float Thickness = 0.0f,
		float DepthBias = 0.0f,
		bool bScreenSpace = false
		) OVERRIDE;
	virtual void DrawPoint(
		const FVector& Position,
		const FLinearColor& Color,
		float PointSize,
		uint8 DepthPriorityGroup
		) OVERRIDE;

	// Accessors.
	bool IsPreFog() const
	{
		return bPreFog;
	}
	bool IsDirty() const
	{
		return bDirty;
	}
	void ClearDirtyFlag()
	{
		bDirty = false;
	}

	/**
	 * @return true if rendering is occurring during velocity pass 
	 */
	virtual bool IsRenderingVelocities() const { return bIsVelocityRendering; }

private:
	/** The view which is being rendered. */
	const FViewInfo* const View;

	/** The drawing context passed to the drawing policy for the mesh elements rendered by this drawer. */
	typename DrawingPolicyFactoryType::ContextType DrawingContext;

	/** The primitive being rendered. */
	const FPrimitiveSceneProxy* PrimitiveSceneProxy;

	/** The current hit proxy ID being rendered. */
	FHitProxyId HitProxyId;

	/** The batched simple elements. */
	FBatchedElements BatchedElements;

	/** The dynamic resources which have been registered with this drawer. */
	TArray<FDynamicPrimitiveResource*,SceneRenderingAllocator> DynamicResources;

	/** true if fog has not yet been rendered. */
	uint32 bPreFog : 1;

	/** Tracks whether any elements have been rendered by this drawer. */
	uint32 bDirty : 1;

	/** true if hit proxies are being drawn. */
	uint32 bIsHitTesting : 1;

	/** true if rendering is occuring during velocity pass */
	uint32 bIsVelocityRendering : 1;

	/** true if rendering is occuring during editor compositing */
	uint32 bEditorCompositeDepthTest : 1;

	/** true if we are currently rendering the selection outline */
	uint32 bIsSelectionOutlineRendering : 1;
};

/**
 * Draws a view's elements with the specified drawing policy factory type.
 * @param View - The view to draw the meshes for.
 * @param DrawingContext - The drawing policy type specific context for the drawing.
 * @param DPGIndex World or Foreground DPG index for draw order
 * @param bPreFog - true if the draw call is occurring before fog has been rendered.
 */
template<class DrawingPolicyFactoryType>
bool DrawViewElements(
	const FViewInfo& View,
	const typename DrawingPolicyFactoryType::ContextType& DrawingContext,
	uint8 DPGIndex,
	bool bPreFog
	);

/**
 * Draws a given set of dynamic primitives to a RHI command interface, using the specified drawing policy type.
 * @param View - The view to draw the meshes for.
 * @param DrawingContext - The drawing policy type specific context for the drawing.
 * @param bPreFog - true if the draw call is occurring before fog has been rendered.
 */
template<class DrawingPolicyFactoryType>
bool DrawDynamicPrimitiveSet(
	const FViewInfo& View,
	const TArray<const FPrimitiveSceneInfo*,SceneRenderingAllocator>& PrimitiveSet,
	const typename DrawingPolicyFactoryType::ContextType& DrawingContext,
	bool bPreFog,
	bool bIsHitTesting = false
	);

/** A primitive draw interface which adds the drawn elements to the view's batched elements. */
class FViewElementPDI : public FPrimitiveDrawInterface
{
public:

	FViewElementPDI(FViewInfo* InViewInfo,FHitProxyConsumer* InHitProxyConsumer);

	// FPrimitiveDrawInterface interface.
	virtual bool IsHitTesting() OVERRIDE;
	virtual void SetHitProxy(HHitProxy* HitProxy) OVERRIDE;
	virtual void RegisterDynamicResource(FDynamicPrimitiveResource* DynamicResource) OVERRIDE;
	virtual void AddReserveLines(uint8 DepthPriorityGroup, int32 NumLines, bool bDepthBiased = false) OVERRIDE;
	virtual void DrawSprite(
		const FVector& Position,
		float SizeX,
		float SizeY,
		const FTexture* Sprite,
		const FLinearColor& Color,
		uint8 DepthPriorityGroup,
		float U,
		float UL,
		float V,
		float VL,
		uint8 BlendMode = SE_BLEND_Masked
		) OVERRIDE;
	virtual void DrawLine(
		const FVector& Start,
		const FVector& End,
		const FLinearColor& Color,
		uint8 DepthPriorityGroup,
		float Thickness = 0.0f,
		float DepthBias = 0.0f,
		bool bScreenSpace = false
		) OVERRIDE;
	virtual void DrawPoint(
		const FVector& Position,
		const FLinearColor& Color,
		float PointSize,
		uint8 DepthPriorityGroup
		) OVERRIDE;
	virtual int32 DrawMesh(const FMeshBatch& Mesh) OVERRIDE;

private:
	FViewInfo* ViewInfo;
	TRefCountPtr<HHitProxy> CurrentHitProxy;
	FHitProxyConsumer* HitProxyConsumer;

	/** Depending of the DPG we return a different FBatchedElement instance. */
	FBatchedElements& GetElements(uint8 DepthPriorityGroup) const;
};

#include "DynamicPrimitiveDrawing.inl"

#endif
