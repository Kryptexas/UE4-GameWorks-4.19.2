// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EngineSettingsPrivatePCH.h"


/**
 * Implements the EngineSettings module.
 */
class FEngineSettingsModule
	: public IModuleInterface
{
public:

	// IModuleInterface interface

	virtual void StartupModule( ) override { }
	virtual void ShutdownModule( ) override { }

	virtual bool SupportsDynamicReloading( ) override
	{
		return true;
	}
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
