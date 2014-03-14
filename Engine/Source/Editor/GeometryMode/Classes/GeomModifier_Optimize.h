// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/**
 * Optimizes selected objects by attempting to merge their polygons back together.
 */

#pragma once
#include "GeomModifier_Optimize.generated.h"

UCLASS()
class UGeomModifier_Optimize : public UGeomModifier_Edit
{
	GENERATED_UCLASS_BODY()


	// Begin UGeomModifier Interface
	virtual bool Supports() OVERRIDE;
protected:
	virtual bool OnApply() OVERRIDE;
	// End UGeomModifier Interface
};



