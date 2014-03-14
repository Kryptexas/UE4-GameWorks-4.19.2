// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SceneManagement.h: Scene manager definitions.
=============================================================================*/

#pragma once

// Includes the draw mesh macros
#include "SceneUtils.h"
#include "UniformBuffer.h"
#include "EngineSceneClasses.h"
#include "BufferVisualizationData.h"
#include "ConvexVolume.h"
#include "SystemSettings.h"
#include "Engine/TextureLightProfile.h"
#include "Engine/World.h"
#include "RendererInterface.h"

// Forward declarations.
class FSceneViewFamily;
class FSceneInterface;
class FLightSceneInfo;
class ULightComponent;
class UDecalComponent;
class FIndexBuffer;
class FVertexBuffer;
class FVertexFactory;
class FAtmosphericFogSceneInfo;
class ISceneViewExtension;


DECLARE_LOG_CATEGORY_EXTERN(LogBufferVisualization, Log, All);

/** 
 * Class used to identify UPrimitiveComponents on the rendering thread without having to pass the pointer around, 
 * Which would make it easy for people to access game thread variables from the rendering thread.
 */
class FPrimitiveComponentId
{
public:

	FPrimitiveComponentId() : Value(0)
	{}

	inline bool IsValid() const
	{
		return Value > 0;
	}

	inline bool operator==(FPrimitiveComponentId OtherId) const
	{
		return Value == OtherId.Value;
	}

	friend uint32 GetTypeHash( FPrimitiveComponentId Id )
	{
		return GetTypeHash(Id.Value);
	}

	uint64 Value;
};

// -----------------------------------------------------------------------------

struct FViewMatrices
{
	FViewMatrices()
	{
		ProjMatrix.SetIdentity();
		ViewMatrix.SetIdentity();
		TranslatedViewProjectionMatrix.SetIdentity();
		InvTranslatedViewProjectionMatrix.SetIdentity();
		PreViewTranslation = FVector::ZeroVector;
		ViewOrigin = FVector::ZeroVector;
	}

	/** ViewToClip : UE4 projection matrix projects such that clip space Z=1 is the near plane, and Z=0 is the infinite far plane. */
	FMatrix		ProjMatrix;
	// WorldToView..
	FMatrix		ViewMatrix;
	/** The view-projection transform, starting from world-space points translated by -ViewOrigin. */
	FMatrix		TranslatedViewProjectionMatrix;
	/** The inverse view-projection transform, ending with world-space points translated by -ViewOrigin. */
	FMatrix		InvTranslatedViewProjectionMatrix;
	/** The translation to apply to the world before TranslatedViewProjectionMatrix. Usually it is -ViewOrigin but with rereflections this can differ */
	FVector		PreViewTranslation;
	/** To support ortho and other modes this is redundant, in world space */
	FVector		ViewOrigin;

	//
	// World = TranslatedWorld - PreViewTranslation
	// TranslatedWorld = World + PreViewTranslation
	// 

// ----------------

	/** @return true:perspective, false:orthographic */
	inline bool IsPerspectiveProjection() const
	{
		return ProjMatrix.M[3][3] < 1.0f;
	}

	FMatrix GetViewProjMatrix() const
	{
		return ViewMatrix * ProjMatrix;
	}

	FMatrix GetViewRotationProjMatrix() const
	{
		return ViewMatrix.RemoveTranslation() * ProjMatrix;
	}

	FMatrix GetInvProjMatrix() const
	{
		return ProjMatrix.Inverse();
	}

	FMatrix GetInvViewMatrix() const
	{
		// can be optimized: it's not a perspective matrix so transpose would be enough
		return ViewMatrix.Inverse();
	}
	
	FMatrix GetInvViewProjMatrix() const
	{
		return GetInvProjMatrix() * GetInvViewMatrix();
	}
};

/**
 * The scene manager's persistent view state.
 */
class FSceneViewStateInterface
{
public:
	FSceneViewStateInterface()
		:	ViewParent( NULL )
		,	NumChildren( 0 )
	{}
	
	/** Called in the game thread to destroy the view state. */
	virtual void Destroy() = 0;

public:
	/** Sets the view state's scene parent. */
	void SetViewParent(FSceneViewStateInterface* InViewParent)
	{
		if ( ViewParent )
		{
			// Assert that the existing parent does not have a parent.
			check( !ViewParent->HasViewParent() );
			// Decrement ref ctr of existing parent.
			--ViewParent->NumChildren;
		}

		if ( InViewParent && InViewParent != this )
		{
			// Assert that the incoming parent does not have a parent.
			check( !InViewParent->HasViewParent() );
			ViewParent = InViewParent;
			// Increment ref ctr of new parent.
			InViewParent->NumChildren++;
		}
		else
		{
			ViewParent = NULL;
		}
	}
	/** @return			The view state's scene parent, or NULL if none present. */
	FSceneViewStateInterface* GetViewParent()
	{
		return ViewParent;
	}
	/** @return			The view state's scene parent, or NULL if none present. */
	const FSceneViewStateInterface* GetViewParent() const
	{
		return ViewParent;
	}
	/** @return			true if the scene state has a parent, false otherwise. */
	bool HasViewParent() const
	{
		return GetViewParent() != NULL;
	}
	/** @return			true if this scene state is a parent, false otherwise. */
	bool IsViewParent() const
	{
		return NumChildren > 0;
	}
	
	virtual void AddReferencedObjects(FReferenceCollector& Collector) = 0;

	virtual SIZE_T GetSizeBytes() const { return 0; }

	/** called in InitViews() */
	virtual void OnStartFrame(FSceneView& CurrentView) = 0;

	/** Resets pool for GetReusableMID() */
	virtual void OnStartPostProcessing(FSceneView& CurrentView) = 0;
	/** Allows MIDs being created and released during view rendering without the overhead of creating and relasing objects */
	virtual UMaterialInstanceDynamic* GetReusableMID(class UMaterialInterface* ParentMaterial) = 0;
	/** Called on world origin chnages */
	virtual void ApplyWorldOffset(FVector InOffset) = 0;

protected:
	// Don't allow direct deletion of the view state, Destroy should be called instead.
	virtual ~FSceneViewStateInterface() {}

private:
	/** This scene state's view parent; NULL if no parent present. */
	FSceneViewStateInterface*	ViewParent;
	/** Reference counts the number of children parented to this state. */
	int32							NumChildren;
};

/** 
 * Class used to reference an FSceneViewStateInterface that allows destruction and recreation of all FSceneViewStateInterface's when needed. 
 * This is used to support reloading the renderer module on the fly.
 */
class FSceneViewStateReference
{
public:
	FSceneViewStateReference() :
		Reference(NULL)
	{}

	ENGINE_API virtual ~FSceneViewStateReference();

	/** Allocates the Scene view state. */
	ENGINE_API void Allocate();

	/** Destorys the Scene view state. */
	ENGINE_API void Destroy();

	/** Destroys all view states, but does not remove them from the linked list. */
	ENGINE_API static void DestroyAll();

	/** Recreates all view states in the global list. */
	ENGINE_API static void AllocateAll();

	FSceneViewStateInterface* GetReference()
	{
		return Reference;
	}

private:
	FSceneViewStateInterface* Reference;
	TLinkedList<FSceneViewStateReference*> GlobalListLink;

	static TLinkedList<FSceneViewStateReference*>*& GetSceneViewStateList();
};

/**
 * The types of interactions between a light and a primitive.
 */
enum ELightInteractionType
{
	LIT_CachedIrrelevant,
	LIT_CachedLightMap,
	LIT_Dynamic,
	LIT_CachedSignedDistanceFieldShadowMap2D
};

/**
 * Information about an interaction between a light and a mesh.
 */
class FLightInteraction
{
public:

	// Factory functions.
	static FLightInteraction Dynamic() { return FLightInteraction(LIT_Dynamic); }
	static FLightInteraction LightMap() { return FLightInteraction(LIT_CachedLightMap); }
	static FLightInteraction Irrelevant() { return FLightInteraction(LIT_CachedIrrelevant); }
	static FLightInteraction ShadowMap2D() { return FLightInteraction(LIT_CachedSignedDistanceFieldShadowMap2D); }

	// Accessors.
	ELightInteractionType GetType() const { return Type; }

private:

	/**
	 * Minimal initialization constructor.
	 */
	FLightInteraction(
		ELightInteractionType InType
		):
		Type(InType)
	{}

	ELightInteractionType Type;
};

/**
 * The types of interactions between a light and a primitive.
 */
enum ELightMapInteractionType
{
	LMIT_None	= 0,
	LMIT_Texture= 2,

	LMIT_NumBits= 3
};

enum EShadowMapInteractionType
{
	SMIT_None	= 0,
	SMIT_Texture= 2,

	SMIT_NumBits= 3
};

/** The number of coefficients that are stored for each light sample. */ 
static const int32 NUM_STORED_LIGHTMAP_COEF = 4;

/** The number of directional coefficients which the lightmap stores for each light sample. */ 
static const int32 NUM_HQ_LIGHTMAP_COEF = 2;

/** The number of simple coefficients which the lightmap stores for each light sample. */ 
static const int32 NUM_LQ_LIGHTMAP_COEF = 2;

/** The index at which simple coefficients are stored in any array containing all NUM_STORED_LIGHTMAP_COEF coefficients. */ 
static const int32 LQ_LIGHTMAP_COEF_INDEX = 2;

/** Compile out low quality lightmaps to save memory */
// @todo-mobile: Need to fix this!
#define ALLOW_LQ_LIGHTMAPS (PLATFORM_DESKTOP || PLATFORM_IOS || PLATFORM_ANDROID || PLATFORM_HTML5 )

/** Compile out high quality lightmaps to save memory */
#define ALLOW_HQ_LIGHTMAPS 1

/** Make sure at least one is defined */
#if !ALLOW_LQ_LIGHTMAPS && !ALLOW_HQ_LIGHTMAPS
#error At least one of ALLOW_LQ_LIGHTMAPS and ALLOW_HQ_LIGHTMAPS needs to be defined!
#endif

/**
 * Information about an interaction between a light and a mesh.
 */
class FLightMapInteraction
{
public:

	// Factory functions.
	static FLightMapInteraction None()
	{
		FLightMapInteraction Result;
		Result.Type = LMIT_None;
		return Result;
	}
	static FLightMapInteraction Texture(
		const class ULightMapTexture2D* const* InTextures,
		const ULightMapTexture2D* InSkyOcclusionTexture,
		const FVector4* InCoefficientScales,
		const FVector4* InCoefficientAdds,
		const FVector2D& InCoordinateScale,
		const FVector2D& InCoordinateBias,
		bool bAllowHighQualityLightMaps);

	/** Default constructor. */
	FLightMapInteraction():
		SkyOcclusionTexture(NULL),
		Type(LMIT_None)
	{}

	// Accessors.
	ELightMapInteractionType GetType() const { return Type; }
	
	const ULightMapTexture2D* GetTexture() const
	{
		check(Type == LMIT_Texture);
#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
		return AllowsHighQualityLightmaps() ? HighQualityTexture : LowQualityTexture;
#elif ALLOW_HQ_LIGHTMAPS
		return HighQualityTexture;
#else
		return LowQualityTexture;
#endif
	}

	const ULightMapTexture2D* GetSkyOcclusionTexture() const
	{
		check(Type == LMIT_Texture);
#if ALLOW_HQ_LIGHTMAPS
		return SkyOcclusionTexture;
#else
		return NULL;
#endif
	}

	const FVector4* GetScaleArray() const
	{
#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
		return AllowsHighQualityLightmaps() ? HighQualityCoefficientScales : LowQualityCoefficientScales;
#elif ALLOW_HQ_LIGHTMAPS
		return HighQualityCoefficientScales;
#else
		return LowQualityCoefficientScales;
#endif
	}

	const FVector4* GetAddArray() const
	{
#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
		return AllowsHighQualityLightmaps() ? HighQualityCoefficientAdds : LowQualityCoefficientAdds;
#elif ALLOW_HQ_LIGHTMAPS
		return HighQualityCoefficientAdds;
#else
		return LowQualityCoefficientAdds;
#endif
	}
	
	const FVector2D& GetCoordinateScale() const
	{
		check(Type == LMIT_Texture);
		return CoordinateScale;
	}
	const FVector2D& GetCoordinateBias() const
	{
		check(Type == LMIT_Texture);
		return CoordinateBias;
	}

	uint32 GetNumLightmapCoefficients() const
	{
#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
#if PLATFORM_DESKTOP && (!(UE_BUILD_SHIPPING || UE_BUILD_TEST) || WITH_EDITOR)		// This is to allow for dynamic switching between simple and directional light maps in the PC editor
		if( !AllowsHighQualityLightmaps() )
		{
			return NUM_LQ_LIGHTMAP_COEF;
		}
#endif
		return NumLightmapCoefficients;
#elif ALLOW_HQ_LIGHTMAPS
		return NUM_HQ_LIGHTMAP_COEF;
#else
		return NUM_LQ_LIGHTMAP_COEF;
#endif
	}

	/**
	* @return true if high quality lightmaps are allowed
	*/
	FORCEINLINE bool AllowsHighQualityLightmaps() const
	{
#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
		return bAllowHighQualityLightMaps;
#elif ALLOW_HQ_LIGHTMAPS
		return true;
#else
		return false;
#endif
	}

