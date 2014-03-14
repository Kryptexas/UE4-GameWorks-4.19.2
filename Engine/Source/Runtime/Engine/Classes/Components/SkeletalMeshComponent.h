// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "SkeletalMeshComponent.generated.h"

namespace physx{ 

	class PxPlane;

	namespace apex {
	class NxClothingAsset;
	class NxClothingActor;

  //for clothing collision
	class NxClothingCollision;
	class NxClothingPlane;
	class NxClothingSphere;
	class NxClothingConvex;
	class NxClothingCollision;
	class NxClothingCapsule;
}}

class FPhysScene;

/** thie is a class to manage an APEX clothing actor */
class FClothingActor
{
public:
	void Clear(bool bReleaseResource = false);

	/** 
	 * to check whether this actor is valid or not 
	 * because clothing asset can be changed by editing 
	 */
	TSharedPtr<class FClothingAssetWrapper>	ParentClothingAsset;
	/** APEX clothing actor is created from APEX clothing asset for cloth simulation */
	physx::apex::NxClothingActor*		ApexClothingActor;
	FPhysScene * PhysScene;
	uint32 SceneType;
};

/** data for updating cloth section from the results of clothing simulation */
struct FClothSimulData
{
	TArray<FVector4> ClothSimulPositions;
	TArray<FVector4> ClothSimulNormals;
};

#if WITH_CLOTH_COLLISION_DETECTION

class FClothCollisionPrimitive
{
public:
	enum ClothCollisionPrimType
	{
		SPHERE,
		CAPSULE,
		CONVEX,
		PLANE,		
	};

public:
	// for sphere and convex ( also used in debug draw of capsule )
	FVector Origin;
	// for sphere and capsule
	float	Radius;
	// for capsule ( needs 2 spheres to make an capsule, top end and bottom end )
	FVector SpherePos1;
	FVector SpherePos2;
	// for convex
	TArray<FPlane> ConvexPlanes;

	ClothCollisionPrimType	PrimType;
};

/** to interact with environments */
struct FApexClothCollisionInfo
{
	enum CollisionInfoState
	{
		CIS_INVALID,
		CIS_ADDED,
		CIS_RETAINED
	};

	uint32 Revision;
	// ClothingCollisions will be all released when clothing doesn't intersect with this component anymore
	TArray<physx::apex::NxClothingCollision*> ClothingCollisions;			
};
#endif // #if WITH_CLOTH_COLLISION_DETECTION

/** This enum defines how you'd like to update bones to physics world
	If bone is simulating, you don't have to waste time on updating bone transform from kinematic
	But also sometimes you don't like fixed bones to be updated by animation data **/
UENUM()
namespace EKinematicBonesUpdateToPhysics
{
	enum Type
	{
		/** This only updates anything non-simulating bones - i.e. (fixed) or (default if owner is not simulating)**/
		SkipSimulatingBones,
		/** This updates everything except (fixed) or (unfixed) or (default if owner is simulating)**/
		SkipFixedAndSimulatingBones, 
		/** Skip any physics update from kinematic changes **/
		SkipAllBones
	};
}

UENUM()
namespace EAnimationMode
{
	enum Type
	{
		AnimationBlueprint UMETA(DisplayName="Use Animation Blueprint"), 
		AnimationSingleNode UMETA(DisplayName="Use Animation Asset")
	};
}

USTRUCT()
struct FSingleAnimationPlayData
{
	GENERATED_USTRUCT_BODY()

	// @todo in the future, we should make this one UObject
	// and have detail customization to display different things
	// The default sequence to play on this skeletal mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Animation)
	class UAnimationAsset* AnimToPlay;

	// @fixme : until we properly support it I'm commenting out editable property part
	// The default sequence to play on this skeletal mesh
	UPROPERTY()//EditAnywhere, BlueprintReadWrite, Category=Animation)
	class UVertexAnimation* VertexAnimToPlay;

	// Default setting for looping for SequenceToPlay. This is not current state of looping.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Animation, meta=(FriendlyName="Looping"))
	uint32 bSavedLooping:1;

	// Default setting for playing for SequenceToPlay. This is not current state of playing.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Animation, meta=(FriendlyName="Playing"))
	uint32 bSavedPlaying:1;

	// Default setting for position of SequenceToPlay to play. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Animation, meta=(FriendlyName="InitialPosition"))
	float SavedPosition;

