// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Welds selected objects.
 */

#pragma once
#include "GeomModifier_Weld.generated.h"

UCLASS()
class UGeomModifier_Weld : public UGeomModifier_Edit
{
	GENERATED_UCLASS_BODY()


	// Begin UGeomModifier Interface
	virtual bool Supports() OVERRIDE;
protected:
	virtual bool OnApply() OVERRIDE;
	// End UGeomModifier Interface
};