	/** These functions are used for the Dummy lightmap policy used in LightMap density view mode. */
	/** 
	 *	Set the type.
	 *
	 *	@param	InType				The type to set it to.
	 */
	void SetLightMapInteractionType(ELightMapInteractionType InType)
	{
		Type = InType;
	}
	/** 
	 *	Set the coordinate scale.
	 *
	 *	@param	InCoordinateScale	The scale to set it to.
	 */
	void SetCoordinateScale(const FVector2D& InCoordinateScale)
	{
		CoordinateScale = InCoordinateScale;
	}
	/** 
	 *	Set the coordinate bias.
	 *
	 *	@param	InCoordinateBias	The bias to set it to.
	 */
	void SetCoordinateBias(const FVector2D& InCoordinateBias)
	{
		CoordinateBias = InCoordinateBias;
	}

private:

#if ALLOW_HQ_LIGHTMAPS
	FVector4 HighQualityCoefficientScales[NUM_HQ_LIGHTMAP_COEF];
	FVector4 HighQualityCoefficientAdds[NUM_HQ_LIGHTMAP_COEF];
	const class ULightMapTexture2D* HighQualityTexture;
	const ULightMapTexture2D* SkyOcclusionTexture;
#endif

#if ALLOW_LQ_LIGHTMAPS
	FVector4 LowQualityCoefficientScales[NUM_LQ_LIGHTMAP_COEF];
	FVector4 LowQualityCoefficientAdds[NUM_LQ_LIGHTMAP_COEF];
	const class ULightMapTexture2D* LowQualityTexture;
#endif

#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
	bool bAllowHighQualityLightMaps;
	uint32 NumLightmapCoefficients;
#endif

	ELightMapInteractionType Type;

	FVector2D CoordinateScale;
	FVector2D CoordinateBias;
};

/** Information about the static shadowing information for a primitive. */
class FShadowMapInteraction
{
public:

	// Factory functions.
	static FShadowMapInteraction None()
	{
		FShadowMapInteraction Result;
		Result.Type = SMIT_None;
		return Result;
	}
	static FShadowMapInteraction Texture(
		class UShadowMapTexture2D* InTexture,
		const FVector2D& InCoordinateScale,
		const FVector2D& InCoordinateBias,
		const bool* InChannelValid)
	{
		FShadowMapInteraction Result;
		Result.Type = SMIT_Texture;
		Result.ShadowTexture = InTexture;
		Result.CoordinateScale = InCoordinateScale;
		Result.CoordinateBias = InCoordinateBias;
		
		for (int Channel = 0; Channel < 4; Channel++)
		{
			Result.bChannelValid[Channel] = InChannelValid[Channel];
		}

		return Result;
	}

	/** Default constructor. */
	FShadowMapInteraction() :
		Type(SMIT_None),
		ShadowTexture(NULL)
	{
		for (int Channel = 0; Channel < ARRAY_COUNT(bChannelValid); Channel++)
		{
			bChannelValid[Channel] = false;
		}
	}

	// Accessors.
	EShadowMapInteractionType GetType() const { return Type; }

	UShadowMapTexture2D* GetTexture() const
	{
		checkSlow(Type == SMIT_Texture);
		return ShadowTexture;
	}

	const FVector2D& GetCoordinateScale() const
	{
		checkSlow(Type == SMIT_Texture);
		return CoordinateScale;
	}

	const FVector2D& GetCoordinateBias() const
	{
		checkSlow(Type == SMIT_Texture);
		return CoordinateBias;
	}

	bool GetChannelValid(int32 ChannelIndex) const
	{
		checkSlow(Type == SMIT_Texture);
		return bChannelValid[ChannelIndex];
	}

private:

	EShadowMapInteractionType Type;
	UShadowMapTexture2D* ShadowTexture;
	FVector2D CoordinateScale;
	FVector2D CoordinateBias;
	bool bChannelValid[4];
};

/**
 * An interface to cached lighting for a specific mesh.
 */
class FLightCacheInterface
{
public:
	virtual FLightInteraction GetInteraction(const class FLightSceneProxy* LightSceneProxy) const = 0;
	virtual FLightMapInteraction GetLightMapInteraction() const = 0;
	virtual FShadowMapInteraction GetShadowMapInteraction() const { return FShadowMapInteraction::None(); }
};

// Information about a single shadow cascade.
class FShadowCascadeSettings
{
public:
	// The following 3 floats represent the view space depth of the split planes for this cascade.
	// SplitNear <= FadePlane <= SplitFar

	// The distance from the camera to the near split plane, in world units (linear).
	float SplitNear;

	// The distance from the camera to the far split plane, in world units (linear).
	float SplitFar;

	// in world units (linear).
	float SplitNearFadeRegion;

	// in world units (linear).
	float SplitFarFadeRegion;

	// ??
	// The distance from the camera to the start of the fade region, in world units (linear).
	// The area between the fade plane and the far split plane is blended to smooth between cascades.
	float FadePlaneOffset;

	// The length of the fade region (SplitFar - FadePlaneOffset), in world units (linear).
	float FadePlaneLength;

	// The accurate bounds of the cascade used for primitive culling.
	FConvexVolume ShadowBoundsAccurate;

	FPlane NearFrustumPlane;
	FPlane FarFrustumPlane;

	FShadowCascadeSettings()
		: SplitNear(0.0f)
		, SplitFar(WORLD_MAX)
		, SplitNearFadeRegion(0.0f)
		, SplitFarFadeRegion(0.0f)
		, FadePlaneOffset(SplitFar)
		, FadePlaneLength(SplitFar - FadePlaneOffset)
	{
	}
};

/** A projected shadow transform. */
class ENGINE_API FProjectedShadowInitializer
{
public:

	/** A translation that is applied to world-space before transforming by one of the shadow matrices. */
	FVector PreShadowTranslation;

	FMatrix WorldToLight;
	/** Non-uniform scale to be applied after WorldToLight. */
	FVector Scales;

	FVector FaceDirection;
	FBoxSphereBounds SubjectBounds;
	FVector4 WAxis;
	float MinLightW;
	float MaxDistanceToCastInLightW;

	/** Whether the shadow is for a directional light. */
	uint32 bDirectionalLight : 1;

	/** Default constructor. */
	FProjectedShadowInitializer()
	:	bDirectionalLight(false)
	{}
};

/** Information needed to create a per-object projected shadow. */
class ENGINE_API FPerObjectProjectedShadowInitializer : public FProjectedShadowInitializer
{
public:

};

/** Information needed to create a whole scene reflective shadow map projected shadow. */
class ENGINE_API FRsmWholeSceneProjectedShadowInitializer : public FProjectedShadowInitializer
{
public:
	FRsmWholeSceneProjectedShadowInitializer()
	 : FProjectedShadowInitializer() 
	 , LightPropagationVolumeBounds(ForceInitToZero)
	{}

	FBox LightPropagationVolumeBounds;
	FShadowCascadeSettings	CascadeSettings;
};

/** Information needed to create a whole scene projected shadow. */
class ENGINE_API FWholeSceneProjectedShadowInitializer : public FProjectedShadowInitializer
{
public:
	int32 SplitIndex;
	
	FShadowCascadeSettings CascadeSettings;

	/** Whether the shadow is a point light shadow that renders all faces of a cubemap in one pass. */
	uint32 bOnePassPointLightShadow : 1;

	/** Culling planes used by CSM. */
	TArray<FPlane, TInlineAllocator<8> > FrustumCullPlanes;

	FWholeSceneProjectedShadowInitializer()
	:	SplitIndex(INDEX_NONE)
	,	bOnePassPointLightShadow(false)
	{}	
};

/** Represents a USkyLightComponent to the rendering thread. */
class ENGINE_API FSkyLightSceneProxy
{
public:

	/** Initialization constructor. */
	FSkyLightSceneProxy(const class USkyLightComponent* InLightComponent);

	const USkyLightComponent* LightComponent;
	FTexture* ProcessedTexture;
	float SkyDistanceThreshold;
	bool bCastShadows;
	bool bPrecomputedLightingIsValid;
	FLinearColor LightColor;
	FSHVectorRGB3 IrradianceEnvironmentMap;
};

/** different light component types */
enum ELightComponentType
{
	LightType_Directional = 0,
	LightType_Point,
	LightType_Spot,
	LightType_MAX,
	LightType_NumBits = 2
};

/** Encapsulates the data which is used to render a light parallel to the game thread. */
class ENGINE_API FLightSceneProxy
{
public:

	/** Initialization constructor. */
	FLightSceneProxy(const ULightComponent* InLightComponent);

	virtual ~FLightSceneProxy() 
	{
	}

	/**
	 * Tests whether the light affects the given bounding volume.
	 * @param Bounds - The bounding volume to test.
	 * @return True if the light affects the bounding volume
	 */
	virtual bool AffectsBounds(const FBoxSphereBounds& Bounds) const
	{
		return true;
	}

	virtual FSphere GetBoundingSphere() const
	{
		// Directional lights will have a radius of WORLD_MAX
		return FSphere(GetPosition(), FMath::Min(GetRadius(), (float)WORLD_MAX));
	}

	/** @return radius of the light */
	virtual float GetRadius() const { return FLT_MAX; }
	virtual float GetOuterConeAngle() const { return 0.0f; }
	virtual float GetSourceRadius() const { return 0.0f; }
	virtual bool IsInverseSquared() const { return false; }

	virtual FVector2D GetLightShaftConeParams() const
	{
		return FVector2D::ZeroVector;
	}

	virtual float GetDepthBiasScale() const 
	{ 
		return 1;
	}

	virtual float GetShadowTransitionScale() const 
	{ 
		return 1;
	}

	/** Accesses parameters needed for rendering the light. */
	virtual void GetParameters(FVector4& LightPositionAndInvRadius, FVector4& LightColorAndFalloffExponent, FVector& NormalizedLightDirection, FVector2D& SpotAngles, float& LightSourceRadius, float& LightSourceLength, float& LightMinRoughness) const {}

	virtual FVector2D GetDirectionalLightDistanceFadeParameters() const
	{
		return FVector2D(0, 0);
	}

	virtual bool GetLightShaftOcclusionParameters(float& OutOcclusionMaskDarkness, float& OutOcclusionDepthRange) const
	{
		OutOcclusionMaskDarkness = 0;
		OutOcclusionDepthRange = 1;
		return false;
	}

	virtual FVector GetLightPositionForLightShafts(FVector ViewOrigin) const
	{
		return GetPosition();
	}

	/**
	 * Sets up a projected shadow initializer for shadows from the entire scene.
	 * @return True if the whole-scene projected shadow should be used.
	 */
	virtual bool GetWholeSceneProjectedShadowInitializer(const FSceneViewFamily& ViewFamily, TArray<class FWholeSceneProjectedShadowInitializer, TInlineAllocator<6> >& OutInitializers) const
	{
		return false;
	}

	/** Called when precomputed lighting has been determined to be invalid */
	virtual void InvalidatePrecomputedLighting(bool bIsEditor) {}

	/** Whether this light should create per object shadows for dynamic objects. */
	virtual bool ShouldCreatePerObjectShadowsForDynamicObjects() const;

	virtual int32 GetNumViewDependentWholeSceneShadows(const FSceneView& View) const { return 0; }

	/**
	 * Sets up a projected shadow initializer that's dependent on the current view for shadows from the entire scene.
	 * @return True if the whole-scene projected shadow should be used.
	 */
	virtual bool GetViewDependentWholeSceneProjectedShadowInitializer(
		const class FSceneView& View, 
		int32 SplitIndex,
		class FWholeSceneProjectedShadowInitializer& OutInitializer) const
	{
		return false;
	}

	/**
	 * Sets up a projected shadow initializer for a reflective shadow map that's dependent on the current view for shadows from the entire scene.
	 * @return True if the whole-scene projected shadow should be used.
	 */
	virtual bool GetViewDependentRsmWholeSceneProjectedShadowInitializer(
		const class FSceneView& View, 
		const FBox& LightPropagationVolumeBounds,
		class FRsmWholeSceneProjectedShadowInitializer& OutInitializer ) const
	{
		return false;
	}

	/**
	 * Sets up a projected shadow initializer for the given subject.
	 * @param SubjectBounds - The bounding volume of the subject.
	 * @param OutInitializer - Upon successful return, contains the initialization parameters for the shadow.
	 * @return True if a projected shadow should be cast by this subject-light pair.
	 */
	virtual bool GetPerObjectProjectedShadowInitializer(const FBoxSphereBounds& SubjectBounds,class FPerObjectProjectedShadowInitializer& OutInitializer) const
	{
		return false;
	}

	// @param OutCascadeSettings can be 0
	virtual FSphere GetShadowSplitBounds(const class FSceneView& View, int32 SplitIndex, FShadowCascadeSettings* OutCascadeSettings) const { return FSphere(FVector::ZeroVector, 0); }

	virtual void SetScissorRect(const FSceneView& View) const
	{
	}

	// Accessors.
	float GetUserShadowBias() const { return ShadowBias; }

