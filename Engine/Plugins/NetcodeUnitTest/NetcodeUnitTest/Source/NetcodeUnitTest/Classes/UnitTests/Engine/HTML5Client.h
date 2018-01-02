// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnitTests/Engine/IPClient.h"

#include "HTML5Client.generated.h"

/**
 * Basic unit test for verifying simple client connection to a server, using the HTML5 net driver.
 */
UCLASS()
class UHTML5Client : public UIPClient
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void InitializeEnvironmentSettings() override;

	virtual void NotifyAlterMinClient(FMinClientParms& Parms) override;
};