	// Default setting for playrate of SequenceToPlay to play. 
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=Animation, meta=(FriendlyName="PlayRate"))
	float SavedPlayRate;

	FSingleAnimationPlayData()
	{
		SavedPlayRate = 1.0f;
		bSavedLooping = true;
		bSavedPlaying = true;
		SavedPosition = 0.f;
	}

	/** Called when initialized **/
	ENGINE_API void Initialize(class UAnimSingleNodeInstance* Instance);

	/** Populates this play data with the current state of the supplied instance */
	ENGINE_API void PopulateFrom(UAnimSingleNodeInstance* Instance);

	void ValidatePosition();
};

/**
 * SkeletalMeshComponent is a mesh that supports skeletal animation and physics.
 */
UCLASS(HeaderGroup=Component, ClassGroup=(Rendering, Common), hidecategories=Object, config=Engine, editinlinenew, meta=(BlueprintSpawnableComponent))
class ENGINE_API USkeletalMeshComponent : public USkinnedMeshComponent
{
	GENERATED_UCLASS_BODY()
	
	/**
	 * Animation 
	 */
	
	/** @Todo anim: Matinee related data start - this needs to be replaced to new system **/
	
	/** @Todo anim: Matinee related data end - this needs to be replaced to new system **/

protected:
	/** Whether to use Animation Blueprint or play Single Animation Asset*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
	TEnumAsByte<EAnimationMode::Type>	AnimationMode;

public:
#if WITH_EDITORONLY_DATA
	/**
	 * The blueprint for creating an AnimationScript
	 */
	UPROPERTY()
	class UAnimBlueprint* AnimationBlueprint_DEPRECATED;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Animation)
	class UAnimBlueprintGeneratedClass* AnimBlueprintGeneratedClass;

	/** The active animation graph program instance */
	UPROPERTY(transient)
	class UAnimInstance* AnimScriptInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Animation, meta=(ShowOnlyInnerProperties))
	struct FSingleAnimationPlayData AnimationData;

	/** Temporary array of local-space (ie relative to parent bone) rotation/translation for each bone. */
	TArray<FTransform> LocalAtoms;
	
	// Update Rate

	/** Cached LocalAtoms for Update Rate optimization. */
	UPROPERTY(Transient)
	TArray<FTransform> CachedLocalAtoms;

	/** Cached SpaceBases for Update Rate optimization. */
	UPROPERTY(Transient)
	TArray<FTransform> CachedSpaceBases;

	/** Used to scale speed of all animations on this skeletal mesh. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=Animation)
	float GlobalAnimRateScale;

	/**
	 * Only instance Root Bone rigid body for physics. Mostly used by Vehicles.
	 * Other Rigid Bodies are ignored for physics, but still considered for traces.
	 */
	UPROPERTY()
	uint32 bUseSingleBodyPhysics:1;

	/** If true, there is at least one body in the current PhysicsAsset with a valid bone in the current SkeletalMesh */
	UPROPERTY(transient)
	uint32 bHasValidBodies:1;

	/** If we are running physics, should we update non-simulated bones based on the animation bone positions. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=SkeletalMesh)
	TEnumAsByte<EKinematicBonesUpdateToPhysics::Type> KinematicBonesUpdateType;

	/** Enables blending in of physics bodies whether Simulate or not*/
	UPROPERTY(transient)
	uint32 bBlendPhysics:1;

	/** temporary variable to enable/disable skeletalmeshcomponent physics SIM/blending in dedicated server 
	 * @todo this should move to more global option **/
	UPROPERTY()
	uint32 bEnablePhysicsOnDedicatedServer:1;

	/**
	 *	If we should pass joint position to joints each frame, so that they can be used by motorized joints to drive the
	 *	ragdoll based on the animation.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=SkeletalMesh)
	uint32 bUpdateJointsFromAnimation:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Clothing)
	uint32	bFreezeClothSection:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Clothing)
	uint32	bAutomaticLodCloth:1;
	/** Disable cloth simulation and play original animation without simulation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Clothing)
	uint32 bDisableClothSimulation:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Clothing)
	uint32 bCollideWithEnvironment:1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Clothing)
	uint32 bCollideWithAttachedChildren:1;

	/** Draw the APEX Clothing Normals on clothing sections. */
	uint32 bDisplayClothingNormals:1;

	/** Draw Computed Normal Vectors in Tangent space for clothing section */
	uint32 bDisplayClothingTangents:1;

	/** Draw Collision Volumes from apex clothing asset */
	uint32 bDisplayClothingCollisionVolumes:1;

	/** Draw only clothing sections, disables non-clothing sections */
	uint32 bDisplayOnlyClothSections:1;
	/**
	 * Vertex Animation
	 */
	
	/** Offset of the root bone from the reference pose. Used to offset bounding box. */
	UPROPERTY(transient)
	FVector RootBoneTranslation;

	/** 
	 * Optimization
	 */
	
	/** Skips Ticking and Bone Refresh. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=SkeletalMesh)
	uint32 bNoSkeletonUpdate:1;

	/** If true, tick anim nodes even when our Owner has not been rendered recently  */
	UPROPERTY()
	uint32 bTickAnimationWhenNotRendered_DEPRECATED:1;

	/** pauses this component's animations (doesn't tick them, but still refreshes bones) */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=Animation)
	uint32 bPauseAnims:1;

	/**
	 * Misc 
	 */
	
	/** If true, force the mesh into the reference pose - is an optimization. */
	UPROPERTY()
	uint32 bForceRefpose:1;

	/** If bForceRefPose was set last tick. */
	UPROPERTY()
	uint32 bOldForceRefPose:1;

	/** Bool that enables debug drawing of the skeleton before it is passed to the physics. Useful for debugging animation-driven physics. */
	UPROPERTY()
	uint32 bShowPrePhysBones:1;

	/** If false, indicates that on the next call to UpdateSkelPose the RequiredBones array should be recalculated. */
	UPROPERTY(transient)
	uint32 bRequiredBonesUpToDate:1;

	/** If true, AnimTree has been initialised. */
	UPROPERTY(transient)
	uint32 bAnimTreeInitialised:1;

	/** If true, line checks will test against the bounding box of this skeletal mesh component and return a hit if there is a collision. */
	UPROPERTY()
	uint32 bEnableLineCheckWithBounds:1;

	/** If bEnableLineCheckWithBounds is true, scale the bounds by this value before doing line check. */
	UPROPERTY()
	FVector LineCheckBoundsScale;

	/** Notification when constraint is broken. */
	UPROPERTY(BlueprintAssignable)
	FConstraintBrokenSignature OnConstraintBroken;

	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetAnimClass(class UClass* NewClass);

	/** 
	 * Returns the animation instance that is driving the class (if available). This is typically an instance of
	 * the class set as AnimBlueprintGeneratedClass (generated by an animation blueprint)
	 */
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh", meta=(Keywords = "AnimBlueprint"))
	class UAnimInstance * GetAnimInstance() const;

	/** Below are the interface to control animation when animation mode, not blueprint mode **/
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetAnimationMode(EAnimationMode::Type InAnimationMode);
	
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	EAnimationMode::Type GetAnimationMode() const;

	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void PlayAnimation(class UAnimationAsset* NewAnimToPlay, bool bLooping);

	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetAnimation(class UAnimationAsset* NewAnimToPlay);

	// @todo block this until we support vertex animation 