	/** 
	 * Note: The Rendering thread must not dereference UObjects!  
	 * The game thread owns UObject state and may be writing to them at any time.
	 * Mirror the data in the scene proxy and access that instead.
	 */
	inline const ULightComponent* GetLightComponent() const { return LightComponent; }
	inline FLightSceneInfo* GetLightSceneInfo() const { return LightSceneInfo; }
	inline const FMatrix& GetWorldToLight() const { return WorldToLight; }
	inline const FMatrix& GetLightToWorld() const { return LightToWorld; }
	inline FVector GetDirection() const { return FVector(WorldToLight.M[0][0],WorldToLight.M[1][0],WorldToLight.M[2][0]); }
	inline FVector GetOrigin() const { return LightToWorld.GetOrigin(); }
	inline FVector4 GetPosition() const { return Position; }
	inline const FLinearColor& GetColor() const { return Color; }
	inline float GetIndirectLightingScale() const { return IndirectLightingScale; }
	inline FGuid GetLightGuid() const { return LightGuid; }
	inline float GetShadowSharpen() const { return ShadowSharpen; }
	inline FVector GetLightFunctionScale() const { return LightFunctionScale; }
	inline float GetLightFunctionFadeDistance() const { return LightFunctionFadeDistance; }
	inline float GetLightFunctionDisabledBrightness() const { return LightFunctionDisabledBrightness; }
	inline UTextureLightProfile* GetIESTexture() const { return IESTexture; }
	inline FTexture* GetIESTextureResource() const { return IESTexture ? IESTexture->Resource : 0; }
	inline const FMaterialRenderProxy* GetLightFunctionMaterial() const { return LightFunctionMaterial; }
	inline bool HasStaticLighting() const { return bStaticLighting; }
	inline bool HasStaticShadowing() const { return bStaticShadowing; }
	inline bool CastsDynamicShadow() const { return bCastDynamicShadow; }
	inline bool CastsStaticShadow() const { return bCastStaticShadow; }
	inline bool CastsTranslucentShadows() const { return bCastTranslucentShadows; }
	inline bool AffectsTranslucentLighting() const { return bAffectTranslucentLighting; }
	inline uint8 GetLightType() const { return LightType; }
	inline FName GetComponentName() const { return ComponentName; }
	inline FName GetLevelName() const { return LevelName; }
	FORCEINLINE TStatId GetStatId() const 
	{ 
		return StatId; 
	}	
	inline int32 GetShadowMapChannel() const { return ShadowMapChannel; }
	inline bool IsUsedAsAtmosphereSunLight() const { return bUsedAsAtmosphereSunLight; }
	inline int32 GetPreviewShadowMapChannel() const { return PreviewShadowMapChannel; }

	inline bool HasReflectiveShadowMap() const { return bHasReflectiveShadowMap; }
	inline bool NeedsLPVInjection() const { return bAffectDynamicIndirectLighting; }

	/**
	 * Shifts light position and all relevant data by an arbitrary delta.
	 * Called on world origin changes
	 * @param InOffset - The delta to shift by
	 */
	virtual void ApplyWorldOffset(FVector InOffset);

protected:

	friend class FScene;

	/** The light component. */
	const ULightComponent* LightComponent;

	/** The light's scene info. */
	class FLightSceneInfo* LightSceneInfo;

	/** A transform from world space into light space. */
	FMatrix WorldToLight;

	/** A transform from light space into world space. */
	FMatrix LightToWorld;

	/** The homogenous position of the light. */
	FVector4 Position;

	/** The light color. */
	FLinearColor Color;

	/** Scale for indirect lighting from this light.  When 0, indirect lighting is disabled. */
	float IndirectLightingScale;

	/** 
	 * Controls how accurate self shadowing of whole scene shadows from this light are.  
	 * At close to 0, shadows will start far from their caster, and there won't be self shadowing artifacts.
	 * At close to 1, shadows will start very close to their caster, but there will be many self shadowing artifacts.
	 */
	float SelfShadowingAccuracy;

	/** User setting from light component, 0:no bias, 0.5:reasonable, larger object might appear to float */
	float ShadowBias;

	/** Sharpen shadow filtering */
	float ShadowSharpen;

	/** Min roughness */
	float MinRoughness;

	/** The light's persistent shadowing GUID. */
	FGuid LightGuid;

	/** 
	 * Shadow map channel which is used to match up with the appropriate static shadowing during a deferred shading pass.
	 * This is generated during a lighting build.
	 */
	int32 ShadowMapChannel;

	/** Transient shadowmap channel used to preview the results of stationary light shadowmap packing. */
	int32 PreviewShadowMapChannel;

	/** Light function parameters. */
	FVector	LightFunctionScale;
	float LightFunctionFadeDistance;
	float LightFunctionDisabledBrightness;
	const FMaterialRenderProxy* LightFunctionMaterial;

	/**
	 * IES texture (light profiles from real world measured data)
	 * We are safe to store a U pointer as those objects get deleted deferred, storing an FTexture pointer would crash if we recreate the texture 
	 */
	UTextureLightProfile* IESTexture;

	/**
	 * Return True if a light's parameters as well as its position is static during gameplay, and can thus use static lighting.
	 * A light with HasStaticLighting() == true will always have HasStaticShadowing() == true as well.
	 */
	const uint32 bStaticLighting : 1;

	/** 
	 * Whether the light has static direct shadowing.  
	 * The light may still have dynamic brightness and color. 
	 * The light may or may not also have static lighting.
	 */
	const uint32 bStaticShadowing : 1;

	/** True if the light casts dynamic shadows. */
	const uint32 bCastDynamicShadow : 1;

	/** True if the light casts static shadows. */
	const uint32 bCastStaticShadow : 1;

	/** Whether the light is allowed to cast dynamic shadows from translucency. */
	const uint32 bCastTranslucentShadows : 1;

	/** Whether the light affects translucency or not.  Disabling this can save GPU time when there are many small lights. */
	const uint32 bAffectTranslucentLighting : 1;

	/** Whether to consider light as a sunlight for atmospheric scattering. */
	const uint32 bUsedAsAtmosphereSunLight : 1;

	/** Does the light have dynamic GI? */
	const uint32 bAffectDynamicIndirectLighting : 1;
	const uint32 bHasReflectiveShadowMap : 1;

	/** The light type (ELightComponentType) */
	const uint8 LightType;

	/** The name of the light component. */
	FName ComponentName;

	/** The name of the level the light is in. */
	FName LevelName;

	/** Used for dynamic stats */
	TStatId StatId;

	/**
	 * Updates the light proxy's cached transforms.
	 * @param InLightToWorld - The new light-to-world transform.
	 * @param InPosition - The new position of the light.
	 */
	void SetTransform(const FMatrix& InLightToWorld,const FVector4& InPosition);

	/** Updates the light's color. */
	void SetColor(const FLinearColor& InColor);
};


/** Encapsulates the data which is used to render a decal parallel to the game thread. */
class ENGINE_API FDeferredDecalProxy
{
public:
	/** constructor */
	FDeferredDecalProxy(const UDecalComponent* InComponent);

	/**
	 * Updates the decal proxy's cached transform.
	 * @param InComponentToWorld - The new component-to-world transform.
	 */
	void SetTransform(const FTransform& InComponentToWorld);

	/** Pointer back to the game thread decal component. */
	const UDecalComponent* Component;

	UMaterialInterface* DecalMaterial;

	/** Used to compute the projection matrix on the render thread side  */
	FTransform ComponentTrans;

	/** 
	 * Whether the decal should be drawn or not
	 * This has to be passed to the rendering thread to handle G mode in the editor, where there is no game world, but we don't want to show components with HiddenGame set. 
	 */
	bool DrawInGame;

	bool bOwnerSelected;

	/** Larger values draw later (on top). */
	int32 SortOrder;
};

/** Reflection capture shapes. */
namespace EReflectionCaptureShape
{
	enum Type
	{
		Sphere,
		Box,
		Plane,
		Num
	};
}

/** Represents a reflection capture to the renderer. */
class ENGINE_API FReflectionCaptureProxy
{
public:
	const class UReflectionCaptureComponent* Component;

	int32 PackedIndex;

	/** Used in Feature level SM4 */
	FTexture* SM4FullHDRCubemap;

	/** Used in Feature level ES2 */
	FTexture* EncodedHDRCubemap;

	EReflectionCaptureShape::Type Shape;

	// Properties shared among all shapes
	FVector Position;
	float InfluenceRadius;
	float Brightness;
	uint32 Guid;

	// Box properties
	FMatrix BoxTransform;
	FVector BoxScales;
	float BoxTransitionDistance;

	// Plane properties
	FPlane ReflectionPlane;
	FVector4 ReflectionXAxisAndYScale;

	FReflectionCaptureProxy(const class UReflectionCaptureComponent* InComponent);

	void SetTransform(const FMatrix& InTransform);
};

/** Represents a wind source component to the scene manager in the rendering thread. */
class ENGINE_API FWindSourceSceneProxy
{
public:

	/** Initialization constructor. */
	FWindSourceSceneProxy(const FVector& InDirection,float InStrength,float InSpeed):
	  Position(FVector::ZeroVector),
		  Direction(InDirection),
		  Strength(InStrength),
		  Speed(InSpeed),
		  Radius(0),
		  bIsPointSource(false)
	  {}

	  /** Initialization constructor. */
	  FWindSourceSceneProxy(const FVector& InPosition,float InStrength,float InSpeed,float InRadius):
	  Position(InPosition),
		  Direction(FVector::ZeroVector),
		  Strength(InStrength),
		  Speed(InSpeed),
		  Radius(InRadius),
		  bIsPointSource(true)
	  {}

	  bool GetWindParameters(const FVector& EvaluatePosition, FVector4& WindDirectionAndSpeed, float& Strength) const;
	  bool GetDirectionalWindParameters(FVector4& WindDirectionAndSpeed, float& Strength) const;
	  void ApplyWorldOffset(FVector InOffset);

private:

	FVector Position;
	FVector	Direction;
	float Strength;
	float Speed;
	float Radius;
	bool bIsPointSource;
};

/** The uniform shader parameters associated with a primitive. */
BEGIN_UNIFORM_BUFFER_STRUCT(FPrimitiveUniformShaderParameters,ENGINE_API)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,LocalToWorld)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,WorldToLocal)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4,ObjectWorldPositionAndRadius)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector,ObjectBounds)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,LocalToWorldDeterminantSign,EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector,ActorWorldPosition)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,DecalReceiverMask,EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector4,ObjectOrientation,EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector4,NonUniformScale,EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector4,InvNonUniformScale,EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, LocalObjectBoundsMin)		// This is used in a custom material function (ObjectLocalBounds.uasset)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, LocalObjectBoundsMax)		// This is used in a custom material function (ObjectLocalBounds.uasset)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,LpvBiasMultiplier)
END_UNIFORM_BUFFER_STRUCT(FPrimitiveUniformShaderParameters)

/** Initializes the primitive uniform shader parameters. */
inline FPrimitiveUniformShaderParameters GetPrimitiveUniformShaderParameters(
	const FMatrix& LocalToWorld,
	FVector ActorPosition,
	const FBoxSphereBounds& WorldBounds,
	const FBoxSphereBounds& LocalBounds,
	bool bReceivesDecals,
	float LpvBiasMultiplier = 1.0f
)
{
	FPrimitiveUniformShaderParameters Result;
	Result.LocalToWorld = LocalToWorld;
	Result.WorldToLocal = LocalToWorld.Inverse();
	Result.ObjectWorldPositionAndRadius = FVector4(WorldBounds.Origin, WorldBounds.SphereRadius);
	Result.ObjectBounds = WorldBounds.BoxExtent;
	Result.LocalObjectBoundsMin = LocalBounds.GetBoxExtrema(0); // 0 == minimum
	Result.LocalObjectBoundsMax = LocalBounds.GetBoxExtrema(1); // 1 == maximum
	Result.ObjectOrientation = LocalToWorld.GetUnitAxis( EAxis::Z );
	Result.ActorWorldPosition = ActorPosition;
	Result.LpvBiasMultiplier = LpvBiasMultiplier;

	{
		// Extract per axis scales from LocalToWorld transform
		FVector4 WorldX = FVector4(LocalToWorld.M[0][0],LocalToWorld.M[0][1],LocalToWorld.M[0][2],0);
		FVector4 WorldY = FVector4(LocalToWorld.M[1][0],LocalToWorld.M[1][1],LocalToWorld.M[1][2],0);
		FVector4 WorldZ = FVector4(LocalToWorld.M[2][0],LocalToWorld.M[2][1],LocalToWorld.M[2][2],0);
		float ScaleX = FVector(WorldX).Size();
		float ScaleY = FVector(WorldY).Size();
		float ScaleZ = FVector(WorldZ).Size();
		Result.NonUniformScale = FVector4(ScaleX,ScaleY,ScaleZ,0);
		Result.InvNonUniformScale = FVector4(
			ScaleX > KINDA_SMALL_NUMBER ? 1.0f/ScaleX : 0.0f,
			ScaleY > KINDA_SMALL_NUMBER ? 1.0f/ScaleY : 0.0f,
			ScaleZ > KINDA_SMALL_NUMBER ? 1.0f/ScaleZ : 0.0f,
			0.0f
			);
	}

	Result.LocalToWorldDeterminantSign = FMath::FloatSelect(LocalToWorld.RotDeterminant(),1,-1);
	Result.DecalReceiverMask = bReceivesDecals ? 1 : 0;
	return Result;
}

inline TUniformBufferRef<FPrimitiveUniformShaderParameters> CreatePrimitiveUniformBufferImmediate(
	const FMatrix& LocalToWorld,
	const FBoxSphereBounds& WorldBounds,
	const FBoxSphereBounds& LocalBounds,
	bool bReceivesDecals,
	float LpvBiasMultiplier = 1.0f
	)
{
	check(IsInRenderingThread());
	return TUniformBufferRef<FPrimitiveUniformShaderParameters>::CreateUniformBufferImmediate(
		GetPrimitiveUniformShaderParameters(LocalToWorld, WorldBounds.Origin, WorldBounds, LocalBounds, bReceivesDecals, LpvBiasMultiplier ),
		UniformBuffer_MultiUse
		);
}

/**
 * Primitive uniform buffer containing only identity transforms.
 */
class FIdentityPrimitiveUniformBuffer : public TUniformBuffer<FPrimitiveUniformShaderParameters>
{
public:

	/** Default constructor. */
	FIdentityPrimitiveUniformBuffer()
	{
		SetContents(GetPrimitiveUniformShaderParameters(
			FMatrix(
				FPlane(1.0f,0.0f,0.0f,0.0f),
				FPlane(0.0f,1.0f,0.0f,0.0f),
				FPlane(0.0f,0.0f,1.0f,0.0f),
				FPlane(0.0f,0.0f,0.0f,1.0f)
				),
			FVector(0.0f,0.0f,0.0f),
			FBoxSphereBounds(EForceInit::ForceInit),
			FBoxSphereBounds(EForceInit::ForceInit),
			true,
			1.0f		// LPV bias
			));
	}
};

