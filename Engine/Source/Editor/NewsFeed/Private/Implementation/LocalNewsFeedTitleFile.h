// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once


/**
 * Local file system based title file implementation (for testing).
 * This code was salvaged from the deprecated EpicContent plug-in.
 */
class FLocalNewsFeedTitleFile
	: public IOnlineTitleFile
{
public:

	static IOnlineTitleFilePtr Create( const FString& RootDirectory );

public:

	// Begin IOnlineTitleFile interface

	virtual bool GetFileContents(const FString& DLName, TArray<uint8>& FileContents) OVERRIDE;

	virtual bool ClearFiles() OVERRIDE;

	virtual bool ClearFile(const FString& DLName) OVERRIDE;

	virtual bool EnumerateFiles() OVERRIDE;

	virtual void GetFileList(TArray<FCloudFileHeader>& InFileHeaders) OVERRIDE;

	virtual bool ReadFile(const FString& DLName) OVERRIDE;

	// End IOnlineTitleFile interface

protected:

	FString GetFileNameFromDLName( const FString& DLName ) const;

private:

	FLocalNewsFeedTitleFile( const FString& InRootDirectory );

private:

	FString RootDirectory;
	TArray<FCloudFileHeader> FileHeaders;
	TMap< FString, TArray<uint8> > DLNameToFileContents;
};
