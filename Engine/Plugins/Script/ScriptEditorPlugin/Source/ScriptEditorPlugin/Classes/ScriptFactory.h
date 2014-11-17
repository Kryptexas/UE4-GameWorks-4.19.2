// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ScriptFactory.generated.h"

UCLASS(collapsecategories, hidecategories=Object)
class SCRIPTEDITORPLUGIN_API UScriptFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:

	// Begin UObject Interface
	virtual void PostInitProperties() override;
	// End UObject Interface

	// Begin UFactory Interface
	virtual bool DoesSupportClass(UClass* Class) override;
	virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
	// End UFactory Interface
};
