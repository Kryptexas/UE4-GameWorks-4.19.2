// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "RHIDefinitions.h"
#include "ComponentInstanceDataCache.h" // for FActorComponentInstanceData
#include "SceneComponent.generated.h"

/** Overlap info consisting of the primitive and the body that is overlapping */
USTRUCT()
struct ENGINE_API FOverlapInfo
{
	GENERATED_USTRUCT_BODY()

	FOverlapInfo()
	{}

	FOverlapInfo(const FHitResult& InSweepResult)
		: bFromSweep(true), OverlapInfo(InSweepResult)
	{
	}

	FOverlapInfo(class UPrimitiveComponent* InComponent, int32 InBodyIndex = INDEX_NONE);
	
	int32 GetBodyIndex() const { return OverlapInfo.Item;  }

	//This function completely ignores SweepResult information. It seems that places that use this function do not care, but it still seems risky
	friend bool operator == (const FOverlapInfo& LHS, const FOverlapInfo& RHS) { return LHS.OverlapInfo.Component == RHS.OverlapInfo.Component && LHS.OverlapInfo.Item == RHS.OverlapInfo.Item; }
	bool bFromSweep;

	/** Information for both sweep and overlap queries. Different parts are valid depending on bFromSweep.
	  * If bFromSweep is true then FHitResult is completely valid just like a regular sweep result.
	  * If bFromSweep is false only FHitResult::Component, FHitResult::Actor, FHitResult::Item are valid as this is really just an FOverlapResult*/
	FHitResult OverlapInfo;
};

/** Detail mode for scene component rendering. */
UENUM()
enum EDetailMode
{
	DM_Low UMETA(DisplayName="Low"),
	DM_Medium UMETA(DisplayName="Medium"),
	DM_High UMETA(DisplayName="High"),
	DM_MAX,
};

/** The space for the transform */
UENUM()
enum ERelativeTransformSpace
{
	/** World space transform. */
	RTS_World,
	/** Actor space transform. */
	RTS_Actor,
	/** Component space transform. */
	RTS_Component,
};

//
// MoveComponent options.
//
enum EMoveComponentFlags
{
	// Bitflags.
	MOVECOMP_NoFlags					= 0x0000,	// no flags
	MOVECOMP_IgnoreBases				= 0x0001,	// ignore collisions with things the Actor is based on
	MOVECOMP_SkipPhysicsMove			= 0x0002,	// when moving this component, do not move the physics representation. Used internally to avoid looping updates when syncing with physics.
	MOVECOMP_NeverIgnoreBlockingOverlaps= 0x0004,	// never ignore initial blocking overlaps during movement, which are usually ignored when moving out of an object. MOVECOMP_IgnoreBases is still respected.
};

#define SCENECOMPONENT_QUAT_TOLERANCE		(1.e-8f) // Comparison tolerance for checking if two FQuats are the same when moving SceneComponents.
#define SCENECOMPONENT_ROTATOR_TOLERANCE	(1.e-4f) // Comparison tolerance for checking if two FRotators are the same when moving SceneComponents.

FORCEINLINE EMoveComponentFlags operator|(EMoveComponentFlags Arg1,EMoveComponentFlags Arg2)	{ return EMoveComponentFlags(uint32(Arg1) | uint32(Arg2)); }
FORCEINLINE EMoveComponentFlags operator&(EMoveComponentFlags Arg1,EMoveComponentFlags Arg2)	{ return EMoveComponentFlags(uint32(Arg1) & uint32(Arg2)); }
FORCEINLINE void operator&=(EMoveComponentFlags& Dest,EMoveComponentFlags Arg)					{ Dest = EMoveComponentFlags(Dest & Arg); }
FORCEINLINE void operator|=(EMoveComponentFlags& Dest,EMoveComponentFlags Arg)					{ Dest = EMoveComponentFlags(Dest | Arg); }

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPhysicsVolumeChanged, class APhysicsVolume*, NewVolume);


/**
 * A SceneComponent has a transform and supports attachment, but has no rendering or collision capabilities.
 * Useful as a 'dummy' component in the hierarchy to offset others.
 * @see [Scene Components](https://docs.unrealengine.com/latest/INT/Programming/UnrealArchitecture/Actors/Components/index.html#scenecomponents)
 */
UCLASS(ClassGroup=(Utility, Common), BlueprintType, hideCategories=(Trigger, PhysicsVolume), meta=(BlueprintSpawnableComponent, IgnoreCategoryKeywordsInSubclasses, ShortTooltip="A Scene Component is a component that has a scene transform and can be attached to other scene components."))
class ENGINE_API USceneComponent : public UActorComponent
{
	GENERATED_BODY()
public:

	/** The name to use for the default scene root variable */
	static const FName& GetDefaultSceneRootVariableName();

	/**
	 * UObject constructor that takes an ObjectInitializer
	 */
	USceneComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** What we are currently attached to. If valid, RelativeLocation etc. are used relative to this object */
	UPROPERTY(Replicated)
	class USceneComponent* AttachParent;

	/** List of child SceneComponents that are attached to us. */
	UPROPERTY(transient)
	TArray< USceneComponent* > AttachChildren;

	/** Optional socket name on AttachParent that we are attached to. */
	UPROPERTY(Replicated)
	FName AttachSocketName;

	/** if true, will call GetCustomLocation instead or returning the location part of ComponentToWorld */
	UPROPERTY()
	uint32 bRequiresCustomLocation:1;

	/** If RelativeLocation should be considered relative to the world, rather than the parent */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, ReplicatedUsing=OnRep_Transform, Category=Transform)
	uint32 bAbsoluteLocation:1;

