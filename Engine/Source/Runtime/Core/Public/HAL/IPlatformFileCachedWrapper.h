// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class CORE_API FCachedFileHandle : public IFileHandle
{
public:
	FCachedFileHandle(IFileHandle* InFileHandle, bool bInReadable, bool bInWritable)
		: FileHandle(InFileHandle)
		, FilePos(0)
		, TellPos(0)
		, FileSize(InFileHandle->Size())
		, bWritable(bInWritable)
		, bReadable(bInReadable)
		, CurrentCache(0)
	{
		FlushCache();
	}
	
	virtual ~FCachedFileHandle()
	{
	}

	
	virtual int64		Tell() OVERRIDE
	{
		return FilePos;
	}

	virtual bool		Seek(int64 NewPosition) OVERRIDE
	{
		if (NewPosition < 0 || NewPosition > FileSize)
		{
			return false;
		}
		FilePos=NewPosition;
		return true;
	}

	virtual bool		SeekFromEnd(int64 NewPositionRelativeToEnd = 0) OVERRIDE
	{
		return Seek(FileSize - NewPositionRelativeToEnd);
	}

	virtual bool		Read(uint8* Destination, int64 BytesToRead) OVERRIDE
	{
		if (!bReadable || BytesToRead < 0 || (BytesToRead + FilePos > FileSize))
		{
			return false;
		}

		if (BytesToRead == 0)
		{
			return true;
		}

		bool Result = false;
		if (BytesToRead > BufferCacheSize) // reading more than we cache
		{
			// if the file position is within the cache, copy out the remainder of the cache
			int32 CacheIndex=GetCacheIndex(FilePos);
			if (CacheIndex < CacheCount)
			{
				int64 CopyBytes = CacheEnd[CacheIndex]-FilePos;
				FMemory::Memcpy(Destination, BufferCache[CacheIndex]+(FilePos-CacheStart[CacheIndex]), CopyBytes);
				FilePos += CopyBytes;
				BytesToRead -= CopyBytes;
				Destination += CopyBytes;
			}

			if (InnerSeek(FilePos))
			{
				Result = InnerRead(Destination, BytesToRead);
			}
			if (Result)
			{
				FilePos += BytesToRead;
			}
		}
		else
		{
			Result = true;
			
			while (BytesToRead && Result) 
			{
				uint32 CacheIndex=GetCacheIndex(FilePos);
				if (CacheIndex > CacheCount)
				{
					// need to update the cache
					uint64 AlignedFilePos=FilePos&BufferSizeMask; // Aligned Version
					uint64 SizeToRead=FMath::Min<uint64>(BufferCacheSize, FileSize-AlignedFilePos);
					InnerSeek(AlignedFilePos);
					Result = InnerRead(BufferCache[CurrentCache], SizeToRead);

					if (Result)
					{
						CacheStart[CurrentCache] = AlignedFilePos;
						CacheEnd[CurrentCache] = AlignedFilePos+SizeToRead;
						CacheIndex = CurrentCache;
						// move to next cache for update
						CurrentCache++;
						CurrentCache%=CacheCount;
					}
				}

				// copy from the cache to the destination
				if (Result)
				{
					uint64 CorrectedBytesToRead=FMath::Min<uint64>(BytesToRead, CacheEnd[CacheIndex]-FilePos);
					FMemory::Memcpy(Destination, BufferCache[CacheIndex]+(FilePos-CacheStart[CacheIndex]), CorrectedBytesToRead);
					FilePos += CorrectedBytesToRead;
					Destination += CorrectedBytesToRead;
					BytesToRead -= CorrectedBytesToRead;
				}
			}
		}
		return Result;
	}

	virtual bool		Write(const uint8* Source, int64 BytesToWrite) OVERRIDE
	{
		if (!bWritable || BytesToWrite < 0)
		{
			return false;
		}

		if (BytesToWrite == 0)
		{
			return true;
		}

		InnerSeek(FilePos);
		bool Result = FileHandle->Write(Source, BytesToWrite);
		if (Result)
		{
			FilePos += BytesToWrite;
			FileSize = FMath::Max<int64>(FilePos, FileSize);
			FlushCache();
			TellPos = FilePos;
		}
		return Result;
	}

	virtual int64		Size() OVERRIDE
	{
		return FileSize;
	}

private:

	static const uint32 BufferCacheSize = 64 * 1024; // Seems to be the magic number for best perf
	static const uint64 BufferSizeMask  = ~((uint64)BufferCacheSize-1);
	static const uint32	CacheCount		= 2;

	bool InnerSeek(uint64 Pos)
	{
		if (Pos==TellPos) 
		{
			return true;
		}
		bool bOk=FileHandle->Seek(Pos);
		if (bOk)
		{
			TellPos=Pos;
		}
		return bOk;
	}
	bool InnerRead(uint8* Dest, uint64 BytesToRead)
	{
		if (FileHandle->Read(Dest, BytesToRead))
		{
			TellPos += BytesToRead;
			return true;
		}
		return false;
	}
	int32 GetCacheIndex(int64 Pos) const
	{
		for (uint32 i=0; i<ARRAY_COUNT(CacheStart); ++i)
		{
			if (Pos >= CacheStart[i] && Pos < CacheEnd[i]) 
			{
				return i;
			}
		}
		return CacheCount+1;
	}
	void FlushCache() 
	{
		for (uint32 i=0; i<CacheCount; ++i)
		{
			CacheStart[i] = CacheEnd[i] = -1;
		}
	}

	TAutoPtr<IFileHandle>	FileHandle;
	int64					FilePos; /* Desired position in the file stream, this can be different to FilePos due to the cache */
	int64					TellPos; /* Actual position in the file,  this can be different to FilePos */
	int64					FileSize;
	bool					bWritable;
	bool					bReadable;
	uint8					BufferCache[CacheCount][BufferCacheSize];
	int64					CacheStart[CacheCount];
	int64					CacheEnd[CacheCount];
	int32					CurrentCache;
};

