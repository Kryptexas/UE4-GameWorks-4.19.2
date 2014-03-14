// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

PAKFILE_API DECLARE_LOG_CATEGORY_EXTERN(LogPakFile, Log, All);

/**
 * Struct which holds pak file info (version, index offset, hash value).
 */
struct FPakInfo
{
	enum 
	{
		/** Magic number to use in header */
		PakFile_Magic = 0x5A6F12E1,
		/** Size of cached data. */
		MaxChunkDataSize = 256*1024,
	};

	/** Version numbers. */
	enum
	{
		PakFile_Version_Initial = 1,
		PakFile_Version_NoTimestamps = 2,

		PakFile_Version_Latest = PakFile_Version_NoTimestamps
	};

	/** Pak file magic value. */
	uint32 Magic;
	/** Pak file version. */
	int32 Version;
	/** Offset to pak file index. */
	int64 IndexOffset;
	/** Size (in bytes) of pak file index. */
	int64 IndexSize;
	/** Index SHA1 value. */
	uint8 IndexHash[20];

	/**
	 * Constructor.
	 */
	FPakInfo()
		: Magic(PakFile_Magic)
		, Version(PakFile_Version_Latest)
		, IndexOffset(-1)
		, IndexSize(0)
	{
		FMemory::Memset(IndexHash, 0, sizeof(IndexHash));
	}

	/**
	 * Gets the size of data serialized by this struct.
	 *
	 * @return Serialized data size.
	 */
	int64 GetSerializedSize() const
	{
		return sizeof(Magic) + sizeof(Version) + sizeof(IndexOffset) + sizeof(IndexSize) + sizeof(IndexHash);
	}

	/**
	 * Serializes this struct.
	 *
	 * @param Ar Archive to serialize data with.
	 */
	void Serialize(FArchive& Ar)
	{
		if (Ar.IsLoading() && Ar.TotalSize() < (Ar.Tell() + GetSerializedSize()))
		{
			Magic = 0;
			return;
		}

		Ar << Magic;
		Ar << Version;
		Ar << IndexOffset;
		Ar << IndexSize;
		Ar.Serialize(IndexHash, sizeof(IndexHash));
	}
};

/**
 * Struct holding info about a single file stored in pak file.
 */
struct FPakEntry
{
	/** Offset into pak file where the file is stored.*/
	int64 Offset;
	/** Serialized file size. */
	int64 Size;
	/** Uncompressed file size. */
	int64 UncompressedSize;
	/** Compression method. */
	int32 CompressionMethod;
	/** File SHA1 value. */
	uint8 Hash[20];

	/**
	 * Constructor.
	 */
	FPakEntry()
		: Offset(-1)
		, Size(0)
		, UncompressedSize(0)
		, CompressionMethod(0)
	{
		FMemory::Memset(Hash, 0, sizeof(Hash));
	}

	/**
	 * Gets the size of data serialized by this struct.
	 *
	 * @return Serialized data size.
	 */
	int64 GetSerializedSize(int32 Version) const
	{
		int64 SerializedSize = sizeof(Offset) + sizeof(Size) + sizeof(UncompressedSize) + sizeof(CompressionMethod) + sizeof(Hash);
		if (Version < FPakInfo::PakFile_Version_NoTimestamps)
		{
			// Timestamp
			SerializedSize += sizeof(int64);
		}
		return SerializedSize;
	}

	/**
	 * Compares two FPakEntry structs.
	 */
	bool operator == (const FPakEntry& B) const
	{
		// Offsets are not compared here because they're not
		// serialized with file headers anyway.
		return Size == B.Size && 
			UncompressedSize == B.UncompressedSize &&
			CompressionMethod == B.CompressionMethod &&
			FMemory::Memcmp(Hash, B.Hash, sizeof(Hash)) == 0;
	}

	/**
	 * Compares two FPakEntry structs.
	 */
	bool operator != (const FPakEntry& B) const
	{
		// Offsets are not compared here because they're not
		// serialized with file headers anyway.
		return Size != B.Size || 
			UncompressedSize != B.UncompressedSize ||
			CompressionMethod != B.CompressionMethod ||
			FMemory::Memcmp(Hash, B.Hash, sizeof(Hash)) != 0;
	}

