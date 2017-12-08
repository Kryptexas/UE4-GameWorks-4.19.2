// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "DatasmithContentModule.h"
#include "ModuleManager.h"

/**
 * DatasmithContent module implementation (private)
 */
class FDatasmithContentModule : public IDatasmithContentModule
{
public:
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FDatasmithContentModule, DatasmithContent);
