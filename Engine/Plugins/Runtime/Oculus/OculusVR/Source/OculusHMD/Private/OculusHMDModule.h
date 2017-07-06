// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OculusHMDPrivate.h"
#include "IHeadMountedDisplay.h"
#include "OculusFunctionLibrary.h"


//-------------------------------------------------------------------------------------------------
// FOculusHMDModule
//-------------------------------------------------------------------------------------------------

class FOculusHMDModule : public IOculusHMDModule 
{
public:
	FOculusHMDModule();

	static inline FOculusHMDModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FOculusHMDModule >("OculusHMD");
	}

	// IModuleInterface
	virtual void ShutdownModule() override;


	// IHeadMountedDisplayModule
	virtual FString GetModuleKeyName() const override;
	virtual bool PreInit() override;
	virtual bool IsHMDConnected() override;
	virtual int GetGraphicsAdapter() override;
	virtual FString GetAudioInputDevice() override;
	virtual FString GetAudioOutputDevice() override;
	virtual TSharedPtr< IHeadMountedDisplay, ESPMode::ThreadSafe > CreateHeadMountedDisplay() override;

	// IOculusHMDModule
	virtual void GetPose(FRotator& DeviceRotation, FVector& DevicePosition, FVector& NeckPosition, bool bUseOrienationForPlayerCamera = false, bool bUsePositionForPlayerCamera = false, const FVector PositionScale = FVector::ZeroVector) override
	{
		UOculusFunctionLibrary::GetPose(DeviceRotation, DevicePosition, NeckPosition, bUseOrienationForPlayerCamera, bUsePositionForPlayerCamera, PositionScale);
	}

	virtual void GetRawSensorData(FVector& AngularAcceleration, FVector& LinearAcceleration, FVector& AngularVelocity, FVector& LinearVelocity, float& TimeInSeconds) override
	{
		UOculusFunctionLibrary::GetRawSensorData(ETrackedDeviceType::HMD, AngularAcceleration, LinearAcceleration, AngularVelocity, LinearVelocity, TimeInSeconds);
	}

	virtual bool GetUserProfile(struct FHmdUserProfile& Profile) override
	{
		return UOculusFunctionLibrary::GetUserProfile(Profile);
	}

	virtual void SetBaseRotationAndBaseOffsetInMeters(FRotator Rotation, FVector BaseOffsetInMeters, EOrientPositionSelector::Type Options) override
	{
		UOculusFunctionLibrary::SetBaseRotationAndBaseOffsetInMeters(Rotation, BaseOffsetInMeters, Options);
	}

	virtual void GetBaseRotationAndBaseOffsetInMeters(FRotator& OutRotation, FVector& OutBaseOffsetInMeters) override
	{
		UOculusFunctionLibrary::GetBaseRotationAndBaseOffsetInMeters(OutRotation, OutBaseOffsetInMeters);
	}

	virtual void SetBaseRotationAndPositionOffset(FRotator BaseRot, FVector PosOffset, EOrientPositionSelector::Type Options) override
	{
		UOculusFunctionLibrary::SetBaseRotationAndPositionOffset(BaseRot, PosOffset, Options);
	}

	virtual void GetBaseRotationAndPositionOffset(FRotator& OutRot, FVector& OutPosOffset) override
	{
		UOculusFunctionLibrary::GetBaseRotationAndPositionOffset(OutRot, OutPosOffset);
	}

	virtual class IStereoLayers* GetStereoLayers() override
	{
		return UOculusFunctionLibrary::GetStereoLayers();
	}

	bool IsOVRPluginAvailable() const
	{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
		return OVRPluginHandle != nullptr;
#else
		return false;
#endif
	}

#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OCULUSHMD_API static void* GetOVRPluginHandle();
	virtual bool PoseToOrientationAndPosition(const FQuat& InOrientation, const FVector& InPosition, FQuat& OutOrientation, FVector& OutPosition) const override;

protected:
	void SetGraphicsAdapter(const void* luid);

	bool bPreInit;
	bool bPreInitCalled;
	void *OVRPluginHandle;
	TWeakPtr< IHeadMountedDisplay, ESPMode::ThreadSafe > HeadMountedDisplay;
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
};
