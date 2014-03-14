// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PakFilePrivatePCH.h"
#include "IPlatformFilePak.h"
#include "SecureHash.h"
#include "FileManagerGeneric.h"
#include "ModuleManager.h"
#include "IPlatformFileModule.h"
#include "IOBase.h"
#include "BigInt.h"
#include "SignedArchiveReader.h"
#include "PublicKey.inl"

DEFINE_LOG_CATEGORY(LogPakFile);

FPakFile::FPakFile(const TCHAR* Filename, bool bIsSigned)
	: PakFilename(Filename)
	, bSigned(bIsSigned)
	, bIsValid(false)
{
	FArchive* Reader = GetSharedReader(NULL);
	if (Reader)
	{
		Timestamp = IFileManager::Get().GetTimeStamp(Filename);
		Initialize(Reader);
	}
}

FPakFile::FPakFile(IPlatformFile* LowerLevel, const TCHAR* Filename, bool bIsSigned)
	: PakFilename(Filename)
	, bSigned(bIsSigned)
	, bIsValid(false)
{
	FArchive* Reader = GetSharedReader(LowerLevel);
	if (Reader)
	{
		Timestamp = LowerLevel->GetTimeStamp(Filename);
		Initialize(Reader);
	}
}

FPakFile::~FPakFile()
{
}

FArchive* FPakFile::CreatePakReader(const TCHAR* Filename)
{
	FArchive* ReaderArchive = IFileManager::Get().CreateFileReader(Filename);
	return SetupSignedPakReader(ReaderArchive);
}

FArchive* FPakFile::CreatePakReader(IFileHandle& InHandle, const TCHAR* Filename)
{
	FArchive* ReaderArchive = new FArchiveFileReaderGeneric(&InHandle, Filename, InHandle.Size());
	return SetupSignedPakReader(ReaderArchive);
}

FArchive* FPakFile::SetupSignedPakReader(FArchive* ReaderArchive)
{
#if !USING_SIGNED_CONTENT
	if (bSigned || FParse::Param(FCommandLine::Get(), TEXT("signedpak")) || FParse::Param(FCommandLine::Get(), TEXT("signed")))
#endif
	{	
		if (!Decryptor.IsValid())
		{
			Decryptor = new FChunkCacheWorker(ReaderArchive);
		}
		ReaderArchive = new FSignedArchiveReader(ReaderArchive, Decryptor);
	}
	return ReaderArchive;
}

void FPakFile::Initialize(FArchive* Reader)
{
	if (Reader->TotalSize() < Info.GetSerializedSize())
	{
		UE_LOG(LogPakFile, Fatal, TEXT("Corrupted pak file (too short)."));
	}
	else
	{
		// Serialize trailer and check if everything is as expected.
		Reader->Seek(Reader->TotalSize() - Info.GetSerializedSize());
		Info.Serialize(*Reader);
		check(Info.Magic == FPakInfo::PakFile_Magic);
		check(Info.Version >= FPakInfo::PakFile_Version_Initial && Info.Version <= FPakInfo::PakFile_Version_Latest);

		LoadIndex(Reader);
		// LoadIndex should crash in case of an error, so just assume everything is ok if we got here.
		bIsValid = true;
	}	
}