	/**
	 * Serializes FPakEntry struct.
	 *
	 * @param Ar Archive to serialize data with.
	 * @param Entry Data to serialize.
	 */
	void Serialize(FArchive& Ar, int32 Version)
	{
		Ar << Offset;
		Ar << Size;
		Ar << UncompressedSize;
		Ar << CompressionMethod;
		if (Version <= FPakInfo::PakFile_Version_Initial)
		{
			FDateTime Timestamp;
			Ar << Timestamp;
		}
		Ar.Serialize(Hash, sizeof(Hash));
	}
};

/** Pak directory type. */
typedef TMap<FString, FPakEntry*> FPakDirectory;

/**
 * Pak file.
 */
class PAKFILE_API FPakFile
{
	/** Pak filename. */
	FString PakFilename;
	/** Archive to serialize the pak file from. */
	TAutoPtr<class FChunkCacheWorker> Decryptor;
	/** Map of readers assigned to threads. */
	TMap<uint32, TAutoPtr<FArchive>> ReaderMap;
	/** Critical section for accessing ReaderMap. */
	FCriticalSection CriticalSection;
	/** Pak file info (trailer). */
	FPakInfo Info;
	/** Mount point. */
	FString MountPoint;
	/** Info on all files stored in pak. */
	TArray<FPakEntry> Files;	
	/** Pak Index organized as a map of directories for faster Directory iteration. */
	TMap<FString, FPakDirectory> Index;
	/** Timestamp of this pak file. */
	FDateTime Timestamp;	
	/** True if this is a signed pak file. */
	bool bSigned;
	/** True if this pak file is valid and usable */
	bool bIsValid;

	FArchive* CreatePakReader(const TCHAR* Filename);
	FArchive* CreatePakReader(IFileHandle& InHandle, const TCHAR* Filename);
	FArchive* SetupSignedPakReader(FArchive* Reader);

public:

	/**
	 * Opens a pak file given its filename.
	 *
	 * @param Filename Pak filename.
	 * @param bIsSigned true if the pak is signed
	 */
	FPakFile(const TCHAR* Filename, bool bIsSigned);	

	/**
	 * Creates a pak file using the supplied file handle.
	 *
	 * @param LowerLevel Lower level platform file.
	 * @param Filename Filename.
	 * @param bIsSigned true if the pak is signed
	 */
	FPakFile(IPlatformFile* LowerLevel, const TCHAR* Filename, bool bIsSigned);

	~FPakFile();

	/**
	 * Checks if the pak file is valid.
	 *
	 * @return true if this pak file is valid, false otherwise.
	 */
	bool IsValid() const
	{
		return bIsValid;
	}

	/**
	 * Gets pak filename.
	 *
	 * @return Pak filename.
	 */
	const FString& GetFilename() const
	{
		return PakFilename;
	}

	/**
	 * Gets pak file index.
	 *
	 * @return Pak index.
	 */
	const TMap<FString, FPakDirectory>& GetIndex() const
	{
		return Index;
	}

	/**
	 * Gets shared pak file archive for given thrad
	 *
	 * @return Pointer to pak file archive used to read data from pak.
	 */
	FArchive* GetSharedReader(IPlatformFile* LowerLevel);

	/**
	 * Finds an entry in the pak file matching the given filename.
	 *
	 * @param Filename File to find.
	 * @return Pointer to pak file entry if the file was found, NULL otherwise.
	 */
	const FPakEntry* Find(const FString& Filename) const
	{		
		const FPakEntry*const * FoundFile = NULL;
		if (Filename.StartsWith(MountPoint))
		{
			FString Path(FPaths::GetPath(Filename));
			const FPakDirectory* PakDirectory = FindDirectory(*Path);
			if (PakDirectory != NULL)
			{
				FString RelativeFilename(Filename.Mid(MountPoint.Len()));
				FoundFile = PakDirectory->Find(RelativeFilename);
			}
		}
		return FoundFile ? *FoundFile : NULL;
	}

	/**
	 * Sets the pak file mount point.
	 *
	 * @param Path New mount point path.
	 */
	void SetMountPoint(const TCHAR* Path)
	{
		MountPoint = Path;
		MakeDirectoryFromPath(MountPoint);
	}

	/**
	 * Gets pak file mount point.
	 *
	 * @return Mount point path.
	 */
	const FString& GetMountPoint() const
	{
		return MountPoint;
	}

