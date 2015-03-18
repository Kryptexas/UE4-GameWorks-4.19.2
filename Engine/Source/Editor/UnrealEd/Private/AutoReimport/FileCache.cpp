// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "FileCache.h"
#include "IDirectoryWatcher.h"
#include "DirectoryWatcherModule.h"
#include "AutoReimportUtilities.h"

/** Convert a FFileChangeData::EFileChangeAction into an EFileAction */
EFileAction ToFileAction(FFileChangeData::EFileChangeAction InAction)
{
	switch (InAction)
	{
	case FFileChangeData::FCA_Added: 	return EFileAction::Added;
	case FFileChangeData::FCA_Modified:	return EFileAction::Modified;
	case FFileChangeData::FCA_Removed:	return EFileAction::Removed;
	default:							return EFileAction::Modified;
	}
}

/** Synchronously read a file's MD5 */
FMD5Hash ReadFileMD5(const FString& Filename, TArray<uint8>* Buffer = nullptr)
{
	FArchive* Ar = IFileManager::Get().CreateFileReader(*Filename);

	FMD5Hash Hash;
	if (Ar)
	{
		TArray<uint8> LocalScratch;
		if (!Buffer)
		{
			LocalScratch.SetNumUninitialized(1024*64);
			Buffer = &LocalScratch;
		}
		FMD5 MD5;

		const int64 Size = Ar->TotalSize();
		int64 Position = 0;

		// Read in BufferSize chunks
		while (Position < Size)
		{
			const auto ReadNum = FMath::Min(Size - Position, (int64)Buffer->Num());
			Ar->Serialize(Buffer->GetData(), ReadNum);
			MD5.Update(Buffer->GetData(), ReadNum);

			Position += ReadNum;
		}

		Hash.Set(MD5);
		delete Ar;
	}

	return Hash;
}

const FGuid FFileCacheCustomVersion::Key(0x8E7DDCB3, 0x80DA47BB, 0x9FD346A2, 0x93984DF6);
FCustomVersionRegistration GRegisterFileCacheVersion(FFileCacheCustomVersion::Key, FFileCacheCustomVersion::Latest, TEXT("FileCacheVersion"));

/** Single runnable thread used to parse file cache directories without blocking the main thread */
struct FAsyncDirectoryReaderThread : public FRunnable
{
	typedef TArray<TWeakPtr<FAsyncDirectoryReader, ESPMode::ThreadSafe>> FReaderArray;

	FAsyncDirectoryReaderThread() : Thread(nullptr) {}

	/** Add a reader to this thread which will get ticked periodically until complete */
	void AddReader(TSharedPtr<FAsyncDirectoryReader, ESPMode::ThreadSafe> InReader)
	{
		FScopeLock Lock(&ReaderArrayMutex);
		Readers.Add(InReader);

		if (!Thread)
		{
			static int32 Index = 0;
			Thread = FRunnableThread::Create(this, *FString::Printf(TEXT("AsyncDirectoryReaders_%d"), ++Index));
		}
	}

	/** Run this thread */
	virtual uint32 Run()
	{
		for(;;)
		{
			// Copy the array while we tick the readers
			FReaderArray Dupl;
			{
				FScopeLock Lock(&ReaderArrayMutex);
				Dupl = Readers;
			}

			// Tick each one for a second
			for (auto& Reader : Dupl)
			{
				auto PinnedReader = Reader.Pin();
				if (PinnedReader.IsValid())
				{
					PinnedReader->Tick(FTimeLimit(1));
				}
			}

			// Cleanup dead/finished readers
			FScopeLock Lock(&ReaderArrayMutex);
			for (int32 Index = 0; Index < Readers.Num(); )
			{
				auto Reader = Readers[Index].Pin();
				if (!Reader.IsValid() || Reader->IsComplete())
				{
					Readers.RemoveAt(Index);
				}
				else
				{
					++Index;
				}
			}

			// Shutdown the thread if we've nothing left to do
			if (Readers.Num() == 0)
			{
				Thread = nullptr;
				break;
			}
		}

		return 0;
	}

private:
	/** We start our own thread if one doesn't already exist. Guard it with a critical section */
	FRunnableThread* Thread;

