// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runtime/Engine/Classes/Engine/LatentActionManager.h"
#include "Actor.generated.h"

ENGINE_API DECLARE_LOG_CATEGORY_EXTERN(LogActor, Log, Warning);
 
// Delegate signatures
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams( FTakeAnyDamageSignature, float, Damage, const class UDamageType*, DamageType, class AController*, InstigatedBy, class AActor*, DamageCauser );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_EightParams( FTakePointDamageSignature, float, Damage, class AController*, InstigatedBy, FVector, HitLocation, class UPrimitiveComponent*, FHitComponent, FName, BoneName, FVector, ShotFromDirection, const class UDamageType*, DamageType, class AActor*, DamageCauser );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FActorBeginOverlapSignature, class AActor*, OtherActor );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FActorEndOverlapSignature, class AActor*, OtherActor );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams( FActorHitSignature, class AActor*, SelfActor, class AActor*, OtherActor, FVector, NormalImpulse, const FHitResult&, Hit );

DECLARE_DYNAMIC_MULTICAST_DELEGATE( FActorBeginCursorOverSignature );
DECLARE_DYNAMIC_MULTICAST_DELEGATE( FActorEndCursorOverSignature );
DECLARE_DYNAMIC_MULTICAST_DELEGATE( FActorOnClickedSignature );
DECLARE_DYNAMIC_MULTICAST_DELEGATE( FActorOnReleasedSignature );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FActorOnInputTouchBeginSignature, ETouchIndex::Type, FingerIndex );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FActorOnInputTouchEndSignature, ETouchIndex::Type, FingerIndex );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FActorBeginTouchOverSignature, ETouchIndex::Type, FingerIndex );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FActorEndTouchOverSignature, ETouchIndex::Type, FingerIndex );

DECLARE_DYNAMIC_MULTICAST_DELEGATE( FActorDestroyedSignature );

DECLARE_DELEGATE_FourParams(FMakeNoiseDelegate, class AActor*, float, class APawn*, const FVector&);

DECLARE_CYCLE_STAT_EXTERN(TEXT("GetComponentsTime"),STAT_GetComponentsTime,STATGROUP_Engine,ENGINE_API);

/** Container for Animation Update Rate parameters.
 * They are shared for all components of an Actor, so they can be updated in sync. */
USTRUCT()
struct FAnimUpdateRateParameters
{
	GENERATED_USTRUCT_BODY()

private:
	/** How often animation will be updated/ticked. 1 = every frame, 2 = every 2 frames, etc. */
	UPROPERTY()
	int32 UpdateRate;

	/** How often animation will be evaluated. 1 = every frame, 2 = every 2 frames, etc.
	 *  has to be a multiple of UpdateRate. */
	UPROPERTY()
	int32 EvaluationRate;

	/** When skipping a frame, should it be interpolated or frozen? */
	UPROPERTY()
	bool bInterpolateSkippedFrames;

	/** (This frame) animation update should be skipped. */
	UPROPERTY()
	bool bSkipUpdate;

	/** (This frame) animation evaluation should be skipped. */
	UPROPERTY()
	bool bSkipEvaluation;

public:
	/** Default constructor */
	FAnimUpdateRateParameters()
		: UpdateRate(1)
		, EvaluationRate(1)
		, bInterpolateSkippedFrames(false)
		, bSkipUpdate(false)
		, bSkipEvaluation(false)
	{
	}

	/** Set parameters and verify inputs.
	 * @param : Owner Actor owner calling this.
	 * @param : NewUpdateRate. How often animation will be updated/ticked. 1 = every frame, 2 = every 2 frames, etc.
	 * @param : NewEvaluationRate. How often animation will be evaluated. 1 = every frame, 2 = every 2 frames, etc.
	 * @param : bNewInterpSkippedFrames. When skipping a frame, should it be interpolated or frozen?
	 */
	void Set(const class AActor & Owner, const int32 & NewUpdateRate, const int32 & NewEvaluationRate, const bool & bNewInterpSkippedFrames);

	/* Getter for UpdateRate */
	int32 GetUpdateRate() const
	{
		return UpdateRate;
	}

	/* Getter for EvaluationRate */
	int32 GetEvaluationRate() const
	{
		return EvaluationRate;
	}

	/* Getter for bSkipUpdate */
	bool ShouldSkipUpdate() const
	{
		return bSkipUpdate;
	}

	/* Getter for bSkipEvaluation */
	bool ShouldSkipEvaluation() const
	{
		return bSkipEvaluation;
	}

	/* Getter for bInterpolateSkippedFrames */
	bool ShouldInterpolateSkippedFrames() const
	{
		return bInterpolateSkippedFrames;
	}
};

/**
 * Base class for an object that can be placed or spawned in a level. Actors may contain a collection of Components, and support network replication.
*
* The functions of interest to initialization order for an Actor is roughly as follows:
* PostLoad/PostActorCreated - Do any setup of the actor required for construction. PostLoad for serialized actors, PostActorCreated for spawned.  
* AActor::OnConstruction - The construction of the actor, this is where Blueprint actors have their components created and blueprint variables are initialized
* AActor::PreInitializeComponents - Called before InitializeComponent is called on the actor's components (if bWantsInitialize is true)
* UActorComponent::InitializeComponent - Each component in the actor's components array gets an initialize call (if bWantsInitializeComponent is true for that component)
* AActor::PostInitializeComponents - Called after the actor's components have been initialized (if bWantsInitialize is true)
* AActor::BeginPlay - Called when the level is started
*/
UCLASS(abstract, dependson=(UEngineBaseTypes, UActorComponent, UEngineTypes), BlueprintType, Blueprintable, config=Engine)
class ENGINE_API AActor : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Primary Actor tick function */
	UPROPERTY()
	struct FActorTickFunction PrimaryActorTick;

	/** A fence to track when the primitive is detached from the scene in the rendering thread. */ // @todo UE4 move?
	FRenderCommandFence DetachFence;
	
	/** Array of ActorComponents that is actually serialized per-instance. */
	UPROPERTY(TextExportTransient)
	TArray< class UActorComponent* > SerializedComponents;

	/** Allow each actor to run at a different time speed. The DeltaTime for a frame is multiplied by the global TimeDilation (in WorldSettings) and this CustomTimeDilation for this actor's tick.  */
	UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category="Misc")
	float CustomTimeDilation;

private:
	/** Used for replication (bNetUseOwnerRelevancy & bOnlyRelevantToOwner) and visibility (PrimitiveComponent bOwnerNoSee and bOnlyOwnerSee) */
	UPROPERTY(replicated)
	class AActor* Owner;

	/** Enables any collision on this actor */
	UPROPERTY()
	bool bActorEnableCollision;

public:
	/** Allows us to only see this Actor in the Editor, and not in the actual game. */
	UPROPERTY(EditAnywhere, Category=Rendering, BlueprintReadOnly, replicated, meta=(DisplayName = "Actor Hidden In Game"))
	uint32 bHidden:1;

	/** Whether to route PreInitializeComponents/PostInitializeComponents to this Actor. */
	UPROPERTY()
	uint32 bWantsInitialize:1;

	/** Used for replication of our RootComponent's position and velocity */
	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedMovement)
	struct FRepMovement ReplicatedMovement;

	/** Used for replicating attachment of this actor's rootcomponent to another actor */
	UPROPERTY(replicatedUsing=OnRep_AttachmentReplication)
	struct FRepAttachment AttachmentReplication;

	/** Called on client when updated AttachmentReplication value is received for this actor. */
	UFUNCTION()
	virtual void OnRep_AttachmentReplication();

	/** Tear-off simulation in network play. */
	UPROPERTY()
	uint32 bNetTemporary:1;

	/** This actor was loaded directly from the map, and for networking purposes can be addressed by its full path name */
	UPROPERTY()
	uint32 bNetStartup:1;

	/** This actor is only relevant to its owner. If this flag is changed during play, all non-owner channels would need to be explicitly closed. */
	UPROPERTY()
	uint32 bOnlyRelevantToOwner:1;

	/** Always relevant for network (overrides bOnlyRelevantToOwner). */
	UPROPERTY(Category=Replication, EditDefaultsOnly, BlueprintReadWrite)
	uint32 bAlwaysRelevant:1;    

	/** Replicate instigator to client (used by bNetTemporary projectiles for example). */
	UPROPERTY()
	uint32 bReplicateInstigator:1;    

	/** If true, replicate movement/location related properties */
	UPROPERTY(Replicated, Category=Replication, EditDefaultsOnly)
	uint32 bReplicateMovement:1;    

	/** if true, this actor is no longer replicated to new clients, and is "torn off" (becomes a ROLE_Authority) on clients to which it was being replicated. */
	UPROPERTY(replicated)
	uint32 bTearOff:1;    

	/** whether we already exchanged Role/RemoteRole on the client, as removing then re-adding a streaming level
	 * causes all initialization to be performed again even though the actor may not have actually been reloaded */
	UPROPERTY(transient)
	uint32 bExchangedRoles:1;

	/** Is this actor still pending a full net update due to clients that weren't able to replicate the actor at the time of LastNetUpdateTime */
	UPROPERTY(transient)
	uint32 bPendingNetUpdate:1;

	/** This actor will be loaded on network clients during map load */
	UPROPERTY(Category=Replication, EditDefaultsOnly)
	uint32 bNetLoadOnClient:1;

	/** If actor has valid Owner, call Owner's IsNetRelevantFor and GetNetPriority */
	UPROPERTY(Category=Replication, EditDefaultsOnly, BlueprintReadWrite)
	uint32 bNetUseOwnerRelevancy:1;

	/** If true, all input on the stack below this actor will not be considered */
	UPROPERTY(EditDefaultsOnly, Category=Input)
	uint32 bBlockInput:1;

private:
	/** Cached value whether this actor is relevant for navigation generation. Usually will be calculated in actor's constructor, but can be changed at runtime as well */
	uint32 bNavigationRelevant:1;

	/** True if this actor is currently running user construction script (used to defer component registration) */
	uint32 bRunningUserConstructionScript:1;

protected:
	// This actor will replicate to remote machines
	UPROPERTY(Category = Replication, EditDefaultsOnly, BlueprintReadOnly)
	uint32 bReplicates : 1;

	// This function should only be used in the constructor of classes that need to set the RemoteRole
	// to for backwards compatibility purposes
	void SetRemoteRoleForBackwardsCompat(const ENetRole InRemoteRole) { RemoteRole = InRemoteRole; }

