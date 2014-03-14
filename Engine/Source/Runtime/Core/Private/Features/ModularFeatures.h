// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Runtime/Core/Public/Features/IModularFeatures.h"


/**
 * Private implementation of modular features interface
 */
class FModularFeatures : public IModularFeatures
{

public:

	/** IModularFeatures interface */
	virtual int32 GetModularFeatureImplementationCount( const FName Type ) OVERRIDE;
	virtual IModularFeature* GetModularFeatureImplementation( const FName Type, const int32 Index ) OVERRIDE;
	virtual void RegisterModularFeature( const FName Type, class IModularFeature* ModularFeature ) OVERRIDE;
	virtual void UnregisterModularFeature( const FName Type, class IModularFeature* ModularFeature ) OVERRIDE;
	DECLARE_DERIVED_EVENT(FModularFeatures, IModularFeatures::FOnModularFeatureRegistered, FOnModularFeatureRegistered);
	virtual IModularFeatures::FOnModularFeatureRegistered& OnModularFeatureRegistered() OVERRIDE;
	DECLARE_DERIVED_EVENT(FModularFeatures, IModularFeatures::FOnModularFeatureUnregistered, FOnModularFeatureUnregistered);
	virtual IModularFeatures::FOnModularFeatureUnregistered& OnModularFeatureUnregistered() OVERRIDE;

private:


private:

	/** Maps each feature type to a list of known providers of that feature */
	TMultiMap< FName, class IModularFeature* > ModularFeaturesMap;

	/** Event used to inform clients that a modular feature has been registered */
	FOnModularFeatureRegistered ModularFeatureRegisteredEvent;

	/** Event used to inform clients that a modular feature has been unregistered */
	FOnModularFeatureUnregistered ModularFeatureUnregisteredEvent;
};