/** Global primitive uniform buffer resource containing identity transformations. */
extern TGlobalResource<FIdentityPrimitiveUniformBuffer> GIdentityPrimitiveUniformBuffer;

/**
 * A batch mesh element definition.
 */
struct FMeshBatchElement
{
	const TUniformBuffer<FPrimitiveUniformShaderParameters>* PrimitiveUniformBufferResource;
	TUniformBufferRef<FPrimitiveUniformShaderParameters> PrimitiveUniformBuffer;

	const FIndexBuffer* IndexBuffer;
	uint32 FirstIndex;
	uint32 NumPrimitives;
	uint32 NumInstances;
	uint32 MinVertexIndex;
	uint32 MaxVertexIndex;
	int32 GPUSkinCacheKey;	// -1 if not using GPU skin cache
	void* UserData;

	/** 
	 *	DynamicIndexData - pointer to user memory containing the index data.
	 *	Used for rendering dynamic data directly.
	 */
	const void* DynamicIndexData;
	uint16 DynamicIndexStride;
	
	FMeshBatchElement()
	:	PrimitiveUniformBufferResource(NULL)
	,	IndexBuffer(NULL)
	,	NumInstances(1)
	,	GPUSkinCacheKey(-1)
	,	UserData(NULL)
	,	DynamicIndexData(NULL)
	{
	}
};

/**
 * A batch of mesh elements, all with the same material and vertex buffer
 */
struct FMeshBatch
{
	TArray<FMeshBatchElement,TInlineAllocator<1> > Elements;

	uint32 UseDynamicData : 1;
	uint32 ReverseCulling : 1;
	uint32 bDisableBackfaceCulling : 1;
	uint32 CastShadow : 1;
	uint32 bWireframe : 1;
	uint32 Type : PT_NumBits;
	uint32 DepthPriorityGroup : SDPG_NumBits;
	uint32 bUseAsOccluder : 1;

	const FLightCacheInterface* LCI;

	/** 
	 *	DynamicVertexData - pointer to user memory containing the vertex data.
	 *	Used for rendering dynamic data directly.
	 */
	const void* DynamicVertexData;
	uint16 DynamicVertexStride;

	const FVertexFactory* VertexFactory;
	const FMaterialRenderProxy* MaterialRenderProxy;

	/** LOD index of the mesh, used for fading LOD transitions. */
	int8 LODIndex;

	FORCEINLINE bool IsTranslucent() const
	{
		// Note: blend mode does not depend on the feature level we are actually rendering in.
		return MaterialRenderProxy && IsTranslucentBlendMode(MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->GetBlendMode());
	}

	FORCEINLINE bool IsMasked() const
	{
		// Note: blend mode does not depend on the feature level we are actually rendering in.
		return MaterialRenderProxy && MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->IsMasked();
	}

	/** Converts from an int32 index into a int8 */
	static int8 QuantizeLODIndex(int32 NewLODIndex)
	{
		checkSlow(NewLODIndex >= SCHAR_MIN && NewLODIndex <= SCHAR_MAX);
		return (int8)NewLODIndex;
	}

	/** 
	* @return vertex stride specified for the mesh. 0 if not dynamic
	*/
	FORCEINLINE uint32 GetDynamicVertexStride() const
	{
		if (UseDynamicData && DynamicVertexData)
		{
			return DynamicVertexStride;
		}
		else
		{
			return 0;
		}
	}

	FORCEINLINE int32 GetNumPrimitives() const
	{
		int32 Count=0;
		for( int32 ElementIdx=0;ElementIdx<Elements.Num();ElementIdx++ )
		{
			Count += Elements[ElementIdx].NumPrimitives;
		}
		return Count;
	}

#if DO_CHECK
	FORCEINLINE void CheckUniformBuffers() const
	{
		for( int32 ElementIdx=0;ElementIdx<Elements.Num();ElementIdx++ )
		{
			check(IsValidRef(Elements[ElementIdx].PrimitiveUniformBuffer) || Elements[ElementIdx].PrimitiveUniformBufferResource != NULL);
		}
	}
#endif	

	/** Default constructor. */
	FMeshBatch()
	:	UseDynamicData(false)
	,	ReverseCulling(false)
	,	bDisableBackfaceCulling(false)
	,	CastShadow(true)
	,	bWireframe(false)
	,	Type(PT_TriangleList)
	,	bUseAsOccluder(true)
	,	LCI(NULL)
	,	DynamicVertexData(NULL)
	,	VertexFactory(NULL)
	,	MaterialRenderProxy(NULL)
	,	LODIndex(INDEX_NONE)
	{
		// By default always add the first element.
		new(Elements) FMeshBatchElement;
	}
};

/**
 * An interface implemented by dynamic resources which need to be initialized and cleaned up by the rendering thread.
 */
class FDynamicPrimitiveResource
{
public:

	virtual void InitPrimitiveResource() = 0;
	virtual void ReleasePrimitiveResource() = 0;
};

/**
 * The base interface used to query a primitive for its dynamic elements.
 */
class FPrimitiveDrawInterface
{
public:

	const FSceneView* const View;

	/** Initialization constructor. */
	FPrimitiveDrawInterface(const FSceneView* InView):
		View(InView)
	{}

	virtual bool IsHitTesting() = 0;
	virtual void SetHitProxy(HHitProxy* HitProxy) = 0;

	virtual void RegisterDynamicResource(FDynamicPrimitiveResource* DynamicResource) = 0;

	virtual void AddReserveLines(uint8 DepthPriorityGroup, int32 NumLines, bool bDepthBiased = false) = 0;

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
		uint8 BlendMode = 1 /*SE_BLEND_Masked*/
		) = 0;

	virtual void DrawLine(
		const FVector& Start,
		const FVector& End,
		const FLinearColor& Color,
		uint8 DepthPriorityGroup,
		float Thickness = 0.0f,
		float DepthBias = 0.0f,
		bool bScreenSpace = false
		) = 0;

	virtual void DrawPoint(
		const FVector& Position,
		const FLinearColor& Color,
		float PointSize,
		uint8 DepthPriorityGroup
		) = 0;

	/**
	 * Determines whether a particular material will be ignored in this context.
	 * @param MaterialRenderProxy - The render proxy of the material to check.
	 * @return true if meshes using the material will be ignored in this context.
	 */
	virtual bool IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy) const
	{
		return false;
	}

	/**
	 * @returns true if this PDI is rendering for the selection outline post process.
	 */
	virtual bool IsRenderingSelectionOutline() const
	{
		return false;
	}

	/**
	 * Draw a mesh element.
	 * This should only be called through the DrawMesh function.
	 *
	 * @return Number of passes rendered for the mesh
	 */
	virtual int32 DrawMesh(const FMeshBatch& Mesh) = 0;
};

/**
 * An interface to a scene interaction.
 */
class ENGINE_API FViewElementDrawer
{
public:

	/**
	 * Draws the interaction using the given draw interface.
	 */
	virtual void Draw(const FSceneView* View,FPrimitiveDrawInterface* PDI) {}
};

/**
 * An interface used to query a primitive for its static elements.
 */
class FStaticPrimitiveDrawInterface
{
public:
	virtual void SetHitProxy(HHitProxy* HitProxy) = 0;
	virtual void DrawMesh(
		const FMeshBatch& Mesh,
		float MinDrawDistance,
		float MaxDrawDistance,
		bool bShadowOnly = false
		) = 0;
};

/**
 * The different types of relevance a primitive scene proxy can declare towards a particular scene view.
 */
struct FPrimitiveViewRelevance
{
	/** The primitive's static elements are rendered for the view. */
	uint32 bStaticRelevance : 1; 
	/** The primitive's dynamic elements are rendered for the view. */
	uint32 bDynamicRelevance : 1;
	/** The primitive is drawn. */
	uint32 bDrawRelevance : 1;
	/** The primitive is casting a shadow. */
	uint32 bShadowRelevance : 1;
	/** The primitive should render to the custom depth pass. */
	uint32 bRenderCustomDepth : 1;
	/** The primitive should render to the main depth pass. */
	uint32 bRenderInMainPass : 1;
	/** The primitive is drawn only in the editor and composited onto the scene after post processing */
	uint32 bEditorPrimitiveRelevance : 1;
	/** The primitive needs PreRenderView to be called before rendering. */
	uint32 bNeedsPreRenderView : 1;
	/** The primitive should have GatherSimpleLights called on the proxy when gathering simple lights. */
	uint32 bHasSimpleLights : 1;

	/** The primitive has one or more opaque or masked elements. */
	uint32 bOpaqueRelevance : 1;
	/** The primitive has one or more masked elements. */
	uint32 bMaskedRelevance : 1;
	/** The primitive has one or more distortion elements. */
	uint32 bDistortionRelevance : 1;
	/** The primitive reads from scene color. */
	uint32 bSceneColorRelevance : 1;
	/** The primitive has one or more elements that have SeparateTranslucency. */
	uint32 bSeparateTranslucencyRelevance : 1;
	/** The primitive has one or more elements that have normal translucency. */
	uint32 bNormalTranslucencyRelevance : 1;

	/** 
	 * Whether this primitive view relevance has been initialized this frame.  
	 * Primitives that have not had ComputeRelevanceForView called on them (because they were culled) will not be initialized,
	 * But we may still need to render them from other views like shadow passes, so this tracks whether we can reuse the cached relevance or not.
	 */
	uint32 bInitializedThisFrame : 1;

	bool HasTranslucency() const 
	{
		return bSeparateTranslucencyRelevance || bNormalTranslucencyRelevance;
	}

	/** Initialization constructor. */
	FPrimitiveViewRelevance():
		bStaticRelevance(false),
		bDynamicRelevance(false),
		bDrawRelevance(false),
		bShadowRelevance(false),
		bRenderCustomDepth(false),
		bRenderInMainPass(true),
		bEditorPrimitiveRelevance(false),
		bNeedsPreRenderView(false),
		bHasSimpleLights(false),
		bOpaqueRelevance(true),
		bMaskedRelevance(false),
		bDistortionRelevance(false),
		bSceneColorRelevance(false),
		bSeparateTranslucencyRelevance(false),
		bNormalTranslucencyRelevance(false),
		bInitializedThisFrame(false)
	{}

	/** Bitwise OR operator.  Sets any relevance bits which are present in either FPrimitiveViewRelevance. */
	FPrimitiveViewRelevance& operator|=(const FPrimitiveViewRelevance& B)
	{
		bStaticRelevance |= B.bStaticRelevance != 0;
		bDynamicRelevance |= B.bDynamicRelevance != 0;
		bDrawRelevance |= B.bDrawRelevance != 0;
		bShadowRelevance |= B.bShadowRelevance != 0;
		bOpaqueRelevance |= B.bOpaqueRelevance != 0;
		bMaskedRelevance |= B.bMaskedRelevance != 0;
		bDistortionRelevance |= B.bDistortionRelevance != 0;
		bRenderCustomDepth |= B.bRenderCustomDepth != 0;
		bRenderInMainPass |= B.bRenderInMainPass !=0;
		bSceneColorRelevance |= B.bSceneColorRelevance != 0;
		bNeedsPreRenderView |= B.bNeedsPreRenderView != 0;
		bHasSimpleLights |= B.bHasSimpleLights != 0;
		bSeparateTranslucencyRelevance |= B.bSeparateTranslucencyRelevance != 0;
		bNormalTranslucencyRelevance |= B.bNormalTranslucencyRelevance != 0;
		bInitializedThisFrame |= B.bInitializedThisFrame;
		return *this;
	}

	/** Binary bitwise OR operator. */
	friend FPrimitiveViewRelevance operator|(const FPrimitiveViewRelevance& A,const FPrimitiveViewRelevance& B)
	{
		FPrimitiveViewRelevance Result(A);
		Result |= B;
		return Result;
	}
};

/**
 *	Helper structure for storing motion blur information for a primitive
 */
struct FMotionBlurInfo
{
	FMotionBlurInfo(FPrimitiveComponentId InComponentId, FPrimitiveSceneInfo* InPrimitiveSceneInfo)
		: ComponentId(InComponentId), MBPrimitiveSceneInfo(InPrimitiveSceneInfo), bKeepAndUpdateThisFrame(true)
	{
	}

	/**  */
	void UpdateMotionBlurInfo();

	/** Call if you want to keep the existing motionblur */
	void RestoreForPausedMotionBlur();

	void SetKeepAndUpdateThisFrame(bool bValue = true)
	{
		if(bValue)
		{
			// we update right away so when it comes to HasVelocity this frame we detect no movement and next frame we actually render it with correct velocity
			UpdateMotionBlurInfo();
		}

		bKeepAndUpdateThisFrame = bValue;
	}

	bool GetKeepAndUpdateThisFrame() const
	{
		return bKeepAndUpdateThisFrame; 
	}

	FMatrix GetPreviousLocalToWorld() const
	{
		return PreviousLocalToWorld;
	}

	FPrimitiveSceneInfo* GetPrimitiveSceneInfo() const
	{
		return MBPrimitiveSceneInfo;
	}

	void SetPrimitiveSceneInfo(FPrimitiveSceneInfo* Value)
	{
		MBPrimitiveSceneInfo = Value;
	}

