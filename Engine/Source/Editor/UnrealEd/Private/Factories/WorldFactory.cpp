// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#define LOCTEXT_NAMESPACE "WorldFactory"

UWorldFactory::UWorldFactory(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCreateNew = FParse::Param(FCommandLine::Get(), TEXT("WorldAssets"));
	SupportedClass = UWorld::StaticClass();
	WorldType = EWorldType::Inactive;
	bInformEngineOfWorld = false;
}

bool UWorldFactory::ConfigureProperties()
{
	return true;
}

UObject* UWorldFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// Create a new world.
	const bool bAddToRoot = false;
	UWorld* NewWorld = UWorld::CreateWorld(WorldType, bInformEngineOfWorld, Name, Cast<UPackage>(InParent), bAddToRoot);
	NewWorld->SetFlags(Flags);

	return NewWorld;
}

#undef LOCTEXT_NAMESPACE
