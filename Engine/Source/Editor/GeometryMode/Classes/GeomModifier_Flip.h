// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/**
 * Flips selected objects.
 */

#pragma once
#include "GeomModifier_Flip.generated.h"

UCLASS()
class UGeomModifier_Flip : public UGeomModifier_Edit
{
	GENERATED_UCLASS_BODY()


	// Begin UGeomModifier Interface
	virtual bool Supports() OVERRIDE;
protected:
	virtual bool OnApply() OVERRIDE;
	// End UGeomModifier Interface
};



