// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NetworkFileServerClientConnection.h: Declares the NetworkFileServerClientConnection class.
=============================================================================*/

#pragma once


/**
 * This class processes all incoming messages from the client.
 */
class FNetworkFileServerClientConnection
	: public FRunnable
	, public INetworkFileServerConnection
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InSocket - The client socket to use.
	 * @param InFileRequestDelegate - A delegate to be invoked when the client requests a file.
	 */
	FNetworkFileServerClientConnection( FSocket* InSocket, const FFileRequestDelegate& InFileRequestDelegate, const FRecompileShadersDelegate& InRecompileShadersDelegate );

	/**
	 * Destructor.
	 */
	~FNetworkFileServerClientConnection( );

	virtual FString GetDescription() const OVERRIDE
	{
		TSharedRef<FInternetAddr> ClientAddr = ISocketSubsystem::Get()->CreateInternetAddr();
		Socket->GetAddress(*ClientAddr);
		const FString Desc = ClientAddr->ToString( true );
		return Desc;
	}

	// Begin FRunnable Interface.

	virtual bool Init( ) OVERRIDE;

	virtual uint32 Run( ) OVERRIDE;

	virtual void Stop( ) OVERRIDE;

	virtual void Exit( ) OVERRIDE;

	// End FRunnable Interface

	// Begin INetworkFileServerConnection interface

	virtual void Close( ) OVERRIDE
	{
		Stop();
	}

	virtual bool IsOpen( ) const OVERRIDE
	{
		return !bNeedsToStop;
	}

	// End INetworkFileServerConnection interface


protected:

	/**
	 *	Convert the given filename from the client to the server version of it
	 *	NOTE: Potentially modifies the input FString!!!!
	 *
	 *	@param	FilenameToConvert		Upon input, the client version of the filename. After the call, the server version
	 */
	void ConvertClientFilenameToServerFilename(FString& FilenameToConvert);

	/**
	 *	Convert the given filename from the server to the client version of it
	 *	NOTE: Potentially modifies the input FString!!!!
	 *
	 *	@param	FilenameToConvert		Upon input, the server version of the filename. After the call, the client version
	 */
	void ConvertServerFilenameToClientFilename(FString& FilenameToConvert);

	/** Executes actions from the received payload. */
	void ProcessPayload(FArchive& Ar);

	/** Opens a file for reading or writing. */
	void ProcessOpenFile(FArchive& In, FArchive& Out, bool bIsWriting);

	/** Reads from file. */
	void ProcessReadFile(FArchive& In, FArchive& Out);

	/** Writes to file. */
	void ProcessWriteFile(FArchive& In, FArchive& Out);

	/** Seeks in file. */
	void ProcessSeekFile(FArchive& In, FArchive& Out);

	/** Closes file handle and removes it from the open handles list. */
	void ProcessCloseFile(FArchive& In, FArchive& Out);

	/** Gets info on the specified file. */
	void ProcessGetFileInfo(FArchive& In, FArchive& Out);

	/** Moves file. */
	void ProcessMoveFile(FArchive& In, FArchive& Out);

	/** Deletes file. */
	void ProcessDeleteFile(FArchive& In, FArchive& Out);

	/** Copies file. */
	void ProcessCopyFile(FArchive& In, FArchive& Out);

	/** Sets file timestamp. */
	void ProcessSetTimeStamp(FArchive& In, FArchive& Out);

	/** Sets read only flag. */
	void ProcessSetReadOnly(FArchive& In, FArchive& Out);

	/** Creates directory. */
	void ProcessCreateDirectory(FArchive& In, FArchive& Out);

	/** Deletes directory. */
	void ProcessDeleteDirectory(FArchive& In, FArchive& Out);

	/** Deletes directory recursively. */
	void ProcessDeleteDirectoryRecursively(FArchive& In, FArchive& Out);

	/** ConvertToAbsolutePathForExternalAppForRead. */
	void ProcessToAbsolutePathForRead(FArchive& In, FArchive& Out);

	/** ConvertToAbsolutePathForExternalAppForWrite. */
	void ProcessToAbsolutePathForWrite(FArchive& In, FArchive& Out);

	/** Reposts local files. */
	void ProcessReportLocalFiles(FArchive& In, FArchive& Out);

	/** Walk over a set of directories, and get all files (recursively) in them, along with their timestamps. */
	void ProcessGetFileList(FArchive& In, FArchive& Out);

	/** Heartbeat. */
	void ProcessHeartbeat(FArchive& In, FArchive& Out);

	/** 
	 * Finds open file handle by its ID.
	 *
	 * @param HandleId
	 * @return Pointer to the file handle, NULL if the specified handle id doesn't exist.
	 */
	FORCEINLINE IFileHandle* FindOpenFile( uint64 HandleId )
	{
		IFileHandle** OpenFile = OpenFiles.Find(HandleId);

		return OpenFile ? *OpenFile : NULL;
	}

	void PackageFile( FString& Filename, FArchive& Out );

	/**
	 * Processes a RecompileShaders message.
	 *
	 * @param In -
	 * @param Out -
	 */
	void ProcessRecompileShaders( FArchive& In, FArchive& Out );

	/**
	 * Processes a heartbeat message.
	 *
	 * @param In -
	 * @param Out -
	 */
	void ProcessSyncFile( FArchive& In, FArchive& Out );


private:

	// Hold the name of the currently connected platform.
	FString ConnectedPlatformName;

	// Hold the engine directory from the connected platform.
	FString ConnectedEngineDir;

	// Hold the game directory from the connected platform.
	FString ConnectedGameDir;

	// Holds the last assigned handle id (0 = invalid).
	uint64 LastHandleId;

	// Holds the multi-channel socket.
	class FMultichannelTcpSocket* MCSocket;

	// Holds the list of files found by the directory watcher.
	TArray<FString> ModifiedFiles;

	// Holds a critical section to protect the ModifiedFiles array.
	FCriticalSection ModifiedFilesSection;

	// Holds all currently open file handles.
	TMap<uint64, IFileHandle*> OpenFiles;

	// Holds the file interface for local (to the server) files - all file ops MUST go through here, NOT IFileManager.
	FSandboxPlatformFile* Sandbox;

	// Holds the client socket.
	FSocket* Socket;

	// Holds the client connection thread.
	FRunnableThread* Thread;

	// Holds the list of unsolicited files to send in separate packets.
	TArray<FString> UnsolictedFiles;

	// Holds the list of directories being watched.
	TArray<FString> WatchedDirectories;

	// Holds a delegate to be invoked on every sync request.
	FFileRequestDelegate FileRequestDelegate;

	// Holds a delegate to be invoked when a client requests a shader recompile.
	FRecompileShadersDelegate RecompileShadersDelegate;

	/** Holds a flag indicating that the thread should be stopped. */
	bool bNeedsToStop;
};