// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISourceControlRevision.h"

class FPerforceSourceControlRevision : public ISourceControlRevision, public TSharedFromThis<FPerforceSourceControlRevision, ESPMode::ThreadSafe>
{
public:
	FPerforceSourceControlRevision()
		: RevisionNumber(0)
		, ChangelistNumber(0)
		, FileSize(0)
	{
	}

	/** ISourceControlRevision interface */
	virtual bool Get( FString& InOutFilename ) const OVERRIDE;
	virtual bool GetAnnotated( TArray<FAnnotationLine>& OutLines ) const OVERRIDE;
	virtual bool GetAnnotated( FString& InOutFilename ) const OVERRIDE;
	virtual const FString& GetFilename() const OVERRIDE;
	virtual int32 GetRevisionNumber() const OVERRIDE;
	virtual const FString& GetDescription() const OVERRIDE;
	virtual const FString& GetUserName() const OVERRIDE;
	virtual const FString& GetClientSpec() const OVERRIDE;
	virtual const FString& GetAction() const OVERRIDE;
	virtual const FDateTime& GetDate() const OVERRIDE;
	virtual int32 GetCheckInIdentifier() const OVERRIDE;
	virtual int32 GetFileSize() const OVERRIDE;

public:
	/** The local filename the this revision refers to */
	FString FileName;

	/** The revision number of this file */
	int32 RevisionNumber;

	/** The changelist description of this revision */
	FString Description;

	/** THe user that made the change for this revision */
	FString UserName;

	/** THe workspace the change was made form */
	FString ClientSpec;

	/** The action (edit, add etc.) that was performed for at this revision */
	FString Action;

	/** The date of this revision */
	FDateTime Date;

	/** The changelist number of this revision */
	int32 ChangelistNumber;

	/** The size of the change */
	int32 FileSize;
};