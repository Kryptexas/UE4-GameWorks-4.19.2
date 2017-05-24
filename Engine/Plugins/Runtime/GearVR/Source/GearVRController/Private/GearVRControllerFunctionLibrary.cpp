// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
//

#include "../Public/GearVRControllerFunctionLibrary.h"
#include "GearVRController.h"

UGearVRControllerFunctionLibrary::UGearVRControllerFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}
#if GEARVR_SUPPORTED_PLATFORMS

FGearVRController* GetGearVRController()
{
	FGearVRController* GearVRController;
	TArray<IMotionController*> MotionControllers = IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>(IMotionController::GetModularFeatureName());
	for (auto MotionController : MotionControllers)
	{
		if (MotionController != nullptr)
		{
			GearVRController = static_cast<FGearVRController*>(MotionController);
			if (GearVRController != nullptr)
				return GearVRController;
		}
	}

	return nullptr;
}

#endif

EGearVRControllerHandedness UGearVRControllerFunctionLibrary::GetGearVRControllerHandedness()
{
#if GEARVR_SUPPORTED_PLATFORMS
	FGearVRController* GearVRController = GetGearVRController();
	if (GearVRController)
	{
		return GearVRController->isRightHanded ? EGearVRControllerHandedness::RightHanded : EGearVRControllerHandedness::LeftHanded;
	}
#endif
	return EGearVRControllerHandedness::Unknown;
}

void UGearVRControllerFunctionLibrary::EnableArmModel(bool bArmModelEnable)
{
#if GEARVR_SUPPORTED_PLATFORMS

	FGearVRController* GearVRController = GetGearVRController();
	if (GearVRController)
	{
		GearVRController->useArmModel = bArmModelEnable;
	}
#endif
}

bool UGearVRControllerFunctionLibrary::IsControllerActive()
{
#if GEARVR_SUPPORTED_PLATFORMS

	FGearVRController* GearVRController = GetGearVRController();
	if (GearVRController)
	{
		return GearVRController->GetControllerTrackingStatus(0, GearVRController->isRightHanded ? EControllerHand::Right : EControllerHand::Left) != ETrackingStatus::NotTracked;
	}
#endif
	return false;
}

