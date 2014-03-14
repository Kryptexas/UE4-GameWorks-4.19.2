// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runtime/Engine/Classes/PhysicsEngine/BodyInstance.h"
#include "PrimitiveSceneProxy.h"
#include "StaticArray.h"
#include "Components/SceneComponent.h"

#include "PrimitiveComponent.generated.h"


/** Information about a vertex of a primitive's triangle. */
struct FPrimitiveTriangleVertex
{
	FVector WorldPosition;
	FVector WorldTangentX;
	FVector WorldTangentY;
	FVector WorldTangentZ;
};

/** An interface to some consumer of the primitive's triangles. */
class FPrimitiveTriangleDefinitionInterface
{
public:

	/** Defines a triangle by its vertices. */
	virtual void DefineTriangle(const TStaticArray<FPrimitiveTriangleVertex,3>& InVertices,const UMaterialInterface* Material) = 0;
};

/** Information about a streaming texture that a primitive uses for rendering. */
struct FStreamingTexturePrimitiveInfo
{
	UTexture* Texture;
	FSphere Bounds;
	float TexelFactor;
};

/** Enum for controlling the falloff of strength of a radial impulse as a function of distance from Origin. */
UENUM()
enum ERadialImpulseFalloff
{
	/** Impulse is a constant strength, up to the limit of its range. */
	RIF_Constant,
	/** Impulse should get linearly weaker the further from origin. */
	RIF_Linear,
	RIF_MAX,
};

UENUM()
enum ECanBeCharacterBase
{
	// can never be base for character to stand on
	ECB_No,
	// can always be character base
	ECB_Yes,
	// owning acter determines whether can be character base
	ECB_Owner,
	ECB_MAX,
};

//UENUM()
namespace EHasCustomNavigableGeometry
{
	enum Type
	{
		// Primitive doesn't have custom navigation geometry, if collision is enabled then its convex/trimesh collision will be used for generating the navmesh
		No,

		// If primitive would normally affect navmesh, DoCustomNavigableGeometryExport() should be called to export this primitive's navigable geometry
		Yes,

		// DoCustomNavigableGeometryExport() should be called even if the mesh is non-collidable and wouldn't normally affect the navmesh
		EvenIfNotCollidable,
	};
};

/** Mirrored from Scene.h */
USTRUCT()
struct FMaterialRelevance
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	uint32 bOpaque:1;

	UPROPERTY()
	uint32 bMasked:1;

	UPROPERTY()
	uint32 bDistortion:1;

	UPROPERTY()
	uint32 bUsesSceneColor:1;

	UPROPERTY()
	uint32 bSeparateTranslucency:1;

	UPROPERTY()
	uint32 bNormalTranslucency:1;

	UPROPERTY()
	uint32 bDisableDepthTest:1;

	/** Default constructor. */
	FMaterialRelevance()
		:	bOpaque(false)
		,	bMasked(false)
		,	bDistortion(false)
		,	bUsesSceneColor(false)
		,	bSeparateTranslucency(false)
		,	bNormalTranslucency(false)
		,	bDisableDepthTest(false)
		{}

		/** Bitwise OR operator.  Sets any relevance bits which are present in either FMaterialRelevance. */
		FMaterialRelevance& operator|=(const FMaterialRelevance& B)
		{
			bOpaque |= B.bOpaque;
			bMasked |= B.bMasked;
			bDistortion |= B.bDistortion;
			bUsesSceneColor |= B.bUsesSceneColor;
			bSeparateTranslucency |= B.bSeparateTranslucency;
			bNormalTranslucency |= B.bNormalTranslucency;
			bDisableDepthTest |= B.bDisableDepthTest;
			return *this;
		}
	
		/** Binary bitwise OR operator. */
		friend FMaterialRelevance operator|(const FMaterialRelevance& A,const FMaterialRelevance& B)
		{
			FMaterialRelevance Result(A);
			Result |= B;
			return Result;
		}

		/** Copies the material's relevance flags to a primitive's view relevance flags. */
		void SetPrimitiveViewRelevance(FPrimitiveViewRelevance& OutViewRelevance) const
		{
			OutViewRelevance.bOpaqueRelevance = bOpaque;
			OutViewRelevance.bMaskedRelevance = bMasked;
			OutViewRelevance.bDistortionRelevance = bDistortion;
			OutViewRelevance.bSceneColorRelevance = bUsesSceneColor;
			OutViewRelevance.bSeparateTranslucencyRelevance = bSeparateTranslucency;
			OutViewRelevance.bNormalTranslucencyRelevance = bNormalTranslucency;
		}
};

/** Information about the sprite category */
USTRUCT()
struct FSpriteCategoryInfo
{
	GENERATED_USTRUCT_BODY()

	/** Sprite category that the component belongs to */
	UPROPERTY()
	FName Category;

	/** Localized name of the sprite category */
	UPROPERTY()
	FText DisplayName;

	/** Localized description of the sprite category */
	UPROPERTY()
	FText Description;
};

/** Delegate for notification of blocking collision against a specific component.  
 * NormalImpulse will be filled in for physics-simulating bodies, but will be zero for swept-component blocking collisions. 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams( FComponentHitSignature, class AActor*, OtherActor, class UPrimitiveComponent*, OtherComp, FVector, NormalImpulse, const FHitResult&, Hit );
/** Delegate for notification of start of overlap of a specific component */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams( FComponentBeginOverlapSignature,class AActor*, OtherActor, class UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex );
/** Delegate for notification of end of overlap of a specific component */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams( FComponentEndOverlapSignature, class AActor*, OtherActor, class UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex );

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FComponentBeginCursorOverSignature, UPrimitiveComponent*, TouchedComponent );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FComponentEndCursorOverSignature, UPrimitiveComponent*, TouchedComponent );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FComponentOnClickedSignature, UPrimitiveComponent*, TouchedComponent );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FComponentOnReleasedSignature, UPrimitiveComponent*, TouchedComponent );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FComponentOnInputTouchBeginSignature, ETouchIndex::Type, FingerIndex, UPrimitiveComponent*, TouchedComponent );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FComponentOnInputTouchEndSignature, ETouchIndex::Type, FingerIndex, UPrimitiveComponent*, TouchedComponent );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FComponentBeginTouchOverSignature, ETouchIndex::Type, FingerIndex, UPrimitiveComponent*, TouchedComponent );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FComponentEndTouchOverSignature, ETouchIndex::Type, FingerIndex, UPrimitiveComponent*, TouchedComponent );