private:
	// Describes how much control the remote machine has over the actor
	// old UPROPERTY maintained for backwards compatibility, new non-uproperty as proper value
	UPROPERTY(Replicated, transient)
	TEnumAsByte<enum ENetRole> RemoteRole;

public:

	/** Changes the RemoteRole property and handles the cases where the actor needs to be added to the network actor list */
	UFUNCTION(BlueprintCallable, Category = "Replication")
	void SetReplicates(bool bInReplicates);
	void SetAutonomousProxy(bool bInAutonomousProxy);
	void CopyRemoteRoleFrom(const AActor* CopyFromActor);

	ENetRole GetRemoteRole() const;

	//Describes how much control the local machine has over the actor
	UPROPERTY(Replicated)
	TEnumAsByte<enum ENetRole> Role;

	/** Dormancy setting for actor taking itself off of the replication list without being destroyed on clients */
	TEnumAsByte<enum ENetDormancy>	NetDormancy;

	/** Register actor to receive input from a player */
	UPROPERTY(EditAnywhere, Category=Input)
	TEnumAsByte<EAutoReceiveInput::Type> AutoReceiveInput;

	UPROPERTY()
	class UInputComponent* InputComponent;

	UPROPERTY()
	TEnumAsByte<enum EInputConsumeOptions> InputConsumeOption_DEPRECATED;

	/**  Square of the max distance from the client's viewpoint that this actor is relevant and will be replicated. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category=Replication)
	float NetCullDistanceSquared;   

	/** Internal - used by UWorld::ServerTickClients() */
	UPROPERTY(transient)
	int32 NetTag;

	/** Next time this actor will be considered for replication, set by SetNetUpdateTime() */
	UPROPERTY()
	float NetUpdateTime;

	/** How often (per second) this actor will be considered for replication, used to determine NetUpdateTime */
	UPROPERTY(Category=Replication, EditDefaultsOnly, BlueprintReadWrite)
	float NetUpdateFrequency;

	/** Priority for this actor when checking for replication in a low bandwidth or saturated situation, higher priority means it is more likely to replicate */
	UPROPERTY(Category=Replication, EditDefaultsOnly, BlueprintReadWrite)
	float NetPriority;

	/** Last time this actor was updated for replication via NetUpdateTime
	 * @warning: internal net driver time, not related to WorldSettings.TimeSeconds */
	UPROPERTY(transient)
	float LastNetUpdateTime;

	/** Used to specify the net driver to replicate on (NAME_None || NAME_GameNetDriver is the default net driver) */
	UPROPERTY()
	FName NetDriverName;

	/** Method that allows an actor to replicate subobjects on its actor channel */
	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags);

	/** Called on the actor when a new subobject is dynamically created via replication */
	virtual void OnSubobjectCreatedFromReplication(UObject *NewSubobject);

	/** Called on the actor when a new subobject is dynamically created via replication */
	virtual void OnSubobjectDestroyFromReplication(UObject *NewSubobject);

	/** Called on the actor right before replication occurs */
	virtual void PreReplication( IRepChangedPropertyTracker & ChangedPropertyTracker );

	/** True to destroy when "finished", meaning all relevant components report that they are done and no timelines or timers are in flight. */
	UPROPERTY(BlueprintReadWrite, Category=Actor)
	uint32 bAutoDestroyWhenFinished:1;

	/** Can take damage. Must be true for damage events (e.g. ReceiveDamage) to be called */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Replicated, Category=Actor)
	uint32 bCanBeDamaged:1;    

	/** set when actor is about to be deleted (since endstate and other functions called during deletion process before MarkPendingKill is called). See IsPendingKillPending(). */
	UPROPERTY()
	uint32 bPendingKillPending:1;    

	/** This actor collides with the world when placing, even if RootComponent collision is disabled */
	UPROPERTY()
	uint32 bCollideWhenPlacing:1;    

	/** Should this actor search for an owned camera component to view thru when used as a view target? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Actor, AdvancedDisplay)
	uint32 bFindCameraComponentWhenViewTarget:1;

	/** Pawn responsible for damage caused by this actor. */
	UPROPERTY(replicatedUsing=OnRep_Instigator)
	class APawn* Instigator;      

	/** The time this actor was created, relative to World->GetTimeSeconds() */
	float CreationTime;    

	/** Array of Actors whose Owner is this actor */
	UPROPERTY(transient)
	TArray<class AActor*> Children;    
	
	// Animation update rate control.
public:
	/** Unique Tag assigned to spread updates of SkinnedMeshes over time. */
	UPROPERTY(Transient)
	uint32 AnimUpdateRateShiftTag;

	/** Frame counter to call AnimUpdateRateTick() just once per frame. */
	UPROPERTY(Transient)
	uint32 AnimUpdateRateFrameCount;

	/** Animation Update Rate optimization parameters. */
	UPROPERTY(Transient)
	struct FAnimUpdateRateParameters AnimUpdateRateParams;

	/** Aimation Update Rate Tick. */
	void AnimUpdateRateTick();

	/** Updates AnimUpdateRateParams, used by SkinnedMeshComponents.
	 * @param bRecentlyRendered : true if at least one SkinnedMeshComponent on this Actor has been rendered in the last second.
	 * @param MaxDistanceFactor : Largest SkinnedMeshComponent of this Actor drawn on screen. */
	void AnimUpdateRateSetParams(const bool & bRecentlyRendered, const float & MaxDistanceFactor, const bool & bPlayingRootMotion);

protected:
	// Collision primitive.
	UPROPERTY()
	class USceneComponent* RootComponent;

	/** The matinee actors that control this actor **/
	UPROPERTY(transient)
	TArray<class AMatineeActor*> ControllingMatineeActors;

	/** How long this Actor lives before dying, 0=forever. Note this is the INITIAL value and should not be set once play has begun */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Actor)
	float InitialLifeSpan; 

	/** If false, the Blueprint ReceiveTick event will be disabled on dedicated servers. */
	UPROPERTY()
	uint32 bAllowReceiveTickEventOnDedicatedServer:1;

public:

	/** Return the value of bAllowReceiveTickEventOnDedicatedServer, indicating whether the Blueprint ReceiveTick event will occur on dedicated servers */
	FORCEINLINE bool AllowReceiveTickEventOnDedicatedServer() const { return bAllowReceiveTickEventOnDedicatedServer; }

protected:

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	uint32 bActorLabelEditable:1;    // Is the actor label editable by the user?

private:
	/** The friendly name for this actor, displayed in the editor.  You should always use AActor::GetActorLabel() to access the actual label to display,
		and call AActor::SetActorLabel() or AActor::SetActorLabelUnique() to change the label.  Never set the label directly. */
	UPROPERTY()
	FString ActorLabel;


public:
	/** Is hidden within the editor at its startup. */
	UPROPERTY()
	uint32 bHiddenEd:1;    

protected:
	/** Whether the actor can be manipulated by editor operations. */
	UPROPERTY()
	uint32 bEditable:1;    

	/** True if this actor should be listed in the scene outliner */
	UPROPERTY()
	uint32 bListedInSceneOutliner:1;    

public:
	/** Is hidden by the layer browser. */
	UPROPERTY()
	uint32 bHiddenEdLayer:1;    

private:
	/** Is temporarily hidden within the editor; used for show/hide/etc. functionality w/o dirtying the actor */
	UPROPERTY(transient)
	uint32 bHiddenEdTemporary:1;    

public:

	/** Is hidden by the level browser. */
	UPROPERTY(transient)
	uint32 bHiddenEdLevel:1;    

	/** Prevent the actor from being moved in the editor. */
	UPROPERTY()
	uint32 bLockLocation:1;    

	// Actor's layer name.
	UPROPERTY()
	FName Layer_DEPRECATED;

	/** Layer's the actor belongs to */
	UPROPERTY()
	TArray< FName > Layers;

	UPROPERTY()
	FName Group_DEPRECATED;

	/** The group this actor is a part of. */
	UPROPERTY(transient)
	class AActor* GroupActor;

	/** The Actor that owns the ChildActorComponent that owns this Actor */
	UPROPERTY()
	TWeakObjectPtr<class AActor> ParentComponentActor;	

	/** The scale to apply to any editor billboard components */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Rendering, meta=(DisplayName="Editor Sprite Scale"))
	float SpriteScale;

	/** Returns how many lights are uncached for this actor */
	int32 GetNumUncachedLights();
#endif // WITH_EDITORONLY_DATA

#if ENABLE_VISUAL_LOG
	/** indicates where given actor will be writing its visual log data. Defaults to 'this' */
	const AActor* VLogRedirection;
#endif // ENABLE_VISUAL_LOG

