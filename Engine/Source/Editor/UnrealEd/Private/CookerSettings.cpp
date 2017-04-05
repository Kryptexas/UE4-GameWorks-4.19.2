// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CookerSettings.h"
#include "UObject/UnrealType.h"


UCookerSettings::UCookerSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bEnableCookOnTheSide(false)
	, bEnableBuildDDCInBackground(false)
	, bIterativeCookingForLaunchOn(false)
	, bUseAssetRegistryForIteration(false)
	, bCompileBlueprintsInDevelopmentMode(true)
	, bCookBlueprintComponentTemplateData(false)
{
	SectionName = TEXT("Cooker");
	DefaultPVRTCQuality = 1;
	DefaultASTCQualityBySize = 3;
	DefaultASTCQualityBySpeed = 3;
}

void UCookerSettings::PostInitProperties()
{
	Super::PostInitProperties();
	UObject::UpdateClassesExcludedFromDedicatedServer(ClassesExcludedOnDedicatedServer, ModulesExcludedOnDedicatedServer);
	UObject::UpdateClassesExcludedFromDedicatedClient(ClassesExcludedOnDedicatedClient, ModulesExcludedOnDedicatedClient);
}

void UCookerSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static FName NAME_ClassesExcludedOnDedicatedServer(TEXT("ClassesExcludedOnDedicatedServer"));
	static FName NAME_ClassesExcludedOnDedicatedClient(TEXT("ClassesExcludedOnDedicatedClient"));

	static FName NAME_ModulesExcludedOnDedicatedServer(TEXT("ModulesExcludedOnDedicatedServer"));
	static FName NAME_ModulesExcludedOnDedicatedClient(TEXT("ModulesExcludedOnDedicatedClient"));

	if(PropertyChangedEvent.Property)
	{
		if(PropertyChangedEvent.Property->GetFName() == NAME_ClassesExcludedOnDedicatedServer
			|| PropertyChangedEvent.Property->GetFName() == NAME_ModulesExcludedOnDedicatedServer)
		{
			UObject::UpdateClassesExcludedFromDedicatedServer(ClassesExcludedOnDedicatedServer, ModulesExcludedOnDedicatedServer);
		}
		else if(PropertyChangedEvent.Property->GetFName() == NAME_ClassesExcludedOnDedicatedClient
			|| PropertyChangedEvent.Property->GetFName() == NAME_ModulesExcludedOnDedicatedClient)
		{
			UObject::UpdateClassesExcludedFromDedicatedClient(ClassesExcludedOnDedicatedClient, ModulesExcludedOnDedicatedClient);
		}
	}
}