class CORE_API FCachedReadPlatformFile : public IPlatformFile
{
	IPlatformFile*		LowerLevel;
public:
	static const TCHAR* GetTypeName()
	{
		return TEXT("CachedReadFile");
	}

	FCachedReadPlatformFile()
		: LowerLevel(NULL)
	{
	}
	virtual bool Initialize(IPlatformFile* Inner, const TCHAR* CommandLineParam) OVERRIDE
	{
		// Inner is required.
		check(Inner != NULL);
		LowerLevel = Inner;
		return !!LowerLevel;
	}
	virtual bool ShouldBeUsed(IPlatformFile* Inner, const TCHAR* CmdLine) const
	{
		return FPlatformProperties::RequiresCookedData();
	}
	IPlatformFile* GetLowerLevel() OVERRIDE
	{
		return LowerLevel;
	}

	virtual const TCHAR* GetName() const OVERRIDE
	{
		return FCachedReadPlatformFile::GetTypeName();
	}
	virtual bool		FileExists(const TCHAR* Filename) OVERRIDE
	{
		return LowerLevel->FileExists(Filename);
	}
	virtual int64		FileSize(const TCHAR* Filename) OVERRIDE
	{
		return LowerLevel->FileSize(Filename);
	}
	virtual bool		DeleteFile(const TCHAR* Filename) OVERRIDE
	{
		return LowerLevel->DeleteFile(Filename);
	}
	virtual bool		IsReadOnly(const TCHAR* Filename) OVERRIDE
	{
		return LowerLevel->IsReadOnly(Filename);
	}
	virtual bool		MoveFile(const TCHAR* To, const TCHAR* From) OVERRIDE
	{
		return LowerLevel->MoveFile(To, From);
	}
	virtual bool		SetReadOnly(const TCHAR* Filename, bool bNewReadOnlyValue) OVERRIDE
	{
		return LowerLevel->SetReadOnly(Filename, bNewReadOnlyValue);
	}
	virtual FDateTime	GetTimeStamp(const TCHAR* Filename) OVERRIDE
	{
		return LowerLevel->GetTimeStamp(Filename);
	}
	virtual void		SetTimeStamp(const TCHAR* Filename, FDateTime DateTime) OVERRIDE
	{
		LowerLevel->SetTimeStamp(Filename, DateTime);
	}
	virtual FDateTime	GetAccessTimeStamp(const TCHAR* Filename) OVERRIDE
	{
		return LowerLevel->GetAccessTimeStamp(Filename);
	}
	virtual IFileHandle*	OpenRead(const TCHAR* Filename) OVERRIDE
	{
		IFileHandle* InnerHandle=LowerLevel->OpenRead(Filename);
		if (!InnerHandle)
		{
			return NULL;
		}
		return new FCachedFileHandle(InnerHandle, true, false);
	}
	virtual IFileHandle*	OpenWrite(const TCHAR* Filename, bool bAppend = false, bool bAllowRead = false) OVERRIDE
	{
		IFileHandle* InnerHandle=LowerLevel->OpenWrite(Filename, bAppend, bAllowRead);
		if (!InnerHandle)
		{
			return NULL;
		}
		return new FCachedFileHandle(InnerHandle, bAllowRead, true);
	}
	virtual bool		DirectoryExists(const TCHAR* Directory) OVERRIDE
	{
		return LowerLevel->DirectoryExists(Directory);
	}
	virtual bool		CreateDirectory(const TCHAR* Directory) OVERRIDE
	{
		return LowerLevel->CreateDirectory(Directory);
	}
	virtual bool		DeleteDirectory(const TCHAR* Directory) OVERRIDE
	{
		return LowerLevel->DeleteDirectory(Directory);
	}
	virtual bool		IterateDirectory(const TCHAR* Directory, IPlatformFile::FDirectoryVisitor& Visitor) OVERRIDE
	{
		return LowerLevel->IterateDirectory(Directory, Visitor);
	}
	virtual bool		IterateDirectoryRecursively(const TCHAR* Directory, IPlatformFile::FDirectoryVisitor& Visitor) OVERRIDE
	{
		return LowerLevel->IterateDirectoryRecursively(Directory, Visitor);
	}
	virtual bool		DeleteDirectoryRecursively(const TCHAR* Directory) OVERRIDE
	{
		return LowerLevel->DeleteDirectoryRecursively(Directory);
	}
	virtual bool		CopyFile(const TCHAR* To, const TCHAR* From) OVERRIDE
	{
		return LowerLevel->CopyFile(To, From);
	}
	virtual bool		CreateDirectoryTree(const TCHAR* Directory) OVERRIDE
	{
		return LowerLevel->CreateDirectoryTree(Directory);
	}
	virtual bool		CopyDirectoryTree(const TCHAR* DestinationDirectory, const TCHAR* Source, bool bOverwriteAllExisting) OVERRIDE
	{
		return LowerLevel->CopyDirectoryTree(DestinationDirectory, Source, bOverwriteAllExisting);
	}
	virtual FString		ConvertToAbsolutePathForExternalAppForRead( const TCHAR* Filename ) OVERRIDE
	{
		return LowerLevel->ConvertToAbsolutePathForExternalAppForRead(Filename);
	}
	virtual FString		ConvertToAbsolutePathForExternalAppForWrite( const TCHAR* Filename ) OVERRIDE
	{
		return LowerLevel->ConvertToAbsolutePathForExternalAppForWrite(Filename);
	}
	virtual bool		SendMessageToServer(const TCHAR* Message, IFileServerMessageHandler* Handler) OVERRIDE
	{
		return LowerLevel->SendMessageToServer(Message, Handler);
	}
};
