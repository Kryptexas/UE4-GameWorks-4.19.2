// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FP_FirstPerson.h"
#include "FP_FirstPersonGameMode.h"
#include "FP_FirstPersonHUD.h"
#include "FP_FirstPersonCharacter.h"

AFP_FirstPersonGameMode::AFP_FirstPersonGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AFP_FirstPersonHUD::StaticClass();
}
