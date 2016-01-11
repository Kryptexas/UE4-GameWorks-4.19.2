// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "TP_Vehicle.h"
#include "TP_VehicleWheelRear.h"

UTP_VehicleWheelRear::UTP_VehicleWheelRear()
{
	ShapeRadius = 35.f;
	ShapeWidth = 10.0f;
	bAffectedByHandbrake = true;
	SteerAngle = 0.f;
}