UCLASS(dependson=(UScene, ULightComponent, UEngineTypes), HeaderGroup=Component, abstract, HideCategories=(Mobility))
class ENGINE_API UPrimitiveComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** Tick function for physics ticking **/
	UPROPERTY()
	struct FPrimitiveComponentPostPhysicsTickFunction PostPhysicsComponentTick;

	/** A fence to track when the primitive is detached from the scene in the rendering thread. */
	FRenderCommandFence DetachFence;

	/**
	 * Incremented by the main thread before being attached to the scene, decremented
	 * by the rendering thread after removal. This counter exists to assert that 
	 * operations are safe in order to help avoid race conditions.
	 *
	 *           *** Runtime logic should NEVER rely on this value. ***
	 *
	 * The only safe assertions to make are:
	 *
	 *     AttachmentCounter == 0: The primitive is not exposed to the rendering
	 *                             thread, it is safe to modify shared members.
	 *                             This assertion is valid ONLY from the main thread.
	 *
	 *     AttachmentCounter >= 1: The primitive IS exposed to the rendering
	 *                             thread and therefore shared members must not
	 *                             be modified. This assertion may be made from
	 *                             any thread. Note that it is valid and expected
	 *                             for AttachmentCounter to be larger than 1, e.g.
	 *                             during reattachment.
	 */
	FThreadSafeCounter AttachmentCounter;

	// Scene data

	/** Replacement primitive to draw instead of this one (multiple UPrim's will point to the same Replacement) */
	UPROPERTY()
	TLazyObjectPtr<class UPrimitiveComponent> ReplacementPrimitive;

	// Rendering
	
	/**
	 * The minimum distance at which the primitive should be rendered, 
	 * measured in world space units from the center of the primitive's bounding sphere to the camera position.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=LOD)
	float MinDrawDistance;

	/**  Max draw distance exposed to LDs. The real max draw distance is the min (disregarding 0) of this and volumes affecting this object. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=LOD, meta=(DisplayName="Desired Max Draw Distance") )
	float LDMaxDrawDistance;

	/**
	 * The distance to cull this primitive at.  
	 * A CachedMaxDrawDistance of 0 indicates that the primitive should not be culled by distance.
	 */
	UPROPERTY(Category=LOD, AdvancedDisplay, VisibleAnywhere, BlueprintReadWrite, meta=(DisplayName="Current Max Draw Distance") )
	float CachedMaxDrawDistance;

	/** The scene depth priority group to draw the primitive in. */
	UPROPERTY()
	TEnumAsByte<enum ESceneDepthPriorityGroup> DepthPriorityGroup;

	/** The scene depth priority group to draw the primitive in, if it's being viewed by its owner. */
	UPROPERTY()
	TEnumAsByte<enum ESceneDepthPriorityGroup> ViewOwnerDepthPriorityGroup;

private:
	/** DEPRECATED, use bGenerateOverlapEvents instead */
	UPROPERTY()
	uint32 bNoEncroachCheck_DEPRECATED:1;

public:
	UPROPERTY()
	uint32 bDisableAllRigidBody_DEPRECATED:1;

	/** 
	 * Indicates if we'd like to create physics state all the time (for collision and simulation). 
	 * If you set this to false, it still will create physics state if collision or simulation activated. 
	 * This can help performance if you'd like to avoid overhead of creating physics state when triggers 
	 **/
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Collision)
	uint32 bAlwaysCreatePhysicsState:1;

	/** If true, this component will generate overlap events when it is overlapping other components (e.g. Begin Overlap) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Collision)
	uint32 bGenerateOverlapEvents:1;

	/** If true, this component will generate individual overlaps for each overlapping physics body if it is a multi-body component. When false, this component will
		generate only one overlap, regardless of how many physics bodies it has and how many of them are overlapping another component/body. This flag has no
		influence on single body components. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=Collision)
	uint32 bMultiBodyOverlap:1;


	/** If true, this component will look for collisions on both physic scenes during movement */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=Collision)
	uint32 bCheckAsyncSceneOnMove:1;

	/** If true, component sweeps with this component should trace against complex collision during movement. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=Collision)
	uint32 bTraceComplexOnMove:1;

	/** If true, component sweeps will return the material in their hit-info. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=Collision)
	uint32 bReturnMaterialOnMove:1;

	/** True if the primitive should be rendered using ViewOwnerDepthPriorityGroup if viewed by its owner. */
	UPROPERTY()
	uint32 bUseViewOwnerDepthPriorityGroup:1;

	/** Whether to accept cull distance volumes to modify cached cull distance. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=LOD)
	uint32 bAllowCullDistanceVolume:1;

	/** true if the primitive has motion blur velocity meshes */
	UPROPERTY()
	uint32 bHasMotionBlurVelocityMeshes:1;
	
	/** If true, this component will be rendered in the CustomDepth pass (usually used for outlines) */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Rendering)
	uint32 bRenderCustomDepth:1;

	/** If true, this component will be rendered in the main pass (z prepass, basepass, transparency) */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=Rendering)
	uint32 bRenderInMainPass:1;

	/** Whether to completely hide the primitive in the game; if true, the primitive is not drawn, does not cast a shadow, and does not affect voxel lighting. */
	UPROPERTY()
	uint32 HiddenGame_DEPRECATED:1;

	/** Whether to draw the primitive in game views, if the primitive isn't hidden. */
	UPROPERTY()
	uint32 DrawInGame_DEPRECATED:1;
	
	/** Whether the primitive receives decals. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Rendering)
	uint32 bReceivesDecals:1;

	/** If this is True, this component won't be visible when the view actor is the component's owner, directly or indirectly. */
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, BlueprintReadOnly, Category=Rendering)
	uint32 bOwnerNoSee:1;

	/** If this is True, this component will only be visible when the view actor is the component's owner, directly or indirectly. */
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, BlueprintReadOnly, Category=Rendering)
	uint32 bOnlyOwnerSee:1;

	/** Treat this primitive as part of the background for occlusion purposes. This can be used as an optimization to reduce the cost of rendering skyboxes, large ground planes that are part of the vista, etc. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Rendering)
	uint32 bTreatAsBackgroundForOcclusion:1;

	/** 
	 * Whether to render the primitive in the depth only pass.  
	 * This should generally be true for all objects, and let the renderer make decisions about whether to render objects in the depth only pass.
	 * @todo - if any rendering features rely on a complete depth only pass, this variable needs to go away.
	 */
	UPROPERTY()
	uint32 bUseAsOccluder:1;

	/** If this is True, this component can be selected in the editor. */
	UPROPERTY()
	uint32 bSelectable:1;

	/** If true, forces mips for textures used by this component to be resident when this component's level is loaded. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=TextureStreaming)
	uint32 bForceMipStreaming:1;

	/** If true a hit-proxy will be generated for each instance of instanced static meshes */
	UPROPERTY()
	uint32 bHasPerInstanceHitProxies:1;

	// Lighting flags
	
	/** Controls whether the primitive component should cast a shadow or not. **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Lighting)
	uint32 CastShadow:1;

	/** Controls whether the primitive should inject light into the Light Propagation Volume.  This flag is only used if CastShadow is true. **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Lighting, AdvancedDisplay)
	uint32 bAffectDynamicIndirectLighting:1;

	/** Controls whether the primitive should cast shadows in the case of non precomputed shadowing.  This flag is only used if CastShadow is true. **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Lighting, AdvancedDisplay)
	uint32 bCastDynamicShadow:1;

	/** Whether the object should cast a static shadow from shadow casting lights.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Lighting, AdvancedDisplay)
	uint32 bCastStaticShadow:1;

	/** 
	 * Whether the object should cast a volumetric translucent shadow.
	 * Volumetric translucent shadows are useful for primitives with smoothly changing opacity like particles representing a volume, 
	 * But have artifacts when used on highly opaque surfaces.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Lighting)
	uint32 bCastVolumetricTranslucentShadow:1;

	/** 
	 * Whether this component should create a per-object shadow that gives higher effective shadow resolution. 
	 * Useful for cinematic character shadowing.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Lighting)
	uint32 bCastInsetShadow:1;

	/** 
	 *	If true, the primitive will cast shadows even if bHidden is true.
	 *	Controls whether the primitive should cast shadows when hidden.
	 *	This flag is only used if CastShadow is true.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Lighting)
	uint32 bCastHiddenShadow:1;

	/** Whether this primitive should cast dynamic shadows as if it were a two sided material. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Lighting)
	uint32 bCastShadowAsTwoSided:1;

	/** 
	 * Whether to light this primitive as if it were static, including generating lightmaps.  
	 * This only has an effect for component types that can bake lighting, like static mesh components.
	 * This is useful for moving meshes that don't change significantly.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Lighting)
	uint32 bLightAsIfStatic:1;

	/** 
	 * Whether to light this component and any attachments as a group.  This only has effect on the root component of an attachment tree.
	 * When enabled, attached component shadowing settings like bCastInsetShadow, bCastVolumetricTranslucentShadow, etc, will be ignored.
	 * This is useful for improving performance when multiple movable components are attached together.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Lighting)
	uint32 bLightAttachmentsAsGroup:1;

	UPROPERTY()
	bool bHasCachedStaticLighting;

	/** If true, asynchronous static build lighting will be enqueued to be applied to this */
	UPROPERTY(Transient)
	bool bStaticLightingBuildEnqueued;
	
	// Physics
	
	/** Will ignore radial impulses applied to this component. */
	UPROPERTY()
	uint32 bIgnoreRadialImpulse:1;

	/** Will ignore radial forces applied to this component. */
	UPROPERTY()
	uint32 bIgnoreRadialForce:1;

	// General flags.
	
	/** If this is True, this component must always be loaded on clients, even if Hidden and CollisionEnabled is NoCollision. */
	UPROPERTY()
	uint32 AlwaysLoadOnClient:1;

	/** If this is True, this component must always be loaded on servers, even if Hidden and CollisionEnabled is NoCollision */
	UPROPERTY()
	uint32 AlwaysLoadOnServer:1;

	/** Determines whether or not we allow shadowing fading.  Some objects (especially in cinematics) having the shadow fade/pop out looks really bad. **/
	UPROPERTY()
	uint32 bAllowShadowFade:1;

	/** Composite the drawing of this component onto the scene after post processing (only applies to editor drawing) */
	UPROPERTY()
	uint32 bUseEditorCompositing:1;
	/**
	 * Translucent objects with a lower sort priority draw behind objects with a higher priority.
	 * Translucent objects with the same priority are rendered from back-to-front based on their bounds origin.
	 *
	 * Ignored if the object is not translucent.  The default priority is zero.
	 * Warning: This should never be set to a non-default value unless you know what you are doing, as it will prevent the renderer from sorting correctly.  
	 * It is especially problematic on dynamic gameplay effects.
	 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category=Rendering)
	int32 TranslucencySortPriority;

	/** Used for precomputed visibility */
	UPROPERTY()
	int32 VisibilityId;

	/** Used by the renderer, to identify a component across re-registers. */
	FPrimitiveComponentId ComponentId;

	/** Multiplier used to scale the Light Propagation Volume light injection bias, to reduce light bleeding. 
	  * Set to 0 for no bias, 1 for default or higher for increased biasing (e.g. for 
	  * thin geometry such as walls)
	  **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category=Rendering, meta=(UIMin = "0.0", UIMax = "3.0"))
	float LpvBiasMultiplier;

	// Internal physics engine data.
	
	/** Physics scene information for this component, holds a single rigid body with multiple shapes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Collision, meta=(ShowOnlyInnerProperties))
	FBodyInstance BodyInstance;

	/** can this component potentially influence navigation */
	UPROPERTY(EditDefaultsOnly, Category=Collision, AdvancedDisplay)
	uint32 bCanEverAffectNavigation:1;