private:
	// DEPRECATED
	UPROPERTY()
	uint32 bAbsoluteTranslation_DEPRECATED:1;

	// Appends all descendants (recursively) of this scene component to the list of Children.  NOTE: It does NOT clear the list first.
	void AppendDescendants(TArray<USceneComponent*>& Children) const;

public:

	/** If RelativeRotation should be considered relative to the world, rather than the parent */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, ReplicatedUsing=OnRep_Transform, Category=Transform)
	uint32 bAbsoluteRotation:1;

	/** If RelativeScale3D should be considered relative to the world, rather than the parent */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, ReplicatedUsing=OnRep_Transform, Category=Transform)
	uint32 bAbsoluteScale:1;

	/** Whether to completely draw the primitive; if false, the primitive is not drawn, does not cast a shadow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Visibility,  Category = Rendering)
	uint32 bVisible:1;

	/** Whether to hide the primitive in game, if the primitive is Visible. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Rendering)
	uint32 bHiddenInGame:1;

	/**
	 * Whether or not the cached PhysicsVolume this component overlaps should be updated when the component is moved.
	 * @see GetPhysicsVolume()
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category=Physics)
	uint32 bShouldUpdatePhysicsVolume:1;

	/** If true, a change in the bounds of this component will call trigger a streaming data rebuild */
	UPROPERTY()
	uint32 bBoundsChangeTriggersStreamingDataRebuild:1;

	/** If true, this component uses its parents bounds when attached.
	 *  This can be a significant optimization with many components attached together.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category=Rendering)
	uint32 bUseAttachParentBound:1;

protected:
	UPROPERTY(Transient)
	uint32 bWorldToComponentUpdated:1;

	// Transient flag that temporarily disables UpdateOverlaps within DetachFromParent().
	uint32 bDisableDetachmentUpdateOverlaps:1;

private:
	/** Physics Volume in which this SceneComponent is located **/
	UPROPERTY(transient)
	TWeakObjectPtr<class APhysicsVolume> PhysicsVolume;

public:
	/** Current bounds of this component */
	FBoxSphereBounds Bounds;

	/** Current transform of this component, relative to the world */
	FTransform ComponentToWorld;

private:
	/** Cache that avoids Quat<->Rotator conversions if possible. Only to be used with ComponentToWorld.GetRotation(). */
	FRotationConversionCache WorldRotationCache;

public:

	/** Location of this component relative to its parent */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Transform, Category = Transform)
	FVector RelativeLocation;

	/** Rotation of this component relative to its parent */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Transform, Category=Transform)
	FRotator RelativeRotation;

private:
	/** Cache that avoids Quat<->Rotator conversions if possible. Only to be used with RelativeRotation. */
	FRotationConversionCache RelativeRotationCache;

public:
	/**
	*	Non-uniform scaling of this component relative to its parent.
	*	Note that scaling is always applied in local space (no shearing etc)
	*/
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Transform, interp, Category=Transform)
	FVector RelativeScale3D;

	UPROPERTY()
	FVector RelativeTranslation_DEPRECATED;

	/** How often this component is allowed to move, used to make various optimizations. Only safe to set in constructor, use SetMobility() during runtime. */
	UPROPERTY(Category = Mobility, EditAnywhere, BlueprintReadOnly)
	TEnumAsByte<EComponentMobility::Type> Mobility;

	/** If detail mode is >= system detail mode, primitive won't be rendered. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=LOD)
	TEnumAsByte<enum EDetailMode> DetailMode;

private:

	bool NetUpdateTransform;

	FName NetOldAttachSocketName;
	USceneComponent *NetOldAttachParent;

	UFUNCTION()
	void OnRep_Transform();

	UFUNCTION()
	void OnRep_Visibility(bool OldValue);

	virtual void PreNetReceive() override;
	virtual void PostNetReceive() override;

public:

	/**
	 * Velocity of this component.
	 * @see GetComponentVelocity()
	 */
	UPROPERTY()
	FVector ComponentVelocity;

