// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EngineSettingsModule.cpp: Implements the FEngineSettingsModule class.
=============================================================================*/

#include "EngineSettingsPrivatePCH.h"


/**
 * Implements the EngineSettings module.
 */
class FEngineSettingsModule
	: public IModuleInterface
{
public:

	// Begin IModuleInterface interface

	virtual void StartupModule( ) OVERRIDE { }

	virtual void ShutdownModule( ) OVERRIDE { }

	virtual bool SupportsDynamicReloading( ) OVERRIDE
	{
		return true;
	}

	// End IModuleInterface interface
};


/* Class constructors
 *****************************************************************************/

UConsoleSettings::UConsoleSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


UGameMapsSettings::UGameMapsSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, bUseSplitscreen(true)
	, TwoPlayerSplitscreenLayout(ETwoPlayerSplitScreenType::Horizontal)
	, ThreePlayerSplitscreenLayout(EThreePlayerSplitScreenType::FavorTop)
{ }


UGameNetworkManagerSettings::UGameNetworkManagerSettings( const class FPostConstructInitializeProperties& PCIP )
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
	UGameMapsSettings* GameMapsSettings = Cast<UGameMapsSettings>(UGameMapsSettings::StaticClass()->GetDefaultObject());

	return IsRunningDedicatedServer() && GameMapsSettings->GlobalDefaultServerGameMode.IsValid()
		? GameMapsSettings->GlobalDefaultServerGameMode.ClassName
		: GameMapsSettings->GlobalDefaultGameMode.ClassName;
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
