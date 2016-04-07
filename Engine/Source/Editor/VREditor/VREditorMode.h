// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IVREditorMode.h"
#include "IVREditorModule.h"
#include "VirtualHand.h"
#include "HeadMountedDisplayTypes.h"	// For EHMDDeviceType::Type
#include "Kismet/HeadMountedDisplayFunctionLibrary.h"	// For EHMDTrackingOrigin::Type
#include "VREditorInputProcessor.h"

// Forward declare the GizmoHandleTypes that is defined in VREditorTransformGizmo
enum class EGizmoHandleTypes : uint8;

/**
 * VR Editor Mode.  Extends editor viewports with functionality for VR controls and object manipulation
 */
class FVREditorMode : public IVREditorMode
{
public:

	/** Default constructor for FVREditorMode, called when the editor starts up after the VREditor module is loaded */
	FVREditorMode();

	/** Cleans up this mode, called when the editor is shutting down */
	virtual ~FVREditorMode();

	/** Static: Gets the ID of our editor mode */
	static FEditorModeID GetVREditorModeID()
	{
		return VREditorModeID;
	}

	/** Static: Sets whether we should actually use an HMD.  Call this before activating VR mode */
	static void SetActuallyUsingVR( const bool bShouldActuallyUseVR )
	{
		bActuallyUsingVR = bShouldActuallyUseVR;
	}

	/** Returns true if we're actually using VR, or false if we're faking it */
	static bool IsActuallyUsingVR()
	{
		return bActuallyUsingVR;
	}

	/** Returns true if the user wants to exit this mode */
	bool WantsToExitMode() const
	{
		return bWantsToExitMode;
	}

	/** Call this to start exiting VR mode */
	void StartExitingVRMode()
	{
		bWantsToExitMode = true;
	}

	// FEdMode interface
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void Tick( FEditorViewportClient* ViewportClient, float DeltaTime ) override;
	virtual bool InputKey( FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event ) override;
	virtual bool InputAxis( FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime ) override;
	virtual bool IsCompatibleWith( FEditorModeID OtherModeID ) const override;
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
	virtual void Render( const FSceneView* SceneView, FViewport* Viewport, FPrimitiveDrawInterface* PDI ) override;	

	bool HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	bool HandleInputAxis(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime);

	// IVREditorMode interface
	virtual bool GetLaserPointer( const int32 HandIndex, FVector& LaserPointerStart, FVector& LaserPointerEnd, const bool bEvenIfUIIsInFront = false, const float LaserLengthOverride = 0.0f) const override;
	virtual bool IsInputCaptured( const FVRAction VRAction ) const;
	virtual AActor* GetAvatarMeshActor() override
	{
		return AvatarMeshActor;
	}
	virtual FOnVRAction& OnVRAction() override
	{
		return OnVRActionEvent;
	}
	virtual FOnVRHoverUpdate& OnVRHoverUpdate() override
	{
		return OnVRHoverUpdateEvent;
	}

	/** Returns the Unreal controller ID for the motion controllers we're using */
	int32 GetMotionControllerID() const
	{
		return MotionControllerID;
	}

	/** Returns the time since the VR Editor mode was last entered */
	inline FTimespan GetTimeSinceModeEntered() const
	{
		return FTimespan::FromSeconds( FApp::GetCurrentTime() ) - AppTimeModeEntered;
	}

	// @todo vreditor: Move this to FMath so we can use it everywhere
	// NOTE: OvershootAmount is usually between 0.5 and 2.0, but can go lower and higher for extreme overshots
	template<class T>
	static T OvershootEaseOut( T Alpha, const float OvershootAmount = 1.0f )
	{
		Alpha--;
		return 1.0f - ( ( Alpha * ( ( OvershootAmount + 1 ) * Alpha + OvershootAmount ) + 1 ) - 1.0f );
	}

	/** Gets the maximum length of a laser pointer */
	float GetLaserPointerMaxLength() const;

