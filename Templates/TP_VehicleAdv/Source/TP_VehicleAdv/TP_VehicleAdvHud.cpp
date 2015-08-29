// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "TP_VehicleAdv.h"
#include "TP_VehicleAdvHud.h"
#include "TP_VehicleAdvPawn.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "CanvasItem.h"

#ifdef HMD_INTGERATION
// Needed for VR Headset
#include "Engine.h"
#include "IHeadMountedDisplay.h"
#endif // HMD_INTGERATION

#define LOCTEXT_NAMESPACE "VehicleHUD"

ATP_VehicleAdvHud::ATP_VehicleAdvHud()
{
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Engine/EngineFonts/RobotoDistanceField"));
	HUDFont = Font.Object;
}

void ATP_VehicleAdvHud::DrawHUD()
{
	Super::DrawHUD();

	// Calculate ratio from 720p
	const float HUDXRatio = Canvas->SizeX / 1280.f;
	const float HUDYRatio = Canvas->SizeY / 720.f;

	bool bHMDDeviceActive = false;

#ifdef HMD_INTGERATION
	// We dont want the onscreen hud when using a HMD device	
	if (GEngine->HMDDevice.IsValid() == true )
	{
		bHMDDeviceActive = GEngine->HMDDevice->IsStereoEnabled();
	}
#endif
	if( bHMDDeviceActive == false )
	{
		// Get our vehicle so we can check if we are in car. If we are we don't want onscreen HUD
		ATP_VehicleAdvPawn* Vehicle = Cast<ATP_VehicleAdvPawn>(GetOwningPawn());
		if ((Vehicle != nullptr) && (Vehicle->bInCarCameraActive == false))
		{
			FVector2D ScaleVec(HUDYRatio * 1.4f, HUDYRatio * 1.4f);

			// Speed
			FCanvasTextItem SpeedTextItem(FVector2D(HUDXRatio * 805.f, HUDYRatio * 455), Vehicle->SpeedDisplayString, HUDFont, FLinearColor::White);
			SpeedTextItem.Scale = ScaleVec;
			Canvas->DrawItem(SpeedTextItem);

			// Gear
			FCanvasTextItem GearTextItem(FVector2D(HUDXRatio * 805.f, HUDYRatio * 500.f), Vehicle->GearDisplayString, HUDFont, Vehicle->bInReverseGear == false ? Vehicle->GearDisplayColor : Vehicle->GearDisplayReverseColor);
			GearTextItem.Scale = ScaleVec;
			Canvas->DrawItem(GearTextItem);
		}
	}
}

#undef LOCTEXT_NAMESPACE
