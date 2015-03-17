// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// LevelFactory
//=============================================================================

#pragma once
#include "LevelFactory.generated.h"

UCLASS(MinimalAPI)
class ULevelFactory : public UFactory
{
	GENERATED_BODY()
public:
	UNREALED_API ULevelFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UFactory Interface
	virtual UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn ) override;
	// Begin UFactory Interface	
};



