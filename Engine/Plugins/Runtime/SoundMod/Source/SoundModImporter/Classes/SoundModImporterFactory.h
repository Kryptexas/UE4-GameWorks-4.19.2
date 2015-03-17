// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoundModImporterFactory.generated.h"

// Imports a sound module file
UCLASS()
class USoundModImporterFactory : public UFactory
{
	GENERATED_BODY()
public:
	USoundModImporterFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// UFactory interface
	virtual UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) override;
	// End of UFactory interface
};

