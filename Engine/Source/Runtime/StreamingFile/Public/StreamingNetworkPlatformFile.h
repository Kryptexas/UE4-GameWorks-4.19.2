// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NetworkMessage.h"
#include "ServerTOC.h"
#include "NetworkPlatformFile.h"

DECLARE_LOG_CATEGORY_EXTERN(LogStreamingPlatformFile, Log, All);

#if 0
/**
 * Visitor to gather local files with their timestamps
 */
class STREAMINGFILE_API FStreamingLocalTimestampVisitor : public IPlatformFile::FDirectoryVisitor
{
private:
	/** The file interface to use for any file operations */
	IPlatformFile& FileInterface;

	/** true if we want directories in this list */
	bool bCacheDirectories;

	/** A list of directories that we should not traverse into */
	TArray<FString> DirectoriesToIgnore;

	/** A list of directories that we should only go one level into */
	TArray<FString> DirectoriesToNotRecurse;

public:
	/** Relative paths to local files and their timestamps */
	TMap<FString, FDateTime> FileTimes;
	
	FStreamingLocalTimestampVisitor(IPlatformFile& InFileInterface, const TArray<FString>& InDirectoriesToIgnore, const TArray<FString>& InDirectoriesToNotRecurse, bool bInCacheDirectories=false)
		: FileInterface(InFileInterface)
		, bCacheDirectories(bInCacheDirectories)
	{
		// make sure the paths are standardized, since the Visitor will assume they are standard
		for (int32 DirIndex = 0; DirIndex < InDirectoriesToIgnore.Num(); DirIndex++)
		{
			FString DirToIgnore = InDirectoriesToIgnore[DirIndex];
			FPaths::MakeStandardFilename(DirToIgnore);
			DirectoriesToIgnore.Add(DirToIgnore);
		}

		for (int32 DirIndex = 0; DirIndex < InDirectoriesToNotRecurse.Num(); DirIndex++)
		{
			FString DirToNotRecurse = InDirectoriesToNotRecurse[DirIndex];
			FPaths::MakeStandardFilename(DirToNotRecurse);
			DirectoriesToNotRecurse.Add(DirToNotRecurse);
		}
	}

	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
	{
		// make sure all paths are "standardized" so the other end can match up with it's own standardized paths
		FString RelativeFilename = FilenameOrDirectory;
		FPaths::MakeStandardFilename(RelativeFilename);

		// cache files and optionally directories
		if (!bIsDirectory)
		{
			FileTimes.Add(RelativeFilename, FileInterface.GetTimeStamp(FilenameOrDirectory));
		}
		else if (bCacheDirectories)
		{
			// we use a timestamp of 0 to indicate a directory
			FileTimes.Add(RelativeFilename, 0);
		}

		// iterate over directories we care about
		if (bIsDirectory)
		{
			bool bShouldRecurse = true;
			// look in all the ignore directories looking for a match
			for (int32 DirIndex = 0; DirIndex < DirectoriesToIgnore.Num() && bShouldRecurse; DirIndex++)
			{
				if (RelativeFilename.StartsWith(DirectoriesToIgnore[DirIndex]))
				{
					bShouldRecurse = false;
				}
			}

			if (bShouldRecurse == true)
			{
				// If it is a directory that we should not recurse (ie we don't want to process subdirectories of it)
				// handle that case as well...
				for (int32 DirIndex = 0; DirIndex < DirectoriesToNotRecurse.Num() && bShouldRecurse; DirIndex++)
				{
					if (RelativeFilename.StartsWith(DirectoriesToNotRecurse[DirIndex]))
					{
						// Are we more than level deep in that directory?
						FString CheckFilename = RelativeFilename.Right(RelativeFilename.Len() - DirectoriesToNotRecurse[DirIndex].Len());
						if (CheckFilename.Len() > 1)
						{
							bShouldRecurse = false;
						}
					}
				}
			}

			// recurse if we should
			if (bShouldRecurse)
			{
				FileInterface.IterateDirectory(FilenameOrDirectory, *this);
			}
		}

		return true;
	}
};
#endif

/**
 * Wrapper to redirect the low level file system to a server
 */
class STREAMINGFILE_API FStreamingNetworkPlatformFile : public FNetworkPlatformFile
{
	friend class FAsyncFileSync;

	// FNetworkPlatformFile interface
	virtual bool InitializeInternal(IPlatformFile* Inner, const TCHAR* HostIP) OVERRIDE;

public:

	static const TCHAR* GetTypeName()
	{
		return TEXT("StreamingFile");
	}

