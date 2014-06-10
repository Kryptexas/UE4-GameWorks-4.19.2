// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AI/Navigation/NavAgentInterface.h"
#include "AI/Navigation/NavigationPathGenerator.h"
#include "AI/Navigation/NavNodeInterface.h"
#include "AI/Navigation/NavLinkHostInterface.h"
#include "AI/Navigation/NavPathObserverInterface.h"
#include "AI/Navigation/NavRelevantActorInterface.h"
#include "AI/Navigation/NavLinkCustomInterface.h"

UNavAgentInterface::UNavAgentInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UNavigationPathGenerator::UNavigationPathGenerator(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UNavNodeInterface::UNavNodeInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UNavLinkHostInterface::UNavLinkHostInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UNavLinkCustomInterface::UNavLinkCustomInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

uint32 INavLinkCustomInterface::GetUniqueId()
{
	static uint32 NextUniqueId = 1;
	return NextUniqueId++;
}

FNavigationLink INavLinkCustomInterface::GetModifier(const INavLinkCustomInterface* CustomNavLink)
{
	FNavigationLink LinkMod;
	LinkMod.AreaClass = CustomNavLink->GetLinkAreaClass();
	LinkMod.UserId = CustomNavLink->GetLinkId();

	ENavLinkDirection::Type LinkDirection = ENavLinkDirection::BothWays;
	CustomNavLink->GetLinkData(LinkMod.Left, LinkMod.Right, LinkDirection);
	LinkMod.Direction = LinkDirection;

	return LinkMod;
}

UNavPathObserverInterface::UNavPathObserverInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UNavRelevantActorInterface::UNavRelevantActorInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}
