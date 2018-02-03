// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ARTypes.h"
#include "ARTraceResult.h"
#include "ARSessionConfig.h"
#include "ARBlueprintLibrary.generated.h"


UCLASS(meta=(ScriptName="ARLibrary"))
class AUGMENTEDREALITY_API UARBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Begin a new Augmented Reality session. Subsequently, use the \c GetARSessionStatus() function to figure out the status of the session.
	 *
	 * @param SessionConfig    Describes the tracking method to use, what kind of geometry to detect in the world, etc.
	 */
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Session", meta = (Keywords = "ar augmentedreality augmented reality session start run running"))
	static void StartARSession(UARSessionConfig* SessionConfig);

	/** Pause a running Augmented Reality session without clearing existing state. */
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Session", meta = (Keywords = "ar augmentedreality augmented reality session stop run running"))
	static void PauseARSession();

	/** Stop a running Augmented Reality session and clear any state. */
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Session", meta = (Keywords = "ar augmentedreality augmented reality session stop run running"))
	static void StopARSession();
	
	/**
	 * It is intended that you check the status of the Augmented Reality session on every frame and take action accordingly.
	 * e.g. if the session stopped for an unexpected reason, you might give the user a prompt to re-start the session
	 *
	 * @return The status of a current Augmented Reality session: e.g. Running or Not running for a specific reason.
	 */
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Session", meta = (Keywords = "ar augmentedreality augmented reality session start stop run running"))
	static FARSessionStatus GetARSessionStatus();

	/** @return the configuration that the current session was started with. */
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Session", meta = (Keywords = "ar augmentedreality augmented reality session"))
	static UARSessionConfig* GetSessionConfig();
	
	/**
	 * Set a transform that will be applied to the tracking space. This effectively moves any camera
	 * possessed by the Augmented Reality system such that it is pointing at a different spot
	 * in Unreal's World Space. This is often done to support AR scenarios that rely on static
	 * geometry and/or lighting.
	 *
	 * Note: any movable components that are pinned will appear to stay in place, while anything
	 * not pinned or is not movable (static or stationary) will appear to move.
	 *
	 * \see PinComponent
	 * \see PinComponentToTraceResult
	 */
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Alignment", meta = (Keywords = "ar augmentedreality augmented reality tracking alignment"))
	static void SetAlignmentTransform( const FTransform& InAlignmentTransform );
	
	
	/**
	 * Perform a line trace against any real-world geometry as tracked by the AR system.
	 *
	 * @param ScreenCoord	         Coordinate of the point on the screen from which to cast a ray into the tracking space.
	 *
	 * @return a list of \c FARTraceResult sorted by distance from camera.
	 */
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Trace Result", meta = (AdvancedDisplay="1", Keywords = "ar augmentedreality augmented reality tracking tracing linetrace"))
	static TArray<FARTraceResult> LineTraceTrackedObjects( const FVector2D ScreenCoord, bool bTestFeaturePoints = true, bool bTestGroundPlane = true, bool bTestPlaneExtents = true, bool bTestPlaneBoundaryPolygon = true );
	
	
	/** @return how well the tracking system is performing at the moment */
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Tracking", meta = (Keywords = "ar augmentedreality augmented reality tracking quality"))
	static EARTrackingQuality GetTrackingQuality();
	
	/** @return a list of all the real-world geometry as currently seen by the Augmented Reality system */
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Tracking", meta = (Keywords = "ar augmentedreality augmented reality tracking geometry anchor"))
	static TArray<UARTrackedGeometry*> GetAllGeometries();
	
	/**
	 * Test whether this type of session is supported by the current Augmented Reality platform.
	 * e.g. is your device capable of doing positional tracking or orientation only?
	 */
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Tracking", meta = (Keywords = "ar augmentedreality augmented reality tracking"))
	static bool IsSessionTypeSupported(EARSessionType SessionType);
	
	
	/** Given some real-world geometry being tracked by the Augmented Reality system, draw it on the screen for debugging purposes (rudimentary)*/
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Debug", meta = (WorldContext = "WorldContextObject", AdvancedDisplay="1", Keywords = "ar augmentedreality augmented reality tracked geometry debug draw"))
	static void DebugDrawTrackedGeometry( UARTrackedGeometry* TrackedGeometry, UObject* WorldContextObject, FLinearColor Color = FLinearColor(1.0f, 1.0f, 0.0f, 0.75f), float OutlineThickness=5.0f, float PersistForSeconds = 0.0f );
	
	/** Given a \c UARPin, draw it for debugging purposes. */
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Debug", meta = (WorldContext = "WorldContextObject", AdvancedDisplay="1", Keywords = "ar augmentedreality augmented reality pin debug draw"))
	static void DebugDrawPin( UARPin* ARPin, UObject* WorldContextObject, FLinearColor Color = FLinearColor(1.0f, 1.0f, 0.0f, 0.75f), float Scale=5.0f, float PersistForSeconds = 0.0f );
	
	
	/**
	 * An AugmentedReality session can be configured to provide light estimates.
	 * The specific approach to light estimation can be configured by the \c UARSessionConfig
	 * specified during \c StartARSession(). This function assumes that you will cast
	 * the returned \c UARLightEstimate to a derived type corresponding to your
	 * session config.
	 *
	 * @return a \c UARLighEstimate that can be cast to a derived class.
	 */
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Light Estimate" )
	static UARLightEstimate* GetCurrentLightEstimate();
	
	
	/**
	 * Pin an Unreal Component to a location in tracking spce (i.e. the real world).
	 *
	 * @param ComponentToPin         The component that should be pinned.
	 * @param PinToWorldTransform    A transform (in Unreal World Space) that corresponds to
	 *                               a physical location where the component should be pinned.
	 * @param TrackedGeometry        An optional, real-world geometry that is recognized by the
	 *                               AR system; any correction to the position of this geometry
	 *                               will be applied to the pinned component.
	 * @param DebugName              An optional name that will be displayed when this
	 *                               pin is being drawn for debugging purposes.
	 *
	 * @return an object representing the pin that connects \c ComponentToPin component to a real-world
	 *         location and optionally to the \c TrackedGeometry.
	 */
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Pin", meta = (AdvancedDisplay="3", Keywords = "ar augmentedreality augmented reality tracking pin tracked geometry pinning anchor"))
	static UARPin* PinComponent( USceneComponent* ComponentToPin, const FTransform& PinToWorldTransform, UARTrackedGeometry* TrackedGeometry = nullptr, const FName DebugName = NAME_None );
	
	/**
	 * A convenient version of \c PinComponent() that can be used in conjunction
	 * with a result of a \c LineTraceTrackedObjects call.
	 */
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Pin", meta = (AdvancedDisplay="2", Keywords = "ar augmentedreality augmented reality tracking pin tracked geometry pinning anchor"))
	static UARPin* PinComponentToTraceResult( USceneComponent* ComponentToPin, const FARTraceResult& TraceResult, const FName DebugName = NAME_None );
	
	/** Given a pinned \c ComponentToUnpin, remove its attachment to the real world. */
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Pin", meta = (Keywords = "ar augmentedreality augmented reality tracking pin tracked geometry pinning anchor"))
	static void UnpinComponent( USceneComponent* ComponentToUnpin );
	
	/** Remove a pin such that it no longer updates the associated component. */
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Pin", meta = (Keywords = "ar augmentedreality augmented reality tracking pin tracked geometry pinning anchor"))
	static void RemovePin( UARPin* PinToRemove );
	
	/** Get a list of all the \c UARPin objects that the Augmented Reality session is currently using to connect virtual objects to real-world, tracked locations. */
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Pin", meta = (Keywords = "ar augmentedreality augmented reality tracking pin anchor"))
	static TArray<UARPin*> GetAllPins();
	
