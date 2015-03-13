// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PrivatePCH.h"
#include "IPortalPackageInstaller.h"
#include "PortalPackageInstallerMessages.h"


/* FPackageInstallerProxy structors
 *****************************************************************************/

FPackageInstallerProxy::FPackageInstallerProxy()
{
	MessageEndpoint = FMessageEndpoint::Builder("FPackageInstallerProxy")
		.Handling<FPortalPackageInstallResponse>(this, &FPackageInstallerProxy::HandlePackageInstallResponse)
		.Handling<FPortalPackageUninstallResponse>(this, &FPackageInstallerProxy::HandlePackageUninstallResponse);
}


/* IPortalPackageInstaller interface
 *****************************************************************************/

TSlowResult<bool> FPackageInstallerProxy::InstallPackage(const FString& Path, const FPortalPackageId& Id)
{
	if (!MessageEndpoint.IsValid())
	{
		return TSlowResult<bool>();
	}

	auto Message = new FPortalPackageInstallRequest();
	{
		Message->AppName = Id.AppName;
		Message->BuildLabel = Id.BuildLabel;
		Message->RequestId = FGuid::NewGuid();
	}

	//MessageEndpoint->Send(Message, )

	return TSlowResult<bool>();
}


TSlowResult<bool> FPackageInstallerProxy::UninstallPackage(const FString& Path, const FString& Id, bool RemoveUserFiles)
{
	if (!MessageEndpoint.IsValid())
	{
		return TSlowResult<bool>();
	}

	return TSlowResult<bool>();
}


/* FPackageInstallerProxy callbacks
 *****************************************************************************/

void FPackageInstallerProxy::HandlePackageInstallResponse(const FPortalPackageInstallResponse& Message, const IMessageContextRef& Context)
{

}


void FPackageInstallerProxy::HandlePackageUninstallResponse(const FPortalPackageUninstallResponse& Message, const IMessageContextRef& Context)
{

}