protected:

	/** Returns true if all descendant components that we can possibly collide with use relative location and rotation. */
	virtual bool AreAllCollideableDescendantsRelative(bool bAllowCachedValue = true) const;

	/** Result of last call to AreAllCollideableDescendantsRelative(). */
	uint32 bCachedAllCollideableDescendantsRelative:1;

	/** Last time we checked AreAllCollideableDescendantsRelative(), so we can throttle those tests since it rarely changes once false. */
	float LastCheckedAllCollideableDescendantsTime;

	/** if set to true then DoCustomNavigableGeometryExport will be called to collect navigable 
	 *	geometry of this component.
	 *	@NOTE that owner Actor needs to be "navigation relevant" (Actor->IsNavigationRelevant() == true)
	 *	to have this mechanism triggered at all */
	TEnumAsByte<EHasCustomNavigableGeometry::Type> bHasCustomNavigableGeometry;

	/** Next id to be used by a component. */
	static uint64 NextComponentId;

public:
	/** 
	 * Scales the bounds of the object.
	 * This is useful when using World Position Offset to animate the vertices of the object outside of its bounds. 
	 * Warning: Increasing the bounds of an object will reduce performance and shadow quality!
	 * Currently only used by StaticMeshComponent and SkeletalMeshComponent.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Rendering, meta=(UIMin = "1", UIMax = "10.0"))
	float BoundsScale;

	/** Last time the component was submitted for rendering (called FScene::AddPrimitive). */
	UPROPERTY(transient)
	float LastSubmitTime;

	/**
	 * The value of WorldSettings->TimeSeconds for the frame when this component was last rendered.  This is written
	 * from the render thread, which is up to a frame behind the game thread, so you should allow this time to
	 * be at least a frame behind the game thread's world time before you consider the actor non-visible.
	 * There's an equivalent variable in PrimitiveComponent.
	 */
	UPROPERTY(transient)
	float LastRenderTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Base)
	TEnumAsByte<enum ECanBeCharacterBase> CanBeCharacterBase;

	/** Set of actors to ignore during component sweeps in MoveComponent() */
	TArray<TWeakObjectPtr<AActor> > MoveIgnoreActors;

#if WITH_EDITOR
	/** Override delegate used for checking the selection state of a component */
	DECLARE_DELEGATE_RetVal_OneParam( bool, FSelectionOverride, const UPrimitiveComponent* );
	FSelectionOverride SelectionOverrideDelegate;
#endif

protected:
	/** Set of components that this component is currently overlapping. */
	TArray<FOverlapInfo> OverlappingComponents;

