// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ShadowRendering.h: Shadow rendering definitions.
=============================================================================*/

#ifndef __ShadowRendering_H__
#define __ShadowRendering_H__

// Forward declarations.
class FProjectedShadowInfo;

/** Utility functions for drawing a sphere */
namespace StencilingGeometry
{
	const int32 NUM_SPHERE_SIDES = 18;
	const int32 NUM_SPHERE_RINGS = 12;

	/** 
	* Calculates the world transform for a sphere.
	* @param OutTransform - The output world transform.
	* @param Sphere - The sphere to generate the transform for.
	* @param PreViewTranslation - The pre-view translation to apply to the transform.
	* @param bConservativelyBoundSphere - when true, the sphere that is drawn will contain all positions in the analytical sphere,
	*		 Otherwise the sphere vertices will lie on the analytical sphere and the positions on the faces will lie inside the sphere.
	*/
	extern void CalcTransform(FVector4& OutPosAndScale, const FSphere& Sphere, const FVector& PreViewTranslation, bool bConservativelyBoundSphere = true);

	/**
	* Draws a sphere using RHIDrawIndexedPrimitive, useful as approximate bounding geometry for deferred passes.
	* Note: The sphere will be of unit size unless transformed by the shader. 
	*/
	extern void DrawSphere();
	/** Renders a cone with a spherical cap, used for rendering spot lights in deferred passes. */
	extern void DrawCone();

	/** 
	* Vertex buffer for a sphere of unit size. Used for drawing a sphere as approximate bounding geometry for deferred passes.
	*/
	class FStencilSphereVertexBuffer : public FVertexBuffer
	{
	public:
		/** 
		* Initialize the RHI for this rendering resource 
		*/
		void InitRHI() OVERRIDE
		{
			const int32 NumSides = StencilingGeometry::NUM_SPHERE_SIDES;
			const int32 NumRings = StencilingGeometry::NUM_SPHERE_RINGS;
			const int32 NumVerts = (NumSides + 1) * (NumRings + 1);

			const float RadiansPerRingSegment = PI / (float)NumRings;
			float Radius = 1;

			TArray<FVector4, TInlineAllocator<NumRings + 1> > ArcVerts;
			ArcVerts.Empty(NumRings + 1);
			// Calculate verts for one arc
			for (int32 i = 0; i < NumRings + 1; i++)
			{
				const float Angle = i * RadiansPerRingSegment;
				ArcVerts.Add(FVector(0.0f, FMath::Sin(Angle), FMath::Cos(Angle)));
			}

			TResourceArray<FVector4, VERTEXBUFFER_ALIGNMENT> Verts;
			Verts.Empty(NumVerts);
			// Then rotate this arc NumSides + 1 times.
			const FVector Center = FVector(0,0,0);
			for (int32 s = 0; s < NumSides + 1; s++)
			{
				FRotator ArcRotator(0, 360.f * ((float)s / NumSides), 0);
				FRotationMatrix ArcRot( ArcRotator );

				for (int32 v = 0; v < NumRings + 1; v++)
				{
					const int32 VIx = (NumRings + 1) * s + v;
					Verts.Add(Center + Radius * ArcRot.TransformPosition(ArcVerts[v]));
				}
			}

			NumSphereVerts = Verts.Num();
			uint32 Size = Verts.GetResourceDataSize();

			// Create vertex buffer. Fill buffer with initial data upon creation
			VertexBufferRHI = RHICreateVertexBuffer(Size,&Verts,BUF_Static);
		}

		int32 GetVertexCount() const { return NumSphereVerts; }

	private:
		int32 NumSphereVerts;
	};

	/** 
	* Stenciling sphere index buffer
	*/
	class FStencilSphereIndexBuffer : public FIndexBuffer
	{
	public:
		/** 
		* Initialize the RHI for this rendering resource 
		*/
		void InitRHI() OVERRIDE
		{
			const int32 NumSides = StencilingGeometry::NUM_SPHERE_SIDES;
			const int32 NumRings = StencilingGeometry::NUM_SPHERE_RINGS;
			TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> Indices;

			// Add triangles for all the vertices generated
			for (int32 s = 0; s < NumSides; s++)
			{
				const int32 a0start = (s + 0) * (NumRings + 1);
				const int32 a1start = (s + 1) * (NumRings + 1);

				for (int32 r = 0; r < NumRings; r++)
				{
					Indices.Add(a0start + r + 0);
					Indices.Add(a1start + r + 0);
					Indices.Add(a0start + r + 1);
					Indices.Add(a1start + r + 0);
					Indices.Add(a1start + r + 1);
					Indices.Add(a0start + r + 1);
				}
			}

			NumIndices = Indices.Num();
			const uint32 Size = Indices.GetResourceDataSize();
			const uint32 Stride = sizeof(uint16);

			// Create index buffer. Fill buffer with initial data upon creation
			IndexBufferRHI = RHICreateIndexBuffer(Stride, Size, &Indices,BUF_Static);
		}

		int32 GetIndexCount() const { return NumIndices; }; 
	
	private:
		int32 NumIndices;
	};

	class FStencilConeIndexBuffer : public FIndexBuffer
	{
	public:
		// A side is a line of vertices going from the cone's origin to the edge of its SphereRadius
		static const int32 NumSides = 18;
		// A slice is a circle of vertices in the cone's XY plane
		static const int32 NumSlices = 12;

		static const uint32 NumVerts = NumSides * NumSlices * 2;

