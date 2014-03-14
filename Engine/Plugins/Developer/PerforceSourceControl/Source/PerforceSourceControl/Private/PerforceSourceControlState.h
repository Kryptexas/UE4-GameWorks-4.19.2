// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISourceControlState.h"
#include "PerforceSourceControlRevision.h"

namespace EPerforceState
{
	enum Type
	{
		/** Don't know or don't care. */
		DontCare		= 0,

		/** File is checked out to current user. */
		CheckedOut		= 1,

		/** File is not checked out (but IS controlled by the source control system). */
		ReadOnly		= 2,

		/** File is not at the head revision - must sync the file before editing. */
		NotCurrent		= 3,

		/** File is new and not in the depot - needs to be added. */
		NotInDepot		= 4,

		/** File is checked out by another user and cannot be checked out locally. */
		CheckedOutOther	= 5,

		/** Certain packages are best ignored by the SCC system (MyLevel, Transient, etc). */
		Ignore			= 6,

		/** File is marked for add */
		OpenForAdd		= 7,

		/** File is marked for delete */
		MarkedForDelete	= 8,
	};
}

class FPerforceSourceControlState : public ISourceControlState, public TSharedFromThis<FPerforceSourceControlState, ESPMode::ThreadSafe>
{
public:
	FPerforceSourceControlState( const FString& InLocalFilename, EPerforceState::Type InState = EPerforceState::DontCare)
		: LocalFilename(InLocalFilename)
		, State(InState)
		, bModifed(false)
		, TimeStamp(0)
	{
	}

	/** ISourceControlState interface */
	virtual int32 GetHistorySize() const OVERRIDE;
	virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> GetHistoryItem( int32 HistoryIndex ) const OVERRIDE;
	virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FindHistoryRevision( int32 RevisionNumber ) const OVERRIDE;
	virtual FName GetIconName() const OVERRIDE;
	virtual FName GetSmallIconName() const OVERRIDE;
	virtual FText GetDisplayName() const OVERRIDE;
	virtual FText GetDisplayTooltip() const OVERRIDE;
	virtual const FString& GetFilename() const OVERRIDE;
	virtual const FDateTime& GetTimeStamp() const OVERRIDE;
	virtual bool CanCheckout() const OVERRIDE;
	virtual bool IsCheckedOut() const OVERRIDE;
	virtual bool IsCheckedOutOther(FString* Who = NULL) const OVERRIDE;
	virtual bool IsCurrent() const OVERRIDE;
	virtual bool IsSourceControlled() const OVERRIDE;
	virtual bool IsAdded() const OVERRIDE;
	virtual bool IsDeleted() const OVERRIDE;
	virtual bool IsIgnored() const OVERRIDE;
	virtual bool CanEdit() const OVERRIDE;
	virtual bool IsUnknown() const OVERRIDE;
	virtual bool IsModified() const OVERRIDE;

	/** Get the state of a file */
	EPerforceState::Type GetState() const
	{
		return State;
	}

	/** Set the state of the file */
	void SetState( EPerforceState::Type InState )
	{
		State = InState;
	}

public:
	/** History of the item, if any */
	TArray< TSharedRef<FPerforceSourceControlRevision, ESPMode::ThreadSafe> > History;

	/** Filename on disk */
	FString LocalFilename;

	/** Filename in the Perforce depot */
	FString DepotFilename;

	/** If another user has this file checked out, this contains their name(s). Multiple users are comma-delimited */
	FString OtherUserCheckedOut;

	/** Status of the file */
	EPerforceState::Type State;

	/** Modified from depot version */
	bool bModifed;

	/** Whether the file is a binary file or not */
	bool bBinary;

	/** Whether the file is a marked for exclusive checkout or not */
	bool bExclusiveCheckout;

	/** The timestamp of the last update */
	FDateTime TimeStamp;
};
