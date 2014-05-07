// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GameMapsSettings.h: Declares the UGameMapsSettings class.
=============================================================================*/

#pragma once

#include "GameMapsSettings.generated.h"


UENUM()
namespace ETwoPlayerSplitScreenType
{
	enum Type
	{
		Horizontal,
		Vertical
	};
}


UENUM()
namespace EThreePlayerSplitScreenType
{
	enum Type
	{
		FavorTop,
		FavorBottom
	};
}


UCLASS(config=Engine, defaultconfig)
class ENGINESETTINGS_API UGameMapsSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

	/**
	 * Get the default map specified in the settings.
	 * Makes a choice based on running as listen server/client vs dedicated server
	 *
	 * @return the default map specified in the settings
	 */
	static const FString& GetGameDefaultMap( );

	/**
	 * Get the global default game type specified in the configuration
	 * Makes a choice based on running as listen server/client vs dedicated server
	 * 
	 * @return the proper global default game type
	 */
	static const FString& GetGlobalDefaultGameMode( );

	/**
	 * Set the default map to use (see GameDefaultMap below)
	 *
	 * @param NewMap name of valid map to use
	 */
	static void SetGameDefaultMap( const FString& NewMap );

public:

	/**
	 * If set, this map will be loaded when the Editor starts up.
	 */
	UPROPERTY(config, EditAnywhere, Category=DefaultMaps)
	FString EditorStartupMap;

	/**
	 * The default options that will be appended to a map being loaded.
	 */
	UPROPERTY(config, EditAnywhere, Category=DefaultMaps, AdvancedDisplay)
	FString LocalMapOptions;

	/**
	 * The map loaded when transition from one map to another.
	 */
	UPROPERTY(config, EditAnywhere, Category=DefaultMaps, AdvancedDisplay)
	FString TransitionMap;

	UPROPERTY(config, EditAnywhere, Category=LocalMultiplayer)
	bool bUseSplitscreen;

	UPROPERTY(config, EditAnywhere, Category=LocalMultiplayer, meta=(editcondition="bUseSplitScreen"))
	TEnumAsByte<ETwoPlayerSplitScreenType::Type> TwoPlayerSplitscreenLayout;

	UPROPERTY(config, EditAnywhere, Category=LocalMultiplayer, meta=(editcondition="bUseSplitScreen"))
	TEnumAsByte<EThreePlayerSplitScreenType::Type> ThreePlayerSplitscreenLayout;

private:

	/**
	 * The map that will be loaded by default when no other map is loaded.
	 */
	UPROPERTY(config, EditAnywhere, Category=DefaultMaps)
	FString GameDefaultMap;

	/**
	 * The map that will be loaded by default when no other map is loaded (DEDICATED SERVER).
	 */
	UPROPERTY(config, EditAnywhere, Category=DefaultMaps, AdvancedDisplay)
	FString ServerDefaultMap;

	/**
	 * GameMode to use if not specified in any other way. (e.g. per-map DefaultGameMode or on the URL).
	 */
	UPROPERTY(config, noclear, EditAnywhere, Category=DefaultModes, meta=(MetaClass="GameMode", DisplayName="Default GameMode"))
	FStringClassReference GlobalDefaultGameMode;

	/**
	 * GameMode to use if not specified in any other way. (e.g. per-map DefaultGameMode or on the URL) (DEDICATED SERVERS).
	 */
	UPROPERTY(config, EditAnywhere, Category=DefaultModes, meta=(MetaClass="GameMode"), AdvancedDisplay)
	FStringClassReference GlobalDefaultServerGameMode;
};