	/** Constructor */
	FStreamingNetworkPlatformFile() {};

	/** Destructor */
	virtual ~FStreamingNetworkPlatformFile();

	// need to override what FNetworkPlatformFile does here
	void InitializeAfterSetActive() OVERRIDE { }

	// Begin IPlatformFile Interface 
	virtual bool ShouldBeUsed(IPlatformFile* Inner, const TCHAR* CmdLine) const OVERRIDE;

	virtual IPlatformFile* GetLowerLevel() OVERRIDE
	{
		return NULL;
	}

	virtual const TCHAR* GetName() const OVERRIDE
	{
		return FStreamingNetworkPlatformFile::GetTypeName();
	}
	virtual bool		DeleteFile(const TCHAR* Filename) OVERRIDE;
	virtual bool		IsReadOnly(const TCHAR* Filename) OVERRIDE
	{
		FFileInfo Info;
		GetFileInfo(Filename, Info);
		return Info.ReadOnly;
	}
	virtual bool		MoveFile(const TCHAR* To, const TCHAR* From) OVERRIDE;
	virtual bool		SetReadOnly(const TCHAR* Filename, bool bNewReadOnlyValue) OVERRIDE;
	virtual FDateTime	GetTimeStamp(const TCHAR* Filename) OVERRIDE
	{
		FFileInfo Info;
		GetFileInfo(Filename, Info);
		return Info.TimeStamp;
	}
	virtual void		SetTimeStamp(const TCHAR* Filename, FDateTime DateTime) OVERRIDE;
	virtual FDateTime	GetAccessTimeStamp(const TCHAR* Filename) OVERRIDE
	{
		FFileInfo Info;
		GetFileInfo(Filename, Info);
		return Info.AccessTimeStamp;
	}
	virtual IFileHandle*	OpenRead(const TCHAR* Filename) OVERRIDE;
	virtual IFileHandle*	OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead) OVERRIDE;
	virtual bool		DirectoryExists(const TCHAR* Directory) OVERRIDE;
	virtual bool		CreateDirectoryTree(const TCHAR* Directory) OVERRIDE;
	virtual bool		CreateDirectory(const TCHAR* Directory) OVERRIDE;
	virtual bool		DeleteDirectory(const TCHAR* Directory) OVERRIDE;

	virtual bool		IterateDirectory(const TCHAR* Directory, IPlatformFile::FDirectoryVisitor& Visitor) OVERRIDE;
	virtual bool		IterateDirectoryRecursively(const TCHAR* Directory, IPlatformFile::FDirectoryVisitor& Visitor) OVERRIDE;
	virtual bool		DeleteDirectoryRecursively(const TCHAR* Directory) OVERRIDE;
	virtual bool		CopyFile(const TCHAR* To, const TCHAR* From) OVERRIDE;

	virtual FString ConvertToAbsolutePathForExternalAppForRead( const TCHAR* Filename ) OVERRIDE;
	virtual FString ConvertToAbsolutePathForExternalAppForWrite( const TCHAR* Filename ) OVERRIDE;

	// End IPlatformFile Interface

	/** Sends Open message to the server and creates a new file handle if successful. */
	class FStreamingNetworkFileHandle* SendOpenMessage(const FString& Filename, bool bIsWriting, bool bAppend, bool bAllowRead);

	/** Sends Read message to the server. */
	bool SendReadMessage(uint64 HandleId, uint8* Destination, int64 BytesToRead);

	/** Sends Write message to the server. */
	bool SendWriteMessage(uint64 HandleId, const uint8* Source, int64 BytesToWrite);	

	/** Sends Seek message to the server. */
	bool SendSeekMessage(uint64 HandleId, int64 NewPosition);	

	/** Sends Close message to the server. */
	bool SendCloseMessage(uint64 HandleId);
	
private:

	// Begin FNetworkPlatformFile interface

	virtual void PerformHeartbeat() OVERRIDE;

	virtual void GetFileInfo(const TCHAR* Filename, FFileInfo& Info) OVERRIDE;

	// End FNetworkPlatformFile interface


	/** Helper for handling network errors during send/receive. */
	bool SendPayloadAndReceiveResponse(class FStreamingNetworkFileArchive& Payload, FArrayReader& Response);


private:

	/** Set files that the server said we should sync asynchronously */
	TArray<FString> FilesToSyncAsync;

	/** Stored information about the files we have already cached */
	TMap<FString, FFileInfo> CachedFileInfo;
};




