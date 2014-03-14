// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnvironmentQueryEditorPrivatePCH.h"
#include "EnvironmentQueryEditorModule.h"

UEnvironmentQueryFactory::UEnvironmentQueryFactory(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SupportedClass = UEnvQuery::StaticClass();
	bEditAfterNew = true;

	// Check ini to see if we should enable creation
	bool bEnableEnvironmentQueryEd=false;
	GConfig->GetBool(TEXT("EnvironmentQueryEd"), TEXT("EnableEnvironmentQueryEd"), bEnableEnvironmentQueryEd, GEngineIni);
	bCreateNew = bEnableEnvironmentQueryEd;
}

UObject* UEnvironmentQueryFactory::FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UEnvQuery::StaticClass()));
	return ConstructObject<UEnvQuery>(Class, InParent, Name, Flags);
}