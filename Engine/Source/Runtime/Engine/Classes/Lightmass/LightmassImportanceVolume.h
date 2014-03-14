// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// LightmassImportanceVolume:  a bounding volume outside of which Lightmass
// photon emissions are decreased
//=============================================================================
#pragma once

#include "LightmassImportanceVolume.generated.h"

UCLASS(hidecategories=(Collision, Brush, Attachment, Physics, Volume), MinimalAPI)
class ALightmassImportanceVolume : public AVolume
{
	GENERATED_UCLASS_BODY()

};