//	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetVertexAnimation(class UVertexAnimation* NewVertexAnimToPlay);

	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void Play(bool bLooping);

	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void Stop();

	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	bool IsPlaying() const;

	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetPosition(float InPos);

	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	float GetPosition() const;

	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetPlayRate(float Rate);

	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	float GetPlayRate() const;

	/**
	 * Set Morph Target with Name and Value(0-1)
	 */
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetMorphTarget(FName MorphTargetName, float Value);

	/**
	 * Clear all Morph Target that are set to this mesh
	 */
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void ClearMorphTargets();

	/**
	 * Get Morph target with given name
	 */
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	float GetMorphTarget(FName MorphTargetName) const;

	/**
	 * Get/Set the max distance scale of clothing mesh vertices
	 */
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	float GetClothMaxDistanceScale();
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetClothMaxDistanceScale(float Scale);

	/** We detach the Component once we are done playing it.
	 *
	 * @param	ParticleSystemComponent that finished
	 */
	virtual void SkelMeshCompOnParticleSystemFinished( class UParticleSystemComponent* PSC );

	class UAnimSingleNodeInstance * GetSingleNodeInstance() const;
	void InitializeAnimScriptInstance(bool bForceReinit=true);

	/** @return true if wind is enabled */
	virtual bool IsWindEnabled() const;