public:
	static void RegisterAsARSystem(const TSharedPtr<FARSystemBase, ESPMode::ThreadSafe>& NewArSystem);
	
private:
	static const TSharedPtr<FARSystemBase, ESPMode::ThreadSafe>& GetARSystem();
	static TSharedPtr<FARSystemBase, ESPMode::ThreadSafe> RegisteredARSystem;
};


UCLASS(meta=(ScriptName="ARLibrary"))
class AUGMENTEDREALITY_API UARTraceResultLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	public:
	
	/** @return  the distance from the camera to the traced location in Unreal Units. */
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static float GetDistanceFromCamera( const FARTraceResult& TraceResult );
	
	/**
	 * @return The transform of the trace result in tracking space (after it is modified by the \c AlignmentTransform).
	 *
	 * \see SetAlignmentTransform()
	 */
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static FTransform GetLocalToTrackingTransform( const FARTraceResult& TraceResult );
	
	/** @return Get the transform of the trace result in Unreal World Space. */
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static FTransform GetLocalToWorldTransform( const FARTraceResult& TraceResult );
	
	/** @return Get the real-world object (as observed by the Augmented Reality system) that was intersected by the line trace. */
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static UARTrackedGeometry* GetTrackedGeometry( const FARTraceResult& TraceResult );
	
	/** @return  Get the type of the tracked object (if any) that effected this trace result. */
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static EARLineTraceChannels GetTraceChannel( const FARTraceResult& TraceResult );
};
