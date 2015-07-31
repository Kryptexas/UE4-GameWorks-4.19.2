// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PartyPrivatePCH.h"
#include "PartyMemberState.h"
#include "PartyGameState.h"
#include "Party.h"

#pragma optimize("", off)
UPartyMemberState::UPartyMemberState(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	bHasAnnouncedJoin(false),
	MemberStateRef(nullptr),
	MemberStateRefScratch(nullptr),
	MemberStateRefDef(nullptr)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
	}
}

void UPartyMemberState::BeginDestroy()
{
	Super::BeginDestroy();

	if (MemberStateRefDef)
	{
		if (MemberStateRefScratch)
		{
			MemberStateRefDef->DestroyStruct(MemberStateRefScratch);
			FMemory::Free(MemberStateRefScratch);
		}
		MemberStateRefDef = nullptr;
	}

	MemberStateRef = nullptr;
}

void UPartyMemberState::UpdatePartyMemberState()
{
	UPartyGameState* Party = GetParty();
	check(Party);
	Party->UpdatePartyMemberState(UniqueId, this);
}

void UPartyMemberState::ComparePartyMemberData(const FPartyMemberRepState& OldPartyMemberState)
{
	UPartyGameState* Party = GetParty();
	check(Party);
}

void UPartyMemberState::Reset()
{
	if (MemberStateRef)
	{
		MemberStateRef->Reset();
	}
}
#pragma optimize("", on)