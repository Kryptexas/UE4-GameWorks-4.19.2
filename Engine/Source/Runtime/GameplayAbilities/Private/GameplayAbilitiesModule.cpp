// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemGlobals.h"


class FGameplayAbilitiesModule : public IGameplayAbilitiesModule
{
	// Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End IModuleInterface

	virtual UAbilitySystemGlobals* GetAbilitySystemGlobals()
	{
		check(AbilitySystemGlobals);
		return AbilitySystemGlobals;
	}

	UAbilitySystemGlobals *AbilitySystemGlobals;

private:
	
};

IMPLEMENT_MODULE(FGameplayAbilitiesModule, GameplayAbilities)

void FGameplayAbilitiesModule::StartupModule()
{	
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	AbilitySystemGlobals = ConstructObject<UAbilitySystemGlobals>(UAbilitySystemGlobals::StaticClass(), GetTransientPackage(), NAME_None, RF_RootSet);
}

void FGameplayAbilitiesModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	AbilitySystemGlobals = NULL;
}