public:
	/** Set the location of this component relative to its parent */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="SetRelativeLocation"))
	void K2_SetRelativeLocation(FVector NewLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void SetRelativeLocation(FVector NewLocation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);


	/** Set the rotation of this component relative to its parent */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="SetRelativeRotation", AdvancedDisplay="bSweep,SweepHitResult,bTeleport"))
	void K2_SetRelativeRotation(FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void SetRelativeRotation(FRotator NewRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetRelativeRotation(const FQuat& NewRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);

	/** Set the transform of this component relative to its parent */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="SetRelativeTransform"))
	void K2_SetRelativeTransform(const FTransform& NewTransform, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void SetRelativeTransform(const FTransform& NewTransform, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);

	/** Returns the transform of this component relative to its parent */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation")
	FTransform GetRelativeTransform() const;

	/** Set the transform of this component relative to its parent */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation")
	void ResetRelativeTransform();

	/** Set the non-uniform of this component relative to its parent */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation")
	virtual void SetRelativeScale3D(FVector NewScale3D);

	/** Adds a delta to the translation of this component relative to its parent */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="AddRelativeLocation"))
	void K2_AddRelativeLocation(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void AddRelativeLocation(FVector DeltaLocation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);

	/** Adds a delta the rotation of this component relative to its parent */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="AddRelativeRotation", AdvancedDisplay="bSweep,SweepHitResult,bTeleport"))
	void K2_AddRelativeRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void AddRelativeRotation(FRotator DeltaRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddRelativeRotation(const FQuat& DeltaRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);

	/** Adds a delta to the location of this component in its local reference frame */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="AddLocalOffset", Keywords="location position"))
	void K2_AddLocalOffset(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void AddLocalOffset(FVector DeltaLocation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);

	
	/** Adds a delta to the rotation of this component in its local reference frame */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="AddLocalRotation", AdvancedDisplay="bSweep,SweepHitResult,bTeleport"))
	void K2_AddLocalRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void AddLocalRotation(FRotator DeltaRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddLocalRotation(const FQuat& DeltaRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);


	/** Adds a delta to the transform of this component in its local reference frame. Scale is unchanged. */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="AddLocalTransform"))
	void K2_AddLocalTransform(const FTransform& DeltaTransform, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void AddLocalTransform(const FTransform& DeltaTransform, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);


	/** Set the relative location of this component to put it at the supplied location in world space. */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="SetWorldLocation"))
	void K2_SetWorldLocation(FVector NewLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void SetWorldLocation(FVector NewLocation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);


	/** Set the relative rotation of this component to put it at the supplied orientation in world space. */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="SetWorldRotation", AdvancedDisplay="bSweep,SweepHitResult,bTeleport"))
	void K2_SetWorldRotation(FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void SetWorldRotation(FRotator NewRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldRotation(const FQuat& NewRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);


	/** Set the relative scale of this component to put it at the supplied scale in world space. */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation")
	void SetWorldScale3D(FVector NewScale);

	/** Set the transform of this component in world space. */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="SetWorldTransform"))
	void K2_SetWorldTransform(const FTransform& NewTransform, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void SetWorldTransform(const FTransform& NewTransform, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);


	/** Adds a delta to the location of this component in world space. */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="AddWorldOffset", Keywords="location position"))
	void K2_AddWorldOffset(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void AddWorldOffset(FVector DeltaLocation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);


	/** Adds a delta to the rotation of this component in world space. */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="AddWorldRotation", AdvancedDisplay="bSweep,SweepHitResult,bTeleport"))
	void K2_AddWorldRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void AddWorldRotation(FRotator DeltaRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddWorldRotation(const FQuat& DeltaRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);

	/** Adds a delta to the transform of this component in world space. Scale is unchanged. */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="AddWorldTransform"))
	void K2_AddWorldTransform(const FTransform& DeltaTransform, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void AddWorldTransform(const FTransform& DeltaTransform, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);

	/** Return location of the component, in world space */
	UFUNCTION(BlueprintCallable, meta=(DisplayName = "GetWorldLocation"), Category="Utilities|Transformation")
	FVector K2_GetComponentLocation() const;

	/** Returns rotation of the component, in world space. */
	UFUNCTION(BlueprintCallable, meta=(DisplayName = "GetWorldRotation"), Category="Utilities|Transformation")
	FRotator K2_GetComponentRotation() const;
	
	/** Returns scale of the component, in world space. */
	UFUNCTION(BlueprintCallable, meta=(DisplayName = "GetWorldScale"), Category="Utilities|Transformation")
	FVector K2_GetComponentScale() const;

	/** Get the current component-to-world transform for this component */
	UFUNCTION(BlueprintCallable, meta=(DisplayName = "GetWorldTransform"), Category="Utilities|Transformation")
	FTransform K2_GetComponentToWorld() const;

	/** Get the forward (X) unit direction vector from this component, in world space.  */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation")
	FVector GetForwardVector() const;

	/** Get the up (Z) unit direction vector from this component, in world space.  */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation")
	FVector GetUpVector() const;

	/** Get the right (Y) unit direction vector from this component, in world space.  */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation")
	FVector GetRightVector() const;

	/** Returns whether the specified body is currently using physics simulation */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual bool IsSimulatingPhysics(FName BoneName = NAME_None) const;

	/** Returns whether the specified body is currently using physics simulation */
	UFUNCTION(BlueprintCallable, Category="Physics")
	virtual bool IsAnySimulatingPhysics() const;

	/** Get a pointer to the USceneComponent we are attached to */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation")
	class USceneComponent* GetAttachParent() const;

	/** Gets all parent components up to and including the root component */
	UFUNCTION(BlueprintCallable, Category="Components")
	void GetParentComponents(TArray<class USceneComponent*>& Parents) const;

	/** Gets the number of attached children components */
	UFUNCTION(BlueprintCallable, Category="Components")
	int32 GetNumChildrenComponents() const;

	/** Gets the attached child component at the specified location */
	UFUNCTION(BlueprintCallable, Category="Components")
	class USceneComponent* GetChildComponent(int32 ChildIndex) const;

	/** 
	 * Gets all the attached child components
	 * @param bIncludeAllDescendants Whether to include all descendants in the list of children (i.e. grandchildren, great grandchildren, etc.)
	 * @param Children The list of attached child components
	 */
	UFUNCTION(BlueprintCallable, Category="Components")
	void GetChildrenComponents(bool bIncludeAllDescendants, TArray<USceneComponent*>& Children) const;

	/** 
	 *   Attach this component to another scene component, optionally at a named socket. It is valid to call this on components whether or not they have been Registered.
	 *   @param bMaintainWorldTransform	If true, update the relative location/rotation of this component to keep its world position the same
	 */
	void AttachTo(class USceneComponent* InParent, FName InSocketName = NAME_None, EAttachLocation::Type AttachType = EAttachLocation::KeepRelativeOffset, bool bWeldSimulatedBodies = false);

	/**
	*   Attach this component to another scene component, optionally at a named socket. It is valid to call this on components whether or not they have been Registered.
	*   @param bMaintainWorldTransform	If true, update the relative location/rotation of this component to keep its world position the same
	*/
	UFUNCTION(BlueprintCallable, Category = "Utilities|Transformation", meta = (DisplayName = "AttachTo", AttachType = "KeepRelativeOffset"))
	void K2_AttachTo(class USceneComponent* InParent, FName InSocketName = NAME_None, EAttachLocation::Type AttachType = EAttachLocation::KeepRelativeOffset, bool bWeldSimulatedBodies = true);

	/** Zeroes out the relative transform of this component, and calls AttachTo(). Useful for attaching directly to a scene component or socket location  */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage = "Use AttachTo with EAttachLocation::SnapToTarget option instead"), Category="Utilities|Transformation")
	void SnapTo(class USceneComponent* InParent, FName InSocketName = NAME_None);

	/** 
	 *	Detach this component from whatever it is attached to. Automatically unwelds components that are welded together (See WeldTo)
	 *   @param bMaintainWorldTransform	If true, update the relative location/rotation of this component to keep its world position the same *	
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation")
	virtual void DetachFromParent(bool bMaintainWorldPosition = false, bool bCallModify = true);


	/** 
	 * Gets the names of all the sockets on the component.
	 * @return Get the names of all the sockets on the component.
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(Keywords="Bone"))
	TArray<FName> GetAllSocketNames() const;

	/** 
	 * Get world-space socket transform.
	 * @param InSocketName Name of the socket or the bone to get the transform 
	 * @return Socket transform in world space if socket if found. Otherwise it will return component's transform in world space.
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(Keywords="Bone"))
	virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = RTS_World) const;

	/** 
	 * Get world-space socket or bone location.
	 * @param InSocketName Name of the socket or the bone to get the transform 
	 * @return Socket transform in world space if socket if found. Otherwise it will return component's transform in world space.
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(Keywords="Bone"))
	virtual FVector GetSocketLocation(FName InSocketName) const;

	/** 
	 * Get world-space socket or bone  FRotator rotation.
	 * @param InSocketName Name of the socket or the bone to get the transform 
	 * @return Socket transform in world space if socket if found. Otherwise it will return component's transform in world space.
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(Keywords="Bone"))
	virtual FRotator GetSocketRotation(FName InSocketName) const;

	/** 
	 * Get world-space socket or bone FQuat rotation.
	 * @param InSocketName Name of the socket or the bone to get the transform 
	 * @return Socket transform in world space if socket if found. Otherwise it will return component's transform in world space.
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(Keywords="Bone", DeprecatedFunction, DeprecationMessage="Use GetSocketRotation instead, Quat is not fully supported in blueprints."))
	virtual FQuat GetSocketQuaternion(FName InSocketName) const;

	/** 
	 * return true if socket with the given name exists
	 * @param InSocketName Name of the socket or the bone to get the transform 
	 * @return true if the socket with the given name exists. Otherwise, return false
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(Keywords="Bone"))
	virtual bool DoesSocketExist(FName InSocketName) const;

	/**
	 * Returns true if this component has any sockets
	 */
	virtual bool HasAnySockets() const;

	/**
	 * Get a list of sockets this component contains
	 */
	virtual void QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const;

	/** 
	 * Get velocity of this component: either ComponentVelocity, or the velocity of the physics body if simulating physics.
	 * @return Velocity of this component
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation")
	virtual FVector GetComponentVelocity() const;

	/** 
	 * Is this component visible or not in game
	 * @return true if visible
	 */
	UFUNCTION(BlueprintCallable, Category="Rendering")
	virtual bool IsVisible() const;

	/** 
	 * Set visibility of the component, if during game use this to turn on/off
	 */
	UFUNCTION(BlueprintCallable, Category="Rendering")
	virtual void SetVisibility(bool bNewVisibility, bool bPropagateToChildren=false);

	/** 
	 * Toggle visibility of the component
	 */
	UFUNCTION(BlueprintCallable, Category="Rendering")
	void ToggleVisibility(bool bPropagateToChildren=false);

	/**
	 * Changes the value of HiddenGame.
	 * @param NewHidden	- The value to assign to HiddenGame.
	 */
	UFUNCTION(BlueprintCallable, Category="Development")
	virtual void SetHiddenInGame(bool NewHidden, bool bPropagateToChildren=false);

