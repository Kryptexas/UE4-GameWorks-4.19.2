// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PartyMemberState.generated.h"

class UPartyGameState;

/**
 * Simple struct for replication and copying of party member data on updates
 */
USTRUCT()
struct FPartyMemberRepState
{
	GENERATED_USTRUCT_BODY()

	/** Reset the variables of this party member state */
	virtual void Reset()
	{}
};

/**
 * Main representation of a party member
 */
UCLASS(config = Game, notplaceable)
class PARTY_API UPartyMemberState : public UObject
{
	GENERATED_UCLASS_BODY()

	// Begin UObject Interface
	virtual void BeginDestroy() override;
	// End UObject Interface

	/** Unique id of this party member */
	UPROPERTY(Transient)
	FUniqueNetIdRepl UniqueId;

	/** Display name of this party member */
	UPROPERTY(Transient)
	FText DisplayName;

	/** @return the party this member is associated with */
	UPartyGameState* GetParty() const { return GetTypedOuter<UPartyGameState>(); }

protected:

	template<typename T>
	void InitPartyMemberState(T* InMemberState)
	{
		MemberStateRef = InMemberState;
		MemberStateRefDef = T::StaticStruct();

		MemberStateRefScratch = (FPartyMemberRepState*)FMemory::Malloc(MemberStateRefDef->GetCppStructOps()->GetSize());
		MemberStateRefDef->InitializeStruct(MemberStateRefScratch);
	}

	void UpdatePartyMemberState();

	/**
	 * Compare current data to old data, triggering delegates
	 *
	 * @param OldPartyMemberState old view of data to compare against
	 */
	virtual void ComparePartyMemberData(const FPartyMemberRepState& OldPartyMemberState);

	/** Reset to default state */
	virtual void Reset();

private:

	/** Have we announced this player joining the game locally */
	UPROPERTY(Transient)
	bool bHasAnnouncedJoin;

	/** Pointer to child USTRUCT that holds the current state of party member (set via InitPartyMemberState) */
	FPartyMemberRepState* MemberStateRef;

	/** Scratch copy of child USTRUCT for handling replication comparisons */
	FPartyMemberRepState* MemberStateRefScratch;

	/** Reflection data for child USTRUCT */
	const UScriptStruct* MemberStateRefDef;

	friend UPartyGameState;
};
