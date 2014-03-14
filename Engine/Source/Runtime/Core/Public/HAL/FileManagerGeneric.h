// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FileManagerGeneric.h: Unreal generic file manager support code.

	This base class simplifies IFileManager implementations by providing
	simple, unoptimized implementations of functions whose implementations
	can be derived from other functions.
=============================================================================*/

#pragma once

/*-----------------------------------------------------------------------------
	File Manager.
-----------------------------------------------------------------------------*/

class CORE_API FFileManagerGeneric : public IFileManager
{
	// instead of caching the LowLevel, we call the singleton each time to never be incorrect
	FORCEINLINE IPlatformFile& GetLowLevel() const
	{
		return FPlatformFileManager::Get().GetPlatformFile();
	}

public:

	FFileManagerGeneric()
	{
	}

	virtual ~FFileManagerGeneric()
	{
	}

	virtual void ProcessCommandLineOptions() OVERRIDE;

	virtual void SetSandboxEnabled(bool bInEnabled) OVERRIDE
	{
		GetLowLevel().SetSandboxEnabled(bInEnabled);
	}

	virtual bool IsSandboxEnabled() const OVERRIDE
	{
		return GetLowLevel().IsSandboxEnabled();
	}

	FArchive* CreateFileReader( const TCHAR* Filename, uint32 ReadFlags=0 ) OVERRIDE;	
	FArchive* CreateFileWriter( const TCHAR* Filename, uint32 WriteFlags=0 ) OVERRIDE;

#if ALLOW_DEBUG_FILES
	FArchive* CreateDebugFileWriter( const TCHAR* Filename, uint32 WriteFlags=0 ) OVERRIDE
	{
		return CreateFileWriter( Filename, WriteFlags );
	}
#endif

	bool Delete( const TCHAR* Filename, bool RequireExists=0, bool EvenReadOnly=0, bool Quiet=0 ) OVERRIDE;
	bool IsReadOnly( const TCHAR* Filename ) OVERRIDE;
	bool Move( const TCHAR* Dest, const TCHAR* Src, bool Replace=1, bool EvenIfReadOnly=0, bool Attributes=0, bool bDoNotRetryOrError=0 ) OVERRIDE;
	bool DirectoryExists(const TCHAR* InDirectory) OVERRIDE;
	void FindFiles( TArray<FString>& Result, const TCHAR* Filename, bool Files, bool Directories ) OVERRIDE;
	void FindFilesRecursive( TArray<FString>& FileNames, const TCHAR* StartDirectory, const TCHAR* Filename, bool Files, bool Directories, bool bClearFileNames=true) OVERRIDE;
	double GetFileAgeSeconds( const TCHAR* Filename ) OVERRIDE;
	FDateTime GetTimeStamp( const TCHAR* Filename ) OVERRIDE;
	FDateTime GetAccessTimeStamp( const TCHAR* Filename ) OVERRIDE;
	bool SetTimeStamp( const TCHAR* Filename, FDateTime Timestamp ) OVERRIDE;

	virtual uint32	Copy( const TCHAR* InDestFile, const TCHAR* InSrcFile, bool ReplaceExisting, bool EvenIfReadOnly, bool Attributes, FCopyProgress* Progress );
	virtual bool	MakeDirectory( const TCHAR* Path, bool Tree=0 );
	virtual bool	DeleteDirectory( const TCHAR* Path, bool RequireExists=0, bool Tree=0 );

	/**
	 * Converts passed in filename to use a relative path.
	 *
	 * @param	Filename	filename to convert to use a relative path
	 * 
	 * @return	filename using relative path
	 */
	static FString DefaultConvertToRelativePath( const TCHAR* Filename );

	/**
	 * Converts passed in filename to use a relative path.
	 *
	 * @param	Filename	filename to convert to use a relative path
	 * 
	 * @return	filename using relative path
	 */
	FString ConvertToRelativePath( const TCHAR* Filename ) OVERRIDE;

	/**
	 * Converts passed in filename to use an absolute path (for reading)
	 *
	 * @param	Filename	filename to convert to use an absolute path, safe to pass in already using absolute path
	 * 
	 * @return	filename using absolute path
	 */
	FString ConvertToAbsolutePathForExternalAppForRead( const TCHAR* Filename ) OVERRIDE;

	/**
	 * Converts passed in filename to use an absolute path (for writing)
	 *
	 * @param	Filename	filename to convert to use an absolute path, safe to pass in already using absolute path
	 * 
	 * @return	filename using absolute path
	 */
	FString ConvertToAbsolutePathForExternalAppForWrite( const TCHAR* Filename ) OVERRIDE;