	/** Array of things that need ticking, and a separate mutex to protect those */
	FCriticalSection ReaderArrayMutex;
	FReaderArray Readers;
};

FAsyncDirectoryReaderThread AsyncReaderThread;

FAsyncDirectoryReader::FAsyncDirectoryReader(const FString& InDirectory, EPathType InPathType)
	: RootPath(InDirectory), PathType(InPathType), StartTime(0)
{
	// Read in files in 1MB chunks
	ScratchBuffer.SetNumUninitialized(1024 * 1024);

	PendingDirectories.Add(InDirectory);
	LiveState.Emplace();
}

TOptional<FDirectoryState> FAsyncDirectoryReader::GetLiveState()
{
	TOptional<FDirectoryState> OldState;
	Swap(OldState, LiveState);

	return OldState;
}


TOptional<FDirectoryState> FAsyncDirectoryReader::GetCachedState()
{
	TOptional<FDirectoryState> OldState;
	Swap(OldState, CachedState);

	return OldState;
}

bool FAsyncDirectoryReader::IsComplete() const
{
	return bIsComplete;
}

FAsyncDirectoryReader::EProgressResult FAsyncDirectoryReader::Tick(const FTimeLimit& TimeLimit)
{
	if (IsComplete())
	{
		return EProgressResult::Finished;
	}

	if (StartTime == 0)
	{
		StartTime = FPlatformTime::Seconds();
	}

	auto& FileManager = IFileManager::Get();
	const int32 RootPathLen = RootPath.Len();

	// Discover files
	for (int32 Index = 0; Index < PendingDirectories.Num(); ++Index)
	{
		ScanDirectory(PendingDirectories[Index]);
		if (TimeLimit.Exceeded())
		{
			// We've spent too long, bail
			PendingDirectories.RemoveAt(0, Index + 1, false);
			return EProgressResult::Pending;
		}
	}
	PendingDirectories.Empty();

	// Process files
	for (int32 Index = 0; Index < PendingFiles.Num(); ++Index)
	{
		const auto& File = PendingFiles[Index];

		// Store the file relative or absolute
		FString Filename = (PathType == EPathType::Relative ? *File + RootPathLen : *File);

		const auto Timestamp = FileManager.GetTimeStamp(*File);
	
		FMD5Hash MD5;
		if (CachedState.IsSet())
		{
			const FFileData* CachedData = CachedState->Files.Find(Filename);
			if (CachedData && CachedData->Timestamp == Timestamp && CachedData->FileHash.IsValid())
			{
				// Use the cached MD5 to avoid opening the file
				MD5 = CachedData->FileHash;
			}
		}

		if (!MD5.IsValid())
		{
			MD5 = ReadFileMD5(File, &ScratchBuffer);
		}

		LiveState->Files.Emplace(MoveTemp(Filename), FFileData(Timestamp, MD5));

		if (TimeLimit.Exceeded())
		{
			// We've spent too long, bail
			PendingFiles.RemoveAt(0, Index + 1, false);
			return EProgressResult::Pending;
		}
	}
	PendingFiles.Empty();

	bIsComplete = true;

	UE_LOG(LogAutoReimportManager, Log, TEXT("Scanning file cache directory took %.2fs"), FPlatformTime::Seconds() - StartTime);
	return EProgressResult::Finished;
}

void FAsyncDirectoryReader::ScanDirectory(const FString& InDirectory)
{
	struct FVisitor : public IPlatformFile::FDirectoryVisitor
	{
		TArray<FString>* PendingFiles;
		TArray<FString>* PendingDirectories;
		FMatchRules* Rules;
		int32 RootPathLength;

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
		{
			FString FileStr(FilenameOrDirectory);
			if (bIsDirectory)
			{
				PendingDirectories->Add(MoveTemp(FileStr));
			}
			else if (Rules->IsFileApplicable(FilenameOrDirectory + RootPathLength))
			{
				PendingFiles->Add(MoveTemp(FileStr));
			}
			return true;
		}
	};

	FVisitor Visitor;
	Visitor.PendingFiles = &PendingFiles;
	Visitor.PendingDirectories = &PendingDirectories;
	Visitor.Rules = &LiveState->Rules;
	Visitor.RootPathLength = RootPath.Len();

	IFileManager::Get().IterateDirectory(*InDirectory, Visitor);
}