public:

	/** 
	 *	Indicates that PreInitializeComponents/PostInitializeComponents have been called on this Actor 
	 *	Prevents re-initializing of actors spawned during level startup
	 */
	UPROPERTY()
	uint32 bActorInitialized:1;

	/** Indicates the actor was pulled through a seamless travel.  */
	UPROPERTY()
	uint32 bActorSeamlessTraveled:1;

	/** Whether this actor should no be affected by world origin shifting  */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Actor)
	uint32 bIgnoresOriginShifting:1;
	
	/** Array of tags that can be used for grouping and categorizing. Can also be accessed from scripting */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=Actor)
	TArray<FName> Tags;

	/** Bitflag to represent which views this actor is hidden in, via per-view layer visibility */
	UPROPERTY(transient)
	uint64 HiddenEditorViews;

	//==============================================================================================
	// Delegates
	
	/** Called when the actor is damaged in any way. */
	UPROPERTY(BlueprintAssignable, Category="Game|Damage")
	FTakeAnyDamageSignature OnTakeAnyDamage;

	/** Called when the actor is damaged by point damage */
	UPROPERTY(BlueprintAssignable, Category="Game|Damage")
	FTakePointDamageSignature OnTakePointDamage;
	
	/** Called when another actor begins to overlap this actor */
	UPROPERTY(BlueprintAssignable, Category="Collision")
	FActorBeginOverlapSignature OnActorBeginOverlap;

	/** Called when another actor ends overlap with this actor */
	UPROPERTY(BlueprintAssignable, Category="Collision")
	FActorEndOverlapSignature OnActorEndOverlap;

	/** Called when the mouse cursor is moved over this actor when this mouse interface is enabled and mouse over events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Mouse Input")
	FActorBeginCursorOverSignature OnBeginCursorOver;

	/** Called when the mouse cursor is moved off this actor when this mouse interface is enabled and mouse over events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Mouse Input")
	FActorEndCursorOverSignature OnEndCursorOver;

	/** Called when the left mouse button is clicked while the mouse is over this actor when the mouse interface is enabled and click events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Mouse Input")
	FActorOnClickedSignature OnClicked;

	/** Called when the left mouse button is released while the mouse is over this actor when the mouse interface is enabled and click events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Mouse Input")
	FActorOnReleasedSignature OnReleased;

	/** Called when a touch input is received over this actor when and click events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Touch Input")
	FActorOnInputTouchBeginSignature OnInputTouchBegin;
		
	/** Called when a touch input is received over this component when and click events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Touch Input")
	FActorOnInputTouchEndSignature OnInputTouchEnd;

	/** Called when a finger is moved over this actor when this mouse interface is enabled and touch over events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Touch Input")
	FActorBeginTouchOverSignature OnInputTouchEnter;

	/** Called when a finger is moved off this actor when this mouse interface is enabled and touch over events are enabled */
	UPROPERTY(BlueprintAssignable, Category="Input|Touch Input")
	FActorEndTouchOverSignature OnInputTouchLeave;

	/** Called when this actor is involved in a blocking collision */
	UPROPERTY(BlueprintAssignable, Category="Collision")
	FActorHitSignature OnActorHit;

	/** List of replicated components. Built at runtime */
	TArray< TWeakObjectPtr<class UActorComponent> >	ReplicatedComponents;

	/** Called on client when Instigator is replicated (only if bReplicateInstigator is true) */
	UFUNCTION()
	virtual void OnRep_Instigator();

	/** 
	 * Pushes this actor on to the stack of input being handled by the player controller
	 * @param PlayerController - The player controller whose input events we are interested in receiving
	 */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void EnableInput(class APlayerController* PlayerController);

	/** 
	 * Removes this actor from the stack of input being handled by the player controller
	 * @param PlayerController - The player controller whose input events we are no longer interested in receiving.  If NULL, actor will be removed from all player controllers
	 */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DisableInput(class APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", BlueprintProtected = "true", HidePin="InputAxisName", ToolTip="Gets the value of the input axis if input is enabled for this actor"))
	float GetInputAxisValue(const FName InputAxisName) const;

	/** Returns the instigator for this actor, or NULL if there is none. */
	UFUNCTION(BlueprintCallable, meta=(BlueprintProtected = "true"), Category="Game|Damage")
	APawn* GetInstigator() const;

	/**
	 * Get the instigator, given a specific class
	 * @return The instigator for this weapon, if it is the specified type. NULL, otherwise
	 */
	template <class T>
	T* GetInstigator() const { return Cast<T>(Instigator); };

	/** Returns the instigator's controller for this actor, or NULL if there is none. */
	UFUNCTION(BlueprintCallable, meta=(BlueprintProtected = "true"), Category="Game|Damage")
	AController* GetInstigatorController() const;


	//=============================================================================
	// General functions.
	
	/** Get the class of this Actor*/
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, FriendlyName = "GetActorClass"), Category="Class")
	class UClass* GetActorClass() const;

	/** Get the actor to world transform */
	UFUNCTION(BlueprintCallable, meta=(FriendlyName = "GetActorTransform"), Category="Utilities|Orientation")
	FTransform GetTransform() const;

	/** Returns location of the RootComponent of this Actor*/
	UFUNCTION(BlueprintCallable, meta=(FriendlyName = "GetActorLocation"), Category="Utilities|Orientation")
	FVector K2_GetActorLocation() const;

	/** 
	 *	Move the Actor instantly to the specified location. 
	 *	@param NewLocation	The new location to teleport the Actor to.
	 *	@param bSweep		Should we sweep to the destination location. If true, will stop short of the target if blocked by something.
	 *	@return	Whether the location was successfully set if not swept, or whether movement occurred if swept.
	 */
	UFUNCTION(BlueprintCallable, meta=(FriendlyName = "SetActorLocation"), Category="Utilities|Orientation")
	bool K2_SetActorLocation(FVector NewLocation, bool bSweep=false);

	/** Returns rotation of the RootComponent of this Actor */
	UFUNCTION(BlueprintCallable, meta=(FriendlyName = "GetActorRotation"), Category="Utilities|Orientation")
	FRotator K2_GetActorRotation() const;

	/** Get the forward (X) vector (length 1.0) from this Actor, in world space.  */
	UFUNCTION(BlueprintCallable, Category = "Utilities|Orientation")
	FVector GetActorForwardVector() const;

	/** Get the up (Z) vector (length 1.0) from this Actor, in world space.  */
	UFUNCTION(BlueprintCallable, Category = "Utilities|Orientation")
	FVector GetActorUpVector() const;

	/** Get the right (Y) vector (length 1.0) from this Actor, in world space.  */
	UFUNCTION(BlueprintCallable, Category = "Utilities|Orientation")
	FVector GetActorRightVector() const;

	/**
	 *	Returns the bounding box of all components that make up this Actor.
	 *	@param	bOnlyCollidingComponents	If TRUE, will only return the bounding box for components with collision enabled.
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(FriendlyName = "GetActorBounds"))
	void GetActorBounds(bool bOnlyCollidingComponents, FVector& Origin, FVector& BoxExtent) const;

	/** Returns the RootComponent of this Actor */
	UFUNCTION(BlueprintCallable, meta=(FriendlyName = "GetRootComponent"), Category="Utilities|Orientation")
	class USceneComponent* K2_GetRootComponent() const;

	/** Returns velocity (in cm/s (Unreal Units/second) of the rootcomponent if it is either using physics or has an associated MovementComponent */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	virtual FVector GetVelocity() const;

	/** 
	 *	Move the Actor instantly to the specified location. 
	 *	@param NewLocation	The new location to teleport the Actor to.
	 *	@param bSweep		Should we sweep to the destination location. If true, will stop short of the target if blocked by something.
	 *	@return	Whether the location was successfully set if not swept, or whether movement occurred if swept.
	 */
	bool SetActorLocation(const FVector& NewLocation, bool bSweep=false);

	/** 
	 *	Set the Actor's rotation instantly to the specified rotation.
	 *	@param	NewRotation	The new rotation for the Actor.
	 *	@return	Whether the rotation was successfully set.
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	bool SetActorRotation(FRotator NewRotation);

	/** 
	 *	Set the Actor's location and rotation instantly to the specified location and rotation.
	 *	@param NewLocation	The new location to teleport the Actor to.
	 *	@param NewRotation	The new rotation for the Actor.
	 *	@param bSweep		Should we sweep to the destination location. If true, will stop short of the target if blocked by something.
	 *	@return	Whether the rotation was successfully set.
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	bool SetActorLocationAndRotation(const FVector& NewLocation, FRotator NewRotation, bool bSweep=false);

	/** Returns the distance from this Actor to OtherActor. */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	float GetDistanceTo(AActor* OtherActor);

	/** Returns the distance from this Actor to OtherActor, ignoring Z. */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	float GetHorizontalDistanceTo(AActor* OtherActor);

	/** Returns the distance from this Actor to OtherActor, ignoring XY. */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	float GetVerticalDistanceTo(AActor* OtherActor);

	/** 
	 *	Set the Actors transform to the specified one.
	 *	@param bSweep		Should we sweep to the destination location. If true, will stop short of the target if blocked by something.
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	bool SetActorTransform(const FTransform& NewTransform, bool bSweep=false);

	/** Adds a delta to the location of this component in its local reference frame */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	void AddActorLocalOffset(FVector DeltaLocation, bool bSweep=false);

	/** Adds a delta to the rotation of this component in its local reference frame */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	void AddActorLocalRotation(FRotator DeltaRotation, bool bSweep=false);

	/** Adds a delta to the transform of this component in its local reference frame */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	void AddActorLocalTransform(const FTransform& NewTransform, bool bSweep=false);

	/**
	 * Set the actor's RootComponent to the specified relative location
	 * @param NewRelativeLocation	New relative location to set the actor's RootComponent to
	 * @param bSweep				Should we sweep to the destination location. If true, will stop short of the target if blocked by something
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	void SetActorRelativeLocation(FVector NewRelativeLocation, bool bSweep=false);

	/**
	 * Set the actor's RootComponent to the specified relative rotation
	 * @param NewRelativeRotation		New relative rotation to set the actor's RootComponent to
	 * @param bSweep					Should we sweep to the destination rotation. If true, will stop short of the target if blocked by something
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	void SetActorRelativeRotation(FRotator NewRelativeRotation, bool bSweep=false);

	/**
	 * Set the actor's RootComponent to the specified relative transform
	 * @param NewRelativeTransform		New relative transform to set the actor's RootComponent to
	 * @param bSweep					Should we sweep to the destination transform. If true, will stop short of the target if blocked by something
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	void SetActorRelativeTransform(const FTransform& NewRelativeTransform, bool bSweep=false);

	/**
	 * Set the actor's RootComponent to the specified relative scale 3d
	 * @param NewRelativeScale	New scale to set the actor's RootComponent to
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Orientation")
	void SetActorRelativeScale3D(FVector NewRelativeScale);

	/**
	 *	Sets the actor to be hidden in the game
	 *	@param	bNewHidden	Whether or not to hide the actor and all its components
	 */
	UFUNCTION(BlueprintCallable, Category="Rendering", meta=( FriendlyName = "Set Actor Hidden In Game", Keywords = "Visible Hidden Show Hide" ))
	virtual void SetActorHiddenInGame(bool bNewHidden);

	/** Allows enabling/disabling collision for the whole actor */
	UFUNCTION(BlueprintCallable, Category="Collision")
	void SetActorEnableCollision(bool bNewActorEnableCollision);

	/** Get current state of collision for the whole actor */
	UFUNCTION(BlueprintPure, Category="Collision")
	bool GetActorEnableCollision();

	/** Destroy the actor */
	UFUNCTION(BlueprintCallable, Category="Utilities", meta=(Keywords = "Delete", FriendlyName = "DestroyActor"))
	virtual void K2_DestroyActor();

	/** Returns whether this actor has network authority */
	UFUNCTION(BlueprintCallable, Category="Networking")
	bool HasAuthority() const;

	/** 
	 * Create a new component given a template name. Template is found in the owning Blueprint.
	 * Automatic attachment causes the first component created to become the root component, and all subsequent components
	 * will be attached to the root component.  In manual mode, it is up to the user to attach or set as root.
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly = "true", DefaultToSelf="ComponentTemplateContext", HidePin="ComponentTemplateContext"))
	class UActorComponent* AddComponent(FName TemplateName, bool bManualAttachment, const FTransform& RelativeTransform, const UObject* ComponentTemplateContext);

	/** DEPRECATED - Use Component::DestroyComponent */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage = "Use Component.DestroyComponent instead", BlueprintProtected = "true", FriendlyName = "DestroyComponent"))
	void K2_DestroyComponent(UActorComponent* Component);

	/** 
	 *  Attaches the RootComponent of this Actor to the supplied component, optionally at a named socket. It is not valid to call this on components that are not Registered. 
	 *   @param AttachLocationType	Type of attachment, AbsoluteWorld to keep its world position, RelativeOffset to keep the object's relative offset and SnapTo to snap to the new parent.
	 */
	UFUNCTION(BlueprintCallable, meta=(FriendlyName = "AttachActorToComponent", AttachLocationType="KeepRelativeOffset"), Category="Utilities|Orientation")
	void AttachRootComponentTo(class USceneComponent* InParent, FName InSocketName = NAME_None, EAttachLocation::Type AttachLocationType = EAttachLocation::KeepRelativeOffset);

	/**
	 * Attaches the RootComponent of this Actor to the RootComponent of the supplied actor, optionally at a named socket.
	 * @param InParentActor				Actor to attach this actor's RootComponent to
	 * @param InSocketName				Socket name to attach to, if any
	 * @param AttachLocationType	Type of attachment, AbsoluteWorld to keep its world position, RelativeOffset to keep the object's relative offset and SnapTo to snap to the new parent.
	 */
	UFUNCTION(BlueprintCallable, meta=(FriendlyName = "AttachActorToActor", AttachLocationType="KeepRelativeOffset"), Category="Utilities|Orientation")
	void AttachRootComponentToActor(AActor* InParentActor, FName InSocketName = NAME_None, EAttachLocation::Type AttachLocationType = EAttachLocation::KeepRelativeOffset);

	/** 
	 *  Snap the RootComponent of this Actor to the supplied Actor's root component, optionally at a named socket. It is not valid to call this on components that are not Registered. 
	 *  If InSocketName == NAME_None, it will attach to origin of the InParentActor. 
	 */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage = "Use AttachRootComponentTo with EAttachLocation::SnapToTarget option instead", FriendlyName = "SnapActorTo"), Category="Utilities|Orientation")
	void SnapRootComponentTo(AActor* InParentActor, FName InSocketName = NAME_None);

	/** 
	 *  Detaches the RootComponent of this Actor from any SceneComponent it is currently attached to. 
	 *   @param bMaintainWorldTransform	If true, update the relative location/rotation of this component to keep its world position the same
	 */
	UFUNCTION(BlueprintCallable, meta=(FriendlyName = "DetachActorFromActor"), Category="Utilities|Orientation")
	void DetachRootComponentFromParent(bool bMaintainWorldPosition = true);

	/** 
	 *	Detaches all SceneComponents in this Actor from the supplied parent SceneComponent. 
	 *	@param InParentComponent		SceneComponent to detach this actor's components from
	 *	@param bMaintainWorldTransform	If true, update the relative location/rotation of this component to keep its world position the same
	 */
	void DetachSceneComponentsFromParent(class USceneComponent* InParentComponent, bool bMaintainWorldPosition = true);

	//==============================================================================
	// Tags

	/** See if this actor contains the supplied tag */
	UFUNCTION(BlueprintCallable, Category="Utilities")
	bool ActorHasTag(FName Tag) const;


	//==============================================================================
	// Misc Blueprint support

	/** Getter for the cached world pointer */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction))
	UWorld* K2_GetWorld() const;

	/** 
	 * Get CustomTimeDilation - this can be used for input control or speed control for slomo
	 *  We don't want to scale input globally because input can be used for UI, which do not care for TimeDilation 
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|Time")
	float GetActorTimeDilation() const;

	/** Adds a dependent actor, so DependentActor ticks before this actor. */
	UFUNCTION(BlueprintCallable, Category="Utilities", meta=(FriendlyName = "SetTickDependency"))
	void SetTickPrerequisite(AActor* DependentActor);

	UFUNCTION(BlueprintCallable, Category="Utilities")
	void SetTickableWhenPaused(bool bTickableWhenPaused);

	/** Allocate a MID for a given parent material. */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage="Use PrimitiveComponent.CreateAndSetMaterialInstanceDynamic instead.", BlueprintProtected = "true"), Category="Rendering|Material")
	class UMaterialInstanceDynamic* MakeMIDForMaterial(class UMaterialInterface* Parent);

	//=============================================================================
	// Sound functions.
	
	/* DEPRECATED - Use UGameplayStatics::PlaySoundAttached */
	UFUNCTION(BlueprintCallable, Category="Audio", meta=(DeprecatedFunction, VolumeMultiplier="1.0", PitchMultiplier="1.0"))
	void PlaySoundOnActor(class USoundCue* InSoundCue, float VolumeMultiplier=1.f, float PitchMultiplier=1.f);

	/* DEPRECATED - Use UGameplayStatics::PlaySoundAtLocation */
	UFUNCTION(BlueprintCallable, Category="Audio", meta=(DeprecatedFunction, VolumeMultiplier="1.0", PitchMultiplier="1.0"))
	void PlaySoundAtLocation(class USoundCue* InSoundCue, FVector SoundLocation, float VolumeMultiplier=1.f, float PitchMultiplier=1.f);

	//=============================================================================
	// AI functions.
	
	/**
	 * Trigger a noise caused by a given Pawn, at a given location.
	 * Note that the NoiseInstigator Pawn MUST have a PawnNoiseEmitterComponent for the noise to be detected by a PawnSensingComponent.
	 * Senders of MakeNoise should have an Instigator if they are not pawns, or pass a NoiseInstigator.
	 *
	 * @param Loudness - is the relative loudness of this noise (range 0.0 to 1.0).  Directly affects the hearing range specified by the SensingComponent's HearingThreshold.
	 * @param NoiseInstigator - Pawn responsible for this noise.  Uses the actor's Instigator if NoiseInstigator=NULL
	 * @param NoiseLocation - Position of noise source.  If zero vector, use the actor's location.
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Audio", meta=(BlueprintProtected = "true"))
	void MakeNoise(float Loudness=1.f, APawn* NoiseInstigator=NULL, FVector NoiseLocation=FVector::ZeroVector);

	//=============================================================================
	// Blueprint
	
	/** Event when play begins for this actor */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "BeginPlay"))
	virtual void ReceiveBeginPlay();

	virtual void BeginPlay();

	/** Event when this actor takes ANY damage */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, meta=(FriendlyName = "AnyDamage"), Category="Damage")
	virtual void ReceiveAnyDamage(float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, class AActor* DamageCauser);
	
	/** 
	 * Event when this actor takes RADIAL damage 
	 * @todo Pass it the full array of hits instead of just one?
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, meta=(FriendlyName = "RadialDamage"), Category="Damage")
	virtual void ReceiveRadialDamage(float DamageReceived, const class UDamageType* DamageType, FVector Origin, const struct FHitResult& HitInfo, class AController* InstigatedBy, class AActor* DamageCauser);

	/** Event when this actor takes POINT damage */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, meta=(FriendlyName = "PointDamage"), Category="Damage")
	virtual void ReceivePointDamage(float Damage, const class UDamageType* DamageType, FVector HitLocation, FVector HitNormal, class UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, class AController* InstigatedBy, class AActor* DamageCauser);

	/** Event called every frame */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "Tick"))
	virtual void ReceiveTick(float DeltaSeconds);

	/** Event when this actor overlaps another actor. */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "ActorBeginOverlap"), Category="Collision")
	virtual void ReceiveActorBeginOverlap(class AActor* OtherActor);

	/** Event when an actor no longer overlaps another actor, and they have separated. */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "ActorEndOverlap"), Category="Collision")
	virtual void ReceiveActorEndOverlap(class AActor* OtherActor);

	/** Event when this actor has the mouse moved over it with the clickable interface. */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "ActorBeginCursorOver"), Category="Mouse Input")
	virtual void ReceiveActorBeginCursorOver();

	/** Event when this actor has the mouse moved off of it with the clickable interface. */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "ActorEndCursorOver"), Category="Mouse Input")
	virtual void ReceiveActorEndCursorOver();

	/** Event when this actor is clicked by the mouse when using the clickable interface. */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "ActorOnClicked"), Category="Mouse Input")
	virtual void ReceiveActorOnClicked();

	/** Event when this actor is under the mouse when left mouse button is released while using the clickable interface. */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "ActorOnReleased"), Category="Mouse Input")
	virtual void ReceiveActorOnReleased();

	/** Event when this actor is touched when click events are enabled. */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "BeginInputTouch"), Category="Touch Input")
	virtual void ReceiveActorOnInputTouchBegin(const ETouchIndex::Type FingerIndex);

	/** Event when this actor is under the finger when untouched when click events are enabled. */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "EndInputTouch"), Category="Touch Input")
	virtual void ReceiveActorOnInputTouchEnd(const ETouchIndex::Type FingerIndex);

	/** Event when this actor has a finger moved over it with the clickable interface. */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "TouchEnter"), Category="Touch Input")
	virtual void ReceiveActorOnInputTouchEnter(const ETouchIndex::Type FingerIndex);

	/** Event when this actor has a finger moved off of it with the clickable interface. */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "TouchLeave"), Category="Touch Input")
	virtual void ReceiveActorOnInputTouchLeave(const ETouchIndex::Type FingerIndex);

	/** Event when keys/touches/tilt/etc happen */
	UFUNCTION(BlueprintImplementableEvent, meta=(DeprecatedFunction))
	virtual void ReceiveInput(const FString& InputName, float Value, FVector VectorValue, bool bStarted, bool bEnded);

	/** 
	 * Returns list of actors this actor is overlapping (any component overlapping any component). Does not return itself.
	 * @param OverlappingActors		[out] Returned list of overlapping actors
	 * @param ClassFilter			[optional] If set, only returns actors of this class or subclasses
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(UnsafeDuringActorConstruction="true"))
	void GetOverlappingActors(TArray<AActor*>& OverlappingActors, UClass* ClassFilter=NULL) const;

	/** Returns list of components this actor is overlapping. */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(UnsafeDuringActorConstruction="true"))
	void GetOverlappingComponents(TArray<UPrimitiveComponent*>& OverlappingComponents) const;

	/** Event when this actor bumps into a blocking object, or blocks another actor that bumps into it. */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "Hit"), Category="Collision")
	virtual void ReceiveHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit);

	/** Set the lifespan of this actor. When it expires the object will be destroyed. If requested lifespan is 0, the timer is cleared and the actor will not be destroyed. */
	UFUNCTION(BlueprintCallable, Category="Utilities", meta=(Keywords = "delete"))
	virtual void SetLifeSpan( float InLifespan );

	/** Get the remaining lifespan of this actor. If zero is returned the actor lives forever. */
	UFUNCTION(BlueprintCallable, Category="Utilities", meta=(Keywords = "delete"))
	virtual float GetLifeSpan() const;

	/**
	 * Construction script, the place to spawn components and do other setup.
	 * @note Name used in CreateBlueprint function
	 * @param	Location	The location.
	 * @param	Rotation	The rotation.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta=(BlueprintInternalUseOnly = "true", FriendlyName = "Construction Script"))
	virtual void UserConstructionScript();

	/*
	 * Destroy this actor. Returns true if destroyed, false if indestructible.
	 * Destruction is latent. It occurs at the end of the tick.
	 * @param	bNetForce				[opt] Ignored unless called during play.  Default is false.
	 * @param	bShouldModifyLevel		[opt] If true, Modify() the level before removing the actor.  Default is true.	
	 * returns	the state of the RF_PendingKill flag
	 */
	bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true );

	/** Event to notify blueprints this actor is about to be deleted. */
	UFUNCTION(BlueprintImplementableEvent, meta=(Keywords = "delete", FriendlyName = "Destroyed"))
	virtual void ReceiveDestroyed();

	/** Called when the actor is destroyed. */
	UPROPERTY(BlueprintAssignable)
	FActorDestroyedSignature OnDestroyed;
	

	// Begin UObject Interface
	virtual void CheckActorComponents() OVERRIDE;
	virtual void PostInitProperties() OVERRIDE;
	virtual bool Modify( bool bAlwaysMarkDirty=true ) OVERRIDE;
	virtual void ProcessEvent( UFunction* Function, void* Parameters ) OVERRIDE;
	virtual int32 GetFunctionCallspace( UFunction* Function, void* Parameters, FFrame* Stack ) OVERRIDE;
	virtual bool CallRemoteFunction( UFunction* Function, void* Parameters, FFrame* Stack ) OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	virtual void PostLoadSubobjects( FObjectInstancingGraph* OuterInstanceGraph ) OVERRIDE;
	virtual void BeginDestroy() OVERRIDE;
	virtual bool IsReadyForFinishDestroy() OVERRIDE;
	virtual bool Rename( const TCHAR* NewName=NULL, UObject* NewOuter=NULL, ERenameFlags Flags=REN_None ) OVERRIDE;