	/** Gets a virtual hand */
	FVirtualHand& GetVirtualHand( const int32 VirtualHandIndex )
	{
		check( VirtualHandIndex >= 0 && VirtualHandIndex < VREditorConstants::NumVirtualHands );
		return VirtualHands[ VirtualHandIndex ];
	}

	/** Gets a virtual hand (const) */
	const FVirtualHand& GetVirtualHand( const int32 VirtualHandIndex ) const
	{
		check( VirtualHandIndex >= 0 && VirtualHandIndex < VREditorConstants::NumVirtualHands );
		return VirtualHands[ VirtualHandIndex ];
	}

	/** Given an index of a hand, gets the index of the opposite hand */
	int32 GetOtherHandIndex( const int32 HandIndex ) const
	{
		check( HandIndex >= 0 && HandIndex < VREditorConstants::NumVirtualHands );
		const int32 OtherHandIndex = ( HandIndex == VREditorConstants::LeftHandIndex ) ? VREditorConstants::RightHandIndex : VREditorConstants::LeftHandIndex;
		return OtherHandIndex;
	}

	/** Gets the other hand, given a hand index */
	FVirtualHand& GetOtherHand( const int32 HandIndex )
	{
		return VirtualHands[ GetOtherHandIndex( HandIndex ) ];
	}

	/** Gets the other hand, given a hand index (const) */
	const FVirtualHand& GetOtherHand( const int32 HandIndex ) const
	{
		return VirtualHands[ GetOtherHandIndex( HandIndex ) ];
	}

	/** Gets the world space transform of the calibrated VR room origin.  When using a seated VR device, this will feel like the
	    camera's world transform (before any HMD positional or rotation adjustments are applied.) */
	FTransform GetRoomTransform() const;

	/** Sets a new transform for the room, in world space.  This is basically setting the editor's camera transform for the viewport */
	void SetRoomTransform( const FTransform& NewRoomTransform );

	/** Gets the transform of the user's HMD in room space */
	FTransform GetRoomSpaceHeadTransform() const;

	/** Gets the transform of the user's HMD in world space */
	FTransform GetHeadTransform() const;

	/** Spawns a transient actor that we can use in the editor world (templated for convenience) */
	template<class T>
	T* SpawnTransientSceneActor( const FString& ActorName, const bool bWithSceneComponent ) const
	{
		return CastChecked<T>( SpawnTransientSceneActor( T::StaticClass(), ActorName, bWithSceneComponent ) );
	}

	/** Spawns a transient actor that we can use in the editor world */
	AActor* SpawnTransientSceneActor( TSubclassOf<class AActor> ActorClass, const FString& ActorName, const bool bWithSceneComponent ) const;

	/** Destroys a transient actor we created earlier and nulls out the pointer */
	void DestroyTransientActor( AActor* Actor ) const;

	/**
	 * Traces along the laser pointer vector and returns what it first hits in the world
	 *
	 * @param HandIndex	Index of the hand that uses has the laser pointer to trace
	 * @param OptionalListOfIgnoredActors Actors to exclude from hit testing
	 * @param bIgnoreGizmos True if no gizmo results should be returned, otherwise they are preferred (x-ray)
	 * @param bEvenIfUIIsInFront If true, ignores any UI that might be blocking the ray
	 * @param LaserLengthOverride If zero the default laser length (VREdMode::GetLaserLength) is used
	 *
	 * @return What the laster pointer hit
	 */
	FHitResult GetHitResultFromLaserPointer( int32 HandIndex, TArray<AActor*>* OptionalListOfIgnoredActors = nullptr, const bool bIgnoreGizmos = false, const bool bEvenIfUIIsInFront = false, const float LaserLengthOverride = 0.0f );

	/** Triggers a force feedback effect on either hand */
	void PlayHapticEffect( const float LeftStrength, const float RightStrength );

	/** Gets access to the world interaction system (const) */
	const class FVREditorWorldInteraction& GetWorldInteraction() const
	{
		return *WorldInteraction;
	}

	/** Gets access to the world interaction system */
	class FVREditorWorldInteraction& GetWorldInteraction()
	{
		return *WorldInteraction;
	}