public:
	/** Temporary array of bone indices required this frame. Filled in by UpdateSkelPose. */
	TArray<FBoneIndexType> RequiredBones;

	/** 
	 *	Index of the 'Root Body', or top body in the asset hierarchy. 
	 *	Filled in by InitInstance, so we don't need to save it.
	 */
	int32 RootBodyIndex;

	/** Array of FBodyInstance objects, storing per-instance state about about each body. */
	TArray<struct FBodyInstance*> Bodies;

	/** Array of FConstraintInstance structs, storing per-instance state about each constraint. */
	TArray<struct FConstraintInstance*> Constraints;

#if WITH_PHYSX
	/** Physics-engine representation of PxAggregate which contains a physics asset instance with more than numbers of bodies. */
	class physx::PxAggregate* Aggregate;

#endif	//WITH_PHYSX

#if WITH_APEX_CLOTHING
	/** 
	* clothing actors will be created from clothing assets for cloth simulation 
	* 1 actor should correspond to 1 asset
	*/
	TArray<FClothingActor> ClothingActors;

	float ClothMaxDistanceScale;

#if WITH_CLOTH_COLLISION_DETECTION
	/** increase every tick to update clothing collision  */
	uint32 ClothingCollisionRevision; 

	TArray<physx::apex::NxClothingCollision*>	ParentCollisions;
	TArray<physx::apex::NxClothingCollision*>	EnvironmentCollisions;

	TMap<TWeakObjectPtr<UPrimitiveComponent>, FApexClothCollisionInfo> ClothOverlappedComponentsMap;
#endif // WITH_CLOTH_COLLISION_DETECTION

#endif // WITH_APEX_CLOTHING

	/** 
	 * Morph Target Curves. This will override AnimInstance MorphTargetCurves
	 * if same curve is found
	 **/
	TMap<FName, float>	MorphTargetCurves;

	// 
	// Animation
	//
	virtual void InitAnim(bool bForceReinit);

	/** Tick Animation system */
	void TickAnimation(float DeltaTime);

	/** Tick Clothing Animation , basically this is called inside TickComponent */
	void TickClothing();

	/** Store cloth simulation data into OutClothSimData */
	void GetUpdateClothSimulationData(TArray<FClothSimulData>& OutClothSimData);
	void ApplyWindForCloth(FClothingActor& ClothingActor);
	void RemoveAllClothingActors();

	bool IsValidClothingActor(int32 ActorIndex) const;
	/** Draws APEX Clothing simulated normals on cloth meshes **/
	void DrawClothingNormals(FPrimitiveDrawInterface* PDI) const;
	/** Draws APEX Clothing Graphical Tangents on cloth meshes **/
	void DrawClothingTangents(FPrimitiveDrawInterface* PDI) const;
	/** Draws internal collision volumes which the character has, colliding with cloth **/
	void DrawClothingCollisionVolumes(FPrimitiveDrawInterface* PDI);

	/** freezing clothing actor now */
	void FreezeClothSection(bool bFreeze);

	/** Evaluate Anim System **/
	void EvaluateAnimation(/*FTransform & ExtractedRootMotionDelta, int32 & bHasRootMotion*/);

	/**
	 * Take the LocalAtoms array (translation vector, rotation quaternion and scale vector) and update the array of component-space bone transformation matrices (SpaceBases).
	 * It will work down hierarchy multiplying the component-space transform of the parent by the relative transform of the child.
	 * This code also applies any per-bone rotators etc. as part of the composition process
	 */
	void FillSpaceBases();

	/** 
	 * Recalculates the RequiredBones array in this SkeletalMeshComponent based on current SkeletalMesh, LOD and PhysicsAsset.
	 * Is called when bRequiredBonesUpToDate = false
	 *
	 * @param	LODIndex	Index of LOD [0-(MaxLOD-1)]
	 */
	void RecalcRequiredBones(int32 LODIndex);

public:
	// Begin UObject interface.
	virtual void Serialize(FArchive& Ar) OVERRIDE;