public:
	/** Delegate that will be called when PhysicsVolume has been changed **/
	UPROPERTY(BlueprintAssignable, Category=PhysicsVolume, meta=(DisplayName="Physics Volume Changed"))
	FPhysicsVolumeChanged PhysicsVolumeChangedDelegate;

	// Begin ActorComponent interface
	virtual void OnRegister() override;
	/** Return true if CreateRenderState() should be called */
	virtual bool ShouldCreateRenderState() const override
	{
		return true;
	}
	virtual void UpdateComponentToWorld(bool bSkipPhysicsMove = false, ETeleportType Teleport = ETeleportType::None) override final;
	virtual void DestroyComponent(bool bPromoteChildren = false) override;
	virtual void OnComponentDestroyed() override;
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;
	virtual class FActorComponentInstanceData* GetComponentInstanceData() const override;
	// End ActorComponent interface

	// Call UpdateComponentToWorld if bWorldToComponentUpdated is false.
	void ConditionalUpdateComponentToWorld();

	// Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostInterpChange(UProperty* PropertyThatChanged) override;
	virtual void BeginDestroy() override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

	// End UObject Interface
protected:
	/**
	 * Internal helper, for use from MoveComponent().  Special codepath since the normal setters call MoveComponent.
	 * @return: true if location or rotation was changed.
	 */
	bool InternalSetWorldLocationAndRotation(FVector NewLocation, const FQuat& NewQuat, bool bNoPhysics = false, ETeleportType Teleport = ETeleportType::None);

	virtual void OnUpdateTransform(bool bSkipPhysicsMove, ETeleportType Teleport = ETeleportType::None);

	/** Check if mobility is set to non-static. If it's static we trigger a PIE warning and return true*/
	bool CheckStaticMobilityAndWarn(const FText& ActionText) const;

private:

	void PropagateTransformUpdate(bool bTransformChanged, bool bSkipPhysicsMove = false, ETeleportType Teleport = ETeleportType::None);
	void UpdateComponentToWorldWithParent(USceneComponent* Parent, bool bSkipPhysicsMove, const FQuat& RelativeRotationQuat, ETeleportType Teleport = ETeleportType::None);


