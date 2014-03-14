// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DrawingPolicy.h: Drawing policy definitions.
=============================================================================*/

#ifndef __DRAWINGPOLICY_H__
#define __DRAWINGPOLICY_H__

/**
 * A macro to compare members of two drawing policies(A and B), and return based on the result.
 * If the members are the same, the macro continues execution rather than returning to the caller.
 */
#define COMPAREDRAWINGPOLICYMEMBERS(MemberName) \
	if(A.MemberName < B.MemberName) { return -1; } \
	else if(A.MemberName > B.MemberName) { return +1; }

/**
 * The base mesh drawing policy.  Subclasses are used to draw meshes with type-specific context variables.
 * May be used either simply as a helper to render a dynamic mesh, or as a static instance shared between
 * similar meshs.
 */
class FMeshDrawingPolicy
{
public:
	struct ElementDataType {};

	FMeshDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& InMaterialResource,
		bool bInOverrideWithShaderComplexity = false,
		bool bInTwoSidedOverride = false
		);

	FMeshDrawingPolicy& operator = (const FMeshDrawingPolicy& Other)
	{ 
		VertexFactory = Other.VertexFactory;
		MaterialRenderProxy = Other.MaterialRenderProxy;
		MaterialResource = Other.MaterialResource;
		bIsTwoSidedMaterial = Other.bIsTwoSidedMaterial;
		bIsWireframeMaterial = Other.bIsWireframeMaterial;
		bNeedsBackfacePass = Other.bNeedsBackfacePass;
		bOverrideWithShaderComplexity = Other.bOverrideWithShaderComplexity;
		return *this; 
	}

	uint32 GetTypeHash() const
	{
		return PointerHash(VertexFactory,PointerHash(MaterialRenderProxy));
	}

	bool Matches(const FMeshDrawingPolicy& OtherDrawer) const
	{
		return
			VertexFactory == OtherDrawer.VertexFactory &&
			MaterialRenderProxy == OtherDrawer.MaterialRenderProxy &&
			bIsTwoSidedMaterial == OtherDrawer.bIsTwoSidedMaterial && 
			bIsWireframeMaterial == OtherDrawer.bIsWireframeMaterial;
	}

	/**
	 * Sets the render states for drawing a mesh.
	 * @param PrimitiveSceneProxy - The primitive drawing the dynamic mesh.  If this is a view element, this will be NULL.
	 */
	void SetMeshRenderState(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		bool bBackFace,
		const ElementDataType& ElementData
		) const;

	/**
	 * Executes the draw commands for a mesh.
	 */
	void DrawMesh(const FMeshBatch& Mesh, int32 BatchElementIndex) const;

	/**
	 * Executes the draw commands which can be shared between any meshes using this drawer.
	 * @param CI - The command interface to execute the draw commands on.
	 * @param View - The view of the scene being drawn.
	 */
	void DrawShared(const FSceneView* View) const;

	/**
	* Get the decl for this mesh policy type and vertexfactory
	*/
	const FVertexDeclarationRHIRef& GetVertexDeclaration() const;

	friend int32 CompareDrawingPolicy(const FMeshDrawingPolicy& A,const FMeshDrawingPolicy& B)
	{
		COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
		COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
		COMPAREDRAWINGPOLICYMEMBERS(bIsTwoSidedMaterial);
		return 0;
	}

	// Accessors.
	bool IsTwoSided() const
	{
		return bIsTwoSidedMaterial;
	}
	bool IsWireframe() const
	{
		return bIsWireframeMaterial;
	}
	bool NeedsBackfacePass() const
	{
		return bNeedsBackfacePass;
	}

	const FVertexFactory* GetVertexFactory() const { return VertexFactory; }
	const FMaterialRenderProxy* GetMaterialRenderProxy() const { return MaterialRenderProxy; }

protected:
	const FVertexFactory* VertexFactory;
	const FMaterialRenderProxy* MaterialRenderProxy;
	const FMaterial* MaterialResource;
	uint32 bIsTwoSidedMaterial : 1;
	uint32 bIsWireframeMaterial : 1;
	uint32 bNeedsBackfacePass : 1;
	uint32 bOverrideWithShaderComplexity : 1;
};


uint32 GetTypeHash(const FBoundShaderStateRHIRef &Key);

#endif
