// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EngineSettingsModule.cpp: Implements the FEngineSettingsModule class.
=============================================================================*/

#include "EngineSettingsPrivatePCH.h"
#include "EngineSettings.generated.inl"


/**
 * Implements the EngineSettings module.
 */
class FEngineSettingsModule
	: public IModuleInterface
{
public:

	virtual void StartupModule( ) OVERRIDE { }

	virtual void ShutdownModule( ) OVERRIDE { }

	virtual bool SupportsDynamicReloading( ) OVERRIDE
	{
		return true;
	}
};

/* Class constructors
 *****************************************************************************/

UGameNetworkManagerSettings::UGameNetworkManagerSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


UGameMapsSettings::UGameMapsSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


UGameSessionSettings::UGameSessionSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


UGeneralEngineSettings::UGeneralEngineSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


UGeneralProjectSettings::UGeneralProjectSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


UHudSettings::UHudSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }

/* Static functions
 *****************************************************************************/

const FString& UGameMapsSettings::GetGameDefaultMap( )
{
	return IsRunningDedicatedServer()
		? GetDefault<UGameMapsSettings>()->ServerDefaultMap
		: GetDefault<UGameMapsSettings>()->GameDefaultMap;
}


const FString& UGameMapsSettings::GetGlobalDefaultGameMode( )
{
	return IsRunningDedicatedServer()
		? GetDefault<UGameMapsSettings>()->GlobalDefaultServerGameMode.ClassName 
		: GetDefault<UGameMapsSettings>()->GlobalDefaultGameMode.ClassName;
}


void UGameMapsSettings::SetGameDefaultMap( const FString& NewMap )
{
	UGameMapsSettings* GameMapsSettings = Cast<UGameMapsSettings>(UGameMapsSettings::StaticClass()->GetDefaultObject());

	if (IsRunningDedicatedServer())
	{
		GameMapsSettings->ServerDefaultMap = NewMap;
	}
	else
	{
		GameMapsSettings->GameDefaultMap = NewMap;
	}
}


IMPLEMENT_MODULE(FEngineSettingsModule, EngineSettings);