	/**
	 * Looks for files or directories within the pak file.
	 *
	 * @param OutFiles List of files or folder matching search criteria.
	 * @param InPath Path to look for files or folder at.
	 * @param bIncludeFiles If true OutFiles will include matching files.
	 * @param bIncludeDirectories If true OutFiles will include matching folders.
	 * @param bRecursive If true, sub-folders will also be checked.
	 */
	template <class ContainerType>
	void FindFilesAtPath(ContainerType& OutFiles, const TCHAR* InPath, bool bIncludeFiles = true, bool bIncludeDirectories = false, bool bRecursive = false) const
	{
		// Make sure all directory names end with '/'.
		FString Directory(InPath);
		MakeDirectoryFromPath(Directory);

		// Check the specified path is under the mount point of this pak file.
		// The reverse case (MountPoint StartsWith Directory) is needed to properly handle
		// pak files that are a subdirectory of the actual directory
		if ((Directory.StartsWith(MountPoint)) || (MountPoint.StartsWith(Directory)))
		{
			TArray<FString> DirectoriesInPak; // List of all unique directories at path
			for (TMap<FString, FPakDirectory>::TConstIterator It(Index); It; ++It)
			{
				FString PakPath(MountPoint + It.Key());
				// Check if the file is under the specified path.
				if (PakPath.StartsWith(Directory))
				{				
					if (bRecursive == true)
					{
						// Add everything
						if (bIncludeFiles)
						{
							for (FPakDirectory::TConstIterator DirectoryIt(It.Value()); DirectoryIt; ++DirectoryIt)
							{
								OutFiles.Add(MountPoint + DirectoryIt.Key());
							}
						}
						if (bIncludeDirectories)
						{
							if (Directory != PakPath)
							{
								DirectoriesInPak.Add(PakPath);
							}
						}
					}
					else
					{
						int32 SubDirIndex = PakPath.Len() > Directory.Len() ? PakPath.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Directory.Len() + 1) : INDEX_NONE;
						// Add files in the specified folder only.
						if (bIncludeFiles && SubDirIndex == INDEX_NONE)
						{
							for (FPakDirectory::TConstIterator DirectoryIt(It.Value()); DirectoryIt; ++DirectoryIt)
							{
								OutFiles.Add(MountPoint + DirectoryIt.Key());
							}
						}
						// Add sub-folders in the specified folder only
						if (bIncludeDirectories && SubDirIndex >= 0)
						{
							DirectoriesInPak.AddUnique(PakPath.Left(SubDirIndex + 1));
						}
					}
				}
			}
			OutFiles.Append(DirectoriesInPak);
		}
	}

	/**
	 * Finds a directory in pak file.
	 *
	 * @param InPath Directory path.
	 * @return Pointer to a map with directory contents if the directory was found, NULL otherwise.
	 */
	const FPakDirectory* FindDirectory(const TCHAR* InPath) const
	{
		FString Directory(InPath);
		MakeDirectoryFromPath(Directory);
		const FPakDirectory* PakDirectory = NULL;

		// Check the specified path is under the mount point of this pak file.
		if (Directory.StartsWith(MountPoint))
		{
			PakDirectory = Index.Find(Directory.Mid(MountPoint.Len()));
		}
		return PakDirectory;
	}

	/**
	 * Checks if a directory exists in pak file.
	 *
	 * @param InPath Directory path.
	 * @return true if the given path exists in pak file, false otherwise.
	 */
	bool DirectoryExists(const TCHAR* InPath) const
	{
		return !!FindDirectory(InPath);
	}

	/** Iterator class used to iterate over all files in pak. */
	class FFileIterator
	{
		/** Owner pak file. */
		const FPakFile& PakFile;
		/** Index iterator. */
		TMap<FString, FPakDirectory>::TConstIterator IndexIt;
		/** Directory iterator. */
		FPakDirectory::TConstIterator DirectoryIt;

	public:
		/**
		 * Constructor.
		 *
		 * @param InPakFile Pak file to iterate.
		 */
		FFileIterator(const FPakFile& InPakFile)
		:	PakFile(InPakFile)
		, IndexIt(PakFile.GetIndex())
		, DirectoryIt(IndexIt.Value())
		{}

		FFileIterator& operator++()		
		{ 
			// Continue with the next file
			++DirectoryIt;
			while (!DirectoryIt && IndexIt)
			{
				// No more files in the current directory, jump to the next one.
				++IndexIt;
				if (IndexIt)
				{
					// No need to check if there's files in the current directory. If a directory
					// exists in the index it is always non-empty.
					DirectoryIt.~TConstIterator();
					new(&DirectoryIt) FPakDirectory::TConstIterator(IndexIt.Value());
				}
			}
			return *this; 
		}

		SAFE_BOOL_OPERATORS(FFileIterator)
		/** conversion to "bool" returning true if the iterator is valid. */
		FORCEINLINE_EXPLICIT_OPERATOR_BOOL() const
		{ 
			return !!IndexIt; 
		}
		/** inverse of the "bool" operator */
		FORCEINLINE bool operator !() const
		{
			return !(bool)*this;
		}

		const FString& Filename() const		{ return DirectoryIt.Key(); }
		const FPakEntry& Info() const	{ return *DirectoryIt.Value(); }
	};

	/**
	 * Gets this pak file info.
	 *
	 * @return Info about this pak file.
	 */
	const FPakInfo& GetInfo() const
	{
		return Info;
	}

	/**
	 * Gets this pak file's tiemstamp.
	 *
	 * @return Timestamp.
	 */
	const FDateTime& GetTimestamp() const
	{
		return Timestamp;
	}