public:

	/** Queries world and updates overlap tracking state for this component */
	virtual void UpdateOverlaps(TArray<FOverlapInfo> const* PendingOverlaps=nullptr, bool bDoNotifies=true, const TArray<FOverlapInfo>* OverlapsAtEndLocation=nullptr);

	/**
	 * Tries to move the component by a movement vector (Delta) and sets rotation to NewRotation.
	 * Assumes that the component's current location is valid and that the component does fit in its current Location.
	 * Dispatches blocking hit notifications (if bSweep is true), and calls UpdateOverlaps() after movement to update overlap state.
	 *
	 * @note This simply calls the virtual MoveComponentImpl() which can be overridden to implement custom behavior.
	 * @note The overload taking rotation as an FQuat is slightly faster than the version using FRotator (which will be converted to an FQuat)..
	 * @param Delta			The desired location change in world space.
	 * @param NewRotation	The new desired rotation in world space.
	 * @param bSweep		True to do a swept move, which will stop at a blocking collision.
	 * @param Hit			Optional output describing the blocking hit that stopped the move, if any.
	 * @param MoveFlags		Flags controlling behavior of the move. @see EMoveComponentFlags
	 * @param Teleport      Determines whether to teleport the physics body or not. Teleporting will maintain constant velocity and avoid collisions along the path
	 * @return				True if some movement occurred, false if no movement occurred.
	 */
	bool MoveComponent( const FVector& Delta, const FQuat& NewRotation,    bool bSweep, FHitResult* Hit=NULL, EMoveComponentFlags MoveFlags = MOVECOMP_NoFlags, ETeleportType Teleport = ETeleportType::None);
	bool MoveComponent( const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult* Hit=NULL, EMoveComponentFlags MoveFlags = MOVECOMP_NoFlags, ETeleportType Teleport = ETeleportType::None);

protected:

	// Override this method for custom behavior.
	virtual bool MoveComponentImpl(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* Hit = NULL, EMoveComponentFlags MoveFlags = MOVECOMP_NoFlags, ETeleportType Teleport = ETeleportType::None);

public:


	/** Returns true if movement is currently within the scope of an FScopedMovementUpdate. */
	bool IsDeferringMovementUpdates() const;

	/** Returns the current scoped movement update, or NULL if there is none. @see FScopedMovementUpdate */
	class FScopedMovementUpdate* GetCurrentScopedMovement() const;

private:

	/** Stack of current movement scopes. */
	TArray<class FScopedMovementUpdate*> ScopedMovementStack;

	void BeginScopedMovementUpdate(class FScopedMovementUpdate& ScopedUpdate);
	void EndScopedMovementUpdate(class FScopedMovementUpdate& ScopedUpdate);

	friend class FScopedMovementUpdate;

#if WITH_EDITORONLY_DATA
protected:
	/** Editor only component used to display the sprite so as to be able to see the location of the Audio Component  */
	class UBillboardComponent* SpriteComponent;

public:
	UPROPERTY()
	uint32 bVisualizeComponent : 1;
#endif

public:

	/** Called when AttachParent changes, to allow the scene to update its attachment state. */
	virtual void OnAttachmentChanged() {}

	/** Return location of the component, in world space */
	FORCEINLINE FVector GetComponentLocation() const
	{
		if (bRequiresCustomLocation)
		{
			return GetCustomLocation();
		}
		else
		{
			return ComponentToWorld.GetLocation();
		}
	}

	/** Internal routine. Only called if bRequiresCustomLocation is true, Return location of the component, in world space */
	virtual FVector GetCustomLocation() const
	{
		check(0); // if you have custom location, you should not end up here!
		return ComponentToWorld.GetLocation();
	}

	/** Return rotation of the component, in world space */
	FORCEINLINE FRotator GetComponentRotation() const
	{
		return WorldRotationCache.NormalizedQuatToRotator(ComponentToWorld.GetRotation());
	}

	/** Return rotation quaternion of the component, in world space */
	FORCEINLINE FQuat GetComponentQuat() const
	{
		return ComponentToWorld.GetRotation();
	}

	/** Return scale of the component, in world space */
	FORCEINLINE FVector GetComponentScale() const
	{
		return ComponentToWorld.GetScale3D();
	}

	/** Get the current component-to-world transform for this component */
	FORCEINLINE FTransform GetComponentToWorld() const 
	{ 
		return ComponentToWorld; // TODO: probably deprecate this in favor of GetComponentTransform
	}

	/** Get the current component-to-world transform for this component */
	FORCEINLINE FTransform GetComponentTransform() const
	{
		return ComponentToWorld;
	}

	/** Update transforms of any components attached to this one. */
	void UpdateChildTransforms(bool bSkipPhysicsMove = false, ETeleportType Teleport = ETeleportType::None);

	/** Calculate the bounds of this component. Default behavior is a bounding box/sphere of zero size. */
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const;

	/**
	 * Calculate the axis-aligned bounding cylinder of this component (radius in X-Y, half-height along Z axis).
	 * Default behavior is just a cylinder around the box of the cached BoxSphereBounds.
	 */
	virtual void CalcBoundingCylinder(float& CylinderRadius, float& CylinderHalfHeight) const;

	/** Update the Bounds of this component.*/
	virtual void UpdateBounds();

	/** If true, bounds should be used when placing component/actor in level, and spawning may fail */
	virtual bool ShouldCollideWhenPlacing() const
	{
		return false;
	}

	/** 
	 * Updates the PhysicsVolume of this SceneComponent, if bShouldUpdatePhysicsVolume is true.
	 * 
	 * @param bTriggerNotifiers		if true, send zone/volume change events
	 */
	virtual void UpdatePhysicsVolume( bool bTriggerNotifiers );

	/**
	 * Replace current PhysicsVolume to input NewVolume
	 *
	 * @param NewVolume				NewVolume to replace
	 * @param bTriggerNotifiers		if true, send zone/volume change events
	 */
	void SetPhysicsVolume( APhysicsVolume * NewVolume,  bool bTriggerNotifiers );

	/** 
	 * Get the PhysicsVolume overlapping this component.
	 */
	UFUNCTION(BlueprintCallable, Category=PhysicsVolume)
	APhysicsVolume* GetPhysicsVolume() const;


	/** Return const reference to CollsionResponseContainer */
	virtual const FCollisionResponseContainer& GetCollisionResponseToChannels() const;

	/** Return true if visible in editor **/
	virtual bool IsVisibleInEditor() const;

	/** return true if it should render **/
	bool ShouldRender() const;

	/** return true if it can ever render **/
	bool CanEverRender() const;

	/** 
	 *  Looking at various values of the component, determines if this
	 *  component should be added to the scene
	 * @return true if the component is visible and should be added to the scene, false otherwise
	 */
	bool ShouldComponentAddToScene() const;

