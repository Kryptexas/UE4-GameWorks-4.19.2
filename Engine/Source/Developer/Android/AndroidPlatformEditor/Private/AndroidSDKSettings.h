// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ITargetPlatformManagerModule.h"
#include "IAndroidDeviceDetection.h"
#include "AndroidSDKSettings.generated.h"


/**
 * Implements the settings for the Android SDK setup.
 */
//UCLASS(config=Engine, defaultconfig)
UCLASS()
class ANDROIDPLATFORMEDITOR_API UAndroidSDKSettings : public UObject
{
public:
	GENERATED_UCLASS_BODY()

//		UPROPERTY(GlobalConfig, EditDefaultsOnly, Category = SDKConfig)
		UPROPERTY()
		FDirectoryPath SDKPath;

//	UPROPERTY(GlobalConfig, EditDefaultsOnly, Category = SDKConfig)
	UPROPERTY()
		FDirectoryPath NDKPath;

//	UPROPERTY(GlobalConfig, EditDefaultsOnly, Category = SDKConfig)
	UPROPERTY()
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
