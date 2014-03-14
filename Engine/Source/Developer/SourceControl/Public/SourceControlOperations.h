// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISourceControlOperation.h"

#define LOCTEXT_NAMESPACE "SourceControl"

/**
 * Operation used to connect (or test a connection) to source control
 */
class FConnect : public ISourceControlOperation
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const OVERRIDE 
	{ 
		return "Connect"; 
	}

	virtual FText GetInProgressString() const OVERRIDE
	{ 
		return LOCTEXT("SourceControl_Connecting", "Connecting to source control..."); 
	}

	const FString& GetPassword() const
	{
		return Password;
	}

	void SetPassword(const FString& InPassword)
	{
		Password = InPassword;
	}

protected:
	/** Password we use for this operation */
	FString Password;
};

/**
 * Operation used to check files into source control
 */
class FCheckIn : public ISourceControlOperation
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const OVERRIDE
	{
		return "CheckIn";
	}

	virtual FText GetInProgressString() const OVERRIDE
	{ 
		return LOCTEXT("SourceControl_CheckIn", "Checking file(s) into Source Control..."); 
	}

	void SetDescription( const FString& InDescription )
	{
		Description = InDescription;
	}

	const FString& GetDescription() const
	{
		return Description;
	}

protected:
	/** Description of the checkin */
	FString Description;
};

/**
 * Operation used to check files out of source control
 */
class FCheckOut : public ISourceControlOperation
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const OVERRIDE
	{
		return "CheckOut";
	}

	virtual FText GetInProgressString() const OVERRIDE
	{ 
		return LOCTEXT("SourceControl_CheckOut", "Checking file(s) out of Source Control..."); 
	}
};

/**
 * Operation used to mark files for add in source control
 */
class FMarkForAdd : public ISourceControlOperation
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const OVERRIDE
	{
		return "MarkForAdd";
	}

	virtual FText GetInProgressString() const OVERRIDE
	{ 
		return LOCTEXT("SourceControl_Add", "Adding file(s) to Source Control..."); 
	}
};

/**
 * Operation used to mark files for delete in source control
 */
class FDelete : public ISourceControlOperation
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const OVERRIDE
	{
		return "Delete";
	}

	virtual FText GetInProgressString() const OVERRIDE
	{ 
		return LOCTEXT("SourceControl_Delete", "Deleting file(s) from Source Control..."); 
	}
};

/**
 * Operation used to revert changes made back to the state they are in source control
 */
class FRevert : public ISourceControlOperation
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const OVERRIDE
	{
		return "Revert";
	}

	virtual FText GetInProgressString() const OVERRIDE
	{ 
		return LOCTEXT("SourceControl_Revert", "Reverting file(s) in Source Control..."); 
	}	
};

/**
 * Operation used to sync files to the state they are in source control
 */
class FSync : public ISourceControlOperation
{
public:
	// ISourceControlOperation interface
	virtual FName GetName() const OVERRIDE
	{
		return "Sync";
	}

	virtual FText GetInProgressString() const OVERRIDE
	{ 
		return LOCTEXT("SourceControl_Sync", "Syncing file(s) from source control..."); 
	}	

	void SetRevision( int32 InRevisionNumber )
	{
		RevisionNumber = InRevisionNumber;
	}

protected:
	/** Revision to sync to */
	int32 RevisionNumber;
};

/**
 * Operation used to update the source control status of files
 */
class FUpdateStatus : public ISourceControlOperation
{
public:
	FUpdateStatus()
		: bUpdateHistory(false)
		, bGetOpenedOnly(false)
		, bUpdateModifiedState(false)
	{
	}

	// ISourceControlOperation interface
	virtual FName GetName() const OVERRIDE
	{
		return "UpdateStatus";
	}

	virtual FText GetInProgressString() const OVERRIDE
	{ 
		return LOCTEXT("SourceControl_Update", "Updating file(s) source control status..."); 
	}	

	void SetUpdateHistory( bool bInUpdateHistory )
	{
		bUpdateHistory = bInUpdateHistory;
	}

	void SetGetOpenedOnly( bool bInGetOpenedOnly )
	{
		bGetOpenedOnly = bInGetOpenedOnly;
	}

	void SetUpdateModifiedState( bool bInUpdateModifiedState )
	{
		bUpdateModifiedState = bInUpdateModifiedState;
	}

	bool ShouldUpdateHistory() const
	{
		return bUpdateHistory;
	}

	bool ShouldGetOpenedOnly() const
	{
		return bGetOpenedOnly;
	}

	bool ShouldUpdateModifiedState() const
	{
		return bUpdateModifiedState;
	}

protected:
	/** Whether to update history */
	bool bUpdateHistory;

	/** Whether to just get files that are opened/edited */
	bool bGetOpenedOnly;

	/** Whether to update the modified state - expensive */
	bool bUpdateModifiedState;
};

#undef LOCTEXT_NAMESPACE