	void ApplyOffset(FVector InOffset)
	{
		PreviousLocalToWorld.SetOrigin(PreviousLocalToWorld.GetOrigin() + InOffset);
		PausedLocalToWorld.SetOrigin(PausedLocalToWorld.GetOrigin() + InOffset);
	}

private:
	/** The component this info represents. */
	FPrimitiveComponentId ComponentId;
	/** The primitive scene info for the component.	*/
	FPrimitiveSceneInfo* MBPrimitiveSceneInfo;
	/** The previous LocalToWorld of the component.	*/
	FMatrix	PreviousLocalToWorld;
	/** Used in case when Pause is activate. */
	FMatrix	PausedLocalToWorld;
	/** if true then the PreviousLocalToWorld has already been updated for the current frame */
	bool bKeepAndUpdateThisFrame;
};

class FMotionBlurInfoData
{
public:

	// constructor
	FMotionBlurInfoData();
	/** 
	 *	Set the primitives motion blur info
	 * 
	 *	@param PrimitiveSceneInfo	The primitive to add
	 */
	void UpdatePrimitiveMotionBlur(FPrimitiveSceneInfo* PrimitiveSceneInfo);

	/** 
	 *	Set the primitives motion blur info
	 * 
	 *	@param PrimitiveSceneInfo	The primitive to add
	 */
	void RemovePrimitiveMotionBlur(FPrimitiveSceneInfo* PrimitiveSceneInfo);

	/**
	 * Creates any needed motion blur infos if needed and saves the transforms of the frame we just completed
	 */
	void UpdateMotionBlurCache();

	/**
	 * Call if you want to keep the existing motionblur
	 */
	void RestoreForPausedMotionBlur();

	/** 
	 *	Get the primitives motion blur info
	 * 
	 *	@param	PrimitiveSceneInfo	The primitive to retrieve the motion blur info for
	 *
	 *	@return	bool				true if the primitive info was found and set
	 */
	bool GetPrimitiveMotionBlurInfo(const FPrimitiveSceneInfo* PrimitiveSceneInfo, FMatrix& OutPreviousLocalToWorld);

	/** */
	void SetClearMotionBlurInfo();

	/**
	 * Shifts motion blur data by arbitrary delta
	 */
	void ApplyOffset(FVector InOffset);

private:
	/** The motion blur info entries for the frame. Accessed on Renderthread only! */
	TMap<FPrimitiveComponentId, FMotionBlurInfo> MotionBlurInfos;
	/** Unique "frame number" counter to make sure we don't double update */
	uint32 CacheUpdateCount;	
	/** */
	bool bShouldClearMotionBlurInfo;

	/**
	 * O(n) with the amount of motion blurred objects but that number should be low
	 * @return 0 if not found, otherwise pointer into MotionBlurInfos, don't store for longer
	 */
	FMotionBlurInfo* FindMBInfoIndex(FPrimitiveComponentId ComponentId);
};

/**
 * An interface to the private scene manager implementation of a scene.  Use GetRendererModule().AllocateScene to create.
 * The scene
 */
class FSceneInterface
{
public:

	// FSceneInterface interface

	/** 
	 * Adds a new primitive component to the scene
	 * 
	 * @param Primitive - primitive component to add
	 */
	virtual void AddPrimitive(UPrimitiveComponent* Primitive) = 0;
	/** 
	 * Removes a primitive component from the scene
	 * 
	 * @param Primitive - primitive component to remove
	 */
	virtual void RemovePrimitive(UPrimitiveComponent* Primitive) = 0;
	/** Called when a primitive is being unregistered and will not be immediately re-registered. */
	virtual void ReleasePrimitive(UPrimitiveComponent* Primitive) = 0;
	/** 
	 * Updates the transform of a primitive which has already been added to the scene. 
	 * 
	 * @param Primitive - primitive component to update
	 */
	virtual void UpdatePrimitiveTransform(UPrimitiveComponent* Primitive) = 0;
	/** Updates primitive attachment state. */
	virtual void UpdatePrimitiveAttachment(UPrimitiveComponent* Primitive) = 0;
	/** 
	 * Adds a new light component to the scene
	 * 
	 * @param Light - light component to add
	 */
	virtual void AddLight(ULightComponent* Light) = 0;
	/** 
	 * Removes a light component from the scene
	 * 
	 * @param Light - light component to remove
	 */
	virtual void RemoveLight(ULightComponent* Light) = 0;
	/** 
	 * Adds a new light component to the scene which is currently invisible, but needed for editor previewing
	 * 
	 * @param Light - light component to add
	 */
	virtual void AddInvisibleLight(ULightComponent* Light) = 0;
	virtual void SetSkyLight(FSkyLightSceneProxy* Light) = 0;
	/** 
	 * Adds a new decal component to the scene
	 * 
	 * @param Component - component to add
	 */
	virtual void AddDecal(UDecalComponent* Component) = 0;
	/** 
	 * Removes a decal component from the scene
	 * 
	 * @param Component - component to remove
	 */
	virtual void RemoveDecal(UDecalComponent* Component) = 0;
	/** 
	 * Updates the transform of a decal which has already been added to the scene. 
	 *
	 * @param Decal - Decal component to update
	 */
	virtual void UpdateDecalTransform(UDecalComponent* Component) = 0;

	/** Adds a reflection capture to the scene. */
	virtual void AddReflectionCapture(class UReflectionCaptureComponent* Component) {}

	/** Removes a reflection capture from the scene. */
	virtual void RemoveReflectionCapture(class UReflectionCaptureComponent* Component) {}

	/** Reads back reflection capture data from the GPU.  Very slow operation that blocks the GPU and rendering thread many times. */
	virtual void GetReflectionCaptureData(UReflectionCaptureComponent* Component, class FReflectionCaptureFullHDRDerivedData& OutDerivedData) {}

	/** Updates a reflection capture's transform, and then re-captures the scene. */
	virtual void UpdateReflectionCaptureTransform(class UReflectionCaptureComponent* Component) {}

	/** 
	 * Allocates reflection captures in the scene's reflection cubemap array and updates them by recapturing the scene.
	 * Existing captures will only be updated.  Must be called from the game thread.
	 */
	virtual void AllocateReflectionCaptures(const TArray<UReflectionCaptureComponent*>& NewCaptures) {}
	virtual void ReleaseReflectionCubemap(UReflectionCaptureComponent* CaptureComponent) {}

	/** 
	 * Updates the contents of the given sky capture by rendering the scene. 
	 * This must be called on the game thread.
	 */
	virtual void UpdateSkyCaptureContents(USkyLightComponent* CaptureComponent) {}

	/** 
	* Updates the contents of the given scene capture by rendering the scene. 
	* This must be called on the game thread.
	*/
	virtual void UpdateSceneCaptureContents(class USceneCaptureComponent2D* CaptureComponent) {}
	virtual void UpdateSceneCaptureContents(class USceneCaptureComponentCube* CaptureComponent) {}

	virtual void AddPrecomputedLightVolume(const class FPrecomputedLightVolume* Volume) {}
	virtual void RemovePrecomputedLightVolume(const class FPrecomputedLightVolume* Volume) {}

	/** 
	 * Updates the transform of a light which has already been added to the scene. 
	 *
	 * @param Light - light component to update
	 */
	virtual void UpdateLightTransform(ULightComponent* Light) = 0;
	/** 
	 * Updates the color and brightness of a light which has already been added to the scene. 
	 *
	 * @param Light - light component to update
	 */
	virtual void UpdateLightColorAndBrightness(ULightComponent* Light) = 0;

	/** Updates the scene's dynamic skylight. */
	virtual void UpdateDynamicSkyLight(const FLinearColor& UpperColor, const FLinearColor& LowerColor) {}

	/** Sets the precomputed visibility handler for the scene, or NULL to clear the current one. */
	virtual void SetPrecomputedVisibility(const class FPrecomputedVisibilityHandler* PrecomputedVisibilityHandler) {}

	/** Sets the precomputed volume distance field for the scene, or NULL to clear the current one. */
	virtual void SetPrecomputedVolumeDistanceField(const class FPrecomputedVolumeDistanceField* PrecomputedVolumeDistanceField) {}

	/** Sets shader maps on the specified materials without blocking. */
	virtual void SetShaderMapsOnMaterialResources(const TMap<FMaterial*, FMaterialShaderMap*>& MaterialsToUpdate) {}

	/** Updates static draw lists for the given set of materials. */
	virtual void UpdateStaticDrawListsForMaterials(const TArray<const FMaterial*>& Materials) {}

	/** 
	 * Adds a new exponential height fog component to the scene
	 * 
	 * @param FogComponent - fog component to add
	 */	
	virtual void AddExponentialHeightFog(class UExponentialHeightFogComponent* FogComponent) = 0;
	/** 
	 * Removes a exponential height fog component from the scene
	 * 
	 * @param FogComponent - fog component to remove
	 */	
	virtual void RemoveExponentialHeightFog(class UExponentialHeightFogComponent* FogComponent) = 0;

	/** 
	 * Adds a new atmospheric fog component to the scene
	 * 
	 * @param FogComponent - fog component to add
	 */	
	virtual void AddAtmosphericFog(class UAtmosphericFogComponent* FogComponent) = 0;

	/** 
	 * Removes a atmospheric fog component from the scene
	 * 
	 * @param FogComponent - fog component to remove
	 */	
	virtual void RemoveAtmosphericFog(class UAtmosphericFogComponent* FogComponent) = 0;

	/**
	 * Adds a wind source component to the scene.
	 * @param WindComponent - The component to add.
	 */
	virtual void AddWindSource(class UWindDirectionalSourceComponent* WindComponent) = 0;
	/**
	 * Removes a wind source component from the scene.
	 * @param WindComponent - The component to remove.
	 */
	virtual void RemoveWindSource(class UWindDirectionalSourceComponent* WindComponent) = 0;
	/**
	 * Accesses the wind source list.  Must be called in the rendering thread.
	 * @return The list of wind sources in the scene.
	 */
	virtual const TArray<class FWindSourceSceneProxy*>& GetWindSources_RenderThread() const = 0;

	/** Accesses wind parameters.  XYZ will contain wind direction * Strength, W contains wind speed. */
	virtual FVector4 GetWindParameters(const FVector& Position) const = 0;

	/** Same as GetWindParameters, but ignores point wind sources. */
	virtual FVector4 GetDirectionalWindParameters() const = 0;

	/** 
	 * Adds a SpeedTree wind computation object to the scene.
	 * @param StaticMesh - The SpeedTree static mesh whose wind to add.
	 */
	virtual void AddSpeedTreeWind(class UStaticMesh* StaticMesh) = 0;

	/** 
	 * Removes a SpeedTree wind computation object to the scene.
	 * @param StaticMesh - The SpeedTree static mesh whose wind to remove.
	 */
	virtual void RemoveSpeedTreeWind(class UStaticMesh* StaticMesh) = 0;

	/** Ticks the SpeedTree wind object and updates the uniform buffer. */
	virtual void UpdateSpeedTreeWind(double CurrentTime) = 0;

	/** 
	 * Looks up the SpeedTree uniform buffer for the passed in vertex factory.
	 * @param VertexFactory - The vertex factory registered for SpeedTree.
	 */
	virtual FUniformBufferRHIParamRef GetSpeedTreeUniformBuffer(const FVertexFactory* VertexFactory) = 0;

	/**
	 * Release this scene and remove it from the rendering thread
	 */
	virtual void Release() = 0;
	/**
	 * Retrieves the lights interacting with the passed in primitive and adds them to the out array.
	 *
	 * @param	Primitive				Primitive to retrieve interacting lights for
	 * @param	RelevantLights	[out]	Array of lights interacting with primitive
	 */
	virtual void GetRelevantLights( UPrimitiveComponent* Primitive, TArray<const ULightComponent*>* RelevantLights ) const = 0;
	/**
	 * Indicates if hit proxies should be processed by this scene
	 *
	 * @return true if hit proxies should be rendered in this scene.
	 */
	virtual bool RequiresHitProxies() const = 0;
	/**
	 * Get the optional UWorld that is associated with this scene
	 * 
 	 * @return UWorld instance used by this scene
	 */
	virtual class UWorld* GetWorld() const = 0;
	/**
	 * Return the scene to be used for rendering. Note that this can return NULL if rendering has
	 * been disabled!
	 */
	virtual class FScene* GetRenderScene()
	{
		return NULL;
	}
	/**
	 * Sets the FX system associated with the scene.
	 */
	virtual void SetFXSystem( class FFXSystemInterface* InFXSystem ) = 0;

	/**
	 * Get the FX system associated with the scene.
	 */
	virtual class FFXSystemInterface* GetFXSystem() = 0;

	virtual void DumpUnbuiltLightIteractions( FOutputDevice& Ar ) const { }

	/**
	 * Dumps static mesh draw list stats to the log.
	 */
	virtual void DumpStaticMeshDrawListStats() const {}

	/**
	 * Request to clear the MB info. Game thread only
	 */
	virtual void SetClearMotionBlurInfoGameThread() {}

	/** Updates the scene's list of parameter collection id's and their uniform buffers. */
	virtual void UpdateParameterCollections(const TArray<class FMaterialParameterCollectionInstanceResource*>& InParameterCollections) {}

	/**
	 * Exports the scene.
	 *
	 * @param	Ar		The Archive used for exporting.
	 **/
	virtual void Export( FArchive& Ar ) const
	{}

	
	/**
	 * Shifts scene data by provided delta
	 * Called on world origin changes
	 * 
	 * @param	InOffset	Delta to shift scene by
	 */
	virtual void ApplyWorldOffset(FVector InOffset) {}

	/**
	 * Notification that level was added to a world
	 * 
	 * @param	InLevelName		Level name
	 */
	virtual void OnLevelAddedToWorld(FName InLevelName) {}

	/**
	 * @return True if there are any lights in the scene
	 */
	virtual bool HasAnyLights() const = 0;