	/** Gets access to the VR UI system (const) */
	const class FVREditorUISystem& GetUISystem() const
	{
		return *UISystem;
	}

	/** Gets access to the VR UI system */
	class FVREditorUISystem& GetUISystem()
	{
		return *UISystem;
	}

	/** Gets the viewport that VR Mode is activated in.  Even though editor modes are available in all
	    level viewports simultaneously, only one viewport is "possessed" by the HMD.  Generally try to avoid using
		this function and instead use the ViewportClient that is passed around through various FEdMode overrides */
	const class SLevelViewport& GetLevelViewportPossessedForVR() const;

	/** Mutable version of above. */
	class SLevelViewport& GetLevelViewportPossessedForVR();

	/** Gets the world scale factor, which can be multiplied by a scale vector to convert to room space */
	float GetWorldScaleFactor() const
	{
		return GetWorld()->GetWorldSettings()->WorldToMeters / 100.0f;
	}

	/** Sets up our avatar mesh, if not already spawned */
	void SpawnAvatarMeshActor();

	/** Gets our VR post process component, creating it if it doesn't exist yet */
	UPostProcessComponent* GetPostProcessComponent();

	/** Gets the snap grid mesh actor */
	AActor* GetSnapGridActor()
	{
		SpawnAvatarMeshActor();
		return SnapGridActor;
	}

	/** Gets the snap grid mesh MID */
	class UMaterialInstanceDynamic* GetSnapGridMID()
	{
		SpawnAvatarMeshActor();
		return SnapGridMID;
	}

	/** Called internally when the user changes maps, enters/exits PIE or SIE, or switched between PIE/SIE */
	void CleanUpActorsBeforeMapChangeOrSimulate();

	/** Invokes the editor's undo system to undo the last change */
	void Undo();
	
	/** Redoes the last change */
	void Redo();

	/** Copies selected objects to the clipboard */
	void Copy();

	/** Pastes the clipboard contents into the scene */
	void Paste();

	/** Duplicates the selected objects */
	void Duplicate();

	/** Snaps the selected objects to the ground */
	void SnapSelectedActorsToGround();

	/** Sets the visuals of the LaserPointer */
	void SetLaserVisuals( const int32 HandIndex, const FLinearColor& NewColor, const float CrawlFade = 0.0f, const float CrawlSpeed = 0.0f );

	/** Given a world space velocity vector, applies inertial damping to it to slow it down */
	void ApplyVelocityDamping( FVector& Velocity, const bool bVelocitySensitive );

	/** Will update the TransformGizmo Actor with the next Gizmo type  */
	void CycleTransformGizmoHandleType();

	/** Gets the current Gizmo handle type */
	EGizmoHandleTypes GetCurrentGizmoType() const;

	/** Switch which trasform gizmo coordinate space we're using. */
	void CycleTransformGizmoCoordinateSpace();

	/** Returns which transform gizmo coordinate space we're using, world or local */
	ECoordSystem GetTransformGizmoCoordinateSpace() const;
		
	/** @return Returns the type of HMD we're dealing with */
	EHMDDeviceType::Type GetHMDDeviceType() const;

	/** @return Checks to see if the specified hand is aiming roughly toward the specified capsule */
	bool IsHandAimingTowardsCapsule( const int32 HandIndex, const FTransform& CapsuleTransform, const FVector CapsuleStart, const FVector CapsuleEnd, const float CapsuleRadius, const float MinDistanceToCapsule, const FVector CapsuleFrontDirection, const float MinDotForAimingAtCapsule ) const;
	
	/**
	 * Creates a hand transform and forward vector for a laser pointer for a given hand
	 *
	 * @param HandIndex			Index of the hand to use
	 * @param OutHandTransform	The created hand transform
	 * @param OutForwardVector	The forward vector of the hand
	 *
	 * @return	True if we have motion controller data for this hand and could return a valid result
	 */
	bool GetHandTransformAndForwardVector( int32 HandIndex, FTransform& OutHandTransform, FVector& OutForwardVector ) const;

