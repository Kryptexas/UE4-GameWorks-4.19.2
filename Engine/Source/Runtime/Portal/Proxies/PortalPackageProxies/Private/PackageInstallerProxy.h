// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IPortalPackageInstaller.h"
#include "SlowResult.h"


class FMessageEndpoint;
class IMessageContext;
struct FPortalPackageInstallResponse;
struct FPortalPackageUninstallResponse;


/**
 * Implements a proxy for the Portal's package installer service.
 */
class FPackageInstallerProxy
	: public IPortalPackageInstaller
{
public:

	/** Default constructor. */
	FPackageInstallerProxy();

public:

	// IPortalPackageInstaller interface

	virtual TSlowResult<bool> InstallPackage(const FString& Path, const FPortalPackageId& Id) override;
	virtual TSlowResult<bool> UninstallPackage(const FString& Path, const FString& Id, bool RemoveUserFiles) override;

private:

	/** Handles FPortalPackageInstallResponse messages. */
	void HandlePackageInstallResponse(const FPortalPackageInstallResponse& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
	void HandlePackageUninstallResponse(const FPortalPackageUninstallResponse& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);

private:

	/** Holds the message endpoint. */
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;
};