#if WITH_EDITOR
	/** Called when this component is moved in the editor */
	virtual void PostEditComponentMove(bool bFinished) {}
	virtual bool CanEditChange( const UProperty* Property ) const override;

	virtual const int32 GetNumUncachedStaticLightingInteractions() const;

	virtual void PreFeatureLevelChange(ERHIFeatureLevel::Type PendingFeatureLevel) {}
#endif // WITH_EDITOR

protected:

	/** Calculate the new ComponentToWorld transform for this component.
		Parent is optional and can be used for computing ComponentToWorld based on arbitrary USceneComponent.
		If Parent is not passed in we use the component's AttachParent*/
	virtual FTransform CalcNewComponentToWorld(const FTransform& NewRelativeTransform, const USceneComponent* Parent = NULL) const;

	
public:
	/** Set the location and rotation of this component relative to its parent */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="SetRelativeLocationAndRotation"))
	void K2_SetRelativeLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void SetRelativeLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetRelativeLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);

	/** Set which parts of the relative transform should be relative to parent, and which should be relative to world */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation")
	void SetAbsolute(bool bNewAbsoluteLocation = false, bool bNewAbsoluteRotation = false, bool bNewAbsoluteScale = false);

	/** Set the relative location and FRotator rotation of this component to put it at the supplied pose in world space. */
	UFUNCTION(BlueprintCallable, Category="Utilities|Transformation", meta=(DisplayName="SetWorldLocationAndRotation"))
	void K2_SetWorldLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
	void SetWorldLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);

	/** Set the relative location and FQuat rotation of this component to put it at the supplied pose in world space. */
	void SetWorldLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep=false, FHitResult* OutSweepHitResult=nullptr, ETeleportType Teleport = ETeleportType::None);

	/** Special version of SetWorldLocationAndRotation that does not affect physics. */
	void SetWorldLocationAndRotationNoPhysics(const FVector& NewLocation, const FRotator& NewRotation);

	/** Is this component considered 'world' geometry */
	virtual bool IsWorldGeometry() const;

	/** Returns the form of collision for this component */
	virtual ECollisionEnabled::Type GetCollisionEnabled() const;

	/** Utility to see if there is any form of collision enabled on this component */
	bool IsCollisionEnabled() const
	{
		return GetCollisionEnabled() != ECollisionEnabled::NoCollision;
	}

	/** Returns the response that this component has to a specific collision channel. */
	virtual ECollisionResponse GetCollisionResponseToChannel(ECollisionChannel Channel) const;

	/** Returns the channel that this component belongs to when it moves. */
	virtual ECollisionChannel GetCollisionObjectType() const;

	/** Compares the CollisionObjectType of each component against the Response of the other, to see what kind of response we should generate */
	ECollisionResponse GetCollisionResponseToComponent(class USceneComponent* OtherComponent) const;

	/** Set how often this component is allowed to move during runtime. Causes a component re-register if the component is already registered */
	virtual void SetMobility(EComponentMobility::Type NewMobility);

	/** Walks up the attachment chain from this SceneComponent and returns the SceneComponent at the top. this->Attachparent is NULL, returns this. */
	class USceneComponent* GetAttachmentRoot() const;
	
	/** Walks up the attachment chain from this SceneComponent and returns the top-level actor it's attached to.  Returns NULL if unattached. */
	class AActor* GetAttachmentRootActor() const;

	/** Walks up the attachment chain to see if this component is attached to the supplied component. If TestComp == this, returns false.*/
	bool IsAttachedTo(class USceneComponent* TestComp) const;

	/**
	 * Find the world-space location and rotation of the given named socket.
	 * If the socket is not found, then it returns the component's location and rotation in world space.
	 * @param InSocketName the name of the socket to find
	 * @param OutLocation (out) set to the world space location of the socket
	 * @param OutRotation (out) set to the world space rotation of the socket
	 * @return whether or not the socket was found
	 */
	void GetSocketWorldLocationAndRotation(FName InSocketName, FVector& OutLocation, FRotator& OutRotation) const;
	void GetSocketWorldLocationAndRotation(FName InSocketName, FVector& OutLocation, FQuat& OutRotation) const;

	/**
	 * Called to see if it's possible to attach another scene component as a child.
	 * Note: This can be called on template component as well!
	 */
	virtual bool CanAttachAsChild(USceneComponent* ChildComponent, FName SocketName) const { return true; }

	/** Get the extent used when placing this component in the editor, used for 'pulling back' hit. */
	virtual FBoxSphereBounds GetPlacementExtent() const;