	/** Returns the maximum user scale */
	float GetMaxScale();

	/** Returns the minimum user scale */
	float GetMinScale();

	/** Sets GNewWorldToMetersScale */
	void SetWorldToMetersScale( const float NewWorldToMetersScale );

private:

	/** Called when someone closes a standalone VR Editor window */
	void OnVREditorWindowClosed( const TSharedRef<SWindow>& ClosedWindow );

	/**
	 * Translates a raw key into a vr editing action
	 *
	 * @param InKey FKey to translate
	 * @return The action mapped to InKey
	 */
	int32 GetHandIndexFromKey( const FKey& InKey ) const;

	/** Conditionally polls input from controllers, if we haven't done that already this frame */
	void PollInputIfNeeded();

	/** Grabs latest data from the controllers and updates our virtual hands.  You should call PollInputIfNeeded() instead. */
	void PollInputFromMotionControllers();
	
	/** Stops playing any haptic effects that have been going for a while.  Called every frame. */
	void StopOldHapticEffects();

	/** Pops up some help text labels for the controller in the specified hand, or hides it, if requested */
	void ShowHelpForHand( const int32 HandIndex, const bool bShowIt );

	/** Called every frame to update the position of any floating help labels */
	void UpdateHelpLabels();

	/** Given a mesh and a key name, tries to find a socket on the mesh that matches a supported key */
	class UStaticMeshSocket* FindMeshSocketForKey( UStaticMesh* StaticMesh, const FKey Key );

	/** FEditorDelegates callbacks */
	void OnMapChange( uint32 MapChangeFlags );
	void OnBeginPIE( const bool bIsSimulatingInEditor );
	void OnEndPIE( const bool bIsSimulatingInEditor );
	void OnSwitchBetweenPIEAndSIE( const bool bIsSimulatingInEditor );

	/** Changes the color of the buttons on the handmesh */
	void ApplyButtonPressColors( FVRAction VRAction, EInputEvent Event );


protected:

	/** Static ID name for our editor mode */
	static FEditorModeID VREditorModeID;


	//
	// Startup/Shutdown
	//

	/** The VR editor window, if it's open right now */
	TWeakPtr< class SWindow > VREditorWindowWeakPtr;

	/** The VR level viewport, if we're in VR mode */
	TWeakPtr< class SLevelViewport > VREditorLevelViewportWeakPtr;

	/** Saved information about the editor and viewport we possessed, so we can restore it after exiting VR mode */
	struct FSavedEditorState
	{
		ELevelViewportType ViewportType;
		FVector ViewLocation;
		FRotator ViewRotation;
		FEngineShowFlags ShowFlags;
		bool bLockedPitch;
		bool bGameView;
		bool bAlwaysShowModeWidgetAfterSelectionChanges;
		float NearClipPlane;
		bool bRealTime;
		float DragTriggerDistance;
		bool bOnScreenMessages;
		EHMDTrackingOrigin::Type TrackingOrigin;

		FSavedEditorState()
			: ViewportType( LVT_Perspective ),
			  ViewLocation( FVector::ZeroVector ),
			  ViewRotation( FRotator::ZeroRotator ),
			  ShowFlags( ESFIM_Editor ),
			  bLockedPitch( false ),
			  bGameView( false ),
			  bAlwaysShowModeWidgetAfterSelectionChanges( false ),
			  NearClipPlane( 0.0f ),
			  bRealTime( false ),
			  DragTriggerDistance( 0.0f ),
			  bOnScreenMessages( false ),
			  TrackingOrigin( EHMDTrackingOrigin::Eye )
		{
		}
		
	} SavedEditorState;

	/** Static: True if we're in using an actual HMD in this mode, or false if we're "faking" VR mode for testing */
	static bool bActuallyUsingVR;

	/** True if we currently want to exit VR mode.  This is used to defer exiting until it is safe to do that */
	bool bWantsToExitMode;

	/** True if VR mode is fully initialized and ready to render */
	bool bIsFullyInitialized;