		void InitRHI() OVERRIDE
		{
			TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> Indices;

			Indices.Empty((NumSlices - 1) * NumSides * 12);
			// Generate triangles for the vertices of the cone shape
			for (int32 SliceIndex = 0; SliceIndex < NumSlices - 1; SliceIndex++)
			{
				for (int32 SideIndex = 0; SideIndex < NumSides; SideIndex++)
				{
					const int32 CurrentIndex = SliceIndex * NumSides + SideIndex % NumSides;
					const int32 NextSideIndex = SliceIndex * NumSides + (SideIndex + 1) % NumSides;
					const int32 NextSliceIndex = (SliceIndex + 1) * NumSides + SideIndex % NumSides;
					const int32 NextSliceAndSideIndex = (SliceIndex + 1) * NumSides + (SideIndex + 1) % NumSides;

					Indices.Add(CurrentIndex);
					Indices.Add(NextSideIndex);
					Indices.Add(NextSliceIndex);
					Indices.Add(NextSliceIndex);
					Indices.Add(NextSideIndex);
					Indices.Add(NextSliceAndSideIndex);
				}
			}

			// Generate triangles for the vertices of the spherical cap
			const int32 CapIndexStart = NumSides * NumSlices;

			for (int32 SliceIndex = 0; SliceIndex < NumSlices - 1; SliceIndex++)
			{
				for (int32 SideIndex = 0; SideIndex < NumSides; SideIndex++)
				{
					const int32 CurrentIndex = SliceIndex * NumSides + SideIndex % NumSides + CapIndexStart;
					const int32 NextSideIndex = SliceIndex * NumSides + (SideIndex + 1) % NumSides + CapIndexStart;
					const int32 NextSliceIndex = (SliceIndex + 1) * NumSides + SideIndex % NumSides + CapIndexStart;
					const int32 NextSliceAndSideIndex = (SliceIndex + 1) * NumSides + (SideIndex + 1) % NumSides + CapIndexStart;

					Indices.Add(CurrentIndex);
					Indices.Add(NextSliceIndex);
					Indices.Add(NextSideIndex);
					Indices.Add(NextSideIndex);
					Indices.Add(NextSliceIndex);
					Indices.Add(NextSliceAndSideIndex);
				}
			}

			const uint32 Size = Indices.GetResourceDataSize();
			const uint32 Stride = sizeof(uint16);

			NumIndices = Indices.Num();

			// Create index buffer. Fill buffer with initial data upon creation
			IndexBufferRHI = RHICreateIndexBuffer(Stride, Size, &Indices,BUF_Static);
		}

		int32 GetIndexCount() const { return NumIndices; } 

	protected:
		int32 NumIndices;
	};

	/** 
	* Vertex buffer for a cone. It holds zero'd out data since the actual math is done on the shader
	*/
	class FStencilConeVertexBuffer : public FVertexBuffer
	{
	public:
		static const int32 NumVerts = FStencilConeIndexBuffer::NumSides * FStencilConeIndexBuffer::NumSlices * 2;

		/** 
		* Initialize the RHI for this rendering resource 
		*/
		void InitRHI() OVERRIDE
		{
			TResourceArray<FVector4, VERTEXBUFFER_ALIGNMENT> Verts;
			Verts.Empty(NumVerts);
			for (int32 s = 0; s < NumVerts; s++)
			{
				Verts.Add(FVector4(0, 0, 0, 0));
			}

			uint32 Size = Verts.GetResourceDataSize();

			// Create vertex buffer. Fill buffer with initial data upon creation
			VertexBufferRHI = RHICreateVertexBuffer(Size, &Verts, BUF_Static);
		}

		int32 GetVertexCount() const { return NumVerts; }
	};

	extern TGlobalResource<FStencilSphereVertexBuffer> GStencilSphereVertexBuffer;
	extern TGlobalResource<FStencilSphereIndexBuffer> GStencilSphereIndexBuffer;
	extern TGlobalResource<FStencilConeVertexBuffer> GStencilConeVertexBuffer;
	extern TGlobalResource<FStencilConeIndexBuffer> GStencilConeIndexBuffer;

}; //End StencilingGeometry



/** Renders a cone with a spherical cap, used for rendering spot lights in deferred passes. */
extern void DrawStencilingCone(const FMatrix& ConeToWorld, float ConeAngle, float SphereRadius, const FVector& PreViewTranslation);

/** Shadow border needs to be wide enough to prevent the shadow filtering from picking up content in other shadowmaps in the atlas. */
const static uint32 SHADOW_BORDER = 4; 



template <bool bRenderingReflectiveShadowMaps> class TShadowDepthBasePS;
class FShadowStaticMeshElement;

/**
 * Outputs no color, but can be used to write the mesh's depth values to the depth buffer.
 */
template <bool bRenderingReflectiveShadowMaps>
class FShadowDepthDrawingPolicy : public FMeshDrawingPolicy
{
public:
	FShadowDepthDrawingPolicy(
		const FMaterial* InMaterialResource,
		bool bInDirectionalLight,
		bool bInOnePassPointLightShadow,
		bool bInPreShadow,
		const FVertexFactory* InVertexFactory = 0,
		const FMaterialRenderProxy* InMaterialRenderProxy = 0,
		bool bInCastShadowAsTwoSided = false,
		bool bReverseCulling = false
		);

	void UpdateElementState(FShadowStaticMeshElement& State);

	FShadowDepthDrawingPolicy& operator = (const FShadowDepthDrawingPolicy& Other)
	{ 
		VertexShader = Other.VertexShader;
		GeometryShader = Other.GeometryShader;
		HullShader = Other.HullShader;
		DomainShader = Other.DomainShader;
		PixelShader = Other.PixelShader;
		bDirectionalLight = Other.bDirectionalLight;
		bReverseCulling = Other.bReverseCulling;
		bOnePassPointLightShadow = Other.bOnePassPointLightShadow;
		bUsePositionOnlyVS = Other.bUsePositionOnlyVS;
		bPreShadow = Other.bPreShadow;
		(FMeshDrawingPolicy&)*this = (const FMeshDrawingPolicy&)Other;
		return *this; 
	}

	// FMeshDrawingPolicy interface.
	bool Matches(const FShadowDepthDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other) 
			&& VertexShader == Other.VertexShader
			&& GeometryShader == Other.GeometryShader
			&& HullShader == Other.HullShader
			&& DomainShader == Other.DomainShader
			&& PixelShader == Other.PixelShader
			&& bDirectionalLight == Other.bDirectionalLight
			&& bReverseCulling == Other.bReverseCulling
			&& bOnePassPointLightShadow == Other.bOnePassPointLightShadow
			&& bUsePositionOnlyVS == Other.bUsePositionOnlyVS
			&& bPreShadow == Other.bPreShadow;
	}
	void DrawShared(const FSceneView* View,FBoundShaderStateRHIParamRef BoundShaderState) const;

	/** 
	 * Create bound shader state using the vertex decl from the mesh draw policy
	 * as well as the shaders needed to draw the mesh
	 * @return new bound shader state object
	 */
	FBoundShaderStateRHIRef CreateBoundShaderState();

	void SetMeshRenderState(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		bool bBackFace,
		const ElementDataType& ElementData
		) const;

	void UnSetMeshRenderState() const;

	template<bool T2>
	friend int32 CompareDrawingPolicy(const FShadowDepthDrawingPolicy<T2>& A,const FShadowDepthDrawingPolicy<T2>& B);

	/** 
	 * Shadow currently being rendered by a FShadowDepthDrawingPolicy.  
	 * This is global so that different shadows can be used with the same static draw list. 
	 */
	static const FProjectedShadowInfo* PolicyShadowInfo;

	bool IsReversingCulling() const
	{
		return bReverseCulling;
	}