void FPakFile::LoadIndex(FArchive* Reader)
{
	if (Reader->TotalSize() < (Info.IndexOffset + Info.IndexSize))
	{
		UE_LOG(LogPakFile, Fatal, TEXT("Corrupted index offset in pak file."));
	}
	else
	{
		// Load index into memory first.
		Reader->Seek(Info.IndexOffset);
		TArray<uint8> IndexData;
		IndexData.AddUninitialized(Info.IndexSize);
		Reader->Serialize(IndexData.GetData(), Info.IndexSize);
		FMemoryReader IndexReader(IndexData);

		// Check SHA1 value.
		uint8 IndexHash[20];
		FSHA1::HashBuffer(IndexData.GetData(), IndexData.Num(), IndexHash);
		if (FMemory::Memcmp(IndexHash, Info.IndexHash, sizeof(IndexHash)) != 0)
		{
			UE_LOG(LogPakFile, Fatal, TEXT("Corrupted index in pak file (CRC mismatch)."));
		}

		// Read the default mount point and all entries.
		int32 NumEntries = 0;
		IndexReader << MountPoint;
		IndexReader << NumEntries;

		MakeDirectoryFromPath(MountPoint);
		// Allocate enough memory to hold all entries (and not reallocate while they're being added to it).
		Files.Empty(NumEntries);

		for (int32 EntryIndex = 0; EntryIndex < NumEntries; EntryIndex++)
		{
			// Serialize from memory.
			FPakEntry Entry;
			FString Filename;
			IndexReader << Filename;
			Entry.Serialize(IndexReader, Info.Version);

			// Add new file info.
			Files.Add(Entry);

			// Construct Index of all directories in pak file.
			FString Path = FPaths::GetPath(Filename);
			MakeDirectoryFromPath(Path);
			FPakDirectory* Directory = Index.Find(Path);
			if (Directory != NULL)
			{
				Directory->Add(Filename, &Files.Last());	
			}
			else
			{
				FPakDirectory NewDirectory;
				NewDirectory.Add(Filename, &Files.Last());
				Index.Add(Path, NewDirectory);

				// add the parent directories up to the mount point
				while (MountPoint != Path)
				{
					Path = Path.Left(Path.Len()-1);
					int32 Offset = 0;
					if (Path.FindLastChar('/', Offset))
					{
						Path = Path.Left(Offset);
						MakeDirectoryFromPath(Path);
						if (Index.Find(Path) == NULL)
						{
							FPakDirectory ParentDirectory;
							Index.Add(Path, ParentDirectory);
						}
					}
					else
					{
						Path = MountPoint;
					}
				}
			}
		}
	}
}

FArchive* FPakFile::GetSharedReader(IPlatformFile* LowerLevel)
{
	uint32 Thread = FPlatformTLS::GetCurrentThreadId();
	FArchive* PakReader = NULL;
	{
		FScopeLock ScopedLock(&CriticalSection);
		TAutoPtr<FArchive>* ExistingReader = ReaderMap.Find(Thread);
		if (ExistingReader)
		{
			PakReader = *ExistingReader;
		}
	}
	if (!PakReader)
	{
		// Create a new FArchive reader and pass it to the new handle.
		if (LowerLevel != NULL)
		{
			IFileHandle* PakHandle = LowerLevel->OpenRead(*GetFilename());
			if (PakHandle)
			{
				PakReader = CreatePakReader(*PakHandle, *GetFilename());
			}
		}
		else
		{
			PakReader = CreatePakReader(*GetFilename());
		}
		if (!PakReader)
		{
			UE_LOG(LogPakFile, Fatal, TEXT("Unable to create pak \"%s\" handle"), *GetFilename());
		}
		{
			FScopeLock ScopedLock(&CriticalSection);
			ReaderMap.Emplace(Thread, PakReader);
		}		
	}
	return PakReader;
}

#if !UE_BUILD_SHIPPING
class FPakExec : private FSelfRegisteringExec
{
	FPakPlatformFile& PlatformFile;

public:

	FPakExec(FPakPlatformFile& InPlatformFile)
		: PlatformFile(InPlatformFile)
	{}

	/** Console commands **/
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) OVERRIDE
	{
		if (FParse::Command(&Cmd, TEXT("Mount")))
		{
			PlatformFile.HandleMountCommand(Cmd, Ar);
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("PakList")))
		{
			PlatformFile.HandlePakListCommand(Cmd, Ar);
			return true;
		}
		return false;
	}
};
static TAutoPtr<FPakExec> GPakExec;

void FPakPlatformFile::HandleMountCommand(const TCHAR* Cmd, FOutputDevice& Ar)
{
	const FString PakFilename = FParse::Token(Cmd, false);
	if (!PakFilename.IsEmpty())
	{
		const FString MountPoint = FParse::Token(Cmd, false);
		Mount(*PakFilename, MountPoint.IsEmpty() ? NULL : *MountPoint);
	}
}

void FPakPlatformFile::HandlePakListCommand(const TCHAR* Cmd, FOutputDevice& Ar)
{
	TArray<FPakFile*> Paks;
	GetMountedPaks(Paks);
	for (auto Pak : Paks)
	{
		Ar.Logf(TEXT("%s"), *Pak->GetFilename());
	}	
}
#endif // !UE_BUILD_SHIPPING