	/** App time that we entered this mode */
	FTimespan AppTimeModeEntered;


	//
	// Avatar visuals
	//

	/** Actor with components to represent the VR avatar in the world, including motion controller meshes */
	class AActor* AvatarMeshActor;

	/** Our avatar's head mesh */
	class UStaticMeshComponent* HeadMeshComponent;


	//
	// World movement grid & FX
	//

	/** The grid that appears while the user is dragging the world around */
	class UStaticMeshComponent* WorldMovementGridMeshComponent;

	/** Grid mesh component dynamic material instance to set the opacity */
	class UMaterialInstanceDynamic* WorldMovementGridMID;

	/** Opacity of the movement grid and post process */
	float WorldMovementGridOpacity;

	/** True if we're currently drawing our world movement post process */
	bool bIsDrawingWorldMovementPostProcess;

	/** Post process material for "greying out" the world while in world movement mode */
	class UMaterialInstanceDynamic* WorldMovementPostProcessMaterial;


	//
	// Snap grid
	//

	/** Actor for the snap grid */
	class AActor* SnapGridActor;

	/** The plane mesh we use to draw a snap grid under selected objects */
	class UStaticMeshComponent* SnapGridMeshComponent;

	/** MID for the snap grid, so we can update snap values on the fly */
	class UMaterialInstanceDynamic* SnapGridMID;


	//
	// World scaling progress bar
	//

	/** Background progressbar scaling mesh */
	class UStaticMeshComponent* ScaleProgressMeshComponent;

	/** Current scale progressbar mesh */
	class UStaticMeshComponent* CurrentScaleProgressMeshComponent;

	/** Current scale text */
	class UTextRenderComponent* UserScaleIndicatorText;


	//
	// Post process
	//

	/** Post process for drawing VR-specific post effects */
	class UPostProcessComponent* PostProcessComponent;

	//
	// Input
	//

	/** The Unreal controller ID for the motion controllers we're using */
	int32 MotionControllerID;

	/** The last frame that input was polled.  We keep track of this so that we can make sure to poll for latest input right before
	    its first needed each frame */
	uint32 LastFrameNumberInputWasPolled;

	/** Mapping of raw keys to actions*/
	TMap<FKey,FVRAction> KeyToActionMap;

	/** List of virtual hands being controlled */
	FVirtualHand VirtualHands[VREditorConstants::NumVirtualHands];

	/** Event that's triggered when the user presses a button on their motion controller device.  Set bWasHandled to true if you reacted to the event. */
	FOnVRAction OnVRActionEvent;

	/** Event that fires every frame to update hover state based on what's under the cursor.  Set bWasHandled to true if you detect something to hover. */
	FOnVRHoverUpdate OnVRHoverUpdateEvent;


	//
	// UI
	//

	/** VR UI system */
	TUniquePtr<class FVREditorUISystem> UISystem;


	//
	// World interaction
	//

	/** World interaction manager */
	TUniquePtr<class FVREditorWorldInteraction> WorldInteraction;

	/** The current Gizmo type that is used for the TransformGizmo Actor */
	EGizmoHandleTypes CurrentGizmoType;

	// Slate Input Processor
	TSharedPtr<FVREditorInputProcessor> InputProcessor;

	//
	// Colors
	//
public:
	// Color types
	enum EColors
	{
		DefaultColor,
		SelectionColor,
		TeleportColor,
		WorldDraggingColor_OneHand,
		WorldDraggingColor_TwoHands,
		RedGizmoColor,
		GreenGizmoColor,
		BlueGizmoColor,
		WhiteGizmoColor,
		HoverGizmoColor,
		DraggingGizmoColor,
		UISelectionBarColor,
		UISelectionBarHoverColor,
		UICloseButtonColor,
		UICloseButtonHoverColor,
		TotalCount
	};

	// Gets the color
	FLinearColor GetColor( const EColors Color ) const;

private:

	// All the colors for this mode
	TArray<FLinearColor> Colors;

	/** If this is the first tick or before */
	bool bFirstTick;
};

