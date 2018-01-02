// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"

#include "GLTFImportFactory.generated.h"

UCLASS(transient)
class UGLTFImportFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:
	UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	bool FactoryCanImport(const FString& Filename) override;
};