protected:
	/**
	 * Called after a child scene component is attached to this component.
	 * Note: Do not change the attachment state of the child during this call.
	 */
	virtual void OnChildAttached(USceneComponent* ChildComponent) {}

	/**
	 * Called after a child scene component is detached from this component.
	 * Note: Do not change the attachment state of the child during this call.
	 */
	virtual void OnChildDetached(USceneComponent* ChildComponent) {}

	/** Called after changing transform, tries to update navigation octree for this component */
	virtual void UpdateNavigationData();

	/** Called after changing transform, tries to update navigation octree for owner */
	void PostUpdateNavigationData();

	/**
	 * Determine if dynamic data is allowed to be changed.
	 * 
	 * @param bIgnoreStationary Whether or not to ignore stationary mobility when checking. Default is true (i.e. - check for static mobility only).
	 * @return Whether or not dynamic data is allowed to be changed.
	 */
	FORCEINLINE bool AreDynamicDataChangesAllowed(bool bIgnoreStationary = true) const
	{
		return (IsOwnerRunningUserConstructionScript()) || !(IsRegistered() && (Mobility == EComponentMobility::Static || (!bIgnoreStationary && Mobility == EComponentMobility::Stationary)));
	}
};


//////////////////////////////////////////////////////////////////////////
// USceneComponent inlines

FORCEINLINE_DEBUGGABLE void USceneComponent::ConditionalUpdateComponentToWorld()
{
	if (!bWorldToComponentUpdated)
	{
		UpdateComponentToWorld();
	}
}

FORCEINLINE_DEBUGGABLE bool USceneComponent::MoveComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* Hit, EMoveComponentFlags MoveFlags, ETeleportType Teleport)
{
	return MoveComponentImpl(Delta, NewRotation, bSweep, Hit, MoveFlags, Teleport);
}

FORCEINLINE_DEBUGGABLE void USceneComponent::SetRelativeLocation(FVector NewLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport)
{
	SetRelativeLocationAndRotation(NewLocation, RelativeRotationCache.RotatorToQuat(RelativeRotation), bSweep, OutSweepHitResult, Teleport);
}

FORCEINLINE_DEBUGGABLE void USceneComponent::SetRelativeRotation(const FQuat& NewRotation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport)
{
	SetRelativeLocationAndRotation(RelativeLocation, NewRotation, bSweep, OutSweepHitResult, Teleport);
}

FORCEINLINE_DEBUGGABLE void USceneComponent::AddRelativeLocation(FVector DeltaLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport)
{
	SetRelativeLocationAndRotation(RelativeLocation + DeltaLocation, RelativeRotationCache.RotatorToQuat(RelativeRotation), bSweep, OutSweepHitResult, Teleport);
}

FORCEINLINE_DEBUGGABLE void USceneComponent::AddRelativeRotation(FRotator DeltaRotation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport)
{
	SetRelativeRotation(RelativeRotation + DeltaRotation, bSweep, OutSweepHitResult, Teleport);
}

//////////////////////////////////////////////////////////////////////////

/** 
 *  Component instance cached data base class for scene components. 
 *  Stores a list of instance components attached to the 
 */
class ENGINE_API FSceneComponentInstanceData : public FActorComponentInstanceData
{
public:
	FSceneComponentInstanceData(const USceneComponent* SourceComponent);
			
	virtual ~FSceneComponentInstanceData()
	{}

	virtual void ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase) override;
	virtual void FindAndReplaceInstances(const TMap<UObject*, UObject*>& OldToNewInstanceMap) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	TArray<USceneComponent*> AttachedInstanceComponents;
};


/**
 * Enum that controls the scoping behavior of FScopedMovementUpdate.
 * Note that EScopedUpdate::ImmediateUpdates is not allowed within outer scopes that defer updates,
 * and any attempt to do so will change the new inner scope to use deferred updates instead.
 */
namespace EScopedUpdate
{
	enum Type
	{
		ImmediateUpdates,
		DeferredUpdates	
	};
}


/**
 * FScopedMovementUpdate creates a new movement scope, within which propagation of moves may be deferred until the end of the outermost scope that does not defer updates.
 * Moves within this scope will avoid updates such as UpdateBounds(), OnUpdateTransform(), UpdatePhysicsVolume(), UpdateChildTransforms() etc
 * until the move is committed (which happens when the last deferred scope goes out of context).
 *
 * Note that non-deferred scopes are not allowed within outer scopes that defer updates, and any attempt to use one will change the inner scope to use deferred updates.
 */
class ENGINE_API FScopedMovementUpdate : private FNoncopyable
{
public:
	
	typedef TArray<struct FHitResult, TInlineAllocator<2>> TBlockingHitArray;

	FScopedMovementUpdate( class USceneComponent* Component, EScopedUpdate::Type ScopeBehavior = EScopedUpdate::DeferredUpdates );
	~FScopedMovementUpdate();

	enum class EHasMovedTransformOption
	{
		eTestTransform,
		eIgnoreTransform
	};

	enum class EOverlapState
	{
		eUseParent,
		eUnknown,
		eIncludesOverlaps,
		eForceUpdate,
	};

	/** Get the scope containing this scope. A scope only has an outer scope if they both defer updates. */
	const FScopedMovementUpdate* GetOuterDeferredScope() const;

	/** Return true if deferring updates, false if updates are applied immediately. */
	bool IsDeferringUpdates() const;
	
	/** Revert movement to the initial location of the Component at the start of the scoped update. Also clears pending overlaps and sets bHasMoved to false. */
	void RevertMove();

	/** Returns whether movement has occurred at all during this scope, optionally checking if the transform is different (since changing scale does not go through a move). RevertMove() sets this back to false. */
	bool HasMoved(EHasMovedTransformOption CheckTransform) const;

	/** Returns true if the Component's transform differs from that at the start of the scoped update. */
	bool IsTransformDirty() const;