	virtual bool IsEditorScene() const { return false; }

protected:
	virtual ~FSceneInterface() {}
};

/** 
 * Enumeration for currently used translucent lighting volume cascades 
 */
enum ETranslucencyVolumeCascade
{
	TVC_Inner,
	TVC_Outer,

	TVC_MAX,
};

/** The uniform shader parameters associated with a view. */
BEGIN_UNIFORM_BUFFER_STRUCT(FViewUniformShaderParameters,ENGINE_API)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,TranslatedWorldToClip)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,WorldToClip)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,TranslatedWorldToView)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,ViewToTranslatedWorld)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,ViewToClip)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,ClipToTranslatedWorld)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,ScreenToWorld)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,ScreenToTranslatedWorld)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector,ViewForward, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector,ViewUp, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector,ViewRight, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4,InvDeviceZToWorldZTransform)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector4,ScreenPositionScaleBias, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector4,ScreenTexelBias, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4,ViewSizeAndSceneTexelSize)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4,ViewOrigin)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4,TranslatedViewOrigin)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector4,DiffuseOverrideParameter, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector4,SpecularOverrideParameter, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector4,NormalOverrideParameter, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector2D,RoughnessOverrideParameter, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector,PreViewTranslation)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,OutOfBoundsMask, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector,ViewOriginDelta)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,CullingSign)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,NearPlane, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,AdaptiveTessellationFactor)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,GameTime)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,RealTime)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32,Random)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32,FrameNumber)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,UnlitViewmodeMask, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,ReflectionLightmapMixingMask, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FLinearColor,DirectionalLightColor, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector,DirectionalLightDirection, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FLinearColor,UpperSkyColor, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FLinearColor,LowerSkyColor, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FVector4,TranslucencyLightingVolumeMin,[TVC_MAX])
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FVector4,TranslucencyLightingVolumeInvSize,[TVC_MAX])
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4,TemporalAAParams)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,ExposureScale, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,DepthOfFieldFocalDistance)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,DepthOfFieldScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,DepthOfFieldFocalLength)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,DepthOfFieldFocalRegion)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,DepthOfFieldNearTransitionRegion)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,DepthOfFieldFarTransitionRegion)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,MotionBlurNormalizedToPixel)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,GeneralPurposeTweak)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,DemosaicVposOffset, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,PrevProjection)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,PrevViewProj)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,PrevViewRotationProj)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,PrevTranslatedWorldToClip)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector,PrevViewOrigin)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector,PrevPreViewTranslation)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,PrevInvViewProj)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix,PrevScreenToTranslatedWorld)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector,IndirectLightingColorScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,HdrMosaic, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector,AtmosphericFogSunDirection)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,AtmosphericFogSunPower, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,AtmosphericFogPower, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,AtmosphericFogDensityScale, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,AtmosphericFogDensityOffset, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,AtmosphericFogGroundOffset, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,AtmosphericFogDistanceScale, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,AtmosphericFogAltitudeScale, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,AtmosphericFogHeightScaleRayleigh, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,AtmosphericFogStartDistance, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(float,AtmosphericFogDistanceOffset, EShaderPrecisionModifier::Half)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32,AtmosphericFogRenderMask)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32,AtmosphericFogInscatterAltitudeSampleNum)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FLinearColor,AtmosphericFogSunColor)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FLinearColor,AmbientCubemapTint)//Used via a custom material node. DO NOT REMOVE.
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,AmbientCubemapIntensity)//Used via a custom material node. DO NOT REMOVE.
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector2D,RenderTargetSize)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float,SkyLightParameters)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4,SceneTextureMinMax)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FLinearColor,SkyLightColor)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FVector4,SkyIrradianceEnvironmentMap,[7])
END_UNIFORM_BUFFER_STRUCT(FViewUniformShaderParameters)

// used to blend IBlendableInterface object data, the member act as a header for a following data block
struct FBlendableEntry
{
	// weight 0..1, 0 to disable this entry
	float Weight;

private:
	// to ensure type safety
	FName BlendableType;
	// to be able to jump over data
	uint32 DataSize;

	// @return pointer to the next object or end (can be compared with container end pointer)
	uint8* GetDataPtr() { check(this); return (uint8*)(this + 1); }
	// @return next or end of the array
	FBlendableEntry* GetNext() { return (FBlendableEntry*)(GetDataPtr() + DataSize); }

	friend class FBlendableManager;
};

// Manager class which allows blending of FBlendableBase objects, stores a copy (for the render thread)
class FBlendableManager
{
public:

	FBlendableManager()
	{
		// can be increased if needed (to avoid reallocations and copies at the cost of more memory allocation)
		Scratch.Reserve(10 * 1024);
	}
	
	// @param InWeight 0..1, excluding 0 as this is used to disable entries
	// @param InData is copied with a memcpy
	template <class T>
	void PushBlendableData(float InWeight, const T& InData)
	{
		FName BlendableType = T::GetFName();
		
		PushBlendableDataPtr(InWeight, BlendableType, (const uint8*)&InData, sizeof(T));
	}

	// find next that has the given type, O(n), n is number of elements after the given one or all if started with 0
	// @param InIterator 0 to start iteration
	// @return 0 if no further one of that type was found
	template <class T>
	T* IterateBlendables(FBlendableEntry*& InIterator) const
	{
		FName BlendableType = T::GetFName();

		do
		{
			InIterator = GetNextBlendableEntryPtr(InIterator);

		} while(InIterator && InIterator->Weight <= 0.0f && InIterator->BlendableType != BlendableType);

		if(InIterator)
		{
			return (T*)InIterator->GetDataPtr();
		}

		return 0;
	}

private:

	// stored multiple elements with a FBlendableEntry header and following data of a size specified in the header
	TArray<uint8> Scratch;

	// find next, no restriction on the type O(n), n is number of elements after the given one or all if started with 0
	FBlendableEntry* GetNextBlendableEntryPtr(FBlendableEntry* InIterator = 0) const
	{
		if(!InIterator)
		{
			// start at the beginning
			InIterator = (FBlendableEntry*)Scratch.GetData();
		}
		else
		{
			// go to next
			InIterator = InIterator->GetNext();
		}

		// end reached?
		if((uint8*)InIterator == Scratch.GetData() + Scratch.Num())
		{
			// no more entries
			return 0;
		}

		return InIterator;
	}

	// @param InWeight 0..1, excluding 0 as this is used to disable entries
	// @param InData is copied
	// @param InDataSize >0
	void PushBlendableDataPtr(float InWeight, FName InBlendableType, const uint8* InData, uint32 InDataSize)
	{
		check(InWeight > 0.0f && InWeight <= 1.0f);
		check(InData);
		check(InDataSize);

		uint32 OldSize = Scratch.AddUninitialized(sizeof(FBlendableEntry) + InDataSize);

		FBlendableEntry* Dst = (FBlendableEntry*)&Scratch[OldSize];

		Dst->Weight = InWeight;
		Dst->BlendableType = InBlendableType;
		Dst->DataSize = InDataSize;
		memcpy(Dst->GetDataPtr(), InData, InDataSize);
	}
};


/** All blended postprocessing in one place, non lerpable data is stored in non merged form */
class FFinalPostProcessSettings : public FPostProcessSettings
{
public:
	FFinalPostProcessSettings()
	: HighResScreenshotMaterial(NULL)
	, HighResScreenshotMaskMaterial(NULL)
	, HighResScreenshotCaptureRegionMaterial(NULL)
	, bBufferVisualizationDumpRequired(false)
	{
		// to avoid reallocations we reserve some elements

		ContributingCubemaps.Empty(8);
		ContributingLUTs.Empty(8);

		SetLUT(0);
	}
	
	struct FCubemapEntry
	{
		// 0..
		FLinearColor AmbientCubemapTintMulScaleValue;
		// can be 0
		class UTexture* AmbientCubemap;

		FCubemapEntry() :
			AmbientCubemapTintMulScaleValue(FLinearColor(0, 0, 0, 0)),
			AmbientCubemap(NULL)
		{}
	};

	struct FLUTBlenderEntry
	{
		// 0..1
		float Weight;
		// can be 0
		class UTexture* LUTTexture;
	};

	// Updated existing one or creates a new one
	// this allows to combined volumes for fading between them but also to darken/disable volumes in some areas
	void UpdateEntry(const FCubemapEntry &Entry, float Weight)
	{
		bool Existing = false;
		for(int32 i = 0; i < ContributingCubemaps.Num(); ++i)
		{
			FCubemapEntry& Local = ContributingCubemaps[i];
			
			if(Local.AmbientCubemap == Entry.AmbientCubemap)
			{
				Local.AmbientCubemapTintMulScaleValue = FMath::Lerp(Local.AmbientCubemapTintMulScaleValue, Entry.AmbientCubemapTintMulScaleValue, Weight);
				Existing = true;
			}
			else
			{
				Local.AmbientCubemapTintMulScaleValue *= 1.0f - Weight;
			}

			if(Local.AmbientCubemapTintMulScaleValue.IsAlmostBlack())
			{
				ContributingCubemaps.RemoveAt(i);
			}
		}

		if( !Existing )
		{
			// We didn't find the entry previously, so we need to Lerp up from zero.
			FCubemapEntry WeightedEntry = Entry;
			WeightedEntry.AmbientCubemapTintMulScaleValue *= Weight;
			if(!WeightedEntry.AmbientCubemapTintMulScaleValue.IsAlmostBlack()) // Only bother to add it if the lerped value isn't near-black.
			{
				ContributingCubemaps.Push(WeightedEntry);
			}
		}
	}

	// @param InTexture - must not be 0
	// @param Weight - 0..1
	void LerpTo(UTexture* InTexture, float Weight)
	{
		check(InTexture);
		check(Weight >= 0.0f && Weight <= 1.0f);

		if(Weight > 254.0f/255.0f || !ContributingLUTs.Num())
		{
			SetLUT(InTexture);
			return;
		}

		for(uint32 i = 0; i < (uint32)ContributingLUTs.Num(); ++i)
		{
			ContributingLUTs[i].Weight *= 1.0f - Weight;
		}

		uint32 ExistingIndex = FindIndex(InTexture);
		if(ExistingIndex == 0xffffffff)
		{
			PushLUT(InTexture, Weight);
		}
		else
		{
			ContributingLUTs[ExistingIndex].Weight += Weight;
		}
	}
	
	/**
	 * add a LUT(look up table) to the ones that are blended together
	 * @param Texture - can be 0
	 * @param Weight - 0..1
	 */
	void PushLUT(UTexture* Texture, float Weight)
	{
		check(Weight >= 0.0f && Weight <= 1.0f);

		FLUTBlenderEntry Entry;

		Entry.LUTTexture = Texture;
		Entry.Weight = Weight;

		ContributingLUTs.Add(Entry);
	}

	/** @return 0xffffffff if not found */
	uint32 FindIndex(UTexture* Tex) const
	{
		for(uint32 i = 0; i < (uint32)ContributingLUTs.Num(); ++i)
		{
			if(ContributingLUTs[i].LUTTexture == Tex)
			{
				return i;
			}
		}

		return 0xffffffff;
	}

	void SetLUT(UTexture* Texture)
	{
		// intentionally no deallocations
		ContributingLUTs.Empty();

		PushLUT(Texture, 1.0f);
	}

	// was not merged during blending unlike e.g. BloomThreshold 
	TArray<FCubemapEntry> ContributingCubemaps;
	TArray<FLUTBlenderEntry> ContributingLUTs;

	// List of materials to use in the buffer visualization overview
	TArray<UMaterialInterface*> BufferVisualizationOverviewMaterials;

	// Material to use for rendering high res screenshot with mask. Post process expects this material to be set all the time.
	UMaterialInterface* HighResScreenshotMaterial;

	// Material to use for rendering just the high res screenshot mask. Post process expects this material to be set all the time.
	UMaterialInterface* HighResScreenshotMaskMaterial;

	// Material to use for rendering the high resolution screenshot capture region. Post processing only draws the region if this material is set.
	UMaterialInstanceDynamic* HighResScreenshotCaptureRegionMaterial;

	// Current buffer visualization dumping settings
	bool bBufferVisualizationDumpRequired;
	FString BufferVisualizationDumpBaseFilename;

	// Maintains a container with IBlendableInterface objects and their data
	FBlendableManager BlendableManager;
};

// Projection data for a FSceneView
struct FSceneViewProjectionData
{
	FMatrix ViewMatrix;

	/** UE4 projection matrix projects such that clip space Z=1 is the near plane, and Z=0 is the infinite far plane. */
	FMatrix ProjectionMatrix;

protected:
	//The unconstrained (no aspect ratio bars applied) view rectangle (also unscaled)
	FIntRect ViewRect;

	// The constrained view rectangle (identical to UnconstrainedUnscaledViewRect if aspect ratio is not constrained)
	FIntRect ConstrainedViewRect;

public:
	void SetViewRectangle(const FIntRect& InViewRect)
	{
		ViewRect = InViewRect;
		ConstrainedViewRect = InViewRect;
	}

	void SetConstrainedViewRectangle(const FIntRect& InViewRect)
	{
		ConstrainedViewRect = InViewRect;
	}

	bool IsValidViewRectangle() const
	{
		return (ConstrainedViewRect.Min.X >= 0) &&
			(ConstrainedViewRect.Min.Y >= 0) &&
			(ConstrainedViewRect.Width() > 0) &&
			(ConstrainedViewRect.Height() > 0);
	}

	const FIntRect& GetViewRect() const { return ViewRect; }
	const FIntRect& GetConstrainedViewRect() const { return ConstrainedViewRect; }
};

