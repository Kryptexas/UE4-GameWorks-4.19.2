// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISourceControlState.h"
#include "SubversionSourceControlRevision.h"

namespace EWorkingCopyState
{
	enum Type
	{
		Unknown,
		Pristine,
		Added,
		Deleted,
		Modified,
		Replaced,
		Conflicted,
		External,
		Ignored,
		Incomplete,
		Merged,
		NotControlled,
		Obstructed,
		Missing,
	};
}

namespace ELockState
{
	enum Type
	{
		Unknown,
		NotLocked,
		Locked,
		LockedOther,
	};
}

class FSubversionSourceControlState : public ISourceControlState, public TSharedFromThis<FSubversionSourceControlState, ESPMode::ThreadSafe>
{
public:
	FSubversionSourceControlState( const FString& InLocalFilename )
		: LocalFilename(InLocalFilename)
		, bNewerVersionOnServer(false)
		, WorkingCopyState(EWorkingCopyState::Unknown)
		, LockState(ELockState::Unknown)
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

public:
	/** History of the item, if any */
	TArray< TSharedRef<FSubversionSourceControlRevision, ESPMode::ThreadSafe> > History;

	/** Filename on disk */
	FString LocalFilename;

	/** Whether a newer version exists on the server */
	bool bNewerVersionOnServer;

	/** State of the working copy */
	EWorkingCopyState::Type WorkingCopyState;

	/** Lock state */
	ELockState::Type LockState;

	/** Name of other user who has file locked */
	FString LockUser;

	/** The timestamp of the last update */
	FDateTime TimeStamp;
};
