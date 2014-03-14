// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsModulePrivatePCH.h"
#include "GameplayTags.generated.inl"


class FGameplayTagsModule : public IGameplayTagsModule
{
	// Begin IModuleInterface
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
	// End IModuleInterface

	// Gets the UGameplayTagsManager manager
	virtual UGameplayTagsManager& GetGameplayTagsManager() OVERRIDE
	{
		check(GameplayTagsManager);
		return *GameplayTagsManager;
	}

private:

	// Stores the UGameplayTagsManager
	UGameplayTagsManager* GameplayTagsManager;
};

IMPLEMENT_MODULE( FGameplayTagsModule, GameplayTags )

void FGameplayTagsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	GameplayTagsManager = ConstructObject<UGameplayTagsManager>(UGameplayTagsManager::StaticClass(),GetTransientPackage(),NAME_None,RF_RootSet);
}

void FGameplayTagsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	GameplayTagsManager = NULL;
}
