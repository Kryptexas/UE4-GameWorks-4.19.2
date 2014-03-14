// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/**
 * Splits selected objects.
 */

#pragma once
#include "GeomModifier_Split.generated.h"

UCLASS()
class UGeomModifier_Split : public UGeomModifier_Edit
{
	GENERATED_UCLASS_BODY()


	// Begin UGeomModifier Interface
	virtual bool Supports() OVERRIDE;
protected:
	virtual bool OnApply() OVERRIDE;
	// End UGeomModifier Interface
};



