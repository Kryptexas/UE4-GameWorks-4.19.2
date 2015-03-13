// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PortalPackageInstallerMessages.generated.h"


/* Service discovery messages
 *****************************************************************************/

/**
 * Implements a request message for Portal package installation.
 */
USTRUCT()
struct FPortalPackageInstallRequest
{
	GENERATED_USTRUCT_BODY()

	/** The application name. */
	UPROPERTY()
	FString AppName;

	/** The application build label. */
	UPROPERTY()
	FString BuildLabel;

	/** The path to install the package contents to. */
	UPROPERTY()
	FString DestinationPath;

	/** Unique identifier for this request. */
	UPROPERTY()
	FGuid RequestId;
};

template<> struct TStructOpsTypeTraits<FPortalPackageInstallRequest> : public TStructOpsTypeTraitsBase { enum { WithMessageHandling = true }; };


/**
 * Implements a reply message to Portal package installation requests.
 */
USTRUCT()
struct FPortalPackageInstallResponse
{
	GENERATED_USTRUCT_BODY()

	/** Identifier of the request that this response is for. */
	UPROPERTY()
	FGuid RequestId;

	UPROPERTY()
	bool Success;
};

template<> struct TStructOpsTypeTraits<FPortalPackageInstallResponse> : public TStructOpsTypeTraitsBase{ enum { WithMessageHandling = true }; };


/**
 * Implements a message for Portal package uninstallation.
 */
USTRUCT()
struct FPortalPackageUninstallRequest
{
	GENERATED_USTRUCT_BODY()

	/** The application name. */
	UPROPERTY()
	FString AppName;

	/** The application build label. */
	UPROPERTY()
	FString BuildLabel;

	/** The path to where the package is installed. */
	UPROPERTY()
	FString InstallationPath;

	/** Whether any user generated files should be removed as well. */
	UPROPERTY()
	bool RemoveUserFiles;

	/** Unique identifier for this request. */
	UPROPERTY()
	FGuid RequestId;
};

template<> struct TStructOpsTypeTraits<FPortalPackageUninstallRequest> : public TStructOpsTypeTraitsBase{ enum { WithMessageHandling = true }; };


/**
 * Implements a reply message to Portal package uninstallation requests.
 */
USTRUCT()
struct FPortalPackageUninstallResponse
{
	GENERATED_USTRUCT_BODY()

	/** Identifier of the request that this response is for. */
	UPROPERTY()
	FGuid RequestId;

	UPROPERTY()
	bool Success;
};

template<> struct TStructOpsTypeTraits<FPortalPackageUninstallResponse> : public TStructOpsTypeTraitsBase{ enum { WithMessageHandling = true }; };