#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyThatWillChange) OVERRIDE;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void PostEditUndo() OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished);
#endif // WITH_EDITOR

	//-----------------------------------------------------------------------------------------------
	// PROPERTY REPLICATION

	/** fills ReplicatedMovement property */
	virtual void GatherCurrentMovement();

	//
	// See if this actor is owned by TestOwner.
	//
	inline bool IsOwnedBy( const AActor* TestOwner ) const
	{
		for( const AActor* Arg=this; Arg; Arg=Arg->Owner )
		{
			if( Arg == TestOwner )
				return true;
		}
		return false;
	}

	/** Returns location of the RootComponent 
		this is a template for no other reason than to delay compilation until USceneComponent is defined
	*/ 
	template<class T>
	static 
#if !defined(_MSC_VER) || _MSC_VER != 1600  //VS210 bug 
		FORCEINLINE
#endif
	FVector GetActorLocation(T* RootComponent)
	{
		FVector Result(0.f);
		if( RootComponent != NULL )
		{
			Result = RootComponent->GetComponentLocation();
		}
		return Result;
	}

	/** Returns rotation of the RootComponent 
		this is a template for no other reason than to delay compilation until USceneComponent is defined
	*/ 
	template<class T>
	static 
#if !defined(_MSC_VER) || _MSC_VER != 1600  //VS210 bug 
		FORCEINLINE