	/**
	 *	Returns the size of a file. (Thread-safe)
	 *
	 *	@param Filename		Platform-independent Unreal filename.
	 *	@return				File size in bytes or INDEX_NONE if the file didn't exist.
	 **/
	int64 FileSize( const TCHAR* Filename ) OVERRIDE;

	/**
	 * Sends a message to the file server, and will block until it's complete. Will return 
	 * immediately if the file manager doesn't support talking to a server.
	 *
	 * @param Message	The string message to send to the server
	 *
	 * @return			true if the message was sent to server and it returned success, or false if there is no server, or the command failed
	 */
	virtual bool SendMessageToServer(const TCHAR* Message, IPlatformFile::IFileServerMessageHandler* Handler) OVERRIDE
	{
		return GetLowLevel().SendMessageToServer(Message, Handler);
	}

private:

	/**
	 * Helper called from Copy if Progress is available
	 */
	uint32	CopyWithProgress( const TCHAR* InDestFile, const TCHAR* InSrcFile, bool ReplaceExisting, bool EvenIfReadOnly, bool Attributes, FCopyProgress* Progress );

	void FindFilesRecursiveInternal( TArray<FString>& FileNames, const TCHAR* StartDirectory, const TCHAR* Filename, bool Files, bool Directories);
};

/*-----------------------------------------------------------------------------
	FArchiveFileReaderGeneric
-----------------------------------------------------------------------------*/

class CORE_API FArchiveFileReaderGeneric : public FArchive
{
public:
	FArchiveFileReaderGeneric( IFileHandle* InHandle, const TCHAR* InFilename, int64 InSize );
	~FArchiveFileReaderGeneric();

	virtual void Seek( int64 InPos ) FINAL;
	virtual int64 Tell() FINAL
	{
		return Pos;
	}
	virtual int64 TotalSize() FINAL
	{
		return Size;
	}
	virtual bool Close() FINAL;
	virtual void Serialize( void* V, int64 Length ) FINAL;

protected:
	bool InternalPrecache( int64 PrecacheOffset, int64 PrecacheSize );
	/** 
	 * Platform specific seek
	 * @param InPos - Offset from beginning of file to seek to
	 * @return false on failure 
	**/
	virtual bool SeekLowLevel(int64 InPos);
	/** Close the file handle **/
	virtual void CloseLowLevel();
	/** 
	 * Platform specific read
	 * @param Dest - Buffer to fill in
	 * @param CountToRead - Number of bytes to read
	 * @param OutBytesRead - Bytes actually read
	**/
	virtual void ReadLowLevel(uint8* Dest, int64 CountToRead, int64& OutBytesRead);

	/** Filename for debugging purposes. */
	FString			Filename;
	int64            Size;
	int64            Pos;
	int64            BufferBase;
	int64            BufferCount;
	TAutoPtr<IFileHandle>	Handle;
	uint8            Buffer[1024];	
};


/*-----------------------------------------------------------------------------
	FArchiveFileWriterGeneric
-----------------------------------------------------------------------------*/

class FArchiveFileWriterGeneric : public FArchive
{
public:
	FArchiveFileWriterGeneric( IFileHandle* InHandle, const TCHAR* InFilename, int64 InPos );
	~FArchiveFileWriterGeneric();

	virtual void Seek( int64 InPos ) FINAL;
	virtual int64 Tell() FINAL
	{
		return Pos;
	}
	virtual int64 TotalSize();
	virtual bool Close() FINAL;
	virtual void Serialize( void* V, int64 Length ) FINAL;
	virtual void Flush() FINAL;

protected:
	/** 
	 * Platform specific seek
	 * @param InPos - Offset from beginning of file to seek to
	 * @return false on failure 
	**/
	virtual bool SeekLowLevel(int64 InPos);
	/** 
	 * Close the file handle
	 * @return false on failure
	**/
	virtual bool CloseLowLevel();
	/** 
	 * Platform specific write
	 * @param Src - Buffer to write out
	 * @param CountToWrite - Number of bytes to write
	 * @return false on failure 
	**/
	virtual bool WriteLowLevel(const uint8* Src, int64 CountToWrite);


	/** Filename for debugging purposes */
	FString			Filename;
	int64            Pos;
	int64            BufferCount;
	TAutoPtr<IFileHandle>		 Handle;
	uint8            Buffer[4096];
	
};

