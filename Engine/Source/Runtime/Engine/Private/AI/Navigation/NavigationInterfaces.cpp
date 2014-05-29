// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AI/Navigation/NavAgentInterface.h"
#include "AI/Navigation/NavigationPathGenerator.h"
#include "AI/Navigation/NavNodeInterface.h"
#include "AI/Navigation/NavLinkHostInterface.h"
#include "AI/Navigation/NavPathObserverInterface.h"
#include "AI/Navigation/NavRelevantActorInterface.h"

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

UNavPathObserverInterface::UNavPathObserverInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UNavRelevantActorInterface::UNavRelevantActorInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}
