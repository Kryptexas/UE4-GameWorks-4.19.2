// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ITargetPlatformManagerModule.h"
#include "IAndroidDeviceDetection.h"
#include "AndroidSDKSettings.generated.h"


/**
 * Implements the settings for the Android SDK setup.
 */
UCLASS(config=Engine, globaluserconfig)
class ANDROIDPLATFORMEDITOR_API UAndroidSDKSettings : public UObject
{
public:
	GENERATED_UCLASS_BODY()

	UPROPERTY(GlobalConfig, EditAnywhere, Category = SDKConfig, Meta = (DisplayName = "Location of Android SDK (the directory usually contains 'android-sdk-')"))
	FDirectoryPath SDKPath;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = SDKConfig, Meta = (DisplayName = "Location of Android NDK (the directory usually contains 'android-ndk-')"))
	FDirectoryPath NDKPath;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = SDKConfig, Meta = (DisplayName = "Location of ANT (the directory usually contains 'apache-ant-')"))
	FDirectoryPath ANTPath;

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface
	void SetTargetModule(ITargetPlatformManagerModule * TargetManagerModule);
	void SetDeviceDetection(IAndroidDeviceDetection * AndroidDeviceDetection);
	void SetupInitialTargetPaths();
	void UpdateTargetModulePaths(bool bForceUpdate);
	ITargetPlatformManagerModule * TargetManagerModule;
	IAndroidDeviceDetection * AndroidDeviceDetection;
#endif
};
