// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IFuncTestManager.h"

class FFunctionalTestingModule : public IModuleInterface
{
public:

	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	/** Gets the debugger singleton or returns NULL */
	static class IFuncTestManager* Get()
	{
		FFunctionalTestingModule& Module = FModuleManager::Get().LoadModuleChecked<FFunctionalTestingModule>("FunctionalTesting");
		return Module.GetSingleton();
	}
private:
	class IFuncTestManager* GetSingleton() const 
	{ 
		return Manager.Get(); 
	}

private:
	TSharedPtr<class IFuncTestManager> Manager;
};
