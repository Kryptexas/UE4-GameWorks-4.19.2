// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ARBlueprintFunctionLibrary.h"
#include "Features/IModularFeature.h"
#include "Features/IModularFeatures.h"
#include "ARHitTestingSupport.h"

bool UARBlueprintFunctionLibrary::ARLineTraceFromScreenPoint(UObject* WorldContextObject, const FVector2D ScreenPosition, TArray<FARHitTestResult>& OutHitResults)
{
	TArray<IARHitTestingSupport*> Providers = IModularFeatures::Get().GetModularFeatureImplementations<IARHitTestingSupport>(IARHitTestingSupport::GetModularFeatureName());
	const int32 NumProviders = Providers.Num();
	ensureMsgf(NumProviders <= 1, TEXT("Expected at most one ARHitTestingSupport provider, but there are %d registered. Using the first."), NumProviders);
	if ( ensureMsgf(NumProviders > 0, TEXT("Expected at least one ARHitTestingSupport provider.")) )
	{
		return Providers[0]->ARLineTraceFromScreenPoint(WorldContextObject, ScreenPosition, OutHitResults);
	}

	return false;
}