FFileCache::FFileCache(const FFileCacheConfig& InConfig)
	: Config(InConfig)
	, bSavedCacheDirty(false)
{
	// Ensure the directory has a trailing /
	Config.Directory /= TEXT("");

	DirectoryReader = MakeShareable(new FAsyncDirectoryReader(Config.Directory, Config.PathType));
	DirectoryReader->SetMatchRules(Config.Rules);

	// Attempt to load an existing cache file
	auto ExistingCache = ReadCache();
	if (ExistingCache.IsSet())
	{
		DirectoryReader->UseCachedState(MoveTemp(ExistingCache.GetValue()));
	}

	AsyncReaderThread.AddReader(DirectoryReader);

	FDirectoryWatcherModule& Module = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	if (IDirectoryWatcher* DirectoryWatcher = Module.Get())
	{
		auto Callback = IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &FFileCache::OnDirectoryChanged);
		DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(Config.Directory, Callback, WatcherDelegate);
	}
}

FFileCache::~FFileCache()
{
	UnbindWatcher();
	WriteCache();
}

void FFileCache::Destroy()
{
	// Delete the cache file, and clear out everything
	bSavedCacheDirty = false;
	if (!Config.CacheFile.IsEmpty())
	{
		IFileManager::Get().Delete(*Config.CacheFile, false, true, true);
	}

	DirectoryReader = nullptr;
	PendingTransactions.Empty();
	DirtyFiles.Empty();
	CachedDirectoryState = FDirectoryState();

	UnbindWatcher();
}

const FFileData* FFileCache::FindFileData(FImmutableString InFilename) const
{
	if (!ensure(HasStartedUp()))
	{
		// It's invalid to call this while the cached state is still being updated on a thread.
		return nullptr;
	}

	return CachedDirectoryState.Files.Find(InFilename);
}

void FFileCache::UnbindWatcher()
{
	if (!WatcherDelegate.IsValid())
	{
		return;
	}

	if (FDirectoryWatcherModule* Module = FModuleManager::GetModulePtr<FDirectoryWatcherModule>(TEXT("DirectoryWatcher")))
	{
		if (IDirectoryWatcher* DirectoryWatcher = Module->Get())
		{
			DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(Config.Directory, WatcherDelegate);
		}
	}

	WatcherDelegate.Reset();
}

TOptional<FDirectoryState> FFileCache::ReadCache() const
{
	TOptional<FDirectoryState> Optional;
	if (!Config.CacheFile.IsEmpty())
	{		
		FArchive* Ar = IFileManager::Get().CreateFileReader(*Config.CacheFile);
		if (Ar)
		{
			FDirectoryState Result;

			*Ar << Result;
			Ar->Close();
			delete Ar;

			Optional.Emplace(MoveTemp(Result));
		}
	}

	return Optional;
}

void FFileCache::WriteCache()
{
	if (bSavedCacheDirty && !Config.CacheFile.IsEmpty())
	{
		const FString ParentFolder = FPaths::GetPath(Config.CacheFile);
		if (!IFileManager::Get().DirectoryExists(*ParentFolder))
		{
			IFileManager::Get().MakeDirectory(*ParentFolder, true);	
		}
		
		FArchive* Ar = IFileManager::Get().CreateFileWriter(*Config.CacheFile);
		if (ensureMsgf(Ar, TEXT("Unable to write file-cache for '%s' to '%s'."), *Config.Directory, *Config.CacheFile))
		{
			*Ar << CachedDirectoryState;

			Ar->Close();
			delete Ar;

			CachedDirectoryState.Files.Shrink();

			bSavedCacheDirty = false;
		}
	}
}

FString FFileCache::GetAbsolutePath(const FString& InTransactionPath) const
{
	if (Config.PathType == EPathType::Relative)
	{
		return Config.Directory / InTransactionPath;
	}
	else
	{
		return InTransactionPath;
	}
}