#if WITH_EDITOR
	DECLARE_MULTICAST_DELEGATE(FOnSkeletalMeshPropertyChangedMulticaster)
	FOnSkeletalMeshPropertyChangedMulticaster OnSkeletalMeshPropertyChanged;
	typedef FOnSkeletalMeshPropertyChangedMulticaster::FDelegate FOnSkeletalMeshPropertyChanged;

	/** Register / Unregister delegates called when the skeletal mesh property is changed */
	void RegisterOnSkeletalMeshPropertyChanged(const FOnSkeletalMeshPropertyChanged& Delegate);
	void UnregisterOnSkeletalMeshPropertyChanged(const FOnSkeletalMeshPropertyChanged& Delegate);

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;

	/** Validates the animation asset or blueprint, making sure it is compatible with the current skeleton */
	void ValidateAnimation();

	virtual void LoadedFromAnotherClass(const FName& OldClassName) OVERRIDE;
	virtual void UpdateCollisionProfile() OVERRIDE;
#endif // WITH_EDITOR
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	// End UObject interface.

	// Begin UActorComponent interface.
	virtual void OnRegister() OVERRIDE;
	virtual void OnUnregister() OVERRIDE;
	virtual void CreateRenderState_Concurrent() OVERRIDE;
	virtual void CreatePhysicsState() OVERRIDE;
	virtual void DestroyPhysicsState() OVERRIDE;
	virtual void InitializeComponent() OVERRIDE;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	// End UActorComponent interface.

	// Begin USceneComponent interface.
	virtual void UpdateBounds();
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	virtual bool IsAnySimulatingPhysics() const OVERRIDE;
	virtual void OnUpdateTransform(bool bSkipPhysicsMove) OVERRIDE;
	virtual void UpdateOverlaps(TArray<FOverlapInfo> const* PendingOverlaps=NULL, bool bDoNotifies=true, const TArray<FOverlapInfo>* OverlapsAtEndLocation=NULL) OVERRIDE;
	
	/**
	 *  Test the collision of the supplied component at the supplied location/rotation, and determine the set of components that it overlaps
	 *  @param  OutOverlaps     Array of overlaps found between this component in specified pose and the world
	 *  @param  World			World to use for overlap test
	 *  @param  Pos             Location to place the component's geometry at to test against the world
	 *  @param  Rot             Rotation to place components' geometry at to test against the world
	 *	@param	ObjectQueryParams	List of object types it's looking for. When this enters, we do object query with component shape
	 *  @return TRUE if OutOverlaps contains any blocking results
	 */
	virtual bool ComponentOverlapMulti(TArray<struct FOverlapResult>& OutOverlaps, const class UWorld* World, const FVector& Pos, const FRotator& Rot, const struct FComponentQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams=FCollisionObjectQueryParams::DefaultObjectQueryParam) const OVERRIDE;
	// End USceneComponent interface.

	// Begin UPrimitiveComponent interface.
	virtual void PostPhysicsTick(FPrimitiveComponentPostPhysicsTickFunction &ThisTickFunction) OVERRIDE;
	virtual class UBodySetup* GetBodySetup() OVERRIDE;
	virtual FBodyInstance* GetBodyInstance(FName BoneName = NAME_None) const OVERRIDE;
	virtual void UpdatePhysicsToRBChannels() OVERRIDE;
	virtual void SetAllPhysicsAngularVelocity(FVector const& NewVel, bool bAddToCurrent = false) OVERRIDE;
	virtual void SetAllPhysicsPosition(FVector NewPos) OVERRIDE;
	virtual void SetAllPhysicsRotation(FRotator NewRot) OVERRIDE;
	virtual void WakeAllRigidBodies() OVERRIDE;
	virtual void PutAllRigidBodiesToSleep() OVERRIDE;
	virtual bool IsAnyRigidBodyAwake() OVERRIDE;
	virtual void OnComponentCollisionSettingsChanged() OVERRIDE;
	virtual void SetPhysMaterialOverride(UPhysicalMaterial* NewPhysMaterial) OVERRIDE;
	virtual bool LineTraceComponent( FHitResult& OutHit, const FVector Start, const FVector End, const FCollisionQueryParams& Params ) OVERRIDE;
	virtual bool SweepComponent( FHitResult& OutHit, const FVector Start, const FVector End, const FCollisionShape& CollisionShape, bool bTraceComplex=false) OVERRIDE;
	virtual bool ShouldTrackOverlaps() const OVERRIDE;
	virtual bool ComponentOverlapComponent(class UPrimitiveComponent* PrimComp, const FVector Pos, const FRotator FRotator, const FCollisionQueryParams& Params) OVERRIDE;
	virtual bool OverlapComponent(const FVector& Pos, const FQuat& Rot, const FCollisionShape& CollisionShape) OVERRIDE;
	virtual void SetSimulatePhysics(bool bEnabled) OVERRIDE;
	virtual void AddRadialImpulse(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange=false) OVERRIDE;
	virtual void AddRadialForce(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff) OVERRIDE;
	virtual void SetAllPhysicsLinearVelocity(FVector NewVel,bool bAddToCurrent = false) OVERRIDE;
	virtual float GetMass() const OVERRIDE;
	virtual float CalculateMass() const OVERRIDE;
	// End UPrimitiveComponent interface.

	// Begin USkinnedMeshComponent interface
	virtual bool UpdateLODStatus() OVERRIDE;
	virtual void RefreshBoneTransforms() OVERRIDE;
	virtual void TickPose( float DeltaTime ) OVERRIDE;
	virtual void UpdateSlaveComponent() OVERRIDE;
	virtual bool ShouldUpdateTransform(bool bLODHasChanged) const OVERRIDE;
	virtual bool ShouldTickPose() const OVERRIDE;
	virtual bool AllocateTransformData() OVERRIDE;
	virtual void DeallocateTransformData() OVERRIDE;
	virtual void HideBone( int32 BoneIndex, EPhysBodyOp PhysBodyOption ) OVERRIDE;
	virtual void UnHideBone( int32 BoneIndex ) OVERRIDE;
	virtual void SetPhysicsAsset(class UPhysicsAsset* NewPhysicsAsset,bool bForceReInit = false) OVERRIDE;
	virtual void SetSkeletalMesh(class USkeletalMesh* NewMesh) OVERRIDE;
	// End USkinnedMeshComponent interface
	/** 
	 *	Iterate over each joint in the physics for this mesh, setting its AngularPositionTarget based on the animation information.
	 */
	void UpdateRBJointMotors();


	/**
	 * Blend of Physics Bones with PhysicsWeight and Animated Bones with (1-PhysicsWeight)
	 *
	 * @param	RequiredBones	List of bones to be blend
	 */
	void BlendPhysicsBones( TArray<FBoneIndexType>& RequiredBones );

	/** Take the results of the physics and blend them with the animation state (based on the PhysicsWeight parameter), and update the SpaceBases array. */
	void BlendInPhysics();	

	/** 
	 * Intialize PhysicsAssetInstance for the physicsAsset 
	 * 
	 * @param	PhysScene	Physics Scene
	 */
	void InitArticulated(FPhysScene* PhysScene);

	/** Turn off all physics and remove the instance. */
	void TermArticulated();


	/** Terminate physics on all bodies below the named bone */
	void TermBodiesBelow(FName ParentBoneName);

	/** Find instance of the constraint that matches the name supplied. */
	FConstraintInstance* FindConstraintInstance(FName ConName);

	/** Utility which returns total mass of all bones below the supplied one in the hierarchy (including this one). */
	float GetTotalMassBelowBone(FName InBoneName);

	/** Set the movement channel of all bodies */
	void SetAllBodiesCollisionObjectType(ECollisionChannel NewChannel);

	/** Set the rigid body notification state for all bodies. */
	void SetAllBodiesNotifyRigidBodyCollision(bool bNewNotifyRigidBodyCollision);

	/** Set bSimulatePhysics to true for all bone bodies. Does not change the component bSimulatePhysics flag. */
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetAllBodiesSimulatePhysics(bool bNewSimulate);

	/** This is global set up for setting physics blend weight
	 * This does multiple things automatically
	 * If PhysicsBlendWeight == 1.f, it will enable Simulation, and if PhysicsBlendWeight == 0.f, it will disable Simulation. 
	 * Also it will respect each body's setup, so if the body is fixed, it won't simulate. Vice versa
	 * So if you'd like all bodies to change manually, do not use this function, but SetAllBodiesPhysicsBlendWeight
	 */
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetPhysicsBlendWeight(float PhysicsBlendWeight);

	/** Disable physics blending of bones **/
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetEnablePhysicsBlending(bool bNewBlendPhysics);

	/** Set all of the bones below passed in bone to be simulated */
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetAllBodiesBelowSimulatePhysics(const FName& InBoneName, bool bNewSimulate );

	/** Allows you to reset bodies Simulate state based on where bUsePhysics is set to true in the BodySetup. */
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void ResetAllBodiesSimulatePhysics();

	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetAllBodiesPhysicsBlendWeight(float PhysicsBlendWeight, bool bSkipCustomPhysicsType = false );

	/** Set all of the bones below passed in bone to be simulated */
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void SetAllBodiesBelowPhysicsBlendWeight(const FName& InBoneName, float PhysicsBlendWeight, bool bSkipCustomPhysicsType = false );

	/** Accumulate AddPhysicsBlendWeight to physics blendweight for all of the bones below passed in bone to be simulated */
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	void AccumulateAllBodiesBelowPhysicsBlendWeight(const FName& InBoneName, float AddPhysicsBlendWeight, bool bSkipCustomPhysicsType = false );

	/** Enable or Disable AngularPositionDrive */
	void SetAllMotorsAngularPositionDrive(bool bEnableSwingDrive, bool bEnableTwistDrive, bool bSkipCustomPhysicsType = false);

	/** Enable or Disable AngularVelocityDrive based on a list of bone names */
	void SetAllMotorsAngularVelocityDrive(bool bEnableSwingDrive, bool bEnableTwistDrive, bool bSkipCustomPhysicsType = false);

	/** Set Angular Drive motors params for all constraint instance */
	void SetAllMotorsAngularDriveParams(float InSpring, float InDamping, float InForceLimit, bool bSkipCustomPhysicsType = false);

	/** Enable or Disable AngularPositionDrive based on a list of bone names */
	void SetNamedMotorsAngularPositionDrive(bool bEnableSwingDrive, bool bEnableTwistDrive, const TArray<FName>& BoneNames, bool bSetOtherBodiesToComplement = false);

	/** Enable or Disable AngularVelocityDrive based on a list of bone names */
	void SetNamedMotorsAngularVelocityDrive(bool bEnableSwingDrive, bool bEnableTwistDrive, const TArray<FName>& BoneNames, bool bSetOtherBodiesToComplement = false);


	/**
	 * Return Transform Matrix for SkeletalMeshComponent considering root motion setups
	 * 
	 * @return Matrix Transform matrix
	 */
	FMatrix GetTransformMatrix() const;
	
	/** 
	 * Change whether to force mesh into ref pose (and use cheaper vertex shader) 
	 *
	 * @param	bNewForceRefPose	true if it would like to force ref pose
	 */
	void SetForceRefPose(bool bNewForceRefPose);
	
	/** Update bHasValidBodies flag */
	void UpdateHasValidBodies();
	
	/** 
	 * Initialize SkelControls
	 */
	void InitSkelControls();
	
	/**
	 * Find Constraint Index from the name
	 * 
	 * @param	ConstraintName	Joint Name of constraint to look for
	 * @return	Constraint Index
	 */
	int32	FindConstraintIndex(FName ConstraintName);
	
	/**
	 * Find Constraint Name from index
	 * 
	 * @param	ConstraintIndex	Index of constraint to look for
	 * @return	Constraint Joint Name
	 */
	FName	FindConstraintBoneName(int32 ConstraintIndex);

	/** 
	 *	Iterate over each physics body in the physics for this mesh, and for each 'kinematic' (ie fixed or default if owner isn't simulating) one, update its
	 *	transform based on the animated transform.
	 */
	void UpdateKinematicBonesToPhysics(bool bTeleport);
	
	/**
	 * Look up all bodies for broken constraints.
	 * Makes sure child bodies of a broken constraints are not fixed and using bone springs, and child joints not motorized.
	 */
	void UpdateMeshForBrokenConstraints();
	
	/**
	 * Notifier when look at control goes beyond of limit - candidate for delegate
	 */
	virtual void NotifySkelControlBeyondLimit(class USkelControlLookAt* LookAt);

	/** 
	 * Break a constraint off a Gore mesh. 
	 * 
	 * @param	Impulse	vector of impulse
	 * @param	HitLocation	location of the hit
	 * @param	InBoneName	Name of bone to break constraint for
	 */
	void BreakConstraint(FVector Impulse, FVector HitLocation, FName InBoneName);
	
	/** iterates through all bodies in our PhysicsAsset and returns the location of the closest bone associated
	 * with a body that has collision enabled.
	 * @param TestLocation - location to check against
	 * @return location of closest colliding rigidbody, or TestLocation if there were no bodies to test
	 */
	FVector GetClosestCollidingRigidBodyLocation(const FVector& TestLocation) const;

#if WITH_APEX_CLOTHING
	/** 
	* APEX clothing actor is created from APEX clothing asset for cloth simulation 
	* create only if became invalid
	*/
	bool CreateClothingActor(int32 AssetIndex, TSharedPtr<FClothingAssetWrapper> ClothingAsset);
	/** should call this method if occurred any changes in clothing assets */
	void ValidateClothingActors();
	/** add bounding box for cloth */
	void AddClothingBounds(FBoxSphereBounds& InOutBounds) const;
	/** changes clothing LODs, if clothing LOD is disabled or LODIndex is greater than apex clothing LODs, simulation will be disabled */
	void SetClothingLOD(int32 LODIndex);
	/** 
	 * Updates all clothing animation states including ComponentToWorld-related states.
	 */
	void UpdateClothState();
	/** 
	 * Updates clothing actor's global pose.
	 * So should be called when ComponentToWorld is changed.
	 */
	void UpdateClothTransform();

	/** only check whether there are valid clothing actors or not */
	bool HasValidClothingActors();

#if WITH_CLOTH_COLLISION_DETECTION

	/** draws currently intersected collisions */
	void DrawDebugClothCollisions();
	/** draws a convex from planes for debug info */
	void DrawDebugConvexFromPlanes(FClothCollisionPrimitive& CollisionPrimitive, FColor& Color, bool bDrawWithPlanes=true);
	void ReleaseClothingCollision(physx::apex::NxClothingCollision* Collision);
	/** create new collisions when newly added  */
	FApexClothCollisionInfo* CreateNewClothingCollsions(UPrimitiveComponent* PrimitiveComponent);


	void RemoveAllOverappedComponentMap();

	void ReleaseAllParentCollisions();

	void ProcessClothCollisionWithEnvironment();

	void CopyCollisionsToChildren();

	/**
  	 * Get collision data only for collision with clothes.
	 * Returns false when failed to get cloth collision data
	*/
	bool GetClothCollisionData(UPrimitiveComponent* PrimComp, TArray<FClothCollisionPrimitive>& ClothCollisionPrimitives);