public:

	/** Returns true, if this component tracks overlaps. This might be false for multi-body components like SkelMeshes or Destructibles. Even if 
		false, other components can still overlap this component. */
	virtual bool ShouldTrackOverlaps() const { return true; };
	/** 
	 * Begin tracking an overlap interaction with the component specified.
	 * @param OtherComp - The component of the other actor that this component is now overlapping
	 * @param bDoNotifies - True to dispatch appropriate begin/end overlap notifications when these events occur.
	 */
	void BeginComponentOverlap(const FOverlapInfo& OtherOverlap, bool bDoNotifies);
	
	/** 
	 * Finish tracking an overlap interaction that is no longer occurring between this component and the component specified. 
	 * @param OtherComp - The component of the other actor to stop overlapping
	 * @param bDoNotifies - True to dispatch appropriate begin/end overlap notifications when these events occur.
	 * @param bNoNotifySelf	- True to skip end overlap notifications to this component's.  Does not affect notifications to OtherComp's actor.
	 */
	void EndComponentOverlap(const FOverlapInfo& OtherOverlap, bool bDoNotifies=true, bool bNoNotifySelf=false);

	/** Returns true if this component is overlapping OtherComp, false otherwise. */
	bool IsOverlappingComponent(UPrimitiveComponent const* OtherComp) const;
	/** Returns true if this component is overlapping OtherComp, false otherwise. */
	bool IsOverlappingComponent(const FOverlapInfo& Overlap) const;

	/** Return true if this component is overlapping any component of the given actor, false otherwise. */
	bool IsOverlappingActor(const AActor* Other) const;

	/** Appends list of overlaps with components owned by the given actor to the 'OutOverlaps' array. Returns the number of overlaps that were added. */
	bool GetOverlapsWithActor(const AActor* Actor, TArray<FOverlapInfo>& OutOverlaps) const;

	/** 
	 * Returns a list of actors that this component is overlapping.
	 * @param OverlappingActors		[out] Returned list of overlapping actors
	 * @param ClassFilter			[optional] If set, only returns actors of this class or subclasses
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(UnsafeDuringActorConstruction="true"))
	void GetOverlappingActors(TArray<AActor*>& OverlappingActors, UClass* ClassFilter=NULL) const;

	/** Returns list of components this component is overlapping. */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(UnsafeDuringActorConstruction="true"))
	void GetOverlappingComponents(TArray<UPrimitiveComponent*>& OverlappingComponents) const;

	/** Returns list of components this component is overlapping. */
	UFUNCTION(BlueprintCallable, Category="Collision")
	const TArray<FOverlapInfo>& GetOverlapInfos() const;

	/** 
	 * Queries world and updates overlap tracking state for this component.
	 * @param PendingOverlaps			An ordered list of components that the MovedComponent overlapped during its movement (i.e. generated during a sweep).
	 *									May not be overlapping them now.
	 * @param bDoNotifies				True to dispatch being/end overlap notifications when these events occur.
	 * @param OverlapsAtEndLocation		If non-null, the given list of overlaps will be used as the overlaps for this component at the current location, rather than checking for them separately.
	 *									Generally this should only be used if this component is the RootComponent of the owning actor and overlaps with other descendant components have been verified.
	 */
	virtual void UpdateOverlaps(TArray<FOverlapInfo> const* PendingOverlaps=NULL, bool bDoNotifies=true, const TArray<FOverlapInfo>* OverlapsAtEndLocation=NULL) OVERRIDE;

	/** Tells this component to ignore collision with the specified actor when being moved. */
	UFUNCTION(BlueprintCallable, Category = "Collision")
	void IgnoreActorWhenMoving(AActor* Actor, bool bShouldIgnore);

	/** Overridden to use the overlaps to find the physics volume. */
	virtual void UpdatePhysicsVolume( bool bTriggerNotifiers ) OVERRIDE;

	/**
	 *  Test the collision of the supplied component at the supplied location/rotation, and determine the set of components that it overlaps
	 *  @param  OutOverlaps     Array of overlaps found between this component in specified pose and the world
	 *  @param  World			World to use for overlap test
	 *  @param  Pos             Location to place the component's geometry at to test against the world
	 *  @param  Rot             Rotation to place components' geometry at to test against the world
	 *	@param	ObjectQueryParams	List of object types it's looking for. When this enters, we do object query with component shape
	 *  @return TRUE if OutOverlaps contains any blocking results
	 */
	virtual bool ComponentOverlapMulti(TArray<struct FOverlapResult>& OutOverlaps, const class UWorld* World, const FVector& Pos, const FRotator& Rot, const struct FComponentQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams=FCollisionObjectQueryParams::DefaultObjectQueryParam) const;

	/** Called when a component is touched */
	UPROPERTY(BlueprintAssignable, Category="Collision")
	FComponentHitSignature OnComponentHit;

	/** Called when something overlaps this component */
	UPROPERTY(BlueprintAssignable, Category="Collision")
	FComponentBeginOverlapSignature OnComponentBeginOverlap;

	/** Called when something ends overlapping this component */
	UPROPERTY(BlueprintAssignable, Category="Collision")
	FComponentEndOverlapSignature OnComponentEndOverlap;

	/** Called when the mouse cursor is moved over this component when this mouse interface is enabled and mouse over events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Mouse Input")
	FComponentBeginCursorOverSignature OnBeginCursorOver;
		 
	/** Called when the mouse cursor is moved off this component when this mouse interface is enabled and mouse over events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Mouse Input")
	FComponentEndCursorOverSignature OnEndCursorOver;

	/** Called when the left mouse button is clicked while the mouse is over this component when the mouse interface is enabled and click events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Mouse Input")
	FComponentOnClickedSignature OnClicked;

	/** Called when the left mouse button is released while the mouse is over this component when the mouse interface is enabled and click events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Mouse Input")
	FComponentOnReleasedSignature OnReleased;
		 
	/** Called when a touch input is received over this component when and click events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Touch Input")
	FComponentOnInputTouchBeginSignature OnInputTouchBegin;

	/** Called when a touch input is received over this component when and click events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Touch Input")
	FComponentOnInputTouchEndSignature OnInputTouchEnd;

	/** Called when a finger is moved over this component when this mouse interface is enabled and touch over events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Touch Input")
	FComponentBeginTouchOverSignature OnInputTouchEnter;

	/** Called when a finger is moved off this component when this mouse interface is enabled and touch over events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Touch Input")
	FComponentEndTouchOverSignature OnInputTouchLeave;

	/**
	 * Returns the material used by the element at the specified index
	 * @param ElementIndex - The element to access the material of.
	 * @return the material used by the indexed element of this mesh.
	 */
	UFUNCTION(BlueprintCallable, Category="Rendering|Material")
	virtual class UMaterialInterface* GetMaterial(int32 ElementIndex) const;

	/**
	 * Changes the material applied to an element of the mesh.
	 * @param ElementIndex - The element to access the material of.
	 * @return the material used by the indexed element of this mesh.
	 */
	UFUNCTION(BlueprintCallable, Category="Rendering|Material")
	virtual void SetMaterial(int32 ElementIndex, class UMaterialInterface* Material);

	/**
	 * Creates a Dynamic Material Instance for the specified element index.  The parent of the instance is set to the material being replaced.
	 * @param ElementIndex - The index of the skin to replace the material for.  If invalid, the material is unchanged and NULL is returned.
	 */
	UFUNCTION(BlueprintCallable, meta=(FriendlyName = "CreateMIDForElement", DeprecatedFunction, DeprecationMessage="Use CreateDynamicMaterialInstance instead."), Category="Rendering|Material")
	virtual class UMaterialInstanceDynamic* CreateAndSetMaterialInstanceDynamic(int32 ElementIndex);

	/**
	 * Creates a Dynamic Material Instance for the specified element index.  The parent of the instance is set to the material being replaced.
	 * @param ElementIndex - The index of the skin to replace the material for.  If invalid, the material is unchanged and NULL is returned.
	 */
	UFUNCTION(BlueprintCallable, meta=(FriendlyName = "CreateMIDForElementFromMaterial", DeprecatedFunction, DeprecationMessage="Use CreateDynamicMaterialInstance instead."), Category="Rendering|Material")
	virtual class UMaterialInstanceDynamic* CreateAndSetMaterialInstanceDynamicFromMaterial(int32 ElementIndex, class UMaterialInterface* Parent);

	/**
	 * Creates a Dynamic Material Instance for the specified element index, optionally from the supplied material.
	 * @param ElementIndex - The index of the skin to replace the material for.  If invalid, the material is unchanged and NULL is returned.
	 */
	UFUNCTION(BlueprintCallable, Category="Rendering|Material")
	virtual class UMaterialInstanceDynamic* CreateDynamicMaterialInstance(int32 ElementIndex, class UMaterialInterface* SourceMaterial = NULL);

	/** Returns the slope override struct for this component. */
	UFUNCTION(BlueprintCallable, Category="Physics")
	const struct FWalkableSlopeOverride& GetWalkableSlopeOverride() const;

	/** 
	 *	Sets whether or not a single body should use physics simulation, or should be 'fixed' (kinematic).
	 *
	 *	@param	bSimulate	New simulation state for single body
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void SetSimulatePhysics(bool bSimulate);

	/**
	 *	Add an impulse to a single rigid body. Good for one time instant burst.
	 *
	 *	@param	Impulse		Magnitude and direction of impulse to apply.
	 *	@param	BoneName	If a SkeletalMeshComponent, name of body to apply impulse to. 'None' indicates root body.
	 *	@param	bVelChange	If true, the Strength is taken as a change in velocity instead of an impulse (ie. mass will have no affect).
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void AddImpulse(FVector Impulse, FName BoneName = NAME_None, bool bVelChange = false);

	/**
	 *	Add an impulse to a single rigid body at a specific location. 
	 *
	 *	@param	Impulse		Magnitude and direction of impulse to apply.
	 *	@param	Location	Point in world space to apply impulse at.
	 *	@param	BoneName	If a SkeletalMeshComponent, name of bone to apply impulse to. 'None' indicates root body.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void AddImpulseAtLocation(FVector Impulse, FVector Location, FName BoneName = NAME_None);

	/**
	 * Add an impulse to all rigid bodies in this component, radiating out from the specified position.
	 *
	 * @param Origin		Point of origin for the radial impulse blast, in world space
	 * @param Radius		Size of radial impulse. Beyond this distance from Origin, there will be no affect.
	 * @param Strength		Maximum strength of impulse applied to body.
	 * @param Falloff		Allows you to control the strength of the impulse as a function of distance from Origin.
	 * @param bVelChange	If true, the Strength is taken as a change in velocity instead of an impulse (ie. mass will have no affect).
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void AddRadialImpulse(FVector Origin, float Radius, float Strength, enum ERadialImpulseFalloff Falloff, bool bVelChange = false);

	/**
	 *	Add a force to a single rigid body.
	 *  This is like a 'thruster'. Good for adding a burst over some (non zero) time. Should be called every frame for the duration of the force.
	 *
	 *	@param	Force		Force vector to apply. Magnitude indicates strength of force.
	 *	@param	BoneName	If a SkeletalMeshComponent, name of body to apply force to. 'None' indicates root body.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void AddForce(FVector Force, FName BoneName = NAME_None);

	/**
	 *	Add a force to a single rigid body at a particular location.
	 *  This is like a 'thruster'. Good for adding a burst over some (non zero) time. Should be called every frame for the duration of the force.
	 *
	 *	@param Force		Force vector to apply. Magnitude indicates strength of force.
	 *	@param Location		Location to apply force, in world space.
	 *	@param BoneName		If a SkeletalMeshComponent, name of body to apply force to. 'None' indicates root body.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void AddForceAtLocation(FVector Force, FVector Location, FName BoneName = NAME_None);

	/**
	 *	Add a force to all bodies in this component, originating from the supplied world-space location.
	 *
	 *	@param Origin		Origin of force in world space.
	 *	@param Radius		Radius within which to apply the force.
	 *	@param Strength		Strength of force to apply.
	 *  @param Falloff		Allows you to control the strength of the force as a function of distance from Origin.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void AddRadialForce(FVector Origin, float Radius, float Strength, enum ERadialImpulseFalloff Falloff);

	/**
	 *	Add a torque to a single rigid body.
	 *	@param Torque		Torque to apply. Direction is axis of rotation and magnitude is strength of torque.
	 *	@param BoneName		If a SkeletalMeshComponent, name of body to apply torque to. 'None' indicates root body.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	void AddTorque(FVector Torque, FName BoneName = NAME_None);

	/**
	 *	Set the linear velocity of a single body.
	 *	This should be used cautiously - it may be better to use AddForce or AddImpulse.
	 *
	 *	@param NewVel			New linear velocity to apply to physics.
	 *	@param bAddToCurrent	If true, NewVel is added to the existing velocity of the body.
	 *	@param BoneName			If a SkeletalMeshComponent, name of body to modify velocity of. 'None' indicates root body.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	void SetPhysicsLinearVelocity(FVector NewVel, bool bAddToCurrent = false, FName BoneName = NAME_None);

	/** 
	 *	Get the linear velocity of a single body. 
	 *	@param BoneName			If a SkeletalMeshComponent, name of body to get velocity of. 'None' indicates root body.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")	
	FVector GetPhysicsLinearVelocity(FName BoneName = NAME_None);

	/**
	 *	Set the linear velocity of all bodies in this component.
	 *
	 *	@param NewVel			New linear velocity to apply to physics.
	 *	@param bAddToCurrent	If true, NewVel is added to the existing velocity of the body.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void SetAllPhysicsLinearVelocity(FVector NewVel, bool bAddToCurrent = false);

	/**
	 *	Set the angular velocity of a single body.
	 *	This should be used cautiously - it may be better to use AddTorque or AddImpulse.
	 *
	 *	@param NewAngVel		New angular velocity to apply to body, in degrees per second.
	 *	@param bAddToCurrent	If true, NewAngVel is added to the existing angular velocity of the body.
	 *	@param BoneName			If a SkeletalMeshComponent, name of body to modify angular velocity of. 'None' indicates root body.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	void SetPhysicsAngularVelocity(FVector NewAngVel, bool bAddToCurrent = false, FName BoneName = NAME_None);

	/** 
	 *	Get the angular velocity of a single body, in degrees per second. 
	 *	@param BoneName			If a SkeletalMeshComponent, name of body to get velocity of. 'None' indicates root body.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")	
	FVector GetPhysicsAngularVelocity(FName BoneName = NAME_None);

	/**
	 *	'Wake' physics simulation for a single body.
	 *	@param	BoneName	If a SkeletalMeshComponent, name of body to wake. 'None' indicates root body.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	void WakeRigidBody(FName BoneName = NAME_None);

	/** 
	 *	Force a single body back to sleep. 
	 *	@param	BoneName	If a SkeletalMeshComponent, name of body to put to sleep. 'None' indicates root body.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	void PutRigidBodyToSleep(FName BoneName = NAME_None);

	/** Changes the value of bNotifyRigidBodyCollision
	 * @param bNewNotifyRigidBodyCollision - The value to assign to bNotifyRigidBodyCollision
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void SetNotifyRigidBodyCollision(bool bNewNotifyRigidBodyCollision);

	/** Changes the value of bOwnerNoSee. */
	UFUNCTION(BlueprintCallable, Category="Rendering")
	void SetOwnerNoSee(bool bNewOwnerNoSee);
	
	/** Changes the value of bOnlyOwnerSee. */
	UFUNCTION(BlueprintCallable, Category="Rendering")
	void SetOnlyOwnerSee(bool bNewOnlyOwnerSee);

	/** Changes the value of CastShadow. */
	UFUNCTION(BlueprintCallable, Category="Rendering")
	void SetCastShadow(bool NewCastShadow);

	/** Changes the value of TranslucentSortPriority. */
	UFUNCTION(BlueprintCallable, Category="Rendering")
	void SetTranslucentSortPriority(int32 NewTranslucentSortPriority);

	/** Controls what kind of collision is enabled for this body */
	UFUNCTION(BlueprintCallable, Category="Collision")
	virtual void SetCollisionEnabled(ECollisionEnabled::Type NewType);

	/**  
	* Set Collision Profile Name
	* This function is called by constructors when they set ProfileName
	* This will change current CollisionProfileName to be this, and overwrite Collision Setting
	* 
	* @param InCollisionProfileName : New Profile Name
	*/
	UFUNCTION(BlueprintCallable, Category="Collision")	
	virtual void SetCollisionProfileName(FName InCollisionProfileName);

	/** Get the collision profile name */
	UFUNCTION(BlueprintCallable, Category="Collision")
	FName GetCollisionProfileName();

	/**
	 *	Changes the collision channel that this object uses when it moves
	 *	@param      Channel     The new channel for this component to use
	 */
	UFUNCTION(BlueprintCallable, Category="Collision")	
	void SetCollisionObjectType(ECollisionChannel Channel);

	/** Perform a line trace against a single component */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(FriendlyName = "Line Trace Component", bTraceComplex="true"))	
	bool K2_LineTraceComponent(FVector TraceStart, FVector TraceEnd, bool bTraceComplex, bool bShowTrace, FVector& HitLocation, FVector& HitNormal);

	/** Sets the bRenderCustomDepth property and marks the render state dirty. */
	UFUNCTION(BlueprintCallable, Category="Rendering")
	void SetRenderCustomDepth(bool bValue);