private:

	class FShadowDepthVS* VertexShader;
	class FOnePassPointShadowProjectionGS* GeometryShader;
	TShadowDepthBasePS<bRenderingReflectiveShadowMaps>* PixelShader;
	class FBaseHS* HullShader;
	class FShadowDepthDS* DomainShader;

public:

	uint32 bDirectionalLight:1;
	uint32 bReverseCulling:1;
	uint32 bOnePassPointLightShadow:1;
	uint32 bUsePositionOnlyVS:1;
	uint32 bPreShadow:1;
};

/** 
 * Shadow currently being rendered by a FShadowDepthDrawingPolicy.  
 * This is global so that different shadows can be used with the same static draw list. 
 */
extern const FProjectedShadowInfo* GShadowInfo;


/**
 * A drawing policy factory for the shadow depth drawing policy.
 */
class FShadowDepthDrawingPolicyFactory
{
public:

	enum { bAllowSimpleElements = false };

	struct ContextType
	{
		const FProjectedShadowInfo* ShadowInfo;

		ContextType(const FProjectedShadowInfo* InShadowInfo) :
			ShadowInfo(InShadowInfo)
		{}
	};

	static void AddStaticMesh(FScene* Scene,FStaticMesh* StaticMesh);

	static bool DrawDynamicMesh(
		const FSceneView& View,
		ContextType Context,
		const FMeshBatch& Mesh,
		bool bBackFace,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		);

	static bool IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy)
	{
		return false;
	}
};

/** 
 * Overrides a material used for shadow depth rendering with the default material when appropriate.
 * Overriding in this manner can reduce state switches and the number of shaders that have to be compiled.
 * This logic needs to stay in sync with shadow depth shader ShouldCache logic.
 */
void OverrideWithDefaultMaterialForShadowDepth(
	const FMaterialRenderProxy*& InOutMaterialRenderProxy, 
	const FMaterial*& InOutMaterialResource,
	bool bReflectiveShadowmap
	);

/** A single static mesh element for shadow depth rendering. */
class FShadowStaticMeshElement
{
public:

	FShadowStaticMeshElement()
		: RenderProxy(0)
		, MaterialResource(0)
		, Mesh(0)
		, bIsTwoSided(false)
	{
	}

	FShadowStaticMeshElement(const FMaterialRenderProxy* InRenderProxy, const FMaterial* InMaterialResource, const FStaticMesh* InMesh, bool bInIsTwoSided) :
		RenderProxy(InRenderProxy),
		MaterialResource(InMaterialResource),
		Mesh(InMesh),
		bIsTwoSided(bInIsTwoSided)
	{}

	bool DoesDeltaRequireADrawSharedCall(const FShadowStaticMeshElement& rhs) const
	{
		checkSlow(rhs.RenderProxy);
		checkSlow(rhs.Mesh);

		// Note: this->RenderProxy or this->Mesh can be 0
		// but in this case rhs.RenderProxy should not be 0
		// so it will early out and there will be no crash on Mesh->VertexFactory
		checkSlow(!RenderProxy || rhs.RenderProxy);

		return RenderProxy != rhs.RenderProxy
			|| bIsTwoSided != rhs.bIsTwoSided
			|| Mesh->VertexFactory != rhs.Mesh->VertexFactory
			|| Mesh->ReverseCulling != rhs.Mesh->ReverseCulling;
	}

	/** Store the FMaterialRenderProxy pointer since it may be different from the one that FStaticMesh stores. */
	const FMaterialRenderProxy* RenderProxy;
	const FMaterial* MaterialResource;
	const FStaticMesh* Mesh;
	bool bIsTwoSided;
};

/**
 * Information about a projected shadow.
 */
class FProjectedShadowInfo : public FRefCountedObject
{
public:

	friend class FShadowDepthVS;
	//	friend class FShadowDepthPS;
	template <bool bRenderingReflectiveShadowMaps> friend class TShadowDepthBasePS;

	friend class FShadowProjectionVS;
	friend class FShadowProjectionPS;
	friend class FShadowDepthDrawingPolicyFactory;

	typedef TArray<const FPrimitiveSceneInfo*,SceneRenderingAllocator> PrimitiveArrayType;

	const FLightSceneInfo* const LightSceneInfo;
	const FLightSceneInfoCompact LightSceneInfoCompact;

	/** Parent primitive of the shadow group that created this shadow, if not a bWholeSceneShadow. */
	const FPrimitiveSceneInfo* const ParentSceneInfo;

	/** The view this shadow must be rendered in, or NULL for a view independent shadow. */
	FViewInfo* DependentView;

	/** Index of the shadow into FVisibleLightInfo::AllProjectedShadows. */
	int32 ShadowId;

	/** A translation that is applied to world-space before transforming by one of the shadow matrices. */
	FVector PreShadowTranslation;

	/** The effective view matrix of the shadow, used as an override to the main view's view matrix when rendering the shadow depth pass. */
	FMatrix ShadowViewMatrix;

	FMatrix SubjectAndReceiverMatrix;
	FMatrix ReceiverMatrix;

	FMatrix InvReceiverMatrix;

	float MaxSubjectDepth;

	/** 
	 * Subject depth extents, in world space units. 
	 * These can be used to convert shadow depth buffer values back into world space units.
	 */
	float MaxSubjectZ;
	float MinSubjectZ;

	FConvexVolume SubjectAndReceiverFrustum;
	FConvexVolume ReceiverFrustum;

	float MinPreSubjectZ;

	FSphere ShadowBounds;

	/** The extruded cascade frustum's culling planes, if a whole scene directional shadow. */
	TArray<FPlane> FrustumCullPlanes;

	FShadowCascadeSettings CascadeSettings;

	/** X and Y position of the shadow in the appropriate depth buffer.  These are only initialized after the shadow has been allocated. */
	uint32 X;
	uint32 Y;

	/** Resolution of the shadow. */
	uint32 ResolutionX;
	uint32 ResolutionY;

	/** The largest percent of either the width or height of any view. */
	float MaxScreenPercent;

	/** Fade Alpha per view. */
	TArray<float, TInlineAllocator<2> > FadeAlphas;

