// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NetworkMessage.h"
#include "ServerTOC.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNetworkPlatformFile, Log, All);

/**
 * Wrapper to redirect the low level file system to a server
 */
class NETWORKFILE_API FNetworkPlatformFile : public IPlatformFile
{
	friend class FAsyncFileSync;

	/**
	 * Initialize network platform file give the specified host IP
	 *
	 * @param Inner Inner platform file
	 * @param HostIP host IP address
	 * @return true if the initialization succeeded, false otherwise
	 */
	virtual bool InitializeInternal(IPlatformFile* Inner, const TCHAR* HostIP);

public:

	static const TCHAR* GetTypeName()
	{
		return TEXT("NetworkFile");
	}

	/** Constructor */
	FNetworkPlatformFile();

	/** Destructor */
	virtual ~FNetworkPlatformFile();

	virtual bool ShouldBeUsed(IPlatformFile* Inner, const TCHAR* CmdLine) const OVERRIDE;
	virtual bool Initialize(IPlatformFile* Inner, const TCHAR* CmdLine) OVERRIDE;
	virtual void InitializeAfterSetActive() OVERRIDE;

	virtual IPlatformFile* GetLowerLevel() OVERRIDE
	{
		return InnerPlatformFile;
	}

	virtual const TCHAR* GetName() const OVERRIDE
	{
		return FNetworkPlatformFile::GetTypeName();
	}

	virtual bool IsUsable()
	{
		return bIsUsable;
	}

	virtual bool		FileExists(const TCHAR* Filename) OVERRIDE
	{
		FFileInfo Info;
		GetFileInfo(Filename, Info);
		return Info.FileExists;
	}
	virtual int64		FileSize(const TCHAR* Filename) OVERRIDE
	{
		FFileInfo Info;
		GetFileInfo(Filename, Info);
		return Info.Size;
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
	virtual IFileHandle*	OpenWrite(const TCHAR* Filename, bool bAppend = false, bool bAllowRead = false) OVERRIDE;
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

	virtual bool SendMessageToServer(const TCHAR* Message, IPlatformFile::IFileServerMessageHandler* Handler) OVERRIDE;




	bool SendReadMessage(uint8* Destination, int64 BytesToRead);
	bool SendWriteMessage(const uint8* Source, int64 BytesToWrite);

	/**
	 * This function will receive a header, and then the payload array from the network, only public for an async task
	 * 
	 * @param OutPayload The archive to read into (the response is APPENDed to any data in the archive already, and the archive will be seeked to the start of new data)
	 *
	 * @return true if successful
	 */
	bool ReceivePayload(FArrayReader& OutPayload);

	static void ConvertServerFilenameToClientFilename(FString& FilenameToConvert, const FString& InServerEngineDir, const FString& InServerGameDir);

protected:

	/**
	 * Send a heartbeat message to the file server. This will tell it we are alive, as well as
	 * get back a list of files that have been updated on the server (so we can toss our copy)
	 */
	virtual void PerformHeartbeat();

	virtual void GetFileInfo(const TCHAR* Filename, FFileInfo& Info);

	/**
	 *	Convert the given filename from the server to the client version of it
	 *	NOTE: Potentially modifies the input FString!!!!
	 *
	 *	@param	FilenameToConvert		Upon input, the server version of the filename. After the call, the client version
	 */
	virtual void ConvertServerFilenameToClientFilename(FString& FilenameToConvert);

	virtual void FillGetFileList(FNetworkFileArchive& Payload, bool bInStreamingFileRequest);

	virtual void ProcessServerInitialResponse(FArrayReader& InResponse, int32 OutServerPackageVersion, int32 OutServerPackageLicenseeVersion);

private:
	
	/**
	 * @return true if the path exists in a directory that should always use the local filesystem
	 * This version does not worry about initialization or thread safety, do not call directly
	 */
	bool IsInLocalDirectoryUnGuarded(const FString& Filename);

	/**
	 * @return true if the path exists in a directory that should always use the local filesystem
	 */
	bool IsInLocalDirectory(const FString& Filename);

	/**
	 * Given a filename, make sure the file exists on the local filesystem
	 */
	void EnsureFileIsLocal(const FString& Filename);

	/**
	 * This function will create a header for the payload, then send the header and 
	 * payload over the network
	 *
	 * @param Payload Bytes to send over the network
	 *
	 * @return true if successful
	 */
	bool WrapAndSendPayload(const TArray<uint8>& Payload);

	/**
	 * This function will send a payload data (with header) and wait for a response, serializing
	 * the response to a FBufferArchive
	 *
	 * @param Payload Bytes to send over the network
	 * @param Response The archive to read the response into
	 *
	 * @return true if successful
	 */
	bool SendPayloadAndReceiveResponse(const TArray<uint8>& Payload, class FArrayReader& Response);


protected:

	/** This is true after the DDC directories have been loaded from the DDC system */
	bool				bHasLoadedDDCDirectories;

	/** The file interface to read/write local files with */
	IPlatformFile*		InnerPlatformFile;

	/** This keeps track of what files have been "EnsureFileIsLocal'd" */
	TSet<FString>		CachedLocalFiles;

	/** The server engine dir */
	FString ServerEngineDir;

	/** The server game dir */
	FString ServerGameDir;

	/** This is the "TOC" of the server */
	FServerTOC ServerFiles;

	/** Set of directories that should use the local filesystem */
	TArray<FString> LocalDirectories;

	FCriticalSection	SynchronizationObject;
	FCriticalSection	LocalDirectoriesCriticalSection;
	bool				bIsUsable;
	int32				FileServerPort;
	class FSocket*		FileSocket;
	class FMultichannelTcpSocket* MCSocket;

private:
	FScopedEvent* FinishedAsyncReadUnsolicitedFiles;
	FScopedEvent* FinishedAsyncWriteUnsolicitedFiles;
};

class SOCKETS_API FNetworkFileHandle : public IFileHandle
{
	FNetworkPlatformFile&		Network;
	FString					Filename;
	int64						FilePos;
	int64						Size;
	bool						bWritable;
	bool						bReadable;
public:

	FNetworkFileHandle(FNetworkPlatformFile& InNetwork, const TCHAR* InFilename, int64 InFilePos, int64 InFileSize, bool bWriting)
		: Network(InNetwork)
		, Filename(InFilename)
		, FilePos(InFilePos)
		, Size(InFileSize)
		, bWritable(bWriting)
		, bReadable(!bWriting)
	{
	}

	virtual int64		Tell() OVERRIDE
	{
		return FilePos;
	}
	virtual bool		Seek(int64 NewPosition) OVERRIDE
	{
		if (NewPosition >= 0 && NewPosition <= Size)
		{
			FilePos = NewPosition;
			return true;
		}
		return false;
	}
	virtual bool		SeekFromEnd(int64 NewPositionRelativeToEnd = 0) OVERRIDE
	{
		return Seek(Size + NewPositionRelativeToEnd);
	}
	virtual bool		Read(uint8* Destination, int64 BytesToRead) OVERRIDE
	{
		bool Result = false;
		if (bReadable && BytesToRead >= 0 && BytesToRead + FilePos <= Size)
		{
			if (BytesToRead == 0)
			{
				Result = true;
			}
			else
			{
				Result = Network.SendReadMessage(Destination, BytesToRead);
				if (Result)
				{
					FilePos += BytesToRead;
				}
			}
		}
		return Result;
	}
	virtual bool		Write(const uint8* Source, int64 BytesToWrite) OVERRIDE
	{
		bool Result = false;
		if (bWritable && BytesToWrite >= 0)
		{
			if (BytesToWrite == 0)
			{
				Result = true;
			}
			else
			{
				Result = Network.SendWriteMessage(Source, BytesToWrite);
				if (Result)
				{
					FilePos += BytesToWrite;
					Size = FMath::Max<int64>(FilePos, Size);
				}
			}
		}
		return Result;
	}
};