public:
	static int32 CurrentTag;

	/** The primitive's scene info. */
	FPrimitiveSceneProxy* SceneProxy;
	
#if WITH_EDITOR
	virtual const int32 GetNumUncachedStaticLightingInteractions() const; // recursive function
#endif

	// Begin UActorComponent Interface
	virtual void InvalidateLightingCacheDetailed(bool bInvalidateBuildEnqueuedLighting, bool bTranslationOnly) OVERRIDE;
	virtual bool IsEditorOnly() const OVERRIDE;
	// End UActorComponent Interface

	/** @return true if this should create physics state */
	virtual bool ShouldCreatePhysicsState() const OVERRIDE;

	/** @return true if the owner is selected and this component is selectable */
	virtual bool ShouldRenderSelected() const;

	/**  @return True if a primitive's parameters as well as its position is static during gameplay, and can thus use static lighting. */
	bool HasStaticLighting() const;

	virtual bool HasValidSettingsForStaticLighting() const 
	{
		return HasStaticLighting();
	}

	/**  @return true if only unlit materials are used for rendering, false otherwise. */
	virtual bool UsesOnlyUnlitMaterials() const;

	/**
	 * Returns the lightmap resolution used for this primitive instance in the case of it supporting texture light/ shadow maps.
	 * 0 if not supported or no static shadowing.
	 *
	 * @param	Width	[out]	Width of light/shadow map
	 * @param	Height	[out]	Height of light/shadow map
	 * @return	bool			true if LightMap values are padded, false if not
	 */
	virtual bool GetLightMapResolution( int32& Width, int32& Height ) const;

	/**
	 *	Returns the static lightmap resolution used for this primitive.
	 *	0 if not supported or no static shadowing.
	 *
	 * @return	int32		The StaticLightmapResolution for the component
	 */
	virtual int32 GetStaticLightMapResolution() const { return 0; }

	/**
	 * Returns the light and shadow map memory for this primitive in its out variables.
	 *
	 * Shadow map memory usage is per light whereof lightmap data is independent of number of lights, assuming at least one.
	 *
	 * @param [out] LightMapMemoryUsage		Memory usage in bytes for light map (either texel or vertex) data
	 * @param [out]	ShadowMapMemoryUsage	Memory usage in bytes for shadow map (either texel or vertex) data
	 */
	virtual void GetLightAndShadowMapMemoryUsage( int32& LightMapMemoryUsage, int32& ShadowMapMemoryUsage ) const;


