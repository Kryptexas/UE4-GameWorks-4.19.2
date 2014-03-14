// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LocalTimestampDirectoryVisitor.h: Declares the FLocalTimestampDirectoryVisitor class.
=============================================================================*/

#pragma once


/**
 * Visitor to gather local files with their timestamps
 */
class CORE_API FLocalTimestampDirectoryVisitor
	: public IPlatformFile::FDirectoryVisitor
{
public:

	/** Relative paths to local files and their timestamps */
	TMap<FString, FDateTime> FileTimes;

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InFileInterface - The platform file interface to use.
	 * @param InDirectoriesToIgnore - The list of directories to ignore.
	 * @param InDirectoriesToNotRecurse - The list of directories to not visit recursively.
	 * @param bInCacheDirectories - Whether to cache the directories.
	 */
	FLocalTimestampDirectoryVisitor( IPlatformFile& InFileInterface, const TArray<FString>& InDirectoriesToIgnore, const TArray<FString>& InDirectoriesToNotRecurse, bool bInCacheDirectories = false );


public:

	// Begin IPlatformFile::FDirectoryVisitor interface

	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory);

	// End IPlatformFile::FDirectoryVisitor interface


private:

	// true if we want directories in this list.
	bool bCacheDirectories;

	// Holds a list of directories that we should not traverse into.
	TArray<FString> DirectoriesToIgnore;

	// Holds a list of directories that we should only go one level into.
	TArray<FString> DirectoriesToNotRecurse;

	// Holds the file interface to use for any file operations.
	IPlatformFile& FileInterface;
};