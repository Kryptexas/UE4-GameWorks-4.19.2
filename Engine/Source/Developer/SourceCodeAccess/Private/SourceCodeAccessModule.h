// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISourceCodeAccessModule.h"
#include "DefaultSourceCodeAccessor.h"

/**
 * Implementation of ISourceCodeAccessModule
 */
class FSourceCodeAccessModule : public ISourceCodeAccessModule
{
public:
	FSourceCodeAccessModule();

	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	/** ISourceCodeAccessModule implementation */
	virtual bool CanAccessSourceCode() const OVERRIDE;
	virtual ISourceCodeAccessor& GetAccessor() const OVERRIDE;
	virtual void SetAccessor(const FName& InName) OVERRIDE;
	virtual FLaunchingCodeAccessor& OnLaunchingCodeAccessor() OVERRIDE;
	virtual FDoneLaunchingCodeAccessor& OnDoneLaunchingCodeAccessor() OVERRIDE;
	virtual FLaunchCodeAccessorDeferred& OnLaunchCodeAccessorDeferred() OVERRIDE;
	virtual FOpenFileFailed& OnOpenFileFailed() OVERRIDE;

private:
	/** Handle when one of the modular features we are interested in is registered */
	void HandleModularFeatureRegistered(const FName& Type);

private:
	/** Event delegate fired when launching code accessor */
	FLaunchingCodeAccessor LaunchingCodeAccessorDelegate;

	/** Event delegate fired when done launching code accessor  */
	FDoneLaunchingCodeAccessor DoneLaunchingCodeAccessorDelegate;

	/** Event delegate fired when launching code accessor deferred */
	FLaunchCodeAccessorDeferred LaunchCodeAccessorDeferredDelegate;

	/** Event delegate fired when opening a file has failed */
	FOpenFileFailed OpenFileFailedDelegate;

	/** The default accessor we will use if we have no IDE */
	FDefaultSourceCodeAccessor DefaultSourceCodeAccessor;

	/** The current accessor */
	ISourceCodeAccessor* CurrentSourceCodeAccessor;
};