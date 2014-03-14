// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// FontFactory: Creates a Font Factory
//=============================================================================

#pragma once
#include "FontFactory.generated.h"

UENUM()
enum EFontCharacterSet
{
	CHARSET_Default,
	CHARSET_Ansi,
	CHARSET_Symbol,
	CHARSET_MAX,
};

UCLASS()
class UFontFactory : public UTextureFactory
{
	GENERATED_UCLASS_BODY()


	// Begin UFactory Interface
	virtual UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn ) OVERRIDE;
	// Begin UFactory Interface	
};



