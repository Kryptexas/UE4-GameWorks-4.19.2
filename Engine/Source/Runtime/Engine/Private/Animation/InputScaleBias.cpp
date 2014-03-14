// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

/////////////////////////////////////////////////////
// FInputScaleBias

float FInputScaleBias::ApplyTo(float Value) const
{
	return FMath::Clamp<float>( Value * Scale + Bias, 0.0f, 1.0f );
}