#endif // WITH_CLOTH_COLLISION_DETECTION


#endif// #if WITH_APEX_CLOTHING
	bool IsAnimBlueprintInstanced() const;

	/** Debug render skeletons bones to supplied canvas */
	void DebugDrawBones(class UCanvas* Canvas, bool bSimpleBones) const;

protected:
	bool NeedToSpawnAnimScriptInstance(bool bForceInit) const;
	
private:
	void RenderAxisGizmo(const FTransform& Transform, class UCanvas* Canvas) const;

	bool ShouldBlendPhysicsBones();	
	void ClearAnimScriptInstance();

	friend class FSkeletalMeshComponentDetails;

private:
	// these are deprecated variables from removing SingleAnimSkeletalComponent
	// remove if this version goes away : VER_UE4_REMOVE_SINGLENODEINSTANCE
	// deprecated variable to be re-save
	UPROPERTY()
	class UAnimSequence* SequenceToPlay_DEPRECATED;

	// The default sequence to play on this skeletal mesh
	UPROPERTY()
	class UAnimationAsset* AnimToPlay_DEPRECATED;

	// Default setting for looping for SequenceToPlay. This is not current state of looping.
	UPROPERTY()
	uint32 bDefaultLooping_DEPRECATED:1;

	// Default setting for playing for SequenceToPlay. This is not current state of playing.
	UPROPERTY()
	uint32 bDefaultPlaying_DEPRECATED:1;

	// Default setting for position of SequenceToPlay to play. 
	UPROPERTY()
	float DefaultPosition_DEPRECATED;

	// Default setting for playrate of SequenceToPlay to play. 
	UPROPERTY()
	float DefaultPlayRate_DEPRECATED;

public:
	/** Keep track of when animation haa been ticked to ensure it is ticked only once per frame. */
	UPROPERTY(Transient)
	float LastTickTime;

	/** Take extracted RootMotion and convert it from local space to world space. */
	FTransform ConvertLocalRootMotionToWorld(const FTransform & InTransform);
};



