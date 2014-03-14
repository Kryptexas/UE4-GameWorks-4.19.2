// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "Runtime/Core/Private/Features/ModularFeatures.h"


IModularFeatures& IModularFeatures::Get()
{
	// Singleton instance
	static FModularFeatures ModularFeatures;
	return ModularFeatures;
}


int32 FModularFeatures::GetModularFeatureImplementationCount( const FName Type )
{
	return ModularFeaturesMap.Num( Type );
}


IModularFeature* FModularFeatures::GetModularFeatureImplementation( const FName Type, const int32 Index )
{
	TArray< IModularFeature* > FeatureImplementations;
	ModularFeaturesMap.MultiFind( Type, FeatureImplementations );
	return FeatureImplementations[ Index ];
}


void FModularFeatures::RegisterModularFeature( const FName Type, IModularFeature* ModularFeature )
{
	ModularFeaturesMap.AddUnique( Type, ModularFeature );
	ModularFeatureRegisteredEvent.Broadcast( Type );
}


void FModularFeatures::UnregisterModularFeature( const FName Type, IModularFeature* ModularFeature )
{
	ModularFeaturesMap.RemoveSingle( Type, ModularFeature );
	ModularFeatureUnregisteredEvent.Broadcast( Type );
}

IModularFeatures::FOnModularFeatureRegistered& FModularFeatures::OnModularFeatureRegistered()
{
	return ModularFeatureRegisteredEvent;
}

IModularFeatures::FOnModularFeatureUnregistered& FModularFeatures::OnModularFeatureUnregistered()
{
	return ModularFeatureUnregisteredEvent;
}