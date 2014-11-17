// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "AIResourceInterface.h"
#include "GenericTeamAgentInterface.h"

UAIResourceInterface::UAIResourceInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FGenericTeamId FGenericTeamId::NoTeam(FGenericTeamId::NoTeamId);

FGenericTeamId FGenericTeamId::GetTeamIdentifier(const AActor* TeamMember)
{
	const IGenericTeamAgentInterface* TeamAgent = InterfaceCast<const IGenericTeamAgentInterface>(TeamMember);
	if (TeamAgent)
	{
		return TeamAgent->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;
}

ETeamAttitude::Type FGenericTeamId::GetAttitude(const AActor* A, const AActor* B)
{
	const IGenericTeamAgentInterface* TeamAgentA = InterfaceCast<const IGenericTeamAgentInterface>(A);

	return TeamAgentA == NULL || B == NULL ? ETeamAttitude::Neutral : TeamAgentA->GetTeamAttitudeTowards(*B);
}

UGenericTeamAgentInterface::UGenericTeamAgentInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}