TOptional<FString> FFileCache::GetTransactionPath(const FString& InAbsolutePath) const
{
	FString Temp = FPaths::ConvertRelativePathToFull(InAbsolutePath);
	FString RelativePath(*Temp + Config.Directory.Len());

	// If it's a directory or is not applicable, ignore it
	if (!Temp.StartsWith(Config.Directory) || IFileManager::Get().DirectoryExists(*Temp) || !Config.Rules.IsFileApplicable(*RelativePath))
	{
		return TOptional<FString>();
	}

	if (Config.PathType == EPathType::Relative)
	{
		return MoveTemp(RelativePath);
	}
	else
	{
		return MoveTemp(Temp);
	}
}

void FFileCache::DiffDirtyFiles(const TSet<FImmutableString>& InDirtyFiles, TArray<FUpdateCacheTransaction>& InOutTransactions, const FDirectoryState* InFileSystemState) const
{
	TMap<FImmutableString, FFileData> AddedFiles, ModifiedFiles;
	TSet<FImmutableString> RemovedFiles;

	auto& FileManager = IFileManager::Get();
	auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	for (const auto& File : InDirtyFiles)
	{
		FString AbsoluteFilename = GetAbsolutePath(File.Get());
		
		const auto* CachedState = CachedDirectoryState.Files.Find(File);

		const bool bFileExists = InFileSystemState ? InFileSystemState->Files.Find(File) != nullptr : PlatformFile.FileExists(*AbsoluteFilename);
		if (bFileExists)
		{
			const FFileData FileData = InFileSystemState ? *InFileSystemState->Files.Find(File) : FFileData(FileManager.GetTimeStamp(*AbsoluteFilename), ReadFileMD5(AbsoluteFilename));

			// Do we think it exists in the cache?
			if (CachedState)
			{
				// Has it changed?
				if (*CachedState != FileData)
				{
					ModifiedFiles.Add(File, FileData);
				}
			}
			else
			{
				AddedFiles.Add(File, FileData);
			}
		}
		// We only report it as removed if it exists in the cache
		else if (CachedState)
		{
			RemovedFiles.Add(File);
		}
	}

	// Rename / move detection
	for (auto RemoveIt = RemovedFiles.CreateIterator(); RemoveIt; ++RemoveIt)
	{
		const auto* CachedState = CachedDirectoryState.Files.Find(*RemoveIt);
		if (CachedState)
		{
			for (auto AdIt = AddedFiles.CreateIterator(); AdIt; ++AdIt)
			{				
				if (AdIt.Value().FileHash == CachedState->FileHash)
				{
					// Found a move destination!
					InOutTransactions.Add(FUpdateCacheTransaction(*RemoveIt, AdIt.Key(), AdIt.Value()));

					AdIt.RemoveCurrent();
					RemoveIt.RemoveCurrent();
					break;
				}
			}
		}
	}

	for (auto& RemovedFile : RemovedFiles)
	{
		InOutTransactions.Add(FUpdateCacheTransaction(MoveTemp(RemovedFile), EFileAction::Removed));
	}
	// RemovedFiles is now bogus

	for (auto& Pair : AddedFiles)
	{
		InOutTransactions.Add(FUpdateCacheTransaction(MoveTemp(Pair.Key), EFileAction::Added, Pair.Value));
	}
	// AddedFiles is now bogus
	
	for (auto& Pair : ModifiedFiles)
	{
		InOutTransactions.Add(FUpdateCacheTransaction(MoveTemp(Pair.Key), EFileAction::Modified, Pair.Value));
	}
	// ModifiedFiles is now bogus
}

TArray<FUpdateCacheTransaction> FFileCache::GetOutstandingChanges()
{
	DiffDirtyFiles(DirtyFiles, PendingTransactions);
	DirtyFiles.Empty();

	TArray<FUpdateCacheTransaction> Moved;
	Swap(PendingTransactions, Moved);
	return Moved;
}

