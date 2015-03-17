// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// DialogueWaveFactory
//=============================================================================

#pragma once
#include "DialogueWaveFactory.generated.h"

UCLASS(hidecategories=Object)
class UDialogueWaveFactory : public UFactory
{
	GENERATED_BODY()
public:
	UDialogueWaveFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};



