// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/**
 * Creates selected objects.
 */

#pragma once
#include "GeomModifier_Create.generated.h"

UCLASS()
class UGeomModifier_Create : public UGeomModifier_Edit
{
	GENERATED_UCLASS_BODY()


	// Begin UGeomModifier Interface
	virtual bool Supports() OVERRIDE;
protected:
	virtual bool OnApply() OVERRIDE;
	// End UGeomModifier Interface
};