	/** 
	 * Index of the split if this is a whole scene shadow from a directional light, 
	 * Or index of the direction if this is a whole scene shadow from a point light, otherwise INDEX_NONE. 
	 */
	int32 SplitIndex;

	/** Whether the shadow has been allocated in the shadow depth buffer, and its X and Y properties have been initialized. */
	uint32 bAllocated : 1;

	/** 
	 * Whether the translucent shadow has been allocated in the per-frame translucent shadow layout, and therefore has a resident shadow map.
	 * If true, the resident shadow map can be re-used during the translucency pass, otherwise it needs to be re-rendered.
	 */
	uint32 bAllocatedInTranslucentLayout : 1;

	/** Whether the shadow's projection has been rendered. */
	uint32 bRendered : 1;

	/** Whether the shadow has been allocated in the preshadow cache, so its X and Y properties offset into the preshadow cache depth buffer. */
	uint32 bAllocatedInPreshadowCache : 1;

	/** Whether the shadow is in the preshadow cache and its depths are up to date. */
	uint32 bDepthsCached : 1;

	uint32 bDirectionalLight : 1;

	/** Whether this shadow affects the whole scene or only a group of objects. */
	uint32 bWholeSceneShadow : 1;

	/** Whether the shadow is a point light shadow that renders all faces of a cubemap in one pass. */
	uint32 bOnePassPointLightShadow : 1;

	/** Whether the shadow needs to render reflective shadow maps. */ 
	uint32 bReflectiveShadowmap : 1; 

	/** Whether this shadow should support casting shadows from translucent surfaces. */
	uint32 bTranslucentShadow : 1;

	/** Whether the shadow is a preshadow or not.  A preshadow is a per object shadow that handles the static environment casting on a dynamic receiver. */
	uint32 bPreShadow : 1;

	uint32 bValidTransform : 1;

	TBitArray<SceneRenderingBitArrayAllocator> StaticMeshWholeSceneShadowDepthMap;
	TArray<uint64,SceneRenderingAllocator> StaticMeshWholeSceneShadowBatchVisibility;

	/** View projection matrices for each cubemap face, used by one pass point light shadows. */
	TArray<FMatrix> OnePassShadowViewProjectionMatrices;

	/** Frustums for each cubemap face, used for object culling one pass point light shadows. */
	TArray<FConvexVolume> OnePassShadowFrustums;

private:

	/** dynamic shadow casting elements */
	PrimitiveArrayType SubjectPrimitives;
	/** For preshadows, this contains the receiver primitives to mask the projection to. */
	PrimitiveArrayType ReceiverPrimitives;

	/** Static shadow casting elements. */
	TArray<FShadowStaticMeshElement,SceneRenderingAllocator> SubjectMeshElements;

	/** Subject primitives with translucent relevance. */
	PrimitiveArrayType SubjectTranslucentPrimitives;

	/**
	 * Bias during in shadowmap rendering, stored redundantly for better performance 
	 * Set by UpdateShaderDepthBias(), get with GetShaderDepthBias(), -1 if not set
	 */
	float ShaderDepthBias;

public:

	/** Initialization constructor for a per-object shadow. e.g. translucent particle system */
	FProjectedShadowInfo(
		FLightSceneInfo* InLightSceneInfo,
		const FPrimitiveSceneInfo* InParentSceneInfo,
		const FPerObjectProjectedShadowInitializer& Initializer,
		bool bInPreShadow,
		uint32 InResolutionX,
		uint32 MaxShadowResolutionY,
		float InMaxScreenPercent,
		const TArray<float, TInlineAllocator<2> >& InFadeAlphas,
		bool bInTranslucentShadow
		);

	/** Initialization constructor for a whole-scene shadow. */
	FProjectedShadowInfo(
		FLightSceneInfo* InLightSceneInfo,
		FViewInfo* InDependentView,
		const FWholeSceneProjectedShadowInitializer& Initializer,
		uint32 InResolutionX,
		uint32 InResolutionY,
		const TArray<float, TInlineAllocator<2> >& InFadeAlphas
		);

	/** Initialization constructor for a whole-scene reflective shadow map */
	FProjectedShadowInfo(
		FLightSceneInfo* InLightSceneInfo,
		FViewInfo* InDependentView,
		const FRsmWholeSceneProjectedShadowInitializer& Initializer,
		uint32 InResolutionX,
		uint32 InResolutionY
		);

	float GetShaderDepthBias() const { check(ShaderDepthBias >= 0); return ShaderDepthBias; }

	/**
	 * Renders the shadow subject depth.
	 */
	void RenderDepth(class FDeferredShadingSceneRenderer* SceneRenderer);

	void ClearDepth(class FDeferredShadingSceneRenderer* SceneRenderer);

	/** Renders shadow maps for translucent primitives. */
	void RenderTranslucencyDepths(class FDeferredShadingSceneRenderer* SceneRenderer);

	/**
	 * Projects the shadow onto the scene for a particular view.
	 */
	void RenderProjection(int32 ViewIndex, const class FViewInfo* View) const;

	/** Render one pass point light shadow projections. */
	void RenderOnePassPointLightProjection(int32 ViewIndex, const FViewInfo& View) const;

	/**
	 * Renders the projected shadow's frustum wireframe with the given FPrimitiveDrawInterface.
	 */
	void RenderFrustumWireframe(FPrimitiveDrawInterface* PDI) const;

	/**
	 * Adds a primitive to the shadow's subject list.
	 */
	void AddSubjectPrimitive(FPrimitiveSceneInfo* PrimitiveSceneInfo, TArray<FViewInfo>* ViewArray);

	/**
	* @return TRUE if this shadow info has any casting subject prims to render
	*/
	bool HasSubjectPrims() const;

	/**
	 * Adds a primitive to the shadow's receiver list.
	 */
	void AddReceiverPrimitive(FPrimitiveSceneInfo* PrimitiveSceneInfo);

	/** 
	 * @param View view to check visibility in
	 * @return true if this shadow info has any subject prims visible in the view
	 */
	bool SubjectsVisible(const FViewInfo& View) const;

	/** Clears arrays allocated with the scene rendering allocator. */
	void ClearTransientArrays();
	
	/** Hash function. */
	friend uint32 GetTypeHash(const FProjectedShadowInfo* ProjectedShadowInfo)
	{
		return PointerHash(ProjectedShadowInfo);
	}

	/** Returns a matrix that transforms a screen space position into shadow space. */
	FMatrix GetScreenToShadowMatrix(const FSceneView& View) const;

