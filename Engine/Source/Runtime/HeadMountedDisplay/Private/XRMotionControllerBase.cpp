// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "XRMotionControllerBase.h"
#include "UObject/UObjectGlobals.h" // for FindObject<>
#include "UObject/Class.h" // for UEnum
#include "UObject/ObjectMacros.h" // for ANY_PACKAGE
#include "UObject/Package.h" // required for ANY_PACKAGE
#include "InputCoreTypes.h" // for EControllerHand

FName FXRMotionControllerBase::LeftHandSourceId(TEXT("Left"));
FName FXRMotionControllerBase::RightHandSourceId(TEXT("Right"));

bool FXRMotionControllerBase::GetControllerOrientationAndPosition(const int32 ControllerIndex, const FName MotionSource, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const
{
	bool bSucess = false;
	if (ControllerIndex != INDEX_NONE)
	{
		EControllerHand DeviceHand;
		if (GetHandEnumForSourceName(MotionSource, DeviceHand))
		{
			if (DeviceHand == EControllerHand::AnyHand)
			{
				bSucess = GetControllerOrientationAndPosition(ControllerIndex, EControllerHand::Left, OutOrientation, OutPosition, WorldToMetersScale);
				if (!bSucess)
				{
					bSucess = GetControllerOrientationAndPosition(ControllerIndex, EControllerHand::Right, OutOrientation, OutPosition, WorldToMetersScale);
				}
			}
			else
			{
				bSucess = GetControllerOrientationAndPosition(ControllerIndex, DeviceHand, OutOrientation, OutPosition, WorldToMetersScale);
			}
		}
	}
	return bSucess;
}

ETrackingStatus FXRMotionControllerBase::GetControllerTrackingStatus(const int32 ControllerIndex, const FName MotionSource) const
{
	if (ControllerIndex != INDEX_NONE)
	{
		EControllerHand DeviceHand;
		if (GetHandEnumForSourceName(MotionSource, DeviceHand))
		{
			if (DeviceHand == EControllerHand::AnyHand)
			{
				FRotator ThrowawayOrientation;
				FVector  ThrowawayPosition;
				// we've moved explicit handling of the 'AnyHand' source into this class from UMotionControllerComponent,
				// to maintain behavior we use the return value of GetControllerOrientationAndPosition() to determine which hand's
				// status we should check
				if (GetControllerOrientationAndPosition(ControllerIndex, EControllerHand::Left, ThrowawayOrientation, ThrowawayPosition, 100.f))
				{
					return GetControllerTrackingStatus(ControllerIndex, EControllerHand::Left);
				}
				return GetControllerTrackingStatus(ControllerIndex, EControllerHand::Right);
			}
			return GetControllerTrackingStatus(ControllerIndex, DeviceHand);
		}
	}
	return ETrackingStatus::NotTracked;
}

void FXRMotionControllerBase::EnumerateSources(TArray<FMotionControllerSource>& SourcesOut) const
{
	UEnum* HandEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EControllerHand"));
	if (HandEnum)
	{
		for (int64 HandVal = 0; HandVal < HandEnum->GetMaxEnumValue(); ++HandVal)
		{
			FString ValueName = HandEnum->GetNameStringByValue(HandVal);
			if (!ValueName.IsEmpty())
			{
				SourcesOut.Add(FName(*ValueName));
			}
		}
	}
}

bool FXRMotionControllerBase::GetHandEnumForSourceName(const FName Source, EControllerHand& OutHand)
{
	UEnum* HandEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EControllerHand"));
	if (HandEnum)
	{
		const int64 NameValue = HandEnum->GetValueByName(Source);
		if (NameValue != INDEX_NONE)
		{
			OutHand = (EControllerHand)NameValue;
			return true;
		}
	}
	return false;
}