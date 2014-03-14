// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FileServerCommandlet.h: Declares the UFileServerCommandlet class.
=============================================================================*/

#pragma once

#include "FileServerCommandlet.generated.h"


/**
 * Implements a file server commandlet.
 */
UCLASS()
class UFileServerCommandlet
	: public UCommandlet
{
	GENERATED_UCLASS_BODY()


public:

	// Begin UCommandlet Interface

	virtual int32 Main( const FString& Params ) OVERRIDE;
	
	// End UCommandlet Interface


private:

	// Holds the instance identifier.
	FGuid InstanceId;
};