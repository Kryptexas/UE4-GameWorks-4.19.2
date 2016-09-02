// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
// VRNotificationsComponent.h: Component to handle receiving notifications from VR HMD

#pragma once

#include "VRNotificationsComponent.generated.h"

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = HeadMountedDisplay)
class HEADMOUNTEDDISPLAY_API UVRNotificationsComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVRNotificationsDelegate);

	// This will be called on Morpheus if the HMD starts up and is not fully initialized (in NOT_STARTED or CALIBRATING states).  
	// The HMD will stay in NOT_STARTED until it is successfully position tracked.  Until it exits NOT_STARTED orientation
	// based reprojection does not happen.  Therefore we do not update rotation at all to avoid user discomfort.
	// Instructions to get the hmd tracked should be shown to the user.
	// Sony may fix this eventually.
	UPROPERTY(BlueprintAssignable)
	FVRNotificationsDelegate HMDTrackingInitializingAndNeedsHMDToBeTrackedDelegate;

	// This will be called on Morpheus when the HMD is done initializing and therefore
	// reprojection will start functioning.
	// The app can continue now.
	UPROPERTY(BlueprintAssignable)
	FVRNotificationsDelegate HMDTrackingInitializedDelegate;

	// This will be called when the application is asked for VR headset recenter.  
	UPROPERTY(BlueprintAssignable)
	FVRNotificationsDelegate HMDRecenteredDelegate;

	// This will be called when connection to HMD is lost.  
	UPROPERTY(BlueprintAssignable)
	FVRNotificationsDelegate HMDLostDelegate;

	// This will be called when connection to HMD is restored.  
	UPROPERTY(BlueprintAssignable)
	FVRNotificationsDelegate HMDReconnectedDelegate;

public:
	void OnRegister() override;
	void OnUnregister() override;

private:
	/** Native handlers that get registered with the actual FCoreDelegates, and then proceed to broadcast to the delegates above */
	void HMDTrackingInitializingAndNeedsHMDToBeTrackedDelegate_Handler()	{ HMDTrackingInitializingAndNeedsHMDToBeTrackedDelegate.Broadcast(); }
	void HMDTrackingInitializedDelegate_Handler()	{ HMDTrackingInitializedDelegate.Broadcast(); }
	void HMDRecenteredDelegate_Handler()	{ HMDRecenteredDelegate.Broadcast(); }
	void HMDLostDelegate_Handler()			{ HMDLostDelegate.Broadcast(); }
	void HMDReconnectedDelegate_Handler()	{ HMDReconnectedDelegate.Broadcast(); }
};