#if WITH_EDITOR
	/**
	 * Requests the information about the component that the static lighting system needs.
	 * @param OutPrimitiveInfo - Upon return, contains the component's static lighting information.
	 * @param InRelevantLights - The lights relevant to the primitive.
	 * @param InOptions - The options for the static lighting build.
	 */
	virtual void GetStaticLightingInfo(struct FStaticLightingPrimitiveInfo& OutPrimitiveInfo,const TArray<class ULightComponent*>& InRelevantLights,const class FLightingBuildOptions& Options) {}
#endif
	/**
	 *	Requests whether the component will use texture, vertex or no lightmaps.
	 *
	 *	@return	ELightMapInteractionType		The type of lightmap interaction the component will use.
	 */
	virtual ELightMapInteractionType GetStaticLightingType() const	{ return LMIT_None;	}

	/**
	 * Enumerates the streaming textures used by the primitive.
	 * @param OutStreamingTextures - Upon return, contains a list of the streaming textures used by the primitive.
	 */
	virtual void GetStreamingTextureInfo(TArray<struct FStreamingTexturePrimitiveInfo>& OutStreamingTextures) const
	{}

	/**
	 * Determines the DPG the primitive's primary elements are drawn in.
	 * Even if the primitive's elements are drawn in multiple DPGs, a primary DPG is needed for occlusion culling and shadow projection.
	 * @return The DPG the primitive's primary elements will be drawn in.
	 */
	virtual uint8 GetStaticDepthPriorityGroup() const
	{
		return DepthPriorityGroup;
	}

	/** 
	 * Retrieves the materials used in this component 
	 * 
	 * @param OutMaterials	The list of used materials.
	 */
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const {}

	/**
	 * Returns the material textures used to render this primitive for the given platform.
	 * Internally calls GetUsedMaterials() and GetUsedTextures() for each material.
	 *
	 * @param OutTextures	[out] The list of used textures.
	 */
	void GetUsedTextures(TArray<UTexture*>& OutTextures, EMaterialQualityLevel::Type QualityLevel);

	/** Controls if we get a post physics tick or not. If set during ticking, will take effect next frame **/
	void SetPostPhysicsComponentTickEnabled(bool bEnable);

	/** Returns whether we have the post physics tick enabled **/
	bool IsPostPhysicsComponentTickEnabled() const;

	/** Tick function called after physics (sync scene) has finished simulation */
	virtual void PostPhysicsTick(FPrimitiveComponentPostPhysicsTickFunction &ThisTickFunction) {}

	/** Return the BodySetup to use for this PrimitiveComponent (single body case) */
	virtual class UBodySetup* GetBodySetup() { return NULL; }



	/** Move this component to match the physics rigid body pose. Note, a warning will be generated if you call this function on a component that is attached to something */
	void SyncComponentToRBPhysics();

	/** 
	 *	Returns the matrix that should be used to render this component. 
	 *	Allows component class to perform graphical distortion to the component not supported by an FTransform 
	 */
	virtual FMatrix GetRenderMatrix() const;

	/** @return number of material elements in this primitive */
	UFUNCTION(BlueprintCallable, Category="Rendering|Material")
	virtual int32 GetNumMaterials() const;
	
	/** Get a BodyInstance from this component. The supplied name is used in the SkeletalMeshComponent case. A name of NAME_None in the skeletal case gives the root body instance. */
	virtual FBodyInstance* GetBodyInstance(FName BoneName = NAME_None) const;

	/** 
	 * returns Distance to closest Body Instance surface. 
	 *
	 * @param Point				World 3D vector
	 * @param OutPointOnBody	Point on the surface of collision closest to Point
	 * 
	 * @return		Success if returns > 0.f, if returns 0.f, it is either not convex or inside of the point
	 *				If returns < 0.f, this primitive does not have collsion
	 */
	float GetDistanceToCollision(const FVector & Point, FVector& ClosestPointOnCollision) const;

	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy()
	{
		return NULL;
	}

	/**
	 * Determines whether the proxy for this primitive type needs to be recreated whenever the primitive moves.
	 * @return true to recreate the proxy when UpdateTransform is called.
	 */
	virtual bool ShouldRecreateProxyOnUpdateTransform() const
	{
		return false;
	}

	/** 
	 * This isn't bound extent, but for shape component to utilize extent is 0. 
	 * For normal primitive, this is 0, for ShapeComponent, this will have valid information
	 */
	virtual bool IsZeroExtent() const
	{
		return false;
	}

	/** Called when a component is 'damaged', allowing for component class specific behaviour */
	virtual void ReceiveComponentDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

protected:

	/** Give the static mesh component recreate render state context access to Create/DestroyRenderState_Concurrent(). */
	friend class FStaticMeshComponentRecreateRenderStateContext;

	// Begin USceneComponent Interface
	virtual void OnUpdateTransform(bool bSkipPhysicsMove) OVERRIDE;

	/** Called when AttachParent changes, to allow the scene to update its attachment state. */
	virtual void OnAttachmentChanged() OVERRIDE;

public:
	virtual bool IsSimulatingPhysics(FName BoneName = NAME_None) const OVERRIDE;

	// End USceneComponentInterface


	// Begin UActorComponent Interface
protected:
	virtual void CreateRenderState_Concurrent() OVERRIDE;
	virtual void SendRenderTransform_Concurrent() OVERRIDE;
	virtual void OnRegister()  OVERRIDE;
	virtual void OnUnregister()  OVERRIDE;
	virtual void DestroyRenderState_Concurrent() OVERRIDE;
	virtual void CreatePhysicsState() OVERRIDE;
	virtual void DestroyPhysicsState() OVERRIDE;
	virtual void OnActorEnableCollisionChanged() OVERRIDE;