	/** Returns true if there are pending overlaps queued in this scope. */
	bool HasPendingOverlaps() const;

	/** Returns the pending overlaps within this scope. */
	const TArray<FOverlapInfo>& GetPendingOverlaps() const;

	/** Returns the list of pending blocking hits, which will be used for notifications once the move is committed. */
	const TBlockingHitArray& GetPendingBlockingHits() const;

	//--------------------------------------------------------------------------------------------------------//
	// These methods are intended only to be used by SceneComponent and derived classes.

	/** Add overlaps to the queued overlaps array. This is intended for use only by SceneComponent and its derived classes whenever movement is performed. */
	void AppendOverlapsAfterMove(const TArray<FOverlapInfo>& NewPendingOverlaps, bool bSweep, bool bIncludesOverlapsAtEnd);

	/** Keep current pending overlaps after a move but make note that there was movement (just a symmetric rotation). */
	void KeepCurrentOverlapsAfterRotation(bool bSweep);

	/** Add blocking hit that will get processed once the move is committed. This is intended for use only by SceneComponent and its derived classes. */
	void AppendBlockingHitAfterMove(const FHitResult& Hit);

	/** Clear overlap state at current location, we don't know what it is. */
	void InvalidateCurrentOverlaps();

	/** Force full overlap update once this scope finishes. */
	void ForceOverlapUpdate();

	//--------------------------------------------------------------------------------------------------------//

private:

	/** Fills in the list of overlaps at the end location (in EndOverlaps). Returns pointer to the list, or null if it can't be computed. */
	const TArray<FOverlapInfo>* GetOverlapsAtEnd(class UPrimitiveComponent& PrimComponent, TArray<FOverlapInfo>& EndOverlaps, bool bTransformChanged) const;

	/** Notify this scope that the given inner scope completed its update (ie is going out of scope). Only occurs for deferred updates. */
	void OnInnerScopeComplete(const FScopedMovementUpdate& InnerScope);
	
	// This class can only be created on the stack, otherwise the ordering constraints
	// of the constructor and destructor between encapsulated scopes could be violated.
	void*	operator new		(size_t);
	void*	operator new[]		(size_t);
	void	operator delete		(void *);
	void	operator delete[]	(void*);

private:

	class USceneComponent* Owner;
	FScopedMovementUpdate* OuterDeferredScope;
	uint32 bDeferUpdates:1;
	uint32 bHasMoved:1;
	EOverlapState CurrentOverlapState;

	FTransform InitialTransform;
	FVector InitialRelativeLocation;
	FRotator InitialRelativeRotation;
	FVector InitialRelativeScale;

	int32 FinalOverlapCandidatesIndex;
	TArray<FOverlapInfo> PendingOverlaps;
	TBlockingHitArray BlockingHits;

	friend class USceneComponent;
};

//////////////////////////////////////////////////////////////////////////
// FScopedMovementUpdate inlines

FORCEINLINE const FScopedMovementUpdate* FScopedMovementUpdate::GetOuterDeferredScope() const
{
	return OuterDeferredScope;
}

FORCEINLINE bool FScopedMovementUpdate::IsDeferringUpdates() const
{
	return bDeferUpdates;
}

FORCEINLINE bool FScopedMovementUpdate::HasMoved(EHasMovedTransformOption CheckTransform) const
{
	return bHasMoved || (CheckTransform == EHasMovedTransformOption::eTestTransform && IsTransformDirty());
}

FORCEINLINE bool FScopedMovementUpdate::HasPendingOverlaps() const
{
	return PendingOverlaps.Num() > 0;
}

FORCEINLINE const TArray<struct FOverlapInfo>& FScopedMovementUpdate::GetPendingOverlaps() const
{
	return PendingOverlaps;
}

FORCEINLINE const FScopedMovementUpdate::TBlockingHitArray& FScopedMovementUpdate::GetPendingBlockingHits() const
{
	return BlockingHits;
}

FORCEINLINE_DEBUGGABLE void FScopedMovementUpdate::AppendBlockingHitAfterMove(const FHitResult& Hit)
{
	BlockingHits.Add(Hit);
}

FORCEINLINE_DEBUGGABLE void FScopedMovementUpdate::KeepCurrentOverlapsAfterRotation(bool bSweep)
{
	bHasMoved = true;
	// CurrentOverlapState is unchanged
}

FORCEINLINE_DEBUGGABLE void FScopedMovementUpdate::InvalidateCurrentOverlaps()
{
	bHasMoved = true;
	CurrentOverlapState = EOverlapState::eUnknown;
	FinalOverlapCandidatesIndex = INDEX_NONE;
}

FORCEINLINE_DEBUGGABLE void FScopedMovementUpdate::ForceOverlapUpdate()
{
	bHasMoved = true;
	CurrentOverlapState = EOverlapState::eForceUpdate;
	FinalOverlapCandidatesIndex = INDEX_NONE;
}



FORCEINLINE_DEBUGGABLE class FScopedMovementUpdate* USceneComponent::GetCurrentScopedMovement() const
{
	if (ScopedMovementStack.Num() > 0)
	{
		return ScopedMovementStack.Last();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
// SceneComponent inlines that depend on FScopedMovementUpdate

FORCEINLINE_DEBUGGABLE bool USceneComponent::IsDeferringMovementUpdates() const
{
	if (ScopedMovementStack.Num() > 0)
	{
		checkSlow(ScopedMovementStack.Last()->IsDeferringUpdates());
		return true;
	}
	return false;
}

FORCEINLINE_DEBUGGABLE void USceneComponent::BeginScopedMovementUpdate(class FScopedMovementUpdate& ScopedUpdate)
{
	checkSlow(IsInGameThread());
	checkSlow(ScopedUpdate.IsDeferringUpdates());
	ScopedMovementStack.Push(&ScopedUpdate);
}

