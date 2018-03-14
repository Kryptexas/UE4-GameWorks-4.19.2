// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "UnitTests/Engine/HTML5Client.h"

#include "MinimalClient.h"
#include "UnitTestEnvironment.h"

/**
 * UHTML5Client
 */

UHTML5Client::UHTML5Client(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UnitTestName = TEXT("HTML5Client");

	UnitTestDate = FDateTime(2017, 10, 5);

	UnitTestBugTrackIDs.Empty();

	ExpectedResult.Add(TEXT("ShooterGame"), EUnitTestVerification::VerifiedFixed);
	ExpectedResult.Add(TEXT("FortniteGame"), EUnitTestVerification::VerifiedFixed);

	NetDriverLog = TEXT("LogHTML5Networking: GameNetDriver WebSocketNetDriver");
}

void UHTML5Client::InitializeEnvironmentSettings()
{
	Super::InitializeEnvironmentSettings();

	BaseServerParameters = UnitEnv->GetDefaultServerParameters(TEXT("LogHTML5Networking log")) +
							TEXT(" -NetDriverOverrides=/Script/HTML5Networking.WebSocketNetDriver");
}

void UHTML5Client::NotifyAlterMinClient(FMinClientParms& Parms)
{
	Super::NotifyAlterMinClient(Parms);

	Parms.NetDriverClass = TEXT("/Script/HTML5Networking.WebSocketNetDriver");
}
