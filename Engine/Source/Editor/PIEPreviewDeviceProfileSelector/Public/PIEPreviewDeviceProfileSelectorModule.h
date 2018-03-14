// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "IDeviceProfileSelectorModule.h"
#include "PIEPreviewDeviceEnumeration.h"
#include "RHIDefinitions.h"
#include "CommandLine.h"
#include "Widgets/SWindow.h"
#include "Private/PIEPreviewWindowCoreStyle.h"
#include "Private/PIEPreviewWindow.h"
#include "IPIEPreviewDeviceModule.h"
/**
* Implements the Preview Device Profile Selector module.
*/

class PIEPREVIEWDEVICEPROFILESELECTOR_API FPIEPreviewDeviceModule
	: public IPIEPreviewDeviceModule
{
public:
	FPIEPreviewDeviceModule() : bInitialized(false)
	{
	}

	//~ Begin IDeviceProfileSelectorModule Interface
	virtual const FString GetRuntimeDeviceProfileName() override;

	//~ End IDeviceProfileSelectorModule Interface

	//~ Begin IModuleInterface Interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~ End IModuleInterface Interface

	/**
	* Virtual destructor.
	*/
	virtual ~FPIEPreviewDeviceModule()
	{
	}

	virtual void ApplyPreviewDeviceState() override;
	
	virtual TSharedRef<SWindow> CreatePIEPreviewDeviceWindow(FVector2D ClientSize, FText WindowTitle, EAutoCenter AutoCenterType, FVector2D ScreenPosition, TOptional<float> MaxWindowWidth, TOptional<float> MaxWindowHeight) override;
	
	virtual const FPIEPreviewDeviceContainer& GetPreviewDeviceContainer() ;
	TSharedPtr<FPIEPreviewDeviceContainerCategory> GetPreviewDeviceRootCategory() const { return EnumeratedDevices.GetRootCategory(); }

	static bool IsRequestingPreviewDevice()
	{
		FString PreviewDeviceDummy;
		return FParse::Value(FCommandLine::Get(), GetPreviewDeviceCommandSwitch(), PreviewDeviceDummy);
	}

private:
	static const TCHAR* GetPreviewDeviceCommandSwitch()
	{
		return TEXT("MobileTargetDevice=");
	}

	const void GetPreviewDeviceResolution(int32& ScreenWidth, int32& ScreenHeight);

	void InitPreviewDevice();
	static FString GetDeviceSpecificationContentDir();
	bool ReadDeviceSpecification();
	ERHIFeatureLevel::Type GetPreviewDeviceFeatureLevel() const;
	FString FindDeviceSpecificationFilePath(const FString& SearchDevice);

	bool bInitialized;
	FString DeviceProfile;
	FString PreviewDevice;

	FPIEPreviewDeviceContainer EnumeratedDevices;
	TSharedPtr<struct FPIEPreviewDeviceSpecifications> DeviceSpecs;
};