#endif
	FRotator GetActorRotation(T* RootComponent)
	{
		FRotator Result(0,0,0);
		if( RootComponent != NULL )
		{
			Result = RootComponent->GetComponentRotation();
		}
		return Result;
	}

	/** Returns scale of the RootComponent 
		this is a template for no other reason than to delay compilation until USceneComponent is defined
	*/ 
	template<class T>
	static 
#if !defined(_MSC_VER) || _MSC_VER != 1600  //VS210 bug 
		FORCEINLINE
#endif
	FVector GetActorScale(T* RootComponent)
	{
		FVector Result(1,1,1);
		if( RootComponent != NULL )
		{
			Result = RootComponent->GetComponentScale();
		}
		return Result;
	}

	/** Returns quaternion of the RootComponent
		this is a template for no other reason than to delay compilation until USceneComponent is defined
	 */ 
	template<class T>
	static 
#if !defined(_MSC_VER) || _MSC_VER != 1600  //VS210 bug 
		FORCEINLINE
#endif
	FQuat GetActorQuat(T* RootComponent)
	{
		FQuat Result(ForceInit);
		if( RootComponent != NULL )
		{
			Result = RootComponent->GetComponentQuat();
		}
		return Result;
	}

	/** Returns this actor's root component. */
	FORCEINLINE class USceneComponent* GetRootComponent() const { return RootComponent; }

	/** Returns this actor's root component cast to a primitive component */
	class UPrimitiveComponent* GetRootPrimitiveComponent() const;

	/** Sets root component to be the specified component.  NewRootComponent's owner should be this actor.  Returns true if successful. */
	bool SetRootComponent(class USceneComponent* NewRootComponent);

	/** Returns location of the RootComponent of this Actor*/ 
	FORCEINLINE FVector GetActorLocation() const
	{
		return GetActorLocation(RootComponent);
	}

	/** Returns rotation of the RootComponent of this Actor */
	FORCEINLINE FRotator GetActorRotation() const
	{
		return GetActorRotation(RootComponent);
	}

	/** Returns scale of the RootComponent of this Actor */
	FORCEINLINE FVector GetActorScale() const
	{
		return GetActorScale(RootComponent);
	}

	/** Returns quaternion of the RootComponent of this Actor */
	FORCEINLINE FQuat GetActorQuat() const
	{
		return GetActorQuat(RootComponent);
	}

/*-----------------------------------------------------------------------------
	Relations.
-----------------------------------------------------------------------------*/

	/** 
	 * Called by owning level to shift an actor location and all relevant data structures by specified delta
	 *  
	 * @param InWorldOffset	 Offset vector to shift actor location
	 * @param bWorldShift	 Whether this call is part of whole world shifting
	 */
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift);

	/** 
	 * Indicates whether this actor should participate in level bounds calculations
	 */
	virtual bool IsLevelBoundsRelevant() const { return true; }

#if WITH_EDITOR
	// Editor specific

	/** @todo: Remove this flag once it is decided that additive interactive scaling is what we want */
	static bool bUsePercentageBasedScaling;

	/**
	 * Called by ApplyDeltaToActor to perform an actor class-specific operation based on widget manipulation.
	 * The default implementation is simply to translate the actor's location.
	 */
	virtual void EditorApplyTranslation(const FVector& DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown);

	/**
	 * Called by ApplyDeltaToActor to perform an actor class-specific operation based on widget manipulation.
	 * The default implementation is simply to modify the actor's rotation.
	 */
	virtual void EditorApplyRotation(const FRotator& DeltaRotation, bool bAltDown, bool bShiftDown, bool bCtrlDown);

	/**
	 * Called by ApplyDeltaToActor to perform an actor class-specific operation based on widget manipulation.
	 * The default implementation is simply to modify the actor's draw scale.
	 */
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown);

	/** Called by MirrorActors to perform a mirroring operation on the actor */
	virtual void EditorApplyMirror(const FVector& MirrorScale, const FVector& PivotLocation);

	/**
	 * Simple accessor to check if the actor is hidden upon editor startup
	 * @return	true if the actor is hidden upon editor startup; false if it is not
	 */
	bool IsHiddenEdAtStartup() const
	{
		return bHiddenEd;
	}

	// Returns true if this actor is hidden in the editor viewports.
	bool IsHiddenEd() const;

	/**
	 * Sets whether or not this actor is hidden in the editor for the duration of the current editor session
	 *
	 * @param bIsHidden	True if the actor is hidden
	 */
	virtual void SetIsTemporarilyHiddenInEditor( bool bIsHidden );

	/**
	 * @return Whether or not this actor is hidden in the editor for the duration of the current editor session
	 */
	bool IsTemporarilyHiddenInEditor() const { return bHiddenEdTemporary; }

	/** @return	Returns true if this actor is allowed to be displayed, selected and manipulated by the editor. */
	bool IsEditable() const;

	/** @return	Returns true if this actor should be shown in the scene outliner */
	bool IsListedInSceneOutliner() const;

	// Called before editor copy, true allow export
	virtual bool ShouldExport() { return true; }

	// Called before editor paste, true allow import
	virtual bool ShouldImport(FString* ActorPropString, bool IsMovingLevel) { return true; }

	// For UUnrealEdEngine::UpdatePropertyWindows()
	virtual bool GetSelectedComponents(TArray<UObject*>& SelectedObjects) { return false; }

	/** Called by InputKey when an unhandled key is pressed with a selected actor */
	virtual void EditorKeyPressed(FKey Key, EInputEvent Event) {}

	/** Called by ReplaceSelectedActors to allow a new actor to copy properties from an old actor when it is replaced */
	virtual void EditorReplacedActor(AActor* OldActor) {}

	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();

	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForDeprecated();

	/**
	 * Returns this actor's current label.  Actor labels are only available in development builds.
	 * @return	The label text
	 */
	const FString& GetActorLabel() const;

	/**
	 * Assigns a new label to this actor.  Actor labels are only available in development builds.
	 * @param	NewActorLabel	The new label string to assign to the actor.  If empty, the actor will have a default label.
	 */
	void SetActorLabel( const FString& NewActorLabel );

	/** Advanced - clear the actor label. */
	void ClearActorLabel();

	/**
	 * Returns if this actor's current label is editable.  Actor labels are only available in development builds.
	 * @return	The editable status of the actor's label
	 */
	bool IsActorLabelEditable() const;

	/**
	 * Used by the "Sync to Content Browser" right-click menu option in the editor.
	 * @param	Objects	Array to add content object references to.
	 * @return	Whether the object references content (all overrides of this function should return true)
	 */
	virtual bool GetReferencedContentObjects( TArray<UObject*>& Objects ) const;

	/** Returns NumUncachedStaticLightingInteractions for this actor */
	const int32 GetNumUncachedStaticLightingInteractions() const;

#endif		// WITH_EDITOR

	/**
	* @param ViewPos		Position of the viewer
	* @param ViewDir		Vector direction of viewer
	* @param Viewer			PlayerController owned by the client for whom net priority is being determined
	* @param InChannel		Channel on which this actor is being replicated.
	* @param Time			Time since actor was last replicated
	* @param bLowBandwidth True if low bandwith of viewer
	* @return				Priority of this actor for replication
	 */
	virtual float GetNetPriority(const FVector& ViewPos, const FVector& ViewDir, class APlayerController* Viewer, UActorChannel* InChannel, float Time, bool bLowBandwidth);

	virtual bool GetNetDormancy(const FVector& ViewPos, const FVector& ViewDir, class APlayerController* Viewer, UActorChannel* InChannel, float Time, bool bLowBandwidth);

	/** 
	 * Allows for a specific response from the actor when the actor channel is opened (client side)
	 * @param InBunch Bunch received at time of open
	 * @param Connection the connection associated with this actor
	 */
	virtual void OnActorChannelOpen(class FInBunch& InBunch, class UNetConnection* Connection) {};

	/**
	 * SerializeNewActor has just been called on the actor before network replication (server side)
	 * @param OutBunch Bunch containing serialized contents of actor prior to replication
	 */
	virtual void OnSerializeNewActor(class FOutBunch& OutBunch) {};

	/** 
	 * Handles cleaning up the associated Actor when killing the connection 
	 * @param Connection the connection associated with this actor
	 */
	virtual void OnNetCleanup(class UNetConnection* Connection) {};

	/** Swaps Role and RemoteRole if client */
	void ExchangeNetRoles(bool bRemoteOwner);

	/**
	 * When called, will call the virtual call chain to register all of the tick functions for both the actor and optionally all components
	 * Do not override this function or make it virtual
	 * @param bRegister - true to register, false, to unregister
	 * @param bDoComponents - true to also apply the change to all components
	 */
	void RegisterAllActorTickFunctions(bool bRegister, bool bDoComponents);

	/** 
	 * Set this actor's tick functions to be enabled or disabled. Only has an effect if the function is registered
	 * This only modifies the tick function on actor itself
	 * @param	bEnabled - Rather it should be enabled or not
	 */
	virtual void SetActorTickEnabled(bool bEnabled);

	/**  Returns whether this actor has tick enabled or not	 */
	bool IsActorTickEnabled() const;

	/**
	 *	ticks the actor
	 *	@param	DeltaTime			The time slice of this tick
	 *	@param	TickType			The type of tick that is happening
	 *	@param	ThisTickFunction	The tick function that is firing, useful for getting the completion handle
	 */
	virtual void TickActor( float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction );

	/**
	 * Called when an actor is done spawning into the world (from UWorld::SpawnActor).
	 * For actors with a root component, the location and rotation will have already been set.
	 * Takes place after any construction scripts have been called
	 */
	virtual void PostActorCreated();

	/** Called when the lifespawn of an actor expires (if he has one). */
	virtual void LifeSpanExpired();

	// Always called immediately before properties are received from the remote.
	virtual void PreNetReceive() OVERRIDE;
	
	// Always called immediately after properties are received from the remote.
	virtual void PostNetReceive() OVERRIDE;

	// Always called immediately after spawning and reading in replicated properties
	virtual void PostNetInit();

	/** ReplicatedMovement struct replication event */
	UFUNCTION()
	virtual void OnRep_ReplicatedMovement();

	/** Update and smooth location, not called for simulated physics! */
	virtual void PostNetReceiveLocation();

	/** Update velocity - typically from ReplicatedMovement, not called for simulated physics! */
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity);

	/** Update and smooth simulated physic state, replaces PostNetReceiveLocation() and PostNetReceiveVelocity() */
	virtual void PostNetReceivePhysicState();

	/** Set the owner for this Actor */
	void SetOwner( AActor* NewOwner );

	AActor* GetOwner() const { return Owner; }

	/**
	 * This will check to see if the Actor is still in the world.  It will check things like
	 * the KillZ, outside world bounds, etc. and handle the situation.
	 */
	virtual bool CheckStillInWorld();

	//--------------------------------------------------------------------------------------
	// Actor overlap tracking
	
	/** 
	 * Queries world and updates overlap detection state for this actor.
	 * @param bDoNotifies		True to dispatch being/end overlap notifications when these events occur.
	 */
	void UpdateOverlaps(bool bDoNotifies=true);
	
	/** 
	 * Check to see if current Actor is overlapping specified Actor
	 * @param Other the Actor to test for
	 * Returns true if any component of this actor is overlapping any component of Other. 
	 */
	bool IsOverlappingActor(const AActor* Other) const;

	/** Returns whether a MatineeActor is currently controlling this Actor */
	bool IsMatineeControlled() const;

	/** See if the root component has ModifyFrequency of MF_Static */
	bool IsRootComponentStatic() const;

	/** See if the root component has Mobility of EComponentMobility::Stationary */
	bool IsRootComponentStationary() const;

	/** See if the root component has Mobility of EComponentMobility::Movable */
	bool IsRootComponentMovable() const;

	//--------------------------------------------------------------------------------------
	// Actor ticking

	/** accessor for the value of bCanEverTick */
	FORCEINLINE bool CanEverTick() const { return PrimaryActorTick.bCanEverTick; }

	/** Called from main actor tick function to implement custom code at the appropriate point in the tick */
	virtual void Tick( float DeltaSeconds );

	/** If true, actor is ticked even if TickType==LEVELTICK_ViewportsOnly	 */
	virtual bool ShouldTickIfViewportsOnly() const;

	//--------------------------------------------------------------------------------------
	// Actor relevancy determination

	/** 
	  * @param RealViewer - is the PlayerController associated with the client for which network relevancy is being checked
	  * @param Viewer - is the Actor being used as the point of view for the PlayerController
	  * @param SrcLocation - is the viewing location
	  *
	  * @return bool - true if this actor is network relevant to the client associated with RealViewer 
	  */
	virtual bool IsNetRelevantFor(class APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation);

	/**
	 * Check if this actor is the owner when doing relevancy checks for actors marked bOnlyRelevantToOwner
	 *
	 * @param ReplicatedActor - the actor we're doing a relevancy test on
	 * @param ActorOwner - the owner of ReplicatedActor
	 * @param ConnectionActor - the controller of the connection that we're doing relevancy checks for
	 *
	 * @return bool - true if this actor should be considered the owner
	 */
	virtual bool IsRelevancyOwnerFor(AActor* ReplicatedActor, AActor* ActorOwner, AActor* ConnectionActor);

	/** Called after the actor is spawned in the world.  Responsible for setting up actor for play. */
	void PostSpawnInitialize(FVector const& SpawnLocation, FRotator const& SpawnRotation, AActor* InOwner, APawn* InInstigator, bool bRemoteOwned, bool bNoFail, bool bDeferConstruction);

	/** Called after the actor has run its construction. Responsible for finishing the actor spawn process. */
	void PostActorConstruction();

	/** Called immediately before gameplay begins. */
	virtual void PreInitializeComponents();

	// Allow actors to initialize themselves on the C++ side
	virtual void PostInitializeComponents();

	/**
	 * Adds a controlling matinee actor for use during matinee playback
	 * @param InMatineeActor	The matinee actor which controls this actor
	 * @todo UE4 would be nice to move this out of Actor to MatineeInterface, but it needs variable, and I can't declare variable in interface
	 *	do we still need this?
	 */
	void AddControllingMatineeActor( AMatineeActor& InMatineeActor );

	/**
	 * Removes a controlling matinee actor
	 * @param InMatineeActor	The matinee actor which currently controls this actor
	 */
	void RemoveControllingMatineeActor( AMatineeActor& InMatineeActor );

	/** Dispatches ReceiveHit virtual and OnComponentHit delegate */
	void DispatchPhysicsCollisionHit(const struct FRigidBodyCollisionInfo& MyInfo, const struct FRigidBodyCollisionInfo& OtherInfo, const FCollisionImpactData& RigidCollisionData);
	
	/** @return the owning UPlayer (if any) of this actor. This will be a local player, a net connection, or NULL. */
	virtual class UPlayer* GetNetOwningPlayer();

	/**
	 * Get the owning connection used for communicating between client/server 
	 * @return NetConnection to the client or server for this actor
	 */
	virtual class UNetConnection* GetNetConnection();

	ENetMode GetNetMode() const;

	class UNetDriver * GetNetDriver() const;

	/** Puts actor in dormant networking state */
	void SetNetDormancy(ENetDormancy NewDormancy);

	/** Forces dormant actor to replicate but doesn't change NetDormancy state (i.e., they will go dormant again if left dormant) */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Networking")
	void FlushNetDormancy();

	/** Ensure that all the components in the Components array are registered */
	virtual void RegisterAllComponents();

	/** Called after all the components in the Components array are registered */
	virtual void PostRegisterAllComponents();

	/** Returns true if Actor has a registered root component */
	bool HasValidRootComponent();

	/** Unregister all currently registered components */
	virtual void UnregisterAllComponents();

	/** Called after all currently registered components are cleared */
	virtual void PostUnregisterAllComponents() {}

	/** Will reregister all components on this actor. Does a lot of work - should only really be used in editor, generally use UpdateComponentTransforms or MarkComponentsRenderStateDirty. */
	virtual void ReregisterAllComponents();

	/** Flags all component's render state as dirty	 */
	void MarkComponentsRenderStateDirty();

	/** Update all components transforms */
	void UpdateComponentTransforms();

	/** Iterate over components array and call InitializeComponent */
	void InitializeComponents();

	/** Debug rendering to visualize the component tree for this actor. */
	void DrawDebugComponents(FColor const& BaseColor=FColor::White) const;

	virtual void MarkComponentsAsPendingKill();

	/** Returns true if this actor has begun the destruction process.
	 *  This is set to true in UWorld::DestroyActor, after the network connection has been closed but before any other shutdown has been performed.
	 *	@return true if this actor has begun destruction, or if this actor has been destroyed already.
	 **/
	inline bool IsPendingKillPending() const
	{
		return bPendingKillPending || IsPendingKill();
	}

	/** Invalidate lighting cache with default options. */
	void InvalidateLightingCache()
	{
		InvalidateLightingCacheDetailed(false);
	}

	/** Invalidates anything produced by the last lighting build. */
	virtual void InvalidateLightingCacheDetailed(bool bTranslationOnly);

	/**
	  * Used for adding actors to levels or teleporting them to a new location.
	  * The result of this function is independent of the actor's current location and rotation.
	  * If the actor doesn't fit exactly at the location specified, tries to slightly move it out of walls and such if bNoCheck is false.
	  *
	  * @param DestLocation The target destination point
	  * @param DestRotation The target rotation at the destination
	  * @param bIsATest is true if this is a test movement, which shouldn't cause any notifications (used by AI pathfinding, for example)
	  * @param bNoCheck is true if we should skip checking for encroachment in the world or other actors
	  * @return true if the actor has been successfully moved, or false if it couldn't fit.
	  */
	virtual bool TeleportTo( const FVector& DestLocation, const FRotator& DestRotation, bool bIsATest=false, bool bNoCheck=false );

	/**
	 * Teleport this actor to a new location. If the actor doesn't fit exactly at the location specified, tries to slightly move it out of walls and such.
	 *
	 * @param DestLocation The target destination point
	 * @param DestRotation The target rotation at the destination
	 * @return true if the actor has been successfully moved, or false if it couldn't fit.
	 */
	UFUNCTION(BlueprintCallable, meta=( FriendlyName="Teleport", Keywords = "Move Position" ), Category="Utilities|Orientation")
	bool K2_TeleportTo( FVector DestLocation, FRotator DestRotation );

	/** Called from TeleportTo() when teleport succeeds */
	virtual void TeleportSucceeded(bool bIsATest) {}

	/**
	 *  Trace a ray against the Components of this Actor and return the first blocking hit
	 *  @param  OutHit          First blocking hit found
	 *  @param  Start           Start location of the ray
	 *  @param  End             End location of the ray
	 *  @param  TraceChannel    The 'channel' that this ray is in, used to determine which components to hit
	 *  @param  Params          Additional parameters used for the trace
	 *  @return TRUE if a blocking hit is found
	 */
	bool ActorLineTraceSingle(struct FHitResult& OutHit, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params);

	/** 
	 * returns Distance to closest Body Instance surface. 
	 * Checks against all components of this Actor having valid collision and blocking TraceChannel.
	 *
	 * @param Point						World 3D vector
	 * @param TraceChannel				The 'channel' used to determine which components to consider.
	 * @param ClosestPointOnCollision	Point on the surface of collision closest to Point
	 * @param OutPrimitiveComponent		PrimitiveComponent ClosestPointOnCollision is on.
	 * 
	 * @return		Success if returns > 0.f, if returns 0.f, it is either not convex or inside of the point
	 *				If returns < 0.f, this Actor does not have any primitive with collision
	 */
	float ActorGetDistanceToCollision(const FVector& Point, ECollisionChannel TraceChannel, FVector& ClosestPointOnCollision, UPrimitiveComponent** OutPrimitiveComponent = NULL) const;

	/**
	 * Returns true if this actor is contained by TestLevel.
	 * @todo seamless: update once Actor->Outer != Level
	 */
	bool IsInLevel(const class ULevel *TestLevel) const;

	/** Return the ULevel that this Actor is part of. */
	ULevel* GetLevel() const;

	/**	Do anything needed to clear out cross level references; Called from ULevel::PreSave	 */
	virtual void ClearCrossLevelReferences();
	
	/** Called when this actor is in a level which is being removed from the world (e.g. my level is getting UWorld::RemoveFromWorld called on it) */
	virtual void OnRemoveFromWorld();

	/** iterates up the Base chain to see whether or not this Actor is based on the given Actor
	 * @param Other the Actor to test for
	 * @return true if this Actor is based on Other Actor
	 */
	virtual bool IsBasedOn( const AActor* Other ) const;

	/** Get the extent used when placing this actor in the editor, used for 'pulling back' hit. */
	FVector GetPlacementExtent() const;

	// Blueprint 

#if WITH_EDITOR
	/** Find all FRandomStream structs in this ACtor and generate new random seeds for them. */
	void SeedAllRandomStreams();
#endif // WITH_EDITOR

	/** Reset private properties to defaults, and all FRandomStream structs in this Actor, so they will start their sequence of random numbers again. */
	void ResetPropertiesForConstruction();

	/** Rerun construction scripts, destroying all autogenerated components; will attempt to preserve the root component location. */
	virtual void RerunConstructionScripts();

	/* 
	 * Debug helper to show the component hierarchy of this actor.
	 * @param Info	Optional String to display at top of info
	 */
	void DebugShowComponentHierarchy( const TCHAR* Info, bool bShowPosition  = true);
	
	/* 
	 * Debug helper for showing the component hierarchy of one component
	 * @param Info	Optional String to display at top of info
	 */
	void DebugShowOneComponentHierarchy( USceneComponent* SceneComp, int32& NestLevel, bool bShowPosition );

	/**
	 * Called when an instance of this class is placed (in editor) or spawned.
	 * @param	Transform	The transform to construct the actor at.
	 */
	virtual void OnConstruction(const FTransform& Transform);

	/**
	 * Helper function to regoster tje specified component, and add it to the serialized components array
	 * @param	Component	Component to be finalized
	 */
	void FinishAndRegisterComponent(UActorComponent* Component);

	/**  Util to create a component based on a template	 */
	UActorComponent* CreateComponentFromTemplate(UActorComponent* Template, const FString & InName = FString() );

	/** Destroys the constructed components. */
	void DestroyConstructedComponents();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	private:
	// this is the old name of the tick function. We just want to avoid mistakes with an attempt to override this
	virtual void Tick( float DeltaTime, enum ELevelTick TickType ) FINAL
	{
		check(0);
	}
#endif

protected:
	/**
	 * Virtual call chain to register all tick functions for the actor class hierarchy
	 * @param bRegister - true to register, false, to unregister
	 */
	virtual void RegisterActorTickFunctions(bool bRegister);

	/** Runs UserConstructionScript, delays component registration until it's complete. */
	void ProcessUserConstructionScript();

public:

	/** Walk up the attachment chain from RootComponent until we encounter a different actor, and return it. If we are not attached to a component in a different actor, returns NULL */
	virtual class AActor* GetAttachParentActor() const;

	/** Walk up the attachment chain from RootComponent until we encounter a different actor, and return the socket name in the component. If we are not attached to a component in a different actor, returns NAME_None */
	virtual FName GetAttachParentSocketName() const;

	/** Find all Actors which are attached directly to a component in this actor */
	virtual void GetAttachedActors(TArray<AActor*>& OutActors) const;

	/**
	 * Sets the ticking group for this actor.
	 * @param NewTickGroup the new value to assign
	 */
	void SetTickGroup(ETickingGroup NewTickGroup);

	/** Called once this actor has been deleted */
	virtual void Destroyed();

	/** Get the local-to-world transform of the RootComponent */
	FTransform ActorToWorld() const;

	/** Call ReceiveHit, as well as delegates on Actor and Component */
	void DispatchBlockingHit(UPrimitiveComponent* MyComp, UPrimitiveComponent* OtherComp, bool bSelfMoved, FHitResult const& Hit);

	/** called when the actor falls out of the world 'safely' (below KillZ and such) */
	virtual void FellOutOfWorld(const class UDamageType& dmgType);

	/** called when the Actor is outside the hard limit on world bounds */
	virtual void OutsideWorldBounds();

	/** 
	 *	Returns the bounding box of all components in this Actor.
	 *	@param bNonColliding Indicates that you want to include non-colliding components in the bounding box
	 */
	virtual FBox GetComponentsBoundingBox(bool bNonColliding = false) const;

	/* Get half-height/radius of a big axis-aligned cylinder around this actors registered colliding components, or all registered components if bNonColliding is false. */
	virtual void GetComponentsBoundingCylinder(float& CollisionRadius, float& CollisionHalfHeight, bool bNonColliding = false) const;

	/**
	 * Get axis-aligned cylinder around this actor, used for simple collision checks (ie Pawns reaching a destination).
	 * If IsRootComponentCollisionRegistered() returns true, just returns its bounding cylinder, otherwise falls back to GetComponentsBoundingCylinder.
	 */
	virtual void GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const;

	/** @returns the radius of the collision cylinder from GetSimpleCollisionCylinder(). */
	float GetSimpleCollisionRadius() const;

	/** @returns the half height of the collision cylinder from GetSimpleCollisionCylinder(). */
	float GetSimpleCollisionHalfHeight() const;

	/** @returns collision extents vector for this Actor, based on GetSimpleCollisionCylinder(). */
	FVector GetSimpleCollisionCylinderExtent() const;

	/** @returns true if the root component is registered and has collision enabled.  */
	virtual bool IsRootComponentCollisionRegistered() const;

	// Networking - called on client when actor is torn off (bTearOff==true)
	virtual void TornOff();

	//=============================================================================
	// Collision functions.
 
	/** 
	 * Get Collision Response to the Channel that entered for all components
	 * It returns Max of state - i.e. if Component A overlaps, but if Component B blocks, it will return block as response
	 * if Component A ignores, but if Component B overlaps, it will return overlap
	 *
	 * @param Channel - The channel to change the response of
	 */
	virtual ECollisionResponse GetComponentsCollisionResponseToChannel(ECollisionChannel Channel) const;

	//=============================================================================
	// Physics

	/** Stop all simulation from all components in this actor */
	void DisableComponentsSimulatePhysics();

public:
	/** @return WorldSettings for the World the actor is in
	 - if you'd like to know UWorld this placed actor (not dynamic spawned actor) belong to, use GetTypedOuter<UWorld>() **/
	class AWorldSettings* GetWorldSettings() const;

	/**
	 * Called from the Component's CanBeBaseForCharacter().
	 * @param APawn - The pawn that wants to be based on this actor
	 * @return bool - True if we don't want the pawn to bounce off
	 */
	virtual bool CanBeBaseForCharacter(class APawn* APawn) const;

	/** Apply damage to this actor.
	 * @param DamageAmount		How much damage to apply
	 * @param DamageEvent		Data package that fully describes the damage received.
	 * @param EventInstigator	The Controller responsible for the damage.
	 * @param DamageCauser		The Actor that directly caused the damage (e.g. the projectile that exploded, the rock that landed on you)
	 * @return					The amount of damage actually applied.
	 */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser);

protected:
	virtual float InternalTakeRadialDamage(float Damage, struct FRadialDamageEvent const& RadialDamageEvent, class AController* EventInstigator, class AActor* DamageCauser);
	virtual float InternalTakePointDamage(float Damage, struct FPointDamageEvent const& RadialDamageEvent, class AController* EventInstigator, class AActor* DamageCauser);
public:

	/* Called by Camera when this actor becomes its ViewTarget */
	virtual void BecomeViewTarget( class APlayerController* PC );

	/* Called by Camera when this actor is no longer its ViewTarget */
	virtual void EndViewTarget( class APlayerController* PC );

	/**
	 *	Calculate camera view point, when viewing this actor.
	 *
	 * @param	DeltaTime	Delta time seconds since last update
	 * @param	OutResult	Camera configuration
	 */
	virtual void CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult);

	// Returns the human readable string representation of an object.
	virtual FString GetHumanReadableName() const;

	/** Reset actor to initial state - used when restarting level without reloading. */
	virtual void Reset();

	/** Returns the most recent time any of this actor's components were rendered */
	virtual float GetLastRenderTime() const;

	/** Forces this actor to be net relevant if it is not already by default	 */
	virtual void ForceNetRelevant();

	/** Updates NetUpdateTime to the new value for future net relevancy checks */
	void SetNetUpdateTime(float NewUpdateTime);

	/** Force actor to be updated to clients */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Networking")
	virtual void ForceNetUpdate();

	/**
	 *	Calls PrestreamTextures() for all the actor's meshcomponents.
	 *	@param Seconds - Number of seconds to force all mip-levels to be resident
	 *	@param bEnableStreaming	- Whether to start (true) or stop (false) streaming
	 *	@param CinematicTextureGroups - Bitfield indicating which texture groups that use extra high-resolution mips
	 */
	virtual void PrestreamTextures( float Seconds, bool bEnableStreaming, int32 CinematicTextureGroups = 0 );

	/**
	 * returns the point of view of the actor.
	 * note that this doesn't mean the camera, but the 'eyes' of the actor.
	 * For example, for a Pawn, this would define the eye height location,
	 * and view rotation (which is different from the pawn rotation which has a zeroed pitch component).
	 * A camera first person view will typically use this view point. Most traces (weapon, AI) will be done from this view point.
	 *
	 * @param	OutLocation - location of view point
	 * @param	OutRotation - view rotation of actor.
	 */
	virtual void GetActorEyesViewPoint( FVector& OutLocation, FRotator& OutRotation ) const;

	/**
	 * @param RequestedBy - the Actor requesting the target location
	 * @return the optimal location to fire weapons at this actor
	 */
	virtual FVector GetTargetLocation(class AActor* RequestedBy = NULL) const;

	/** 
	  * Hook to allow actors to render HUD overlays for themselves.  Called from AHUD::DrawActorOverlays(). 
	  * @param PC is the PlayerController on whose view this overlay is rendered
	  * @param Canvas is the Canvas on which to draw the overlay
	  * @param CameraPosition Position of Camera
	  * @param CameraDir direction camera is pointing in.
	  */
	virtual void PostRenderFor(class APlayerController* PC, class UCanvas* Canvas, FVector CameraPosition, FVector CameraDir);

	/** whether this Actor is in the persistent level, i.e. not a sublevel */
	bool IsInPersistentLevel(bool bIncludeLevelStreamingPersistent = false) const;

	/** Getter for the cached world pointer */
	virtual UWorld* GetWorld() const OVERRIDE;

	/** Get the timer instance from the actors world */
	class FTimerManager& GetWorldTimerManager() const;

	/** Returns true if this is a replicated actor that was placed in the map */
	bool IsNetStartupActor() const;

	/** Searches components array and returns first encountered array by type. */
	virtual UActorComponent* FindComponentByClass(const UClass* Class) const;
	
	/** Templatized version for syntactic nicety. */
	template<class T>
	T* FindComponentByClass() const
	{
		return (T*) FindComponentByClass(T::StaticClass());
	}

	template<class T>
	void GetComponents(TArray<T*>& OutComponents) const
	{
		// This dummy line is to prevent the function from being used for classes outside the ActorComponent hierarchy
		UActorComponent* DummyAC = (T*)NULL; 

		SCOPE_CYCLE_COUNTER(STAT_GetComponentsTime);
		OutComponents.Empty();

		TArray<UObject*> ChildObjects;
		GetObjectsWithOuter(this, ChildObjects, false, RF_PendingKill);

		for (UObject* Child : ChildObjects)
		{
			T* const Component = Cast<T>(Child);
			if (Component)
			{
				OutComponents.Add(Component);
			}
		}
	}

public:
	//=============================================================================
	// Navigation related functions
	// 
	/** Updates bNavigationRelevant. Override for custom navigation behaviors	 */
	virtual bool UpdateNavigationRelevancy();

	/** Indicates whether this actor is to be considered by navigation system a valid actor	 */
	FORCEINLINE bool IsNavigationRelevant() const { return bNavigationRelevant; }

	/**	Sets bNavigationRelevant, and notifies navigation system if value has changed	 */
	void SetNavigationRelevancy(const bool bNewRelevancy);

	//=============================================================================
	// Debugging functions
public:
	/**
	 * Draw important Actor variables on canvas.  HUD will call DisplayDebug() on the current ViewTarget when the ShowDebug exec is used
	 *
	 * @param Canvas - Canvas to draw on
	 * @param DebugDisplay - List of names specifying what debug info to display
	 * @param YL - Height of the current font
	 * @param YPos - Y position on Canvas. YPos += YL, gives position to draw text for next debug line.
	 */
	virtual void DisplayDebug(class UCanvas* Canvas, const TArray<FName>& DebugDisplay, float& YL, float& YPos);

	/** Retrieves actor's name used for logging, or string "NULL" if Actor == NULL */
	static FString GetDebugName(const AActor* Actor) { return Actor ? Actor->GetName() : TEXT("NULL"); }

#if ENABLE_VISUAL_LOG
	/** 
	 *	Hook for Actors to supply visual logger with additional data.
	 *	It's guaranteed that Snapshot != NULL
	 */
	virtual void GrabDebugSnapshot(struct FVisLogEntry* Snapshot) const {}

	FORCEINLINE const AActor* GetVisualLogRedirection() const { return VLogRedirection; }
	void RedirectToVisualLog(const AActor* NewRedir);

private:
	friend class FVisualLog;
	void SetVisualLogRedirection(const AActor* NewRedir) { VLogRedirection = NewRedir; }
#endif // ENABLE_VISUAL_LOG

	//* Sets the friendly actor label and name */
	void SetActorLabelInternal( const FString& NewActorLabelDirty, bool bMakeGloballyUniqueFName );

	static FMakeNoiseDelegate MakeNoiseDelegate;
public:
	static void MakeNoiseImpl(AActor* NoiseMaker, float Loudness, APawn* NoiseInstigator, const FVector& NoiseLocation);
	static void SetMakeNoiseDelegate(const FMakeNoiseDelegate& NewDelegate);
};


//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE float AActor::GetSimpleCollisionRadius() const
{
	float Radius, HalfHeight;
	GetSimpleCollisionCylinder(Radius, HalfHeight);
	return Radius;
}

FORCEINLINE float AActor::GetSimpleCollisionHalfHeight() const
{
	float Radius, HalfHeight;
	GetSimpleCollisionCylinder(Radius, HalfHeight);
	return HalfHeight;
}

FORCEINLINE FVector AActor::GetSimpleCollisionCylinderExtent() const
{
	float Radius, HalfHeight;
	GetSimpleCollisionCylinder(Radius, HalfHeight);
	return FVector(Radius, Radius, HalfHeight);
}

//////////////////////////////////////////////////////////////////////////
// Macro to hide common Transform functions in native code for classes where they don't make sense.
// Note that this doesn't prevent access through function calls from parent classes (ie an AActor*), but
// does prevent use in the class that hides them and any derived child classes.

#define HIDE_ACTOR_TRANSFORM_FUNCTIONS() private: \
	FTransform GetTransform() const { return Super::GetTransform(); } \
	FVector GetActorLocation() const { return Super::GetActorLocation(); } \
	FRotator GetActorRotation() const { return Super::GetActorRotation(); } \
	FQuat GetActorQuat() const { return Super::GetActorQuat(); } \
	FVector GetActorScale() const { return Super::GetActorScale(); } \
	bool SetActorLocation(const FVector& NewLocation, bool bSweep=false) { return Super::SetActorLocation(NewLocation, bSweep); } \
	bool SetActorRotation(FRotator NewRotation) { return Super::SetActorRotation(NewRotation); } \
	bool SetActorLocationAndRotation(const FVector& NewLocation, FRotator NewRotation, bool bSweep=false) { return Super::SetActorLocationAndRotation(NewLocation, NewRotation, bSweep); } \
	virtual bool TeleportTo( const FVector& DestLocation, const FRotator& DestRotation, bool bIsATest, bool bNoCheck ) OVERRIDE { return Super::TeleportTo(DestLocation, DestRotation, bIsATest, bNoCheck); } \
	virtual FVector GetVelocity() const OVERRIDE { return Super::GetVelocity(); } \
	float GetDistanceTo(AActor* OtherActor) { return Super::GetDistanceTo(OtherActor); }