FPakPlatformFile::FPakPlatformFile()
	: LowerLevel(NULL)
	, bSigned(false)
{
}

FPakPlatformFile::~FPakPlatformFile()
{
	// We need to flush async IO... if it hasn't been shut down already.
	if (FIOSystem::HasShutdown() == false)
	{
		FIOSystem& IOSystem = FIOSystem::Get();
		IOSystem.BlockTillAllRequestsFinishedAndFlushHandles();
	}

	{
		FScopeLock ScopedLock(&PakListCritical);
		for (int32 PakFileIndex = 0; PakFileIndex < PakFiles.Num(); PakFileIndex++)
		{
			delete PakFiles[PakFileIndex];
		}
	}
}

void FPakPlatformFile::FindPakFilesInDirectory(IPlatformFile* LowLevelFile, const TCHAR* Directory, TArray<FString>& OutPakFiles)
{
	// Helper class to find all pak files.
	class FPakSearchVisitor : public IPlatformFile::FDirectoryVisitor
	{
		TArray<FString>& FoundPakFiles;
	public:
		FPakSearchVisitor(TArray<FString>& InFoundPakFiles)
			: FoundPakFiles(InFoundPakFiles)
		{}
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
		{
			if (bIsDirectory == false)
			{
				FString Filename(FilenameOrDirectory);
				if (FPaths::GetExtension(Filename) == TEXT("pak"))
				{
					FoundPakFiles.Add(Filename);
				}
			}
			return true;
		}
	};
	// Find all pak files.
	FPakSearchVisitor Visitor(OutPakFiles);
	LowLevelFile->IterateDirectoryRecursively(Directory, Visitor);
}

void FPakPlatformFile::FindAllPakFiles(IPlatformFile* LowLevelFile, const TArray<FString>& PakFolders, TArray<FString>& OutPakFiles)
{
	// Find pak files from the specified directories.	
	for (int32 FolderIndex = 0; FolderIndex < PakFolders.Num(); ++FolderIndex)
	{
		FindPakFilesInDirectory(LowLevelFile, *PakFolders[FolderIndex], OutPakFiles);
	}
}

void FPakPlatformFile::GetPakFolders(const TCHAR* CmdLine, TArray<FString>& OutPakFolders)
{
#if !UE_BUILD_SHIPPING
	// Command line folders
	FString PakDirs;
	if (FParse::Value(CmdLine, TEXT("-pakdir="), PakDirs))
	{
		TArray<FString> CmdLineFolders;
		PakDirs.ParseIntoArray(&CmdLineFolders, TEXT("*"), true);
		OutPakFolders.Append(CmdLineFolders);
	}
#endif

	// @todo plugin urgent: Needs to handle plugin Pak directories, too
	// Hardcoded locations
	OutPakFolders.Add(FString::Printf(TEXT("%sPaks/"), *FPaths::GameContentDir()));
	OutPakFolders.Add(FString::Printf(TEXT("%sPaks/"), *FPaths::GameSavedDir()));
	OutPakFolders.Add(FString::Printf(TEXT("%sPaks/"), *FPaths::EngineContentDir()));
}

bool FPakPlatformFile::CheckIfPakFilesExist(IPlatformFile* LowLevelFile, const TArray<FString>& PakFolders)
{
	TArray<FString> FoundPakFiles;
	FindAllPakFiles(LowLevelFile, PakFolders, FoundPakFiles);
	return FoundPakFiles.Num() > 0;
}

bool FPakPlatformFile::ShouldBeUsed(IPlatformFile* Inner, const TCHAR* CmdLine) const
{
#if !USING_SIGNED_CONTENT
	bool Result = FParse::Param(CmdLine, TEXT("Pak")) || FParse::Param(CmdLine, TEXT("Signedpak")) || FParse::Param(CmdLine, TEXT("Signed"));
	if (FPlatformProperties::RequiresCookedData() && !Result && !FParse::Param(CmdLine, TEXT("NoPak")))
	{
		TArray<FString> PakFolders;
		GetPakFolders(CmdLine, PakFolders);
		Result = CheckIfPakFilesExist(Inner, PakFolders);
	}
	return Result;
#else
	return true;
#endif
}