// Construction parameters for a FSceneView
struct FSceneViewInitOptions : public FSceneViewProjectionData
{
	const FSceneViewFamily* ViewFamily;
	FSceneViewStateInterface* SceneViewStateInterface;
	const AActor* ViewActor;
	FViewElementDrawer* ViewElementDrawer;

	FLinearColor BackgroundColor;
	FLinearColor OverlayColor;
	FLinearColor ColorScale;

	/** For stereoscopic rendering, whether or not this is a full pass, or a left / right eye pass */
	EStereoscopicPass StereoPass;

	/** Conversion from world units (uu) to meters, so we can scale motion to the world appropriately */
	float WorldToMetersScale;

	TSet<FPrimitiveComponentId> HiddenPrimitives;


	float LODDistanceFactor;

	/** If > 0, overrides the view's far clipping plane with a plane at the specified distance. */
	float OverrideFarClippingPlaneDistance;

	// Was there a camera cut this frame?
	bool bInCameraCut;

#if WITH_EDITOR
	// default to 0'th view index, which is a bitfield of 1
	uint64 EditorViewBitflag;

	// this can be specified for ortho views so that it's min draw distance/LOD parenting etc, is controlled by a perspective viewport
	FVector OverrideLODViewOrigin;

	// In case of ortho, generate a fake view position that has a non-zero W component. The view position will be derived based on the view matrix.
	bool bUseFauxOrthoViewPos;
#endif

	FSceneViewInitOptions()
		: ViewFamily(NULL)
		, SceneViewStateInterface(NULL)
		, ViewActor(NULL)
		, ViewElementDrawer(NULL)
		, BackgroundColor(FLinearColor::Transparent)
		, OverlayColor(FLinearColor::Transparent)
		, ColorScale(FLinearColor::White)
		, StereoPass(eSSP_FULL)
		, WorldToMetersScale(100.f)
		, LODDistanceFactor(1.0f)
		, OverrideFarClippingPlaneDistance(-1.0f)
		, bInCameraCut(false)
#if WITH_EDITOR
		, EditorViewBitflag(1)
		, OverrideLODViewOrigin(ForceInitToZero)
		, bUseFauxOrthoViewPos(false)
		//@TODO: , const TBitArray<>& InSpriteCategoryVisibility=TBitArray<>()
#endif
	{
	}
};


/**
 * A projection from scene space into a 2D screen region.
 */
class ENGINE_API FSceneView
{
public:
	const FSceneViewFamily* Family;
	/** can be 0 (thumbnail rendering) */
	FSceneViewStateInterface* State;

	/** The uniform buffer for the view's parameters.  This is only initialized in the rendering thread's copies of the FSceneView. */
	TUniformBufferRef<FViewUniformShaderParameters> UniformBuffer;

	/** The actor which is being viewed from. */
	const AActor* ViewActor;

	/** An interaction which draws the view's interaction elements. */
	FViewElementDrawer* Drawer;

	/* Final position of the view in the final render target (in pixels), potentially scaled down by ScreenPercentage */
	FIntRect ViewRect;

	/* Final position of the view in the final render target (in pixels), potentially constrained by an aspect ratio requirement (black bars) */
	const FIntRect UnscaledViewRect;

	/* Raw view size (in pixels), used for screen space calculations */
	const FIntRect UnconstrainedViewRect;

	/** 
	 * Copy from GFrameNumber
	 */
	uint32 FrameNumber;

	/** Maximum number of shadow cascades to render with. */
	int32 MaxShadowCascades;

    FViewMatrices ViewMatrices;

    /** Variables used to determine the view matrix */
    FVector		ViewLocation;
    FRotator	ViewRotation;
	FQuat		BaseHmdOrientation;
	FVector		BaseHmdLocation;
	float		WorldToMetersScale;

	// normally the same as ViewMatrices unless "r.Shadow.FreezeCamera" is activated
	FViewMatrices ShadowViewMatrices;

	FMatrix ProjectionMatrixUnadjustedForRHI;

	FLinearColor BackgroundColor;
	FLinearColor OverlayColor;

	/** Color scale multiplier used during post processing */
	FLinearColor ColorScale;

	/** For stereoscopic rendering, whether or not this is a full pass, or a left / right eye pass */
	EStereoscopicPass StereoPass;

	/** Current buffer visualization mode */
	FName CurrentBufferVisualizationMode;

	/**
	* These can be used to override material parameters across the scene without recompiling shaders.
	* The last component is how much to include of the material's value for that parameter, so 0 will completely remove the material's value.
	*/
	FVector4 DiffuseOverrideParameter;
	FVector4 SpecularOverrideParameter;
	FVector4 NormalOverrideParameter;
	FVector2D RoughnessOverrideParameter;

	/** The primitives which are hidden for this view. */
	TSet<FPrimitiveComponentId> HiddenPrimitives;

	// Derived members.

	/** redundant, ViewMatrices.GetViewProjMatrix() */
	/* UE4 projection matrix projects such that clip space Z=1 is the near plane, and Z=0 is the infinite far plane. */
	FMatrix ViewProjectionMatrix;
	/** redundant, ViewMatrices.GetInvViewMatrix() */
	FMatrix InvViewMatrix;				
	/** redundant, ViewMatrices.GetInvViewProjMatrix() */
	FMatrix InvViewProjectionMatrix;	

	float TemporalJitterPixelsX;
	float TemporalJitterPixelsY;

	FConvexVolume ViewFrustum;

	bool bHasNearClippingPlane;

	FPlane NearClippingPlane;

	float NearClippingDistance;

	/** true if ViewMatrix.Determinant() is negative. */
	bool bReverseCulling;

	/* Vector used by shaders to convert depth buffer samples into z coordinates in world space */
	FVector4 InvDeviceZToWorldZTransform;

	/** FOV based multiplier for cull distance on objects */
	float LODDistanceFactor;
	/** Square of the FOV based multiplier for cull distance on objects */
	float LODDistanceFactorSquared;

	/** Whether we did a camera cut for this view this frame. */
	bool bCameraCut;

	/** True if this scene was created from a game world. */
	bool bIsGameView;

	/** Whether this view should override the view family's EngineShowFlags.Materials setting and force it on. */
	bool bForceShowMaterials;

	/** For sanity checking casts that are assumed to be safe. */
	bool bIsViewInfo;

	/** Whether this view is being used to render a scene capture. */
	bool bIsSceneCapture;

	/** Whether this view is being used to render a reflection capture. */
	bool bIsReflectionCapture;

	/** 0 if valid (we are rendering a screen postprocess pass )*/
	struct FRenderingCompositePassContext* RenderingCompositePassContext; 

#if WITH_EDITOR
	/** The set of (the first 64) groups' visibility info for this view */
	uint64 EditorViewBitflag;

	/** For ortho views, this can control how to determine LOD parenting (ortho has no "distance-to-camera") */
	FVector OverrideLODViewOrigin;

	/** True if we should draw translucent objects when rendering hit proxies */
	bool bAllowTranslucentPrimitivesInHitProxy;

	/** BitArray representing the visibility state of the various sprite categories in the editor for this view */
	TBitArray<> SpriteCategoryVisibility;
	/** Selection color for the editor (used by post processing) */
	FLinearColor SelectionOutlineColor;
#endif

	/**
	 * The final settings for the current viewer position (blended together from many volumes).
	 * Setup by the main thread, passed to the render thread and never touched again by the main thread.
	 */
	FFinalPostProcessSettings FinalPostProcessSettings;

	/** Parameters for atmospheric fog. */
	FTextureRHIRef AtmosphereTransmittanceTexture;
	FTextureRHIRef AtmosphereIrradianceTexture;
	FTextureRHIRef AtmosphereInscatterTexture;

	/** Areas of the screen where the depth of field should be forced to the max. */
	TArray< FIntRect > UIBlurOverrideRectangles;

	/** Initialization constructor. */
	FSceneView(const FSceneViewInitOptions& InitOptions);

	/** used by ScreenPercentage */
	void SetScaledViewRect(FIntRect InScaledViewRect);

	/** Transforms a point from world-space to the view's screen-space. */
	FVector4 WorldToScreen(const FVector& WorldPoint) const;

	/** Transforms a point from the view's screen-space to world-space. */
	FVector ScreenToWorld(const FVector4& ScreenPoint) const;

	/** Transforms a point from the view's screen-space into pixel coordinates relative to the view's X,Y. */
	bool ScreenToPixel(const FVector4& ScreenPoint,FVector2D& OutPixelLocation) const;

	/** Transforms a point from pixel coordinates relative to the view's X,Y (left, top) into the view's screen-space. */
	FVector4 PixelToScreen(float X,float Y,float Z) const;

	/** Transforms a point from the view's world-space into pixel coordinates relative to the view's X,Y (left, top). */
	bool WorldToPixel(const FVector& WorldPoint,FVector2D& OutPixelLocation) const;

	/** Transforms a point from pixel coordinates relative to the view's X,Y (left, top) into the view's world-space. */
	FVector4 PixelToWorld(float X,float Y,float Z) const;

	/** 
	 * Transforms a point from the view's world-space into the view's screen-space. 
	 * Divides the resulting X, Y, Z by W before returning. 
	 */
	FPlane Project(const FVector& WorldPoint) const;

	/** 
	 * Transforms a point from the view's screen-space into world coordinates
	 * multiplies X, Y, Z by W before transforming. 
	 */
	FVector Deproject(const FPlane& ScreenPoint) const;

	/** transforms 2D screen coordinates into a 3D world-space origin and direction 
	 * @param ScreenPos - screen coordinates in pixels
	 * @param out_WorldOrigin (out) - world-space origin vector
	 * @param out_WorldDirection (out) - world-space direction vector
	 */
	void DeprojectFVector2D(const FVector2D& ScreenPos, FVector& out_WorldOrigin, FVector& out_WorldDirection) const;

	/** transforms 2D screen coordinates into a 3D world-space origin and direction 
	 * @param ScreenPos - screen coordinates in pixels
	 * @param ViewRect - view rectangle
	 * @param InvViewMatrix - inverse view matrix
	 * @param InvProjMatrix - inverse projection matrix
	 * @param out_WorldOrigin (out) - world-space origin vector
	 * @param out_WorldDirection (out) - world-space direction vector
	 */
	static void DeprojectScreenToWorld(const FVector2D& ScreenPos, const FIntRect& ViewRect, const FMatrix& InvViewMatrix, const FMatrix& InvProjMatrix, FVector& out_WorldOrigin, FVector& out_WorldDirection);

	inline FVector GetViewRight() const { return ViewMatrices.ViewMatrix.GetColumn(0); }
	inline FVector GetViewUp() const { return ViewMatrices.ViewMatrix.GetColumn(1); }
	inline FVector GetViewDirection() const { return ViewMatrices.ViewMatrix.GetColumn(2); }

	/** @return true:perspective, false:orthographic */
	inline bool IsPerspectiveProjection() const { return ViewMatrices.IsPerspectiveProjection(); }

	/** Allow things like HMD displays to update the view matrix at the last minute, to minimize perceived latency */
	void UpdateViewMatrix();

	/** Setup defaults and depending on view position (postprocess volumes) */
	void StartFinalPostprocessSettings(FVector ViewLocation);

	/**
	 * custom layers can be combined with the existing settings
	 * @param Weight usually 0..1 but outside range is clamped
	 */
	void OverridePostProcessSettings(const FPostProcessSettings& Src, float Weight);

	/** applied global restrictions from show flags */
	void EndFinalPostprocessSettings();

	/** Configure post process settings for the buffer visualization system */
	void ConfigureBufferVisualizationSettings();
};


/**
 * A set of views into a scene which only have different view transforms and owner actors.
 */
class FSceneViewFamily
{
public:
	/**
	* Helper struct for creating FSceneViewFamily instances
	* If created with specifying a time it will retrieve them from the world in the given scene.
	* 
	* @param InRenderTarget		The render target which the views are being rendered to.
	* @param InScene			The scene being viewed.
	* @param InShowFlags		The show flags for the views.
	*
	*/
	struct ConstructionValues
	{
		ConstructionValues(
			const FRenderTarget* InRenderTarget,
			FSceneInterface* InScene,
			const FEngineShowFlags& InEngineShowFlags
			)
		:	RenderTarget(InRenderTarget)
		,	Scene(InScene)
		,	EngineShowFlags(InEngineShowFlags)
		,	CurrentWorldTime(0.0f)
		,	DeltaWorldTime(0.0f)
		,	CurrentRealTime(0.0f)
		,	GammaCorrection(1.0f)
		,	bRealtimeUpdate(false)
		,	bDeferClear(false)
		,	bResolveScene(true)			
		,	bTimesSet(false)
		{
			if( InScene != NULL )			
			{
				UWorld* World = InScene->GetWorld();
				// Ensure the world is valid and that we are being called from a game thread (GetRealTimeSeconds requires this)
				if( World && IsInGameThread() )
				{					
					CurrentWorldTime = World->GetTimeSeconds();
					DeltaWorldTime = World->GetDeltaSeconds();
					CurrentRealTime = World->GetRealTimeSeconds();
					bTimesSet = true;
				}
			}
		}
		/** The views which make up the family. */
		const FRenderTarget* RenderTarget;

		/** The render target which the views are being rendered to. */
		FSceneInterface* Scene;

		/** The engine show flags for the views. */
		FEngineShowFlags EngineShowFlags;

		/** The current world time. */
		float CurrentWorldTime;

		/** The difference between the last world time and CurrentWorldTime. */
		float DeltaWorldTime;
		
		/** The current real time. */
		float CurrentRealTime;

		/** Gamma correction used when rendering this family. Default is 1.0 */
		float GammaCorrection;
		
		/** Indicates whether the view family is updated in real-time. */
		uint32 bRealtimeUpdate:1;
		
		/** Used to defer the back buffer clearing to just before the back buffer is drawn to */
		uint32 bDeferClear:1;
		
		/** If true then results of scene rendering are copied/resolved to the RenderTarget. */
		uint32 bResolveScene:1;		
		
		/** Safety check to ensure valid times are set either from a valid world/scene pointer or via the SetWorldTimes function */
		uint32 bTimesSet:1;

		/** Set the world time ,difference between the last world time and CurrentWorldTime and current real time. */
		ConstructionValues& SetWorldTimes(const float InCurrentWorldTime,const float InDeltaWorldTime,const float InCurrentRealTime) { CurrentWorldTime = InCurrentWorldTime; DeltaWorldTime = InDeltaWorldTime; CurrentRealTime = InCurrentRealTime;bTimesSet = true;return *this; }
		
		/** Set  whether the view family is updated in real-time. */
		ConstructionValues& SetRealtimeUpdate(const bool Value) { bRealtimeUpdate = Value; return *this; }
		
		/** Set whether to defer the back buffer clearing to just before the back buffer is drawn to */
		ConstructionValues& SetDeferClear(const bool Value) { bDeferClear = Value; return *this; }
		
		/** Setting to if true then results of scene rendering are copied/resolved to the RenderTarget. */
		ConstructionValues& SetResolveScene(const bool Value) { bResolveScene = Value; return *this; }
		
		/** Set Gamma correction used when rendering this family. */
		ConstructionValues& SetGammaCorrection(const float Value) { GammaCorrection = Value; return *this; }		
	};
	
	/** The views which make up the family. */
	TArray<const FSceneView*> Views;

	/** The width in screen pixels of the view family being rendered. */
	uint32 FamilySizeX;

	/** The height in screen pixels of the view family being rendered. */
	uint32 FamilySizeY;

	/** The render target which the views are being rendered to. */
	const FRenderTarget* RenderTarget;

	/** The scene being viewed. */
	FSceneInterface* Scene;

	/** The new show flags for the views (meant to replace the old system). */
	FEngineShowFlags EngineShowFlags;

	/** The current world time. */
	float CurrentWorldTime;

	/** The difference between the last world time and CurrentWorldTime. */
	float DeltaWorldTime;

	/** The current real time. */
	float CurrentRealTime;

	/** Indicates whether the view family is updated in realtime. */
	bool bRealtimeUpdate;

	/** Used to defer the back buffer clearing to just before the back buffer is drawn to */
	bool bDeferClear;

	/** if true then results of scene rendering are copied/resolved to the RenderTarget. */
	bool bResolveScene;

	/** Gamma correction used when rendering this family. Default is 1.0 */
	float GammaCorrection;
	
	/** Editor setting to allow designers to override the automatic expose. 0:Automatic, following indices: -4 .. +4 */
	FExposureSettings ExposureSettings;

    /** Extensions that can modify view parameters on the render thread. */
    TArray<class ISceneViewExtension*> ViewExtensions;

#if WITH_EDITOR
	/** Indicates whether, of not, the base attachment volume should be drawn. */
	bool bDrawBaseInfo;
#endif

	/** Initialization constructor. */
	ENGINE_API FSceneViewFamily( const ConstructionValues& CVS );

	/** Computes FamilySizeX and FamilySizeY from the Views array. */
	ENGINE_API void ComputeFamilySize();
};

/**
 * A view family which deletes its views when it goes out of scope.
 */
class FSceneViewFamilyContext : public FSceneViewFamily
{
public:
	/** Initialization constructor. */
	FSceneViewFamilyContext( const ConstructionValues& CVS)
		:	FSceneViewFamily(CVS)
	{}

	/** Destructor. */
	ENGINE_API ~FSceneViewFamilyContext();
};

//
// Primitive drawing utility functions.
//

// Solid shape drawing utility functions. Not really designed for speed - more for debugging.
// These utilities functions are implemented in UnScene.cpp using GetTRI.

// 10x10 tessellated plane at x=-1..1 y=-1...1 z=0
extern ENGINE_API void DrawPlane10x10(class FPrimitiveDrawInterface* PDI,const FMatrix& ObjectToWorld,float Radii,FVector2D UVMin, FVector2D UVMax,const FMaterialRenderProxy* MaterialRenderProxy,uint8 DepthPriority);
extern ENGINE_API void DrawBox(class FPrimitiveDrawInterface* PDI,const FMatrix& BoxToWorld,const FVector& Radii,const FMaterialRenderProxy* MaterialRenderProxy,uint8 DepthPriority);
extern ENGINE_API void DrawSphere(class FPrimitiveDrawInterface* PDI,const FVector& Center,const FVector& Radii,int32 NumSides,int32 NumRings,const FMaterialRenderProxy* MaterialRenderProxy,uint8 DepthPriority,bool bDisableBackfaceCulling=false);
extern ENGINE_API void DrawCone(class FPrimitiveDrawInterface* PDI,const FMatrix& ConeToWorld, float Angle1, float Angle2, int32 NumSides, bool bDrawSideLines, const FLinearColor& SideLineColor, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriority);


extern ENGINE_API void DrawCylinder(class FPrimitiveDrawInterface* PDI,const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis,
	float Radius, float HalfHeight, int32 Sides, const FMaterialRenderProxy* MaterialInstance, uint8 DepthPriority);

extern ENGINE_API void DrawCylinder(class FPrimitiveDrawInterface* PDI, const FMatrix& CylToWorld, const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis,
	float Radius, float HalfHeight, int32 Sides, const FMaterialRenderProxy* MaterialInstance, uint8 DepthPriority);


extern ENGINE_API void DrawDisc(class FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& XAxis,const FVector& YAxis,FColor Color,float Radius,int32 NumSides, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriority);
extern ENGINE_API void DrawFlatArrow(class FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& XAxis,const FVector& YAxis,FColor Color,float Length,int32 Width, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriority);

// Line drawing utility functions.
extern ENGINE_API void DrawWireBox(class FPrimitiveDrawInterface* PDI,const FBox& Box,const FLinearColor& Color,uint8 DepthPriority);
extern ENGINE_API void DrawCircle(class FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FLinearColor& Color,float Radius,int32 NumSides,uint8 DepthPriority);
extern ENGINE_API void DrawArc(FPrimitiveDrawInterface* PDI, const FVector Base, const FVector X, const FVector Y, const float MinAngle, const float MaxAngle, const float Radius, const int32 Sections, const FLinearColor& Color, uint8 DepthPriority);
extern ENGINE_API void DrawWireSphere(class FPrimitiveDrawInterface* PDI, const FVector& Base, const FLinearColor& Color, float Radius, int32 NumSides, uint8 DepthPriority);
extern ENGINE_API void DrawWireSphereAutoSides(class FPrimitiveDrawInterface* PDI, const FVector& Base, const FLinearColor& Color, float Radius, uint8 DepthPriority);
extern ENGINE_API void DrawWireSphere(class FPrimitiveDrawInterface* PDI, const FTransform& Transform, const FLinearColor& Color, float Radius, int32 NumSides, uint8 DepthPriority);
extern ENGINE_API void DrawWireSphereAutoSides(class FPrimitiveDrawInterface* PDI, const FTransform& Transform, const FLinearColor& Color, float Radius, uint8 DepthPriority);
extern ENGINE_API void DrawWireCylinder(class FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z,const FLinearColor& Color,float Radius,float HalfHeight,int32 NumSides,uint8 DepthPriority);
extern ENGINE_API void DrawWireCapsule(class FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z,const FLinearColor& Color,float Radius,float HalfHeight,int32 NumSides,uint8 DepthPriority);
extern ENGINE_API void DrawWireChoppedCone(class FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z,const FLinearColor& Color,float Radius,float TopRadius,float HalfHeight,int32 NumSides,uint8 DepthPriority);
extern ENGINE_API void DrawWireCone(class FPrimitiveDrawInterface* PDI, const FMatrix& Transform, float ConeRadius, float ConeAngle, int32 ConeSides, const FLinearColor& Color, uint8 DepthPriority, TArray<FVector>& Verts);
extern ENGINE_API void DrawWireCone(class FPrimitiveDrawInterface* PDI, const FTransform& Transform, float ConeRadius, float ConeAngle, int32 ConeSides, const FLinearColor& Color, uint8 DepthPriority, TArray<FVector>& Verts);
extern ENGINE_API void DrawWireSphereCappedCone(FPrimitiveDrawInterface* PDI, const FTransform& Transform, float ConeRadius, float ConeAngle, int32 ConeSides, int32 ArcFrequency, int32 CapSegments, const FLinearColor& Color, uint8 DepthPriority);
extern ENGINE_API void DrawOrientedWireBox(class FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z, FVector Extent, const FLinearColor& Color,uint8 DepthPriority);
extern ENGINE_API void DrawDirectionalArrow(class FPrimitiveDrawInterface* PDI,const FMatrix& ArrowToWorld,const FLinearColor& InColor,float Length,float ArrowSize,uint8 DepthPriority);
extern ENGINE_API void DrawWireStar(class FPrimitiveDrawInterface* PDI,const FVector& Position, float Size, const FLinearColor& Color,uint8 DepthPriority);
extern ENGINE_API void DrawDashedLine(class FPrimitiveDrawInterface* PDI, const FVector& Start, const FVector& End, const FLinearColor& Color, float DashSize, uint8 DepthPriority, float DepthBias = 0.0f);
extern ENGINE_API void DrawWireDiamond(class FPrimitiveDrawInterface* PDI,const FMatrix& DiamondMatrix, float Size, const FLinearColor& InColor,uint8 DepthPriority);
extern ENGINE_API void DrawCoordinateSystem(FPrimitiveDrawInterface* PDI, FVector const& AxisLoc, FRotator const& AxisRot, float Scale, uint8 DepthPriority);

/**
 * Draws a wireframe of the bounds of a frustum as defined by a transform from clip-space into world-space.
 * @param PDI - The interface to draw the wireframe.
 * @param FrustumToWorld - A transform from clip-space to world-space that defines the frustum.
 * @param Color - The color to draw the wireframe with.
 * @param DepthPriority - The depth priority group to draw the wireframe with.
 */
extern ENGINE_API void DrawFrustumWireframe(
	FPrimitiveDrawInterface* PDI,
	const FMatrix& WorldToFrustum,
	FColor Color,
	uint8 DepthPriority
	);

/**
 * Given a base color and a selection state, returns a color which accounts for the selection state.
 * @param BaseColor - The base color of the object.
 * @param bSelected - The selection state of the object.
 * @param bHovered - True if the object has hover focus
 * @param bUseOverlayIntensity - True if the selection color should be modified by the selection intensity
 * @return The color to draw the object with, accounting for the selection state
 */
extern ENGINE_API FLinearColor GetSelectionColor(const FLinearColor& BaseColor,bool bSelected,bool bHovered, bool bUseOverlayIntensity = true);

/** Vertex Color view modes */
namespace EVertexColorViewMode
{
	enum Type
	{
		/** Invalid or undefined */
		Invalid,

		/** Color only */
		Color,
		
		/** Alpha only */
		Alpha,

		/** Red only */
		Red,

		/** Green only */
		Green,

		/** Blue only */
		Blue,
	};
}


/** Global vertex color view mode setting when SHOW_VertexColors show flag is set */
extern ENGINE_API EVertexColorViewMode::Type GVertexColorViewMode;

/**
 * Returns true if the given view is "rich".  Rich means that calling DrawRichMesh for the view will result in a modified draw call
 * being made.
 * A view is rich if is missing the EngineShowFlags.Materials showflag, or has any of the render mode affecting showflags.
 */
extern ENGINE_API bool IsRichView(const FSceneView* View);

/** Emits draw events for a given FMeshBatch and the PrimitiveSceneProxy corresponding to that mesh element. */
extern ENGINE_API void EmitMeshDrawEvents(const class FPrimitiveSceneProxy* PrimitiveSceneProxy, const struct FMeshBatch& Mesh);

/**
 * Draws a mesh, modifying the material which is used depending on the view's show flags.
 * Meshes with materials irrelevant to the pass which the mesh is being drawn for may be entirely ignored.
 *
 * @param PDI - The primitive draw interface to draw the mesh on.
 * @param Mesh - The mesh to draw.
 * @param WireframeColor - The color which is used when rendering the mesh with EngineShowFlags.Wireframe.
 * @param LevelColor - The color which is used when rendering the mesh with EngineShowFlags.LevelColoration.
 * @param PropertyColor - The color to use when rendering the mesh with EngineShowFlags.PropertyColoration.
 * @param PrimitiveInfo - The FScene information about the UPrimitiveComponent.
 * @param bSelected - True if the primitive is selected.
 * @param ExtraDrawFlags - optional flags to override the view family show flags when rendering
 * @return Number of passes rendered for the mesh
 */
extern ENGINE_API int32 DrawRichMesh(
	FPrimitiveDrawInterface* PDI,
	const struct FMeshBatch& Mesh,
	const FLinearColor& WireframeColor,
	const FLinearColor& LevelColor,
	const FLinearColor& PropertyColor,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	bool bSelected,
	bool bDrawInWireframe = false
	);

/** Draws the UV layout of the supplied asset (either StaticMeshRenderData OR SkeletalMeshRenderData, not both!) */
extern ENGINE_API void DrawUVs(FViewport* InViewport, FCanvas* InCanvas, int32 InTextYPos, const int32 LODLevel, int32 UVChannel, TArray<FVector2D> SelectedEdgeTexCoords, class FStaticMeshRenderData* StaticMeshRenderData, class FStaticLODModel* SkeletalMeshRenderData );

/** Returns true if the Material and Vertex Factory combination require adjacency information. */
bool RequiresAdjacencyInformation( class UMaterialInterface* Material, const class FVertexFactoryType* VertexFactoryType );
