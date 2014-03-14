// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// SoundSurroundFactory
//=============================================================================

#pragma once
#include "SoundSurroundFactory.generated.h"

UCLASS(hidecategories=Object)
class USoundSurroundFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	float CueVolume;


	// Begin UFactory Interface
	virtual UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn ) OVERRIDE;
	virtual bool FactoryCanImport( const FString& Filename ) OVERRIDE;
	// End UFactory Interface
};