	/** Returns a matrix that transforms a world space position into shadow space. */
	FMatrix GetWorldToShadowMatrix(FVector4& ShadowmapMinMax, const FIntPoint* ShadowBufferResolutionOverride = nullptr, bool bHasShadowBorder = true ) const;

	/** Returns the resolution of the shadow buffer used for this shadow, based on the shadow's type. */
	FIntPoint GetShadowBufferResolution() const;

	/** Computes and updates ShaderDepthBias */
	void UpdateShaderDepthBias();
	/** How large the soft PCF comparison should be, similar to DepthBias, before this was called TransitionScale and 1/Size */
	float ComputeTransitionSize() const;

	inline bool IsWholeSceneDirectionalShadow() const 
	{ 
		return bWholeSceneShadow && SplitIndex >= 0 && LightSceneInfo->Proxy->GetLightType() == LightType_Directional; 
	}

	inline bool IsWholeScenePointLightShadow() const
	{
		return bWholeSceneShadow && LightSceneInfo->Proxy->GetLightType() == LightType_Point;
	}

	/** Sorts SubjectMeshElements based on state so that rendering the static elements will set as little state as possible. */
	void SortSubjectMeshElements();

private:

	void GetShadowTypeNameForDrawEvent(FString& TypeName) const;

	template <bool bReflectiveShadowmap> friend void DrawShadowMeshElements(const FViewInfo& View, const FProjectedShadowInfo& ShadowInfo);
};

/** Shader parameters for rendering the depth of a mesh for shadowing. */
class FShadowDepthShaderParameters
{
public:

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		ProjectionMatrix.Bind(ParameterMap,TEXT("ProjectionMatrix"));
		InvMaxSubjectDepth.Bind(ParameterMap,TEXT("InvMaxSubjectDepth"));
		DepthBias.Bind(ParameterMap,TEXT("DepthBias"));
		ClampToNearPlane.Bind(ParameterMap,TEXT("bClampToNearPlane"));
	}

	template<typename ShaderRHIParamRef>
	void Set(ShaderRHIParamRef ShaderRHI, const FSceneView& View, const FProjectedShadowInfo* ShadowInfo, const FMaterialRenderProxy* MaterialRenderProxy)
	{
		SetShaderValue(
			ShaderRHI,
			ProjectionMatrix,
			FTranslationMatrix(ShadowInfo->PreShadowTranslation - View.ViewMatrices.PreViewTranslation) * ShadowInfo->SubjectAndReceiverMatrix
			);

		SetShaderValue(ShaderRHI, InvMaxSubjectDepth, 1.0f / ShadowInfo->MaxSubjectDepth);
		SetShaderValue(ShaderRHI, DepthBias, ShadowInfo->GetShaderDepthBias());
		// Only clamp vertices to the near plane when rendering whole scene directional light shadow depths or preshadows from directional lights
		const bool bClampToNearPlaneValue = ShadowInfo->IsWholeSceneDirectionalShadow() || (ShadowInfo->bPreShadow && ShadowInfo->bDirectionalLight);
		SetShaderValue(ShaderRHI,ClampToNearPlane,bClampToNearPlaneValue ? 1.0f : 0.0f);
	}

	/** Set the vertex shader parameter values. */
	void SetVertexShader(FShader* VertexShader, const FSceneView& View, const FProjectedShadowInfo* ShadowInfo, const FMaterialRenderProxy* MaterialRenderProxy)
	{
		Set(VertexShader->GetVertexShader(), View, ShadowInfo, MaterialRenderProxy);
	}

	/** Set the domain shader parameter values. */
	void SetDomainShader(FShader* DomainShader, const FSceneView& View, const FProjectedShadowInfo* ShadowInfo, const FMaterialRenderProxy* MaterialRenderProxy)
	{
		Set(DomainShader->GetDomainShader(), View, ShadowInfo, MaterialRenderProxy);
	}

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FShadowDepthShaderParameters& P)
	{
		Ar << P.ProjectionMatrix;
		Ar << P.InvMaxSubjectDepth;
		Ar << P.DepthBias;
		Ar << P.ClampToNearPlane;
		return Ar;
	}

private:
	FShaderParameter ProjectionMatrix;
	FShaderParameter InvMaxSubjectDepth;
	FShaderParameter DepthBias;
	FShaderParameter ClampToNearPlane;
};

/** 
* Stencil geometry parameters used by multiple shaders. 
*/
class FStencilingGeometryShaderParameters
{
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		StencilGeometryPosAndScale.Bind(ParameterMap, TEXT("StencilingGeometryPosAndScale"));
		StencilConeParameters.Bind(ParameterMap, TEXT("StencilingConeParameters"));
		StencilConeTransform.Bind(ParameterMap, TEXT("StencilingConeTransform"));
		StencilPreViewTranslation.Bind(ParameterMap, TEXT("StencilingPreViewTranslation"));
	}

	void Set(FShader* Shader, const FVector4& InStencilingGeometryPosAndScale) const
	{
		SetShaderValue(Shader->GetVertexShader(), StencilGeometryPosAndScale, InStencilingGeometryPosAndScale);
		SetShaderValue(Shader->GetVertexShader(), StencilConeParameters, FVector4(0.0f, 0.0f, 0.0f, 0.0f));
	}

	void Set(FShader* Shader, const FSceneView& View, const FLightSceneInfo* LightSceneInfo) const
	{
		FVector4 GeometryPosAndScale;
		if(LightSceneInfo->Proxy->GetLightType() == LightType_Point)
		{
			StencilingGeometry::CalcTransform(GeometryPosAndScale, LightSceneInfo->Proxy->GetBoundingSphere(), View.ViewMatrices.PreViewTranslation);
			SetShaderValue(Shader->GetVertexShader(), StencilGeometryPosAndScale, GeometryPosAndScale);
			SetShaderValue(Shader->GetVertexShader(), StencilConeParameters, FVector4(0.0f, 0.0f, 0.0f, 0.0f));
		}
		else if(LightSceneInfo->Proxy->GetLightType() == LightType_Spot)
		{
			SetShaderValue(Shader->GetVertexShader(), StencilConeTransform, LightSceneInfo->Proxy->GetLightToWorld());
			SetShaderValue(
				Shader->GetVertexShader(),
				StencilConeParameters,
				FVector4(
					StencilingGeometry::FStencilConeIndexBuffer::NumSides,
					StencilingGeometry::FStencilConeIndexBuffer::NumSlices,
					LightSceneInfo->Proxy->GetOuterConeAngle(),
					LightSceneInfo->Proxy->GetRadius()));
			SetShaderValue(Shader->GetVertexShader(), StencilPreViewTranslation, View.ViewMatrices.PreViewTranslation);
		}
	}

	/** Serializer. */ 
	friend FArchive& operator<<(FArchive& Ar,FStencilingGeometryShaderParameters& P)
	{
		Ar << P.StencilGeometryPosAndScale;
		Ar << P.StencilConeParameters;
		Ar << P.StencilConeTransform;
		Ar << P.StencilPreViewTranslation;
		return Ar;
	}

private:
	FShaderParameter StencilGeometryPosAndScale;
	FShaderParameter StencilConeParameters;
	FShaderParameter StencilConeTransform;
	FShaderParameter StencilPreViewTranslation;
};

/**
* A vertex shader for projecting a shadow depth buffer onto the scene.
*/
class FShadowProjectionVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShadowProjectionVS,Global);
public:

	FShadowProjectionVS() {}
	FShadowProjectionVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) 
	{
		StencilingGeometryParameters.Bind(Initializer.ParameterMap);
	}

	static bool ShouldCache(EShaderPlatform Platform);
	
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("USE_TRANSFORM"), (uint32)1);
	}

	void SetParameters(const FSceneView& View, const FProjectedShadowInfo* ShadowInfo, int32 ShadowSplitIndex = INDEX_NONE);

	// Begin FShader interface
	virtual bool Serialize(FArchive& Ar) OVERRIDE
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << StencilingGeometryParameters;
		return bShaderHasOutdatedParameters;
	}
	//  End FShader interface 

private:
	FStencilingGeometryShaderParameters StencilingGeometryParameters;
};

class FShadowProjectionNoTransformVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShadowProjectionNoTransformVS,Global);
public:
	FShadowProjectionNoTransformVS() {}
	FShadowProjectionNoTransformVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer) 
	{
	}

	/**
	 * Add any defines required by the shader
	 * @param OutEnvironment - shader environment to modify
	 */
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("USE_TRANSFORM"), (uint32)0);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	inline void SetParameters(const FSceneView& View)
	{
		FGlobalShader::SetParameters(GetVertexShader(),View); 
	}
};

/**
 * FShadowProjectionPixelShaderInterface - used to handle templated versions
 */

class FShadowProjectionPixelShaderInterface : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShadowProjectionPixelShaderInterface,Global);
public:

	FShadowProjectionPixelShaderInterface() 
		:	FGlobalShader()
	{}

	/**
	 * Constructor - binds all shader params and initializes the sample offsets
	 * @param Initializer - init data from shader compiler
	 */
	FShadowProjectionPixelShaderInterface(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{ }

	/**
	 * Sets the current pixel shader params
	 * @param View - current view
	 * @param ShadowInfo - projected shadow info for a single light
	 */
	virtual void SetParameters(
		int32 ViewIndex,
		const FSceneView& View,
		const FProjectedShadowInfo* ShadowInfo
		)
	{ 
		FGlobalShader::SetParameters(GetPixelShader(),View);
	}

};

/** Shadow projection parameters used by multiple shaders. */
class FShadowProjectionShaderParameters
{
public:

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		DeferredParameters.Bind(ParameterMap);
		ScreenToShadowMatrix.Bind(ParameterMap,TEXT("ScreenToShadowMatrix"));
		SoftTransitionScale.Bind(ParameterMap,TEXT("SoftTransitionScale"));
		ShadowBufferSize.Bind(ParameterMap,TEXT("ShadowBufferSize"));
		ShadowDepthTexture.Bind(ParameterMap,TEXT("ShadowDepthTexture"));
		ShadowDepthTextureSampler.Bind(ParameterMap,TEXT("ShadowDepthTextureSampler"));
		ShadowDepthTextureObject.Bind(ParameterMap,TEXT("ShadowDepthTextureObject"));
		ShadowDepthSampler.Bind(ParameterMap,TEXT("ShadowDepthSampler"));
		ProjectionDepthBias.Bind(ParameterMap,TEXT("ProjectionDepthBiasParameters"));
		FadePlaneOffset.Bind(ParameterMap,TEXT("FadePlaneOffset"));
		InvFadePlaneLength.Bind(ParameterMap,TEXT("InvFadePlaneLength"));
	}

	void Set(FShader* Shader, const FSceneView& View, const FProjectedShadowInfo* ShadowInfo)
	{
		const FPixelShaderRHIParamRef ShaderRHI = Shader->GetPixelShader();

		DeferredParameters.Set(ShaderRHI, View);

		// Set the transform from screen coordinates to shadow depth texture coordinates.
		const FMatrix ScreenToShadow = ShadowInfo->GetScreenToShadowMatrix(View);
		SetShaderValue(ShaderRHI, ScreenToShadowMatrix, ScreenToShadow);

		const FIntPoint ShadowBufferResolution = ShadowInfo->GetShadowBufferResolution();

		if (SoftTransitionScale.IsBound())
		{
			const float TransitionSize = ShadowInfo->ComputeTransitionSize();

			SetShaderValue(ShaderRHI, SoftTransitionScale, FVector(0, 0, 1.0f / TransitionSize));
		}

		if (ShadowBufferSize.IsBound())
		{
			FVector2D ShadowBufferSizeValue(ShadowBufferResolution.X, ShadowBufferResolution.Y);

			SetShaderValue(ShaderRHI, ShadowBufferSize,
				FVector4(ShadowBufferSizeValue.X, ShadowBufferSizeValue.Y, 1.0f / ShadowBufferSizeValue.X, 1.0f / ShadowBufferSizeValue.Y));
		}

		FTexture2DRHIRef ShadowDepthTextureValue = GSceneRenderTargets.GetShadowDepthZTexture(ShadowInfo->bAllocatedInPreshadowCache);
		FSamplerStateRHIParamRef DepthSamplerState = TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();

		SetTextureParameter(ShaderRHI, ShadowDepthTexture, ShadowDepthTextureSampler, DepthSamplerState, ShadowDepthTextureValue);
		SetTextureParameter(ShaderRHI, ShadowDepthTextureObject, ShadowDepthTextureValue);

		if (ShadowDepthSampler.IsBound())
		{
			RHISetShaderSampler(
				ShaderRHI, 
				ShadowDepthSampler.GetBaseIndex(), 
				DepthSamplerState
				);
		}

		SetShaderValue(ShaderRHI, ProjectionDepthBias, FVector2D(ShadowInfo->GetShaderDepthBias(), ShadowInfo->MaxSubjectZ - ShadowInfo->MinSubjectZ));
		SetShaderValue(ShaderRHI, FadePlaneOffset, ShadowInfo->CascadeSettings.FadePlaneOffset);

		if(InvFadePlaneLength.IsBound())
		{
			check(ShadowInfo->CascadeSettings.FadePlaneLength > 0);
			SetShaderValue(ShaderRHI, InvFadePlaneLength, 1.0f / ShadowInfo->CascadeSettings.FadePlaneLength);
		}
	}

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FShadowProjectionShaderParameters& P)
	{
		Ar << P.DeferredParameters;
		Ar << P.ScreenToShadowMatrix;
		Ar << P.SoftTransitionScale;
		Ar << P.ShadowBufferSize;
		Ar << P.ShadowDepthTexture;
		Ar << P.ShadowDepthTextureSampler;
		Ar << P.ShadowDepthTextureObject;
		Ar << P.ShadowDepthSampler;
		Ar << P.ProjectionDepthBias;
		Ar << P.FadePlaneOffset;
		Ar << P.InvFadePlaneLength;
		return Ar;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter ScreenToShadowMatrix;
	FShaderParameter SoftTransitionScale;
	FShaderParameter ShadowBufferSize;
	FShaderResourceParameter ShadowDepthTexture;
	FShaderResourceParameter ShadowDepthTextureSampler;
	FShaderResourceParameter ShadowDepthTextureObject;
	FShaderResourceParameter ShadowDepthSampler;
	FShaderParameter ProjectionDepthBias;
	FShaderParameter FadePlaneOffset;
	FShaderParameter InvFadePlaneLength;
};