public:
	virtual void RegisterComponentTickFunctions(bool bRegister) OVERRIDE;
#if WITH_EDITOR
	virtual void CheckForErrors() OVERRIDE;
#endif // WITH_EDITOR	
	// End UActorComponent Interface

protected:
	/** Internal function that updates physics objects to match the component collision settings. */
	virtual void UpdatePhysicsToRBChannels();

	/** Called to send a transform update for this component to the physics engine */
	void SendPhysicsTransform(bool bTeleport);
	/** Ensure physics state created **/
	void EnsurePhysicsStateCreated();
public:

	// Begin UObject interface.
	virtual void Serialize(FArchive& Ar) OVERRIDE;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) OVERRIDE;
	virtual bool CanEditChange(const UProperty* InProperty) const OVERRIDE;
	virtual void UpdateCollisionProfile();
#endif // WITH_EDITOR
	virtual void PostLoad() OVERRIDE;
	virtual void PostDuplicate(bool bDuplicateForPIE) OVERRIDE;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;

#if WITH_EDITOR
	/**
	 * Called after importing property values for this object (paste, duplicate or .t3d import)
	 * Allow the object to perform any cleanup for properties which shouldn't be duplicated or
	 * are unsupported by the script serialization
	 */
	virtual void PostEditImport();
#endif

	virtual void BeginDestroy() OVERRIDE;
	virtual void FinishDestroy() OVERRIDE;
	virtual bool IsReadyForFinishDestroy() OVERRIDE;
	virtual bool NeedsLoadForClient() const OVERRIDE;
	virtual bool NeedsLoadForServer() const OVERRIDE;
	// End UObject interface.

	//Begin USceneComponent Interface
	virtual bool MoveComponent( const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult* OutHit=NULL, EMoveComponentFlags MoveFlags = MOVECOMP_NoFlags ) OVERRIDE;
	virtual bool IsWorldGeometry() const OVERRIDE;
	virtual ECollisionEnabled::Type GetCollisionEnabled() const OVERRIDE;
	virtual ECollisionResponse GetCollisionResponseToChannel(ECollisionChannel Channel) const OVERRIDE;
	virtual ECollisionChannel GetCollisionObjectType() const OVERRIDE;
	virtual const FCollisionResponseContainer & GetCollisionResponseToChannels() const OVERRIDE;
	virtual FVector GetComponentVelocity() const OVERRIDE;
	//End USceneComponent Interface

	/**
	 * Set collision params on OutParams (such as CollisionResponse, bTraceAsyncScene) to match the settings on this PrimitiveComponent.
	 */
	virtual void InitSweepCollisionParams(FCollisionQueryParams &OutParams, FCollisionResponseParams& OutResponseParam) const;

	/**
	 * Return a CollisionShape that most closely matches this primitive.
	 */
	virtual struct FCollisionShape GetCollisionShape(float Inflation = 0.0f) const;

	/**
	 * Returns true if the given transforms result in the same bounds, due to rotational symmetry.
	 * For example, this is true for a sphere with uniform scale undergoing any rotation.
	 * This is NOT intended to detect every case where this is true, only the common cases to aid optimizations.
	 */
	virtual bool AreSymmetricRotations(const FQuat& A, const FQuat& B, const FVector& Scale3D) const { return A.Equals(B); }

	/**
	 * Pushes new selection state to the render thread primitive proxy
	 */
	void PushSelectionToProxy();

	/**
	 * Pushes new hover state to the render thread primitive proxy
	 * @param bInHovered - true if the proxy should display as if hovered
	 */
	void PushHoveredToProxy(const bool bInHovered);

	/** Sends editor visibility updates to the render thread */
	void PushEditorVisibilityToProxy( uint64 InVisibility );

	/** Gets the emissive boost for the primitive component. */
	virtual float GetEmissiveBoost(int32 ElementIndex) const		{ return 1.0f; };

	/** Gets the diffuse boost for the primitive component. */
	virtual float GetDiffuseBoost(int32 ElementIndex) const		{ return 1.0f; };
	
	virtual bool GetShadowIndirectOnly() const { return false; }

#if WITH_EDITOR
	/**
	 *	Setup the information required for rendering LightMap Density mode
	 *	for this component.
	 *
	 *	@param	Proxy		The scene proxy for the component (information is set on it)
	 *	@return	bool		true if successful, false if not.
	 */
	virtual bool SetupLightmapResolutionViewInfo(FPrimitiveSceneProxy& Proxy) const;
#endif // WITH_EDITOR

	/**
	 *	Set the angular velocity of all bodies in this component.
	 *
	 *	@param NewAngVel		New angular velocity to apply to physics, in degrees per second.
	 *	@param bAddToCurrent	If true, NewAngVel is added to the existing angular velocity of all bodies.
	 */
	virtual void SetAllPhysicsAngularVelocity(const FVector& NewAngVel, bool bAddToCurrent = false);

	/**
	 *	Set the position of all bodies in this component.
	 *	If a SkeletalMeshComponent, the root body will be placed at the desired position, and the same delta is applied to all other bodies.
	 *
	 *	@param	NewPos		New position for the body
	 */
	virtual void SetAllPhysicsPosition(FVector NewPos);
	
	/**
	 *	Set the rotation of all bodies in this component.
	 *	If a SkeletalMeshComponent, the root body will be changed to the desired orientation, and the same delta is applied to all other bodies.
	 *
	 *	@param NewRot	New orienatation for the body
	 */
	virtual void SetAllPhysicsRotation(FRotator NewRot);
	
	/**
	 *	Ensure simulation is running for all bodies in this component.
	 */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void WakeAllRigidBodies();
	
	/** Enables/disables whether this component is affected by gravity. This applies only to components with bSimulatePhysics set to true. */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void SetEnableGravity(bool bGravityEnabled);

	/** Returns whether this component is affected by gravity. Returns always false if the component is not simulated. */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual bool IsGravityEnabled() const;

	/** Sets the linear damping of this component. */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void SetLinearDamping(float InDamping);

	/** Returns the linear damping of this component. */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual float GetLinearDamping() const;

	/** Sets the angular damping of this component. */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual void SetAngularDamping(float InDamping);
	
	/** Returns the angular damping of this component. */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual float GetAngularDamping() const;

	/** Returns the mass of this component in kg. */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual float GetMass() const;
	
	/** Returns the calculated mass in kg. This is not 100% exactly the mass physx will calculate, but it is very close ( difference < 0.1kg ). */
	virtual float CalculateMass() const;

	/**
	 *	Force all bodies in this component to sleep.
	 */
	virtual void PutAllRigidBodiesToSleep();
	
	/**
	 *	Returns if a single body is currently awake and simulating.
	 *	@param	BoneName	If a SkeletalMeshComponent, name of body to return wakeful state from. 'None' indicates root body.
	 */
	bool RigidBodyIsAwake(FName BoneName = NAME_None);
	
	/**
	 *	Returns if any body in this component is currently awake and simulating.
	 */
	virtual bool IsAnyRigidBodyAwake();
	
	/**
	 *	Changes a member of the ResponseToChannels container for this PrimitiveComponent.
	 *
	 * @param       Channel      The channel to change the response of
	 * @param       NewResponse  What the new response should be to the supplied Channel
	 */
	UFUNCTION(BlueprintCallable, Category="Collision")
	void SetCollisionResponseToChannel(ECollisionChannel Channel, ECollisionResponse NewResponse);
	
	/**
	 *	Changes all ResponseToChannels container for this PrimitiveComponent. to be NewResponse
	 *
	 * @param       NewResponse  What the new response should be to the supplied Channel
	 */
	UFUNCTION(BlueprintCallable, Category="Collision")
	void SetCollisionResponseToAllChannels(ECollisionResponse NewResponse);
	
	/**
	 *	Changes the whole ResponseToChannels container for this PrimitiveComponent.
	 *
	 * @param       NewResponses  New set of responses for this component
	 */
	void SetCollisionResponseToChannels(const FCollisionResponseContainer& NewReponses);
	
