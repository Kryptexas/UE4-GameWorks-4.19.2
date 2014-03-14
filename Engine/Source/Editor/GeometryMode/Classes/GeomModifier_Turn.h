// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/**
 * Turns selected objects.
 */

#pragma once
#include "GeomModifier_Turn.generated.h"

UCLASS()
class UGeomModifier_Turn : public UGeomModifier_Edit
{
	GENERATED_UCLASS_BODY()


	// Begin UGeomModifier Interface
	virtual bool Supports() OVERRIDE;
protected:
	virtual bool OnApply() OVERRIDE;
	// End UGeomModifier Interface
};



