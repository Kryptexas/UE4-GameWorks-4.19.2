// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "XRMotionControllerBase.h"
#include "UObject/UObjectGlobals.h" // for FindObject<>
#include "UObject/Class.h" // for UEnum
#include "UObject/ObjectMacros.h" // for ANY_PACKAGE
#include "UObject/Package.h" // required for ANY_PACKAGE
#include "InputCoreTypes.h" // for EControllerHand

FName FXRMotionControllerBase::LeftHandSourceId(TEXT("Left"));
FName FXRMotionControllerBase::RightHandSourceId(TEXT("Right"));

namespace XRMotionControllerBase_Impl
{
	static const uint32 HandAliasCount = 2;
	// NOTE: indices should match up with the EControllerHand enum
	static const FName LegacyHandMappings[][HandAliasCount] = {
		{ FXRMotionControllerBase::LeftHandSourceId,  TEXT("EControllerHand::Left")           }, // EControllerHand::Left
		{ FXRMotionControllerBase::RightHandSourceId, TEXT("EControllerHand::Right")          }, // EControllerHand::Right
		{ TEXT("AnyHand"),                            TEXT("EControllerHand::AnyHand")        }, // EControllerHand::AnyHand
		{ TEXT("Pad"),                                TEXT("EControllerHand::Pad")            }, // EControllerHand::Pad
		{ TEXT("ExternalCamera"),                     TEXT("EControllerHand::ExternalCamera") }, // EControllerHand::ExternalCamera
		{ TEXT("Gun"),                                TEXT("EControllerHand::Gun")            }, // EControllerHand::Gun
		{ TEXT("Special_1"),                          TEXT("EControllerHand::Special_1")      }, // EControllerHand::Special_1
		{ TEXT("Special_2"),                          TEXT("EControllerHand::Special_2")      }, // EControllerHand::Special_2
		{ TEXT("Special_3"),                          TEXT("EControllerHand::Special_3")      }, // EControllerHand::Special_3
		{ TEXT("Special_4"),                          TEXT("EControllerHand::Special_4")      }, // EControllerHand::Special_4
		{ TEXT("Special_5"),                          TEXT("EControllerHand::Special_5")      }, // EControllerHand::Special_5
		{ TEXT("Special_6"),                          TEXT("EControllerHand::Special_6")      }, // EControllerHand::Special_6
		{ TEXT("Special_7"),                          TEXT("EControllerHand::Special_7")      }, // EControllerHand::Special_7
		{ TEXT("Special_8"),                          TEXT("EControllerHand::Special_8")      }, // EControllerHand::Special_8
		{ TEXT("Special_9"),                          TEXT("EControllerHand::Special_9")      }, // EControllerHand::Special_9
		{ TEXT("Special_10"),                         TEXT("EControllerHand::Special_10")     }, // EControllerHand::Special_10
		{ TEXT("Special_11"),                         TEXT("EControllerHand::Special_11")     }  // EControllerHand::Special_11
	};
}

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
	const int32 HandsCount = ARRAY_COUNT(XRMotionControllerBase_Impl::LegacyHandMappings);
	ensure(HandsCount == (int32)EControllerHand::ControllerHand_Count);

	for (int32 HandIndex = 0; HandIndex < HandsCount; ++HandIndex)
	{
		SourcesOut.Add(XRMotionControllerBase_Impl::LegacyHandMappings[HandIndex][0]);
	}
}

bool FXRMotionControllerBase::GetHandEnumForSourceName(const FName Source, EControllerHand& OutHand)
{
	const int32 HandsCount = ARRAY_COUNT(XRMotionControllerBase_Impl::LegacyHandMappings);
	ensure(HandsCount == (int32)EControllerHand::ControllerHand_Count);

	bool bFound = false;
	for (int32 HandIndex = 0; HandIndex < HandsCount; ++HandIndex)
	{
		for (int32 AliasIndex = 0; AliasIndex < XRMotionControllerBase_Impl::HandAliasCount; ++AliasIndex)
		{
			if (XRMotionControllerBase_Impl::LegacyHandMappings[HandIndex][AliasIndex] == Source)
			{
				OutHand = (EControllerHand)HandIndex;
				bFound = true;
				break;
			}
		}
	}
	return bFound;
}