private:

	/**
	 * Initializes the pak file.
	 */
	void Initialize(FArchive* Reader);

	/**
	 * Loads and initializes pak file index.
	 */
	void LoadIndex(FArchive* Reader);

public:

	/**
	 * Helper function to append '/' at the end of path.
	 *
	 * @param Path - path to convert in place to directory.
	 */
	static void MakeDirectoryFromPath(FString& Path)
	{
		if (Path.Len() > 0 && Path[Path.Len() - 1] != '/')
		{
			Path += TEXT("/");
		}
	}
};

/**
 * File handle to read from pak file.
 */
class PAKFILE_API FPakFileHandle : public IFileHandle
{	
	/** Pak file entry for this file. */
	const FPakEntry& PakEntry;
	/** Pak file archive to read the data from. */
	FArchive* PakReader;
	/** True if PakReader is shared and should not be deleted by this handle. */
	const bool bSharedReader;
	/** Offset to the file in pak (including the file header). */
	int64 OffsetToFile;
	/** Current read position. */
	int64 ReadPos;

public:

	/**
	 * Constructs pak file handle to read from pak.
	 *
	 * @param InFilename Filename
	 * @param InPakEntry Entry in the pak file.
	 * @param InPakFile Pak file.
	 */
	FPakFileHandle(const FPakFile& PakFile, const FPakEntry& InPakEntry, FArchive* InPakReader, bool bIsSharedReader)
		: PakEntry(InPakEntry)
		, PakReader(InPakReader)
		, bSharedReader(bIsSharedReader)
		, ReadPos(0)
	{
		OffsetToFile = PakEntry.Offset + PakEntry.GetSerializedSize(PakFile.GetInfo().Version);
	}

	/**
	 * Destructor. Cleans up the reader archive if necessary.
	 */
	virtual ~FPakFileHandle()
	{
		if (!bSharedReader)
		{
			delete PakReader;
		}
	}

	// BEGIN IFileHandle Interface
	virtual int64 Tell() OVERRIDE
	{
		return ReadPos;
	}
	virtual bool Seek(int64 NewPosition) OVERRIDE
	{
		if (NewPosition > PakEntry.Size || NewPosition < 0)
		{
			return false;
		}
		ReadPos = NewPosition;
		return true;
	}
	virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd) OVERRIDE
	{
		return Seek(PakEntry.Size - NewPositionRelativeToEnd);
	}
	virtual bool Read(uint8* Destination, int64 BytesToRead) OVERRIDE
	{
		PakReader->Seek(OffsetToFile + ReadPos);
		if (PakEntry.Size >= (ReadPos + BytesToRead))
		{
			// Read directly from Pak.
			PakReader->Serialize(Destination, BytesToRead);
			ReadPos += BytesToRead;
			return true;
		}
		else
		{
			return false;
		}
	}
	virtual bool Write(const uint8* Source, int64 BytesToWrite) OVERRIDE
	{
		// Writing in pak files is not allowed.
		return false;
	}
	virtual int64 Size() OVERRIDE
	{
		return PakEntry.Size;
	}
	/// END IFileHandle Interface
};

/**
 * Platform file wrapper to be able to use pak files.
 **/
class PAKFILE_API FPakPlatformFile : public IPlatformFile
{
	/** Wrapped file */
	IPlatformFile* LowerLevel;
	/** List of all available pak files. */
	TArray<FPakFile*> PakFiles;
	/** True if this we're using signed content. */
	bool bSigned;
	/** Synchronization object for accessing the list of currently mounted pak files. */
	FCriticalSection PakListCritical;

	/**
	 * Gets mounted pak files
	 */
	FORCEINLINE void GetMountedPaks(TArray<FPakFile*>& Paks)
	{
		FScopeLock ScopedLock(&PakListCritical);
		Paks.Append(PakFiles);
	}

	/**
	 * Checks if a directory exists in one of the available pak files.
	 *
	 * @param Directory Directory to look for.
	 * @return true if the directory exists, false otherwise.
	 */
	bool DirectoryExistsInPakFiles(const TCHAR* Directory)
	{
		FString StandardPath = Directory;
		FPaths::MakeStandardFilename(StandardPath);

		TArray<FPakFile*> Paks;
		GetMountedPaks(Paks);

		// Check all pak files.
		for (int32 PakIndex = 0; PakIndex < Paks.Num(); PakIndex++)
		{
			if (Paks[PakIndex]->DirectoryExists(*StandardPath))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * Helper function to copy a file from one handle to another usuing the supplied buffer.
	 *
	 * @param Dest Destination file handle.
	 * @param Source file handle.
	 * @param FileSize size of the source file.
	 * @param Buffer Pointer to the buffer used to copy data.
	 * @param BufferSize Sizeof of the buffer.
	 * @return true if the operation was successfull, false otherwise.
	 */
	bool BufferedCopyFile(IFileHandle& Dest, IFileHandle& Source, const int64 FileSize, uint8* Buffer, const int64 BufferSize) const;

	/**
	 * Creates file handle to read from Pak file.
	 *
	 * @param Filename Filename to create the handle for.
	 * @param PakFile Pak file to read from.
	 * @param FileEntry File entry to create the handle for.
	 * @return Pointer to the new handle.
	 */
	IFileHandle* CreatePakFileHandle(const TCHAR* Filename, FPakFile* PakFile, const FPakEntry* FileEntry);

	/**
	* Verifies index entry with file header to check for corruption.
	*
	* @param Filename Filename
	* @param PakFile pak where the file is located
	* @param FileEntry Entry from the index.
	* @param FileHeader Header.
	*/
	bool VerifyHeaderAndPakEntry(const FPakEntry& FileEntry, const FPakEntry& FileHeader) const;

	/**
	 * Finds all pak files in the given directory.
	 *
	 * @param Directory Directory to (recursively) look for pak files in
	 * @param OutPakFiles List of pak files
	 */
	static void FindPakFilesInDirectory(IPlatformFile* LowLevelFile, const TCHAR* Directory, TArray<FString>& OutPakFiles);

	/**
	 * Finds all pak files in the known pak folders
	 *
	 * @param OutPakFiles List of all found pak files
	 */
	static void FindAllPakFiles(IPlatformFile* LowLevelFile, const TArray<FString>& PakFolders, TArray<FString>& OutPakFiles);

public:

	static const TCHAR* GetTypeName()
	{
		return TEXT("PakFile");
	}

	/**
	 * Checks if pak files exist in any of the known pak file locations.
	 */
	static bool CheckIfPakFilesExist(IPlatformFile* LowLevelFile, const TArray<FString>& PakFolders);

	/**
	 * Gets all pak file locations.
	 */
	static void GetPakFolders(const TCHAR* CmdLine, TArray<FString>& OutPakFolders);

	/**
	 * Constructor.
	 * 
	 * @param InLowerLevel Wrapper platform file.
	 */
	FPakPlatformFile();

	/**
	 * Destructor.
	 */
	virtual ~FPakPlatformFile();

	virtual bool ShouldBeUsed(IPlatformFile* Inner, const TCHAR* CmdLine) const OVERRIDE;
	virtual bool Initialize(IPlatformFile* Inner, const TCHAR* CommandLineParam) OVERRIDE;

	virtual IPlatformFile* GetLowerLevel() OVERRIDE
	{
		return LowerLevel;
	}

	virtual const TCHAR* GetName() const OVERRIDE
	{
		return FPakPlatformFile::GetTypeName();
	}

	/**
	 * Mounts a pak file at the specified path.
	 *
	 * @param InPakFilename Pak filename.
	 * @param InPath Path to mount the pak at.
	 */
	void Mount(const TCHAR* InPakFilename, const TCHAR* InPath = NULL);

	/**
	 * Finds a file in the specified pak files.
	 *
	 * @param Paks Pak files to find the file in.
	 * @param Filename File to find in pak files.
	 * @param OutPakFile Optional pointer to a pak file where the filename was found.
	 * @return Pointer to pak entry if the file was found, NULL otherwise.
	 */
	FORCEINLINE static const FPakEntry* FindFileInPakFiles(TArray<FPakFile*>& Paks, const TCHAR* Filename,  FPakFile** OutPakFile)
	{
		FString StandardFilename(Filename);
		FPaths::MakeStandardFilename(StandardFilename);
		const FPakEntry* FoundEntry = NULL;

		for (int32 PakIndex = 0; !FoundEntry && PakIndex < Paks.Num(); PakIndex++)
		{
			FoundEntry = Paks[PakIndex]->Find(*StandardFilename);
			if (FoundEntry != NULL)
			{
				if (OutPakFile != NULL)
				{
					*OutPakFile = Paks[PakIndex];
				}
			}
		}

		return FoundEntry;
	}

	/**
	 * Finds a file in all available pak files.
	 *
	 * @param Filename File to find in pak files.
	 * @param OutPakFile Optional pointer to a pak file where the filename was found.
	 * @return Pointer to pak entry if the file was found, NULL otherwise.
	 */
	const FPakEntry* FindFileInPakFiles(const TCHAR* Filename, FPakFile** OutPakFile = NULL)
	{
		TArray<FPakFile*> Paks;
		GetMountedPaks(Paks);

		return FindFileInPakFiles(Paks, Filename, OutPakFile);
	}

	// BEGIN IPlatformFile Interface
	virtual bool FileExists(const TCHAR* Filename) OVERRIDE
	{
		// Check pak files first.
		if (FindFileInPakFiles(Filename) != NULL)
		{
			return true;
		}
		// File has not been found in any of the pak files, continue looking in inner platform file.
		bool Result = LowerLevel->FileExists(Filename);
		return Result;
	}

	virtual int64 FileSize(const TCHAR* Filename) OVERRIDE
	{
		// Check pak files first
		const FPakEntry* FileEntry = FindFileInPakFiles(Filename);
		if (FileEntry != NULL)
		{
			return FileEntry->Size;
		}
		// First look for the file in the user dir.
		int64 Result = LowerLevel->FileSize(Filename);
		return Result;
	}

	virtual bool DeleteFile(const TCHAR* Filename) OVERRIDE
	{
		// If file exists in pak file it will never get deleted.
		if (FindFileInPakFiles(Filename) != NULL)
		{
			return false;
		}
		// The file does not exist in pak files, try LowerLevel->
		bool Result = LowerLevel->DeleteFile(Filename);
		return Result;
	}

	virtual bool IsReadOnly(const TCHAR* Filename) OVERRIDE
	{
		// Files in pak file are always read-only.
		if (FindFileInPakFiles(Filename) != NULL)
		{
			return true;
		}
		// The file does not exist in pak files, try LowerLevel->
		bool Result = LowerLevel->IsReadOnly(Filename);
		return Result;
	}

	virtual bool MoveFile(const TCHAR* To, const TCHAR* From) OVERRIDE
	{
		// Files which exist in pak files can't be moved
		if (FindFileInPakFiles(From) != NULL)
		{
			return false;
		}
		// Files not in pak are allowed to be moved.
		bool Result = LowerLevel->MoveFile(To, From);
		return Result;
	}

	virtual bool SetReadOnly(const TCHAR* Filename, bool bNewReadOnlyValue) OVERRIDE
	{
		// Files in pak file will never change their read-only flag.
		if (FindFileInPakFiles(Filename) != NULL)
		{
			// This fails if soemone wants to make files from pak writable.
			return bNewReadOnlyValue;
		}
		// Try lower level
		bool Result = LowerLevel->SetReadOnly(Filename, bNewReadOnlyValue);
		return Result;
	}

	virtual FDateTime GetTimeStamp(const TCHAR* Filename) OVERRIDE
	{
		// Check pak files first.
		FPakFile* PakFile = NULL;
		if (FindFileInPakFiles(Filename, &PakFile) != NULL)
		{
			return PakFile->GetTimestamp();
		}
		// Fall back to lower level.
		FDateTime Result = LowerLevel->GetTimeStamp(Filename);
		return Result;
	}

	virtual void SetTimeStamp(const TCHAR* Filename, FDateTime DateTime) OVERRIDE
	{
		// No modifications allowed on files from pak (although we could theoretically allow this one).
		if (FindFileInPakFiles(Filename) == NULL)
		{
			LowerLevel->SetTimeStamp(Filename, DateTime);
		}
	}

	virtual FDateTime GetAccessTimeStamp(const TCHAR* Filename) OVERRIDE
	{
		// AccessTimestamp not yet supported in pak files (although it is possible).
		FPakFile* PakFile = NULL;
		if (FindFileInPakFiles(Filename, &PakFile) != NULL)
		{
			return PakFile->GetTimestamp();
		}
		// Fall back to lower level.
		FDateTime Result = LowerLevel->GetAccessTimeStamp(Filename);
		return Result;
	}

	virtual IFileHandle* OpenRead(const TCHAR* Filename) OVERRIDE;

	virtual IFileHandle* OpenWrite(const TCHAR* Filename, bool bAppend = false, bool bAllowRead = false) OVERRIDE
	{
		// No modifications allowed on pak files.
		if (FindFileInPakFiles(Filename) != NULL)
		{
			return NULL;
		}
		// Use lower level to handle writing.
		return LowerLevel->OpenWrite(Filename, bAppend, bAllowRead);
	}

	virtual bool DirectoryExists(const TCHAR* Directory) OVERRIDE
	{
		// Check pak files first.
		if (DirectoryExistsInPakFiles(Directory))
		{
			return true;
		}
		// Directory does not exist in any of the pak files, continue searching using inner platform file.
		bool Result = LowerLevel->DirectoryExists(Directory); 
		return Result;
	}

	virtual bool CreateDirectory(const TCHAR* Directory) OVERRIDE
	{
		// Directories can be created only under the normal path
		return LowerLevel->CreateDirectory(Directory);
	}

	virtual bool DeleteDirectory(const TCHAR* Directory) OVERRIDE
	{
		// Even if the same directory exists outside of pak files it will never
		// get truely deleted from pak and will still be reported by Iterate functions.
		// Fail in cases like this.
		if (DirectoryExistsInPakFiles(Directory))
		{
			return false;
		}
		// Directory does not exist in pak files so it's safe to delete.
		return LowerLevel->DeleteDirectory(Directory);
	}

	/**
	 * Helper class to filter out files which have already been visited in one of the pak files.
	 */
	class FPakVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		/** Wrapped visitor. */
		FDirectoryVisitor&	Visitor;
		/** Visited pak files. */
		TSet<FString>& VisitedPakFiles;
		/** Cached list of pak files. */
		TArray<FPakFile*>& Paks;

		/** Constructor. */
		FPakVisitor(FDirectoryVisitor& InVisitor, TArray<FPakFile*>& InPaks, TSet<FString>& InVisitedPakFiles)
			: Visitor(InVisitor)
			, VisitedPakFiles(InVisitedPakFiles)
			, Paks(InPaks)
		{}
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
		{
			if (bIsDirectory == false)
			{
				FString StandardFilename(FilenameOrDirectory);
				FPaths::MakeStandardFilename(StandardFilename);

				if (VisitedPakFiles.Contains(StandardFilename))
				{
					// Already visited, continue iterating.
					return true;
				}
				else if (FPakPlatformFile::FindFileInPakFiles(Paks, FilenameOrDirectory, NULL))
				{
					VisitedPakFiles.Add(StandardFilename);
				}
			}			
			return Visitor.Visit(FilenameOrDirectory, bIsDirectory);
		}
	};

	virtual bool IterateDirectory(const TCHAR* Directory, IPlatformFile::FDirectoryVisitor& Visitor) OVERRIDE
	{
		bool Result = true;
		TSet<FString> FilesVisitedInPak;

		TArray<FPakFile*> Paks;
		GetMountedPaks(Paks);

		// Iterate pak files first
		for (int32 PakIndex = 0; PakIndex < Paks.Num(); PakIndex++)
		{
			FPakFile& PakFile = *Paks[PakIndex];
			
			const bool bIncludeFiles = true;
			const bool bIncludeFolders = true;
			TSet<FString> FilesVisitedInThisPak;
			FString StandardDirectory = Directory;
			FPaths::MakeStandardFilename(StandardDirectory);

			PakFile.FindFilesAtPath(FilesVisitedInThisPak, *StandardDirectory, bIncludeFiles, bIncludeFolders);
			for (TSet<FString>::TConstIterator SetIt(FilesVisitedInThisPak); SetIt && Result; ++SetIt)
			{
				const FString& Filename = *SetIt;
				if (!FilesVisitedInPak.Contains(Filename))
				{
					bool bIsDir = Filename.Len() && Filename[Filename.Len() - 1] == '/';
					if (bIsDir)
					{
						Result = Visitor.Visit(*Filename.LeftChop(1), true) && Result;
					}
					else
					{
						Result = Visitor.Visit(*Filename, false) && Result;
					}
					FilesVisitedInPak.Add(Filename);
				}
			}
		}
		if (Result && LowerLevel->DirectoryExists(Directory))
		{
			if (FilesVisitedInPak.Num())
			{
				// Iterate inner filesystem using FPakVisitor
				FPakVisitor PakVisitor(Visitor, Paks, FilesVisitedInPak);
				Result = LowerLevel->IterateDirectory(Directory, PakVisitor);
			}
			else
			{
				 // No point in using FPakVisitor as it will only slow things down.
				Result = LowerLevel->IterateDirectory(Directory, Visitor);
			}
		}
		return Result;
	}

	virtual bool IterateDirectoryRecursively(const TCHAR* Directory, IPlatformFile::FDirectoryVisitor& Visitor) OVERRIDE
	{
		TSet<FString> FilesVisitedInPak;
		TArray<FPakFile*> Paks;
		GetMountedPaks(Paks);
		FPakVisitor PakVisitor(Visitor, Paks, FilesVisitedInPak);
		return IPlatformFile::IterateDirectoryRecursively(Directory, PakVisitor);
	}

	virtual bool DeleteDirectoryRecursively(const TCHAR* Directory) OVERRIDE
	{
		// Can't delete directories existing in pak files. See DeleteDirectory(..) for more info.
		if (DirectoryExistsInPakFiles(Directory))
		{
			return false;
		}
		// Directory does not exist in pak files so it's safe to delete.
		return LowerLevel->DeleteDirectoryRecursively(Directory);
	}

	virtual bool CreateDirectoryTree(const TCHAR* Directory) OVERRIDE
	{
		// Directories can only be created only under the normal path
		return LowerLevel->CreateDirectoryTree(Directory);
	}

	virtual bool CopyFile(const TCHAR* To, const TCHAR* From) OVERRIDE;

	/**
	 * Converts a filename to a path inside pak file.
	 *
	 * @param Filename Filename to convert.
	 * @param Pak Pak to convert the filename realative to.
	 * @param Relative filename.
	 */
	FString ConvertToPakRelativePath(const TCHAR* Filename, const FPakFile* Pak)
	{
		FString RelativeFilename(Filename);
		return RelativeFilename.Mid(Pak->GetMountPoint().Len());
	}

	FString ConvertToAbsolutePathForExternalAppForRead(const TCHAR* Filename) OVERRIDE
	{
		// Check in Pak file first
		FPakFile* Pak = NULL;
		if (FindFileInPakFiles(Filename, &Pak) != NULL)
		{
			return FString::Printf(TEXT("Pak: %s/%s"), *Pak->GetFilename(), *ConvertToPakRelativePath(Filename, Pak));
		}
		else
		{
			return LowerLevel->ConvertToAbsolutePathForExternalAppForRead(Filename);
		}
	}

	FString ConvertToAbsolutePathForExternalAppForWrite(const TCHAR* Filename) OVERRIDE
	{
		// Check in Pak file first
		FPakFile* Pak = NULL;
		if (FindFileInPakFiles(Filename, &Pak) != NULL)
		{
			return FString::Printf(TEXT("Pak: %s/%s"), *Pak->GetFilename(), *ConvertToPakRelativePath(Filename, Pak));
		}
		else
		{
			return LowerLevel->ConvertToAbsolutePathForExternalAppForWrite(Filename);
		}
	}
	// END IPlatformFile Interface

	// BEGIN Console commands
#if !UE_BUILD_SHIPPING
	void HandlePakListCommand(const TCHAR* Cmd, FOutputDevice& Ar);
	void HandleMountCommand(const TCHAR* Cmd, FOutputDevice& Ar);
#endif
	// END Console commands
};