void FFileCache::IgnoreNewFile(const FString& Filename)
{
	auto TransactionPath = GetTransactionPath(Filename);
	if (TransactionPath.IsSet())
	{
		DirtyFiles.Remove(TransactionPath.GetValue());
		
		const FFileData FileData(IFileManager::Get().GetTimeStamp(*Filename), ReadFileMD5(Filename));
		CompleteTransaction(FUpdateCacheTransaction(MoveTemp(TransactionPath.GetValue()), EFileAction::Added, FileData));	
	}
}

void FFileCache::IgnoreFileModification(const FString& Filename)
{
	auto TransactionPath = GetTransactionPath(Filename);
	if (TransactionPath.IsSet())
	{
		DirtyFiles.Remove(TransactionPath.GetValue());
		
		const FFileData FileData(IFileManager::Get().GetTimeStamp(*Filename), ReadFileMD5(Filename));
		CompleteTransaction(FUpdateCacheTransaction(MoveTemp(TransactionPath.GetValue()), EFileAction::Modified, FileData));	
	}
}

void FFileCache::IgnoreMovedFile(const FString& SrcFilename, const FString& DstFilename)
{
	auto SrcTransactionPath = GetTransactionPath(SrcFilename);
	auto DstTransactionPath = GetTransactionPath(DstFilename);

	if (SrcTransactionPath.IsSet() && DstTransactionPath.IsSet())
	{
		DirtyFiles.Remove(SrcTransactionPath.GetValue());
		DirtyFiles.Remove(DstTransactionPath.GetValue());

		const FFileData FileData(IFileManager::Get().GetTimeStamp(*DstFilename), ReadFileMD5(DstFilename));
		CompleteTransaction(FUpdateCacheTransaction(MoveTemp(SrcTransactionPath.GetValue()), MoveTemp(DstTransactionPath.GetValue()), FileData));	
	}
}

void FFileCache::IgnoreDeletedFile(const FString& Filename)
{
	auto TransactionPath = GetTransactionPath(Filename);
	if (TransactionPath.IsSet())
	{
		DirtyFiles.Remove(TransactionPath.GetValue());
		CompleteTransaction(FUpdateCacheTransaction(MoveTemp(TransactionPath.GetValue()), EFileAction::Removed));	
	}
}

void FFileCache::CompleteTransaction(FUpdateCacheTransaction&& Transaction)
{
	auto* CachedData = CachedDirectoryState.Files.Find(Transaction.Filename);
	switch (Transaction.Action)
	{
	case EFileAction::Moved:
		{
			CachedDirectoryState.Files.Remove(Transaction.MovedFromFilename);
			if (!CachedData)
			{
				CachedDirectoryState.Files.Add(Transaction.Filename, Transaction.FileData);
			}
			else
			{
				*CachedData = Transaction.FileData;
			}

			bSavedCacheDirty = true;
		}
		break;
	case EFileAction::Modified:
		if (CachedData)
		{
			// Update the timestamp
			*CachedData = Transaction.FileData;

			bSavedCacheDirty = true;
		}
		break;
	case EFileAction::Added:
		if (!CachedData)
		{
			// Add the file information to the cache
			CachedDirectoryState.Files.Emplace(Transaction.Filename, Transaction.FileData);

			bSavedCacheDirty = true;
		}
		break;
	case EFileAction::Removed:
		if (CachedData)
		{
			// Remove the file information to the cache
			CachedDirectoryState.Files.Remove(Transaction.Filename);

			bSavedCacheDirty = true;
		}
		break;

	default:
		checkf(false, TEXT("Invalid file cached transaction"));
		break;
	}
}