private:
	/** Called when the BodyInstance ResponseToChannels, CollisionEnabled or bNotifyRigidBodyCollision changes, in case subclasses want to use that information. */
	virtual void OnComponentCollisionSettingsChanged();

	/**
	 *	Applies a RigidBodyState struct to this Actor.
	 *	When we get an update for the physics, we try to do it smoothly if it is less than ..DeltaThreshold.
	 *	We directly fix ..InterpAlpha * error. The rest is fixed by altering the velocity to correct the actor over 1.0/..RecipFixTime seconds.
	 *	So if ..InterpAlpha is 1, we will always just move the actor directly to its correct position (as it the error was over ..DeltaThreshold)
	 *	If ..InterpAlpha is 0, we will correct just by changing the velocity.
	 *
	 * Returns true if restored state is matching requested one (no velocity corrections required)
	 */
	bool ApplyRigidBodyState(const FRigidBodyState& NewState, const FRigidBodyErrorCorrection& ErrorCorrection, FVector& OutDeltaPos, FName BoneName = NAME_None);

public:

	/**
	 * Applies RigidBodyState only if it needs to be updated
	 * NeedsUpdate flag will be removed from UpdatedState after all velocity corrections are finished
	 */
	bool ConditionalApplyRigidBodyState(FRigidBodyState& UpdatedState, const FRigidBodyErrorCorrection& ErrorCorrection, FVector& OutDeltaPos, FName BoneName = NAME_None);

	/** 
	 *	Get the state of the rigid body responsible for this Actor's physics, and fill in the supplied FRigidBodyState struct based on it.
	 *
	 *	@return	true if we successfully found a physics-engine body and update the state structure from it.
	 */
	bool GetRigidBodyState(FRigidBodyState& OutState, FName BoneName = NAME_None);

	/** 
	 *	Changes the current PhysMaterialOverride for this component. 
	 *	Note that if physics is already running on this component, this will _not_ alter its mass/inertia etc,  
	 *	it will only change its surface properties like friction and the damping.
	 */
	virtual void SetPhysMaterialOverride(class UPhysicalMaterial* NewPhysMaterial);

	/** 
	 *  Looking at various values of the component, determines if this
	 *  component should be added to the scene
	 * @return true if the component is visible and should be added to the scene, false otherwise
	 */
	bool ShouldComponentAddToScene() const;
	
	/**
	 * Changes the value of CullDistance.
	 * @param NewCullDistance - The value to assign to CullDistance.
	 */
	void SetCullDistance(float NewCullDistance);
	
	/**
	 * Changes the value of DepthPriorityGroup.
	 * @param NewDepthPriorityGroup - The value to assign to DepthPriorityGroup.
	 */
	void SetDepthPriorityGroup(ESceneDepthPriorityGroup NewDepthPriorityGroup);
	
	/**
	 * Changes the value of bUseViewOwnerDepthPriorityGroup and ViewOwnerDepthPriorityGroup.
	 * @param bNewUseViewOwnerDepthPriorityGroup - The value to assign to bUseViewOwnerDepthPriorityGroup.
	 * @param NewViewOwnerDepthPriorityGroup - The value to assign to ViewOwnerDepthPriorityGroup.
	 */
	void SetViewOwnerDepthPriorityGroup(
		bool bNewUseViewOwnerDepthPriorityGroup,
		ESceneDepthPriorityGroup NewViewOwnerDepthPriorityGroup
		);
	
	/** 
	 *  Trace a ray against just this component.
	 *  @param  OutHit          Information about hit against this component, if true is returned
	 *  @param  Start           Start location of the ray
	 *  @param  End             End location of the ray
	 *  @param  Params          Additional parameters used for the trace
	 *  @return true if a hit is found
	 */
	virtual bool LineTraceComponent( FHitResult& OutHit, const FVector Start, const FVector End, const FCollisionQueryParams& Params );
	
	/** 
	 *  Trace a box against just this component.
	 *  @param  OutHit          Information about hit against this component, if true is returned
	 *  @param  Start           Start location of the box
	 *  @param  End             End location of the box
	 *  @param  BoxHalfExtent 	Half Extent of the box
	 *	@param	bTraceComplex	Whether or not to trace complex
	 *  @return true if a hit is found
	 */
	virtual bool SweepComponent(FHitResult& OutHit, const FVector Start, const FVector End, const FCollisionShape &CollisionShape, bool bTraceComplex=false);
	
	/** 
	 *  Test the collision of the supplied component at the supplied location/rotation, and determine if it overlaps this component
	 *  @param  PrimComp        Component to use geometry from to test against this component. Transform of this component is ignored.
	 *  @param  Pos             Location to place PrimComp geometry at 
	 *  @param  Rot             Rotation to place PrimComp geometry at 
	 *  @param	Params			Parameter for trace. TraceTag is only used.
	 *  @return true if PrimComp overlaps this component at the specified location/rotation
	 */
	virtual bool ComponentOverlapComponent(class UPrimitiveComponent* PrimComp, const FVector Pos, const FRotator Rot, const FCollisionQueryParams& Params);
	
	/** 
	 *  Test the collision of the supplied Sphere at the supplied location, and determine if it overlaps this component
	 *
	 *  @param  Pos             Location to place PrimComp geometry at 
	 *	@param	Rot				Rotation of PrimComp geometry
	 *  @param  CollisionShape 	Shape of collision of PrimComp geometry
	 *  @return true if PrimComp overlaps this component at the specified location/rotation
	 */
	virtual bool OverlapComponent(const FVector& Pos, const FQuat& Rot, const FCollisionShape& CollisionShape);
	
	/**
	 * Called from the Pawn's BaseChanged() event.
	 * @param APawn is the pawn that wants to be based on this actor
	 * @return true if we don't want the pawn to bounce off
	 */
	virtual bool CanBeBaseForCharacter(class APawn* APawn) const;

	/** 
	 *	Indicates whether this actor is to be considered by navigation system a valid actor
	 *	@param bDoChannelCheckOnly allow caller to check whether this component affects/would affect
	 *	navigation, regardless of its current BodyInstance collision value
	 *	@note this function will return "true" also if HasCustomNavigableGeometry == EHasCustomNavigableGeometry::EvenIfNonCollidable
	 */
	bool IsNavigationRelevant(bool bSkipCollisionEnabledCheck = false) const;

	/** Can this component potentially influence navigation */
	bool CanEverAffectNavigation() const 
	{
		return bCanEverAffectNavigation;
	}

	/** turn off navigation relevance, must be called before component is registered! */
	void DisableNavigationRelevance();

	FORCEINLINE EHasCustomNavigableGeometry::Type HasCustomNavigableGeometry() const { return bHasCustomNavigableGeometry; }

	/** Collects custom navigable geometry of component.
	 *	@return true if regular navigable geometry exporting should be run as well */
	virtual bool DoCustomNavigableGeometryExport(struct FNavigableGeometryExport* GeomExport) const { return true; }

	static void DispatchMouseOverEvents(UPrimitiveComponent* CurrentComponent, UPrimitiveComponent* NewComponent);
	static void DispatchTouchOverEvents(ETouchIndex::Type FingerIndex, UPrimitiveComponent* CurrentComponent, UPrimitiveComponent* NewComponent);
	void DispatchOnClicked();
	void DispatchOnReleased();
	void DispatchOnInputTouchBegin(const ETouchIndex::Type Key);
	void DispatchOnInputTouchEnd(const ETouchIndex::Type Key);
};

