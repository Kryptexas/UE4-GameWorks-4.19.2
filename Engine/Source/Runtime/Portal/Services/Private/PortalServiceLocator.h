// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IPortalServiceLocator.h"


class IPortalService;
class FTypeContainer;


class FPortalServiceLocator
	: public IPortalServiceLocator
{
public:

	/**
	 * Create and initialize a new instance.
	 *
	 * @param InServiceDependencies A type container for optional service dependencies.
	 */
	FPortalServiceLocator(const TSharedRef<FTypeContainer>& InServiceDependencies);

	/** Virtual destructor. */
	~FPortalServiceLocator() { }

public:

	// IPortalServiceLocator interface

	virtual void Configure(const FString& ServiceName, const FWildcardString& ProductId, const FName& ServiceModule) override;
	virtual TSharedPtr<IPortalService> GetService(const FString& ServiceName, const FString& ProductId) override;

private:

	struct FConfigEntry
	{
		FWildcardString ProductId;
		TSharedPtr<IPortalService> ServiceInstance;
		FName ServiceModule;
	};

	/** Holds the service configuration entries. */
	TMap<FString, TArray<FConfigEntry>> Configuration;

	/** Optional service dependencies. */
	TSharedPtr<FTypeContainer> ServiceDependencies;
};
