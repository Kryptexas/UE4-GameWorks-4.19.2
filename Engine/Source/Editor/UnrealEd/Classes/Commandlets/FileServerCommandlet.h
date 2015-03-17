// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FileServerCommandlet.h: Declares the UFileServerCommandlet class.
=============================================================================*/

#pragma once
#include "Commandlets/Commandlet.h"
#include "FileServerCommandlet.generated.h"


/**
 * Implements a file server commandlet.
 */
UCLASS()
class UFileServerCommandlet
	: public UCommandlet
{
	GENERATED_BODY()
public:
	UFileServerCommandlet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


public:

	// Begin UCommandlet Interface

	virtual int32 Main( const FString& Params ) override;
	
	// End UCommandlet Interface


private:

	// Holds the instance identifier.
	FGuid InstanceId;
};