bool FPakPlatformFile::Initialize(IPlatformFile* Inner, const TCHAR* CmdLine)
{
	// Inner is required.
	check(Inner != NULL);
	LowerLevel = Inner;

#if !USING_SIGNED_CONTENT
	bSigned = FParse::Param(CmdLine, TEXT("Signedpak")) || FParse::Param(CmdLine, TEXT("Signed"));
	if (!bSigned)
	{
		// Even if -signed is not provided in the command line, use signed reader if the hardcoded key is non-zero.
		FEncryptionKey DecryptionKey;
		DecryptionKey.Exponent.Parse(DECRYPTION_KEY_EXPONENT);
		DecryptionKey.Modulus.Parse(DECYRPTION_KEY_MODULUS);
		bSigned = !DecryptionKey.Exponent.IsZero() && !DecryptionKey.Modulus.IsZero();
	}
#else
	bSigned = true;
#endif
	
	TArray<FString> PaksToLoad;
#if !UE_BUILD_SHIPPING
	// Optionally get a list of pak filenames to load, only these paks will be mounted
	FString CmdLinePaksToLoad;
	if (FParse::Value(CmdLine, TEXT("-paklist="), CmdLinePaksToLoad))
	{
		CmdLinePaksToLoad.ParseIntoArray(&PaksToLoad, TEXT("+"), true);
	}
#endif

	// Find and mount pak files from the specified directories.
	TArray<FString> PakFolders;
	GetPakFolders(CmdLine, PakFolders);
	TArray<FString> FoundPakFiles;
	FindAllPakFiles(LowerLevel, PakFolders, FoundPakFiles);
	// Sort in descending order.
	FoundPakFiles.Sort(TGreater<FString>());
	// Mount all found pak files
	for (int32 PakFileIndex = 0; PakFileIndex < FoundPakFiles.Num(); PakFileIndex++)
	{
		const FString& PakFilename = FoundPakFiles[PakFileIndex];
		bool bLoadPak = true;
		if (PaksToLoad.Num() && !PaksToLoad.Contains(FPaths::GetBaseFilename(PakFilename)))
		{
			bLoadPak = false;
		}
		if (bLoadPak)
		{
			Mount(*PakFilename);
		}
	}

#if !UE_BUILD_SHIPPING
	GPakExec = new FPakExec(*this);
#endif // !UE_BUILD_SHIPPING

	return !!LowerLevel;
}

void FPakPlatformFile::Mount(const TCHAR* InPakFilename, const TCHAR* InPath /*= NULL*/)
{
	IFileHandle* PakHandle = LowerLevel->OpenRead(InPakFilename);
	if (PakHandle != NULL)
	{
		FPakFile* Pak = new FPakFile(LowerLevel, InPakFilename, bSigned);
		if (Pak->IsValid())
		{
			if (InPath != NULL)
			{
				Pak->SetMountPoint(InPath);
			}
			{
				// Add new pak file
				FScopeLock ScopedLock(&PakListCritical);
				PakFiles.Add(Pak);
			}
		}
		else
		{
			UE_LOG(LogPakFile, Warning, TEXT("Failed to mount pak \"%s\", pak is invalid."), InPakFilename);
		}
	}
	else
	{
		UE_LOG(LogPakFile, Warning, TEXT("Pak \"%s\" does not exist!"), InPakFilename);
	}
}

IFileHandle* FPakPlatformFile::CreatePakFileHandle(const TCHAR* Filename, FPakFile* PakFile, const FPakEntry* FileEntry)
{
	IFileHandle* Result = NULL;
	FArchive* PakReader = PakFile->GetSharedReader(LowerLevel);

	// Insufficent but at least minimal pak file corruption protection.
	FPakEntry FileHeader;
	PakReader->Seek(FileEntry->Offset);
	FileHeader.Serialize(*PakReader, PakFile->GetInfo().Version);
	if (VerifyHeaderAndPakEntry(*FileEntry, FileHeader))
	{
		// Ok to create the handle.
		Result = new FPakFileHandle(*PakFile, *FileEntry, PakReader, true);		
	}
	else
	{
		UE_LOG(LogPakFile, Fatal, TEXT("Pak file \"%s\" corruption detected when trying to create a handle for \"%s\"."), *PakFile->GetFilename(), Filename);
	}

	return Result;
}