void FFileCache::Tick()
{
	if (!DirectoryReader.IsValid() || !DirectoryReader->IsComplete())
	{
		return;
	}

	// We should only ever get here once. The directory reader has finished scanning, and we can now diff the results with what we had saved in the cache file.
	check(DirectoryReader->IsComplete());
	
	TOptional<FDirectoryState> LiveState = DirectoryReader->GetLiveState();
	TOptional<FDirectoryState> CachedState = DirectoryReader->GetCachedState();

	// Null out our pointer to the directory reader to indicate that we've finished
	DirectoryReader = nullptr;

	if (!CachedState.IsSet() || !Config.bDetectChangesSinceLastRun)
	{
		// If we don't have any cached data yet, just use the file data we just harvested
		CachedDirectoryState = MoveTemp(LiveState.GetValue());
		bSavedCacheDirty = true;
		return;
	}
	else
	{
		// Use the cache that we gave to the directory reader
		CachedDirectoryState = MoveTemp(CachedState.GetValue());
	}

	// Build up a list of dirty files
	TSet<FImmutableString> LocalDirtyFiles;

	// We already have cached data so we need to compare it with the harvested data
	// to detect additions, modifications, and removals
	for (const auto& FilenameAndData : LiveState->Files)
	{
		const FString& Filename = FilenameAndData.Key.Get();

		const auto* CachedData = CachedDirectoryState.Files.Find(Filename);
		if (!CachedData || *CachedData != FilenameAndData.Value)
		{
			LocalDirtyFiles.Add(Filename);
		}
	}

	// Check for anything that doesn't exist on disk anymore
	for (auto It = CachedDirectoryState.Files.CreateIterator(); It; ++It)
	{
		const FImmutableString& Filename = It.Key();
		if (!LiveState->Files.Contains(Filename))
		{
			LocalDirtyFiles.Add(Filename);
		}
	}

	if (LocalDirtyFiles.Num() != 0)
	{
		TArray<FUpdateCacheTransaction> TempTransactions;
		DiffDirtyFiles(LocalDirtyFiles, TempTransactions, &LiveState.GetValue());

		// Any file changes that we've detected that are not relevant to the previously cached state, we just add to the cache immediately (without telling the user)
		// This is because we have no idea whether they are new changes or not, because they were not previously cached.
		for (auto& Transaction : TempTransactions)
		{
			// Generate paths that are relative to the watch directory to pass to the match rules
			const TCHAR* TransactionFilename = *Transaction.Filename.Get();
			const TCHAR* MovedFromFilename = *Transaction.MovedFromFilename.Get();
			if (Config.PathType == EPathType::Absolute)
			{
				if (Transaction.Filename.Get().Len() > Config.Directory.Len())
				{
					TransactionFilename += Config.Directory.Len();
				}

				if (Transaction.MovedFromFilename.Get().Len() > Config.Directory.Len())
				{
					MovedFromFilename += Config.Directory.Len();
				}
			}

			const bool bAppliesToOldCache = CachedDirectoryState.Rules.IsFileApplicable(TransactionFilename) &&
				(Transaction.Action != EFileAction::Moved || CachedDirectoryState.Rules.IsFileApplicable(MovedFromFilename));

			const bool bAppliesToNewCache = LiveState->Rules.IsFileApplicable(TransactionFilename) &&
				(Transaction.Action != EFileAction::Moved || LiveState->Rules.IsFileApplicable(MovedFromFilename));

			if (bAppliesToOldCache && bAppliesToNewCache)
			{
				// Just throw away the transaction
				continue;
			}
			else
			{
				// Remove it from our dirty files
				LocalDirtyFiles.Remove(Transaction.Filename);
				if (Transaction.Action == EFileAction::Moved)
				{
					LocalDirtyFiles.Remove(Transaction.MovedFromFilename);
				}

				// Update the cache immediately and remove it from the pending transactions
				CompleteTransaction(MoveTemp(Transaction));
			}
		}
	}

	// Now the only files left in LocalDirtyFiles are things that definitely apply to the current cache, and used to apply to the old cache
	DirtyFiles = DirtyFiles.Union(LocalDirtyFiles);

	// Update the applicable extensions now that we've updated the cache
	CachedDirectoryState.Rules = LiveState->Rules;
}

void FFileCache::OnDirectoryChanged(const TArray<FFileChangeData>& FileChanges)
{
	for (const auto& ThisEntry : FileChanges)
	{
		auto TransactionPath = GetTransactionPath(ThisEntry.Filename);
		if (TransactionPath.IsSet())
		{
			DirtyFiles.Add(MoveTemp(TransactionPath.GetValue()));
		}
	}
}