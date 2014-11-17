// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
#include "SkillSystemGlobals.h"


class FSkillSystemModule : public ISkillSystemModule
{
	// Begin IModuleInterface
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
	// End IModuleInterface

	virtual USkillSystemGlobals& GetSkillSystemGlobals()
	{
		check(SkillSystemGlobals);
		return *SkillSystemGlobals;
	}

	USkillSystemGlobals *SkillSystemGlobals;

private:
	
};

IMPLEMENT_MODULE( FSkillSystemModule, SkillSystem )

void FSkillSystemModule::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	SkillSystemGlobals = ConstructObject<USkillSystemGlobals>(USkillSystemGlobals::StaticClass(), GetTransientPackage(), NAME_None, RF_RootSet);
}

void FSkillSystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	SkillSystemGlobals = NULL;
}