bool FPakPlatformFile::VerifyHeaderAndPakEntry(const FPakEntry& FileEntry, const FPakEntry& FileHeader) const
{
	bool bResult = true;
	if (FileEntry.Size != FileHeader.Size)
	{
		UE_LOG(LogPakFile, Error, TEXT("Pak header file size mismatch, got: %lld, expected: %lld"), FileHeader.Size, FileEntry.Size);
		bResult = false;		
	}
	if (FileEntry.UncompressedSize != FileHeader.UncompressedSize)
	{
		UE_LOG(LogPakFile, Error, TEXT("Pak header uncompressed file size mismatch, got: %lld, expected: %lld"), FileHeader.UncompressedSize, FileEntry.UncompressedSize);
		bResult = false;
	}
	if (FileEntry.CompressionMethod != FileHeader.CompressionMethod)
	{
		UE_LOG(LogPakFile, Error, TEXT("Pak header file compression method mismatch, got: %d, expected: %d"), FileHeader.CompressionMethod, FileEntry.CompressionMethod);
		bResult = false;
	}
	if (FMemory::Memcmp(FileEntry.Hash, FileHeader.Hash, sizeof(FileEntry.Hash)) != 0)
	{
		UE_LOG(LogPakFile, Error, TEXT("Pak file hash does not match its index entry"));
		bResult = false;
	}
	return bResult;
}

IFileHandle* FPakPlatformFile::OpenRead(const TCHAR* Filename)
{
	IFileHandle* Result = NULL;
	FPakFile* PakFile = NULL;
	const FPakEntry* FileEntry = FindFileInPakFiles(Filename, &PakFile);	
	if (FileEntry != NULL)
	{
		Result = CreatePakFileHandle(Filename, PakFile, FileEntry);
	}
#if !USING_SIGNED_CONTENT
	else if (!bSigned)
	{
		// Default to wrapped file but only if we don't force use signed content
		Result = LowerLevel->OpenRead(Filename);
	}
#endif
	return Result;
}

bool FPakPlatformFile::BufferedCopyFile(IFileHandle& Dest, IFileHandle& Source, const int64 FileSize, uint8* Buffer, const int64 BufferSize) const
{	
	int64 RemainingSizeToCopy = FileSize;
	// Continue copying chunks using the buffer
	while (RemainingSizeToCopy > 0)
	{
		const int64 SizeToCopy = FMath::Min(BufferSize, RemainingSizeToCopy);
		if (Source.Read(Buffer, SizeToCopy) == false)
		{
			return false;
		}
		if (Dest.Write(Buffer, SizeToCopy) == false)
		{
			return false;
		}
		RemainingSizeToCopy -= SizeToCopy;
	}
	return true;
}

bool FPakPlatformFile::CopyFile(const TCHAR* To, const TCHAR* From)
{
	bool Result = false;
	FPakFile* PakFile = NULL;
	const FPakEntry* FileEntry = FindFileInPakFiles(From, &PakFile);	
	if (FileEntry != NULL)
	{
		// Copy from pak to LowerLevel->
		// Create handles both files.
		TAutoPtr<IFileHandle> DestHandle(LowerLevel->OpenWrite(To));
		TAutoPtr<IFileHandle> SourceHandle(CreatePakFileHandle(From, PakFile, FileEntry));

		if (DestHandle.IsValid() && SourceHandle.IsValid())
		{
			const int64 BufferSize = 64 * 1024; // Copy in 64K chunks.
			uint8* Buffer = (uint8*)FMemory::Malloc(BufferSize);
			Result = BufferedCopyFile(*DestHandle, *SourceHandle, SourceHandle->Size(), Buffer, BufferSize);
			FMemory::Free(Buffer);
		}
	}
	else
	{
		Result = LowerLevel->CopyFile(To, From);
	}
	return Result;
}


/**
 * Module for the pak file
 */
class FPakFileModule : public IPlatformFileModule
{
public:
	virtual IPlatformFile* GetPlatformFile() OVERRIDE
	{
		static TScopedPointer<IPlatformFile> AutoDestroySingleton(new FPakPlatformFile());
		return AutoDestroySingleton.GetOwnedPointer();
	}
};

IMPLEMENT_MODULE(FPakFileModule, PakFile);