/**
 * TShadowProjectionPS
 * A pixel shader for projecting a shadow depth buffer onto the scene.  Used with any light type casting normal shadows.
 */
template<uint32 Quality, bool bUseFadePlane = false> 
class TShadowProjectionPS : public FShadowProjectionPixelShaderInterface
{
	DECLARE_SHADER_TYPE(TShadowProjectionPS,Global);
public:

	TShadowProjectionPS()
		: FShadowProjectionPixelShaderInterface()
	{ 
	}

	/**
	 * Constructor - binds all shader params and initializes the sample offsets
	 * @param Initializer - init data from shader compiler
	 */
	TShadowProjectionPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShadowProjectionPixelShaderInterface(Initializer)
	{
		ProjectionParameters.Bind(Initializer.ParameterMap);
		ShadowFadeFraction.Bind(Initializer.ParameterMap,TEXT("ShadowFadeFraction"));
		ShadowSharpen.Bind(Initializer.ParameterMap,TEXT("ShadowSharpen"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

	/**
	 * Add any defines required by the shader
	 * @param OutEnvironment - shader environment to modify
	 */
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FShadowProjectionPixelShaderInterface::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("SHADOW_QUALITY"), Quality);
		OutEnvironment.SetDefine(TEXT("USE_FADE_PLANE"), (uint32)(bUseFadePlane ? 1 : 0));
	}

	/**
	 * Sets the pixel shader's parameters
	 * @param View - current view
	 * @param ShadowInfo - projected shadow info for a single light
	 */
	virtual void SetParameters(
		int32 ViewIndex,
		const FSceneView& View,
		const FProjectedShadowInfo* ShadowInfo
		)
	{
		FShadowProjectionPixelShaderInterface::SetParameters(ViewIndex,View,ShadowInfo);

		ProjectionParameters.Set(this, View, ShadowInfo);

		SetShaderValue( GetPixelShader(), ShadowFadeFraction, ShadowInfo->FadeAlphas[ViewIndex] );
		SetShaderValue( GetPixelShader(), ShadowSharpen, ShadowInfo->LightSceneInfo->Proxy->GetShadowSharpen() * 7.0f + 1.0f );
	}

	/**
	 * Serialize the parameters for this shader
	 * @param Ar - archive to serialize to
	 */
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FShadowProjectionPixelShaderInterface::Serialize(Ar);
		Ar << ProjectionParameters;
		Ar << ShadowFadeFraction;
		Ar << ShadowSharpen;
		return bShaderHasOutdatedParameters;
	}

protected:
	FShadowProjectionShaderParameters ProjectionParameters;
	FShaderParameter ShadowFadeFraction;
	FShaderParameter ShadowSharpen;
};

/** Translucency shadow projection parameters used by multiple shaders. */
class FTranslucencyShadowProjectionShaderParameters
{
public:

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		TranslucencyShadowTransmission0.Bind(ParameterMap,TEXT("TranslucencyShadowTransmission0"));
		TranslucencyShadowTransmission0Sampler.Bind(ParameterMap,TEXT("TranslucencyShadowTransmission0Sampler"));
		TranslucencyShadowTransmission1.Bind(ParameterMap,TEXT("TranslucencyShadowTransmission1"));
		TranslucencyShadowTransmission1Sampler.Bind(ParameterMap,TEXT("TranslucencyShadowTransmission1Sampler"));
	}

	void Set(FShader* Shader) const
	{
		SetTextureParameter(
			Shader->GetPixelShader(),
			TranslucencyShadowTransmission0,
			TranslucencyShadowTransmission0Sampler,
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			GSceneRenderTargets.TranslucencyShadowTransmission[0]->GetRenderTargetItem().ShaderResourceTexture
			);

		SetTextureParameter(
			Shader->GetPixelShader(),
			TranslucencyShadowTransmission1,
			TranslucencyShadowTransmission1Sampler,
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			GSceneRenderTargets.TranslucencyShadowTransmission[1]->GetRenderTargetItem().ShaderResourceTexture
			);
	}

	/** Serializer. */ 
	friend FArchive& operator<<(FArchive& Ar,FTranslucencyShadowProjectionShaderParameters& P)
	{
		Ar << P.TranslucencyShadowTransmission0;
		Ar << P.TranslucencyShadowTransmission0Sampler;
		Ar << P.TranslucencyShadowTransmission1;
		Ar << P.TranslucencyShadowTransmission1Sampler;
		return Ar;
	}

private:

	FShaderResourceParameter TranslucencyShadowTransmission0;
	FShaderResourceParameter TranslucencyShadowTransmission0Sampler;
	FShaderResourceParameter TranslucencyShadowTransmission1;
	FShaderResourceParameter TranslucencyShadowTransmission1Sampler;
};

/** Pixel shader to project both opaque and translucent shadows onto opaque surfaces. */
template<uint32 Quality> 
class TShadowProjectionFromTranslucencyPS : public TShadowProjectionPS<Quality>
{
	DECLARE_SHADER_TYPE(TShadowProjectionFromTranslucencyPS,Global);
public:

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		TShadowProjectionPS<Quality>::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("APPLY_TRANSLUCENCY_SHADOWS"), 1);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4) && TShadowProjectionPS<Quality>::ShouldCache(Platform);
	}

	TShadowProjectionFromTranslucencyPS() {}

	TShadowProjectionFromTranslucencyPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		TShadowProjectionPS<Quality>(Initializer)
	{
		TranslucencyProjectionParameters.Bind(Initializer.ParameterMap);
	}

	virtual void SetParameters(
		int32 ViewIndex,
		const FSceneView& View,
		const FProjectedShadowInfo* ShadowInfo)
	{
		TShadowProjectionPS<Quality>::SetParameters(ViewIndex, View, ShadowInfo);

		TranslucencyProjectionParameters.Set(this);
	}

	/**
	 * Serialize the parameters for this shader
	 * @param Ar - archive to serialize to
	 */
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = TShadowProjectionPS<Quality>::Serialize(Ar);
		Ar << TranslucencyProjectionParameters;
		return bShaderHasOutdatedParameters;
	}

protected:
	FTranslucencyShadowProjectionShaderParameters TranslucencyProjectionParameters;
};


/** One pass point light shadow projection parameters used by multiple shaders. */
class FOnePassPointShadowProjectionShaderParameters
{
public:

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		ShadowDepthTexture.Bind(ParameterMap,TEXT("ShadowDepthCubeTexture"));
		ShadowDepthCubeComparisonSampler.Bind(ParameterMap,TEXT("ShadowDepthCubeTextureSampler"));
		ShadowViewProjectionMatrices.Bind(ParameterMap, TEXT("ShadowViewProjectionMatrices"));
		InvShadowmapResolution.Bind(ParameterMap, TEXT("InvShadowmapResolution"));
	}

	template<typename ShaderRHIParamRef>
	void Set(const ShaderRHIParamRef ShaderRHI, const FProjectedShadowInfo* ShadowInfo) const
	{
		SetTextureParameter(
			ShaderRHI, 
			ShadowDepthTexture, 
			GSceneRenderTargets.GetCubeShadowDepthZTexture(ShadowInfo->ResolutionX)
			);

		if (ShadowDepthCubeComparisonSampler.IsBound())
		{
			RHISetShaderSampler(
				ShaderRHI, 
				ShadowDepthCubeComparisonSampler.GetBaseIndex(), 
				// Use a comparison sampler to do hardware PCF
				TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp, 0, 0, 0, SCF_Less>::GetRHI()
				);
		}

		SetShaderValueArray<ShaderRHIParamRef, FMatrix>(
			ShaderRHI,
			ShadowViewProjectionMatrices,
			ShadowInfo->OnePassShadowViewProjectionMatrices.GetData(),
			ShadowInfo->OnePassShadowViewProjectionMatrices.Num()
			);

		SetShaderValue(ShaderRHI,InvShadowmapResolution,1.0f / ShadowInfo->ResolutionX);
	}

	/** Serializer. */ 
	friend FArchive& operator<<(FArchive& Ar,FOnePassPointShadowProjectionShaderParameters& P)
	{
		Ar << P.ShadowDepthTexture;
		Ar << P.ShadowDepthCubeComparisonSampler;
		Ar << P.ShadowViewProjectionMatrices;
		Ar << P.InvShadowmapResolution;
		return Ar;
	}

private:

	FShaderResourceParameter ShadowDepthTexture;
	FShaderResourceParameter ShadowDepthCubeComparisonSampler;
	FShaderParameter ShadowViewProjectionMatrices;
	FShaderParameter InvShadowmapResolution;
};

/**
 * Pixel shader used to project one pass point light shadows.
 */
// Quality = 0 / 1
template <uint32 Quality>
class TOnePassPointShadowProjectionPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TOnePassPointShadowProjectionPS,Global);
public:

	TOnePassPointShadowProjectionPS() {}

	TOnePassPointShadowProjectionPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		OnePassShadowParameters.Bind(Initializer.ParameterMap);
		LightPosition.Bind(Initializer.ParameterMap,TEXT("LightPositionAndInvRadius"));
		ShadowFadeFraction.Bind(Initializer.ParameterMap,TEXT("ShadowFadeFraction"));
		ShadowSharpen.Bind(Initializer.ParameterMap,TEXT("ShadowSharpen"));
		PointLightDepthBiasParameters.Bind(Initializer.ParameterMap,TEXT("PointLightDepthBiasParameters"));
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("SHADOW_QUALITY"), Quality);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	void SetParameters(
		int32 ViewIndex,
		const FSceneView& View,
		const FProjectedShadowInfo* ShadowInfo
		)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI,View);

		DeferredParameters.Set(ShaderRHI, View);
		OnePassShadowParameters.Set(ShaderRHI, ShadowInfo);

		SetShaderValue(ShaderRHI, LightPosition, FVector4(ShadowInfo->LightSceneInfo->Proxy->GetPosition(), 1.0f / ShadowInfo->LightSceneInfo->Proxy->GetRadius()));

		SetShaderValue(ShaderRHI, ShadowFadeFraction, ShadowInfo->FadeAlphas[ViewIndex]);
		SetShaderValue(ShaderRHI, ShadowSharpen, ShadowInfo->LightSceneInfo->Proxy->GetShadowSharpen() * 7.0f + 1.0f);
		SetShaderValue(ShaderRHI, PointLightDepthBiasParameters, FVector2D(ShadowInfo->GetShaderDepthBias(), 0.0f));
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << OnePassShadowParameters;
		Ar << LightPosition;
		Ar << ShadowFadeFraction;
		Ar << ShadowSharpen;
		Ar << PointLightDepthBiasParameters;
		return bShaderHasOutdatedParameters;
	}

private:
	FDeferredPixelShaderParameters DeferredParameters;
	FOnePassPointShadowProjectionShaderParameters OnePassShadowParameters;
	FShaderParameter LightPosition;
	FShaderParameter ShadowFadeFraction;
	FShaderParameter ShadowSharpen;
	FShaderParameter PointLightDepthBiasParameters;

};

#endif
