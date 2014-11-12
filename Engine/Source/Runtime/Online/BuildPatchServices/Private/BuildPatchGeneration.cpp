// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	BuildPatchGeneration.cpp: Implements the classes that control build
	installation, and the generation of chunks and manifests from a build image.
	=============================================================================*/

#include "BuildPatchServicesPrivatePCH.h"

#if WITH_BUILDPATCHGENERATION

/**
 * Creates a 32bit hash value from a FSHAHashData. This is so that a TMap can be keyed
 * using a FSHAHashData
 * @param Hash	The SHA1 hash
 * @return The crc for the data
 */
FORCEINLINE uint32 GetTypeHash(const FSHAHashData& Hash)
{
	return FCrc::MemCrc32(Hash.Hash, FSHA1::DigestSize);
}

static bool IsUnixExecutable(const TCHAR* Filename)
{
#if PLATFORM_MAC
	struct stat FileInfo;
	if (stat(TCHAR_TO_UTF8(Filename), &FileInfo) == 0)
	{
		return (FileInfo.st_mode & S_IXUSR) != 0;
	}
#endif
	return false;
}

static FString GetSymlinkTarget(const TCHAR* Filename)
{
#if PLATFORM_MAC
	ANSICHAR SymlinkTarget[MAX_PATH] = { 0 };
	if (readlink(TCHAR_TO_UTF8(Filename), SymlinkTarget, MAX_PATH) != -1)
	{
		return UTF8_TO_TCHAR(SymlinkTarget);
	}
#endif
	return TEXT("");
}

/* FBuildStreamReader implementation
*****************************************************************************/
FBuildStream::FBuildStreamReader::FBuildStreamReader()
	: DepotDirectory(TEXT(""))
	, BuildStream(NULL)
	, Thread(NULL)
{
}

FBuildStream::FBuildStreamReader::~FBuildStreamReader()
{
	if (Thread)
	{
		Thread->Kill();
		delete Thread;
		Thread = NULL;
	}
}

bool FBuildStream::FBuildStreamReader::Init()
{
	return BuildStream != NULL && IFileManager::Get().DirectoryExists(*DepotDirectory);
}

uint32 FBuildStream::FBuildStreamReader::Run()
{
	// Clear the build stream
	BuildStream->Clear();

	TArray< FString > AllFiles;
	IFileManager::Get().FindFilesRecursive(AllFiles, *DepotDirectory, TEXT("*.*"), true, false);
	AllFiles.Sort();

	// Remove the files that appear in an ignore list
	FBuildDataGenerator::StripIgnoredFiles(AllFiles, DepotDirectory, IgnoreListFile);

	// Allocate our file read buffer
	uint8* FileReadBuffer = new uint8[FileBufferSize];

	for (auto& SourceFile : AllFiles)
	{
		// Read the file
		FArchive* FileReader = IFileManager::Get().CreateFileReader(*SourceFile);
		const bool bBuildFileOpenSuccess = FileReader != NULL;
		if (bBuildFileOpenSuccess)
		{
			// Make SourceFile the format we want it in and start a new file
			FPaths::MakePathRelativeTo(SourceFile, *(DepotDirectory + TEXT("/")));
			int64 FileSize = FileReader->TotalSize();
			// Process files that have bytes
			if (FileSize > 0)
			{
				BuildStream->BeginNewFile(SourceFile, FileSize);
				while (!FileReader->AtEnd())
				{
					const int64 SizeLeft = FileSize - FileReader->Tell();
					const uint32 ReadLen = FMath::Min< int64 >(FileBufferSize, SizeLeft);
					FileReader->Serialize(FileReadBuffer, ReadLen);
					// Copy into data stream
					BuildStream->EnqueueData(FileReadBuffer, ReadLen);
				}
			}
			// Special case zero byte files
			else if (FileSize == 0)
			{
				BuildStream->AddEmptyFile(SourceFile);
			}
			FileReader->Close();
			delete FileReader;
		}
		else
		{
			// Not being able to load a required file from the build would be fatal, hard fault.
			GLog->Logf(TEXT("FBuildStreamReader: Could not open file from build! %s"), *SourceFile);
			GLog->PanicFlushThreadedLogs();
			// Use bool variable for easier to understand assert message
			check(bBuildFileOpenSuccess);
		}
	}

	// Mark end of build
	BuildStream->EndOfBuild();

	// Deallocate our file read buffer
	delete[] FileReadBuffer;

	return 0;
}

void FBuildStream::FBuildStreamReader::StartThread()
{
	Thread = FRunnableThread::Create(this, TEXT("BuildStreamReaderThread"));
}

/* FBuildStream implementation
*****************************************************************************/
FBuildStream::FBuildStream(const FString& RootDirectory, const FString& IgnoreListFile)
{
	BuildStreamReader.DepotDirectory = RootDirectory;
	BuildStreamReader.IgnoreListFile = IgnoreListFile;
	BuildStreamReader.BuildStream = this;
	BuildStreamReader.StartThread();
}

FBuildStream::~FBuildStream()
{
}

bool FBuildStream::GetFileSpan(const uint64& StartingIdx, FString& Filename, uint64& FileSize)
{
	bool bFound = false;
	FilesListsCS.Lock();
	// Find the filename
	FFileSpan* FileSpan = FilesParsed.Find(StartingIdx);
	if (FileSpan != NULL)
	{
		Filename = FileSpan->Filename;
		FileSize = FileSpan->Size;
		bFound = true;
	}
	FilesListsCS.Unlock();
	return bFound;
}

uint32 FBuildStream::DequeueData(uint8* Buffer, const uint32& ReqSize, const bool WaitForData)
{
	// Wait for data
	if (WaitForData)
	{
		while (DataAvailable() < ReqSize && !IsEndOfBuild())
		{
			FPlatformProcess::Sleep(0.01f);
		}
	}

	BuildDataStreamCS.Lock();
	uint32 ReadLen = FMath::Min< int64 >(ReqSize, DataAvailable());
	ReadLen = BuildDataStream.Dequeue(Buffer, ReadLen);
	BuildDataStreamCS.Unlock();

	return ReadLen;
}

const TArray< FString > FBuildStream::GetEmptyFiles()
{
	FScopeLock ScopeLock(&FilesListsCS);
	return EmptyFiles;
}

bool FBuildStream::IsEndOfBuild()
{
	bool rtn;
	NoMoreDataCS.Lock();
	rtn = bNoMoreData;
	NoMoreDataCS.Unlock();
	return rtn;
}

bool FBuildStream::IsEndOfData()
{
	bool rtn;
	NoMoreDataCS.Lock();
	rtn = bNoMoreData;
	NoMoreDataCS.Unlock();
	BuildDataStreamCS.Lock();
	rtn &= BuildDataStream.RingDataUsage() == 0;
	BuildDataStreamCS.Unlock();
	return rtn;
}

void FBuildStream::Clear()
{
	EndOfBuild(false);

	BuildDataStreamCS.Lock();
	BuildDataStream.Empty();
	BuildDataStreamCS.Unlock();

	FilesListsCS.Lock();
	FilesParsed.Empty();
	FilesListsCS.Unlock();
}

uint32 FBuildStream::SpaceLeft()
{
	uint32 rtn;
	BuildDataStreamCS.Lock();
	rtn = BuildDataStream.RingDataSize() - BuildDataStream.RingDataUsage();
	BuildDataStreamCS.Unlock();
	return rtn;
}

uint32 FBuildStream::DataAvailable()
{
	uint32 rtn;
	BuildDataStreamCS.Lock();
	rtn = BuildDataStream.RingDataUsage();
	BuildDataStreamCS.Unlock();
	return rtn;
}

void FBuildStream::BeginNewFile(const FString& Filename, const uint64& FileSize)
{
	FFileSpan FileSpan;
	FileSpan.Filename = Filename;
	FileSpan.Size = FileSize;
	BuildDataStreamCS.Lock();
	FileSpan.StartIdx = BuildDataStream.TotalDataPushed();
	BuildDataStreamCS.Unlock();
	FilesListsCS.Lock();
	FilesParsed.Add(FileSpan.StartIdx, FileSpan);
	FilesListsCS.Unlock();
}

void FBuildStream::AddEmptyFile(const FString& Filename)
{
	FilesListsCS.Lock();
	EmptyFiles.Add(Filename);
	FilesListsCS.Unlock();
}

void FBuildStream::EnqueueData(const uint8* Buffer, const uint32& Len)
{
	// Wait for space
	while (SpaceLeft() < Len)
	{
		FPlatformProcess::Sleep(0.01f);
	}
	BuildDataStreamCS.Lock();
	BuildDataStream.Enqueue(Buffer, Len);
	BuildDataStreamCS.Unlock();
}

void FBuildStream::EndOfBuild(bool bIsEnd)
{
	NoMoreDataCS.Lock();
	bNoMoreData = bIsEnd;
	NoMoreDataCS.Unlock();
}

/* FBuildDataProcessor implementation
*****************************************************************************/
FBuildDataChunkProcessor::FBuildDataChunkProcessor(FBuildPatchAppManifestRef InBuildManifest, const FString& InBuildRoot)
	: NumNewChunks(0)
	, NumKnownChunks(0)
	, BuildRoot(InBuildRoot)
	, ChunkWriter(FBuildPatchServicesModule::GetCloudDirectory())
	, CurrentChunkBufferPos(0)
	, CurrentFile(nullptr)
	, IsProcessingChunk(false)
	, IsProcessingChunkPart(false)
	, IsProcessingFile(false)
	, ChunkIsPushed(false)
	, BackupChunkBufferPos(0)
	, BackupProcessingChunk(false)
	, BackupProcessingChunkPart(false)
{
	BuildManifest = InBuildManifest;
	CurrentChunkGuid.Invalidate();
	BackupChunkGuid.Invalidate();
	CurrentChunkBuffer = new uint8[FBuildPatchData::ChunkDataSize];
}

FBuildDataChunkProcessor::~FBuildDataChunkProcessor()
{
	delete[] CurrentChunkBuffer;
}

void FBuildDataChunkProcessor::BeginNewChunk(const bool& bZeroBuffer)
{
	check(IsProcessingChunk == false);
	check(IsProcessingChunkPart == false);
	IsProcessingChunk = true;

	// NB: If you change this function, make sure you make required changes to 
	// Backup of chunk if need be at the end of a recognized chunk

	// Erase the chunk buffer data ready for new chunk parts
	if (bZeroBuffer)
	{
		FMemory::Memzero(CurrentChunkBuffer, FBuildPatchData::ChunkDataSize);
		CurrentChunkBufferPos = 0;
	}

	// Get a new GUID for this chunk for identification
	CurrentChunkGuid = FGuid::NewGuid();
}

void FBuildDataChunkProcessor::BeginNewChunkPart()
{
	check(IsProcessingChunkPart == false);
	check(IsProcessingChunk == true);
	IsProcessingChunkPart = true;

	// The current file should have a new chunk part setup
	if (CurrentFile)
	{
		CurrentFile->FileChunkParts.Add(FChunkPartData());
		FChunkPartData& NewPart = CurrentFile->FileChunkParts.Last();
		NewPart.Guid = CurrentChunkGuid;
		NewPart.Offset = CurrentChunkBufferPos;
	}
}

void FBuildDataChunkProcessor::EndNewChunkPart()
{
	check(IsProcessingChunkPart == true);
	check(IsProcessingChunk == true);
	IsProcessingChunkPart = false;

	// Current file should have it's last chunk part updated with the size value finalized.
	if (CurrentFile)
	{
		FChunkPartData* ChunkPart = &CurrentFile->FileChunkParts.Last();
		ChunkPart->Size = CurrentChunkBufferPos - ChunkPart->Offset;
		check(ChunkPart->Size != 0);
	}
}

void FBuildDataChunkProcessor::EndNewChunk(const uint64& ChunkHash, const uint8* ChunkData, const FGuid& ChunkGuid)
{
	check(IsProcessingChunk == true);
	check(IsProcessingChunkPart == false);
	IsProcessingChunk = false;

	// A bool that will state whether we should update and log chunking progress
	bool bLogProgress = false;

	// If the new chunk was recognized, then we will have got a different Guid through, so fix up guid for files using this chunk
	if (CurrentChunkGuid != ChunkGuid)
	{
		bLogProgress = true;
		++NumKnownChunks;
		for (auto& FileManifest : BuildManifest->Data->FileManifestList)
		{
			for (auto& ChunkPart : FileManifest.FileChunkParts)
			{
				if (ChunkPart.Guid == CurrentChunkGuid)
				{
					ChunkPart.Guid = ChunkGuid;
				}
			}
		}
	}
	// If the chunk is new, count it if we were passed data
	else if (ChunkData)
	{
		bLogProgress = true;
		++NumNewChunks;
	}

	// Always queue the chunk if passed data, it will be skipped automatically if existing already, and we need to make
	// sure the latest version is saved out when recognizing an older version
	if (ChunkData)
	{
		ChunkWriter.QueueChunk(ChunkData, ChunkGuid, ChunkHash);

		// Also add the info to the data
		if (!BuildManifest->ChunkInfoLookup.Contains(ChunkGuid))
		{
			BuildManifest->Data->ChunkList.Add(FChunkInfoData());
			FChunkInfoData& NewChunk = BuildManifest->Data->ChunkList.Last();
			NewChunk.Guid = ChunkGuid;
			NewChunk.Hash = ChunkHash;
			NewChunk.FileSize = INDEX_NONE;
			NewChunk.GroupNumber = FCrc::MemCrc32(&ChunkGuid, sizeof(FGuid)) % 100;
			// Lookup is used just to ensure one copy of each chunk added, we must not use the ptr as array will realloc
			BuildManifest->ChunkInfoLookup.Add(ChunkGuid, nullptr);
		}

	}

	if (bLogProgress)
	{
		// Output to log for builder info
		GLog->Logf(TEXT("%s %s [%d:%d]"), *BuildManifest->GetAppName(), *BuildManifest->GetVersionString(), NumNewChunks, NumKnownChunks);
	}
}

void FBuildDataChunkProcessor::PushChunk()
{
	check(!ChunkIsPushed);
	ChunkIsPushed = true;

	// Then we can skip over this entire chunk using it for current files in processing
	BackupProcessingChunk = IsProcessingChunk;
	BackupProcessingChunkPart = IsProcessingChunkPart;
	BackupChunkGuid = CurrentChunkGuid;
	BackupChunkBufferPos = CurrentChunkBufferPos;
	if (BackupProcessingChunkPart)
	{
		EndNewChunkPart();
	}
	if (BackupProcessingChunk)
	{
		// null ChunkData will stop the incomplete chunk being saved out
		EndNewChunk(0, NULL, CurrentChunkGuid);
	}

	// Start this chunk, must begin from 0
	CurrentChunkBufferPos = 0;
	BeginNewChunk(false);
	BeginNewChunkPart();
}

void FBuildDataChunkProcessor::PopChunk(const uint64& ChunkHash, const uint8* ChunkData, const FGuid& ChunkGuid)
{
	check(ChunkIsPushed);
	ChunkIsPushed = false;

	EndNewChunkPart();
	EndNewChunk(ChunkHash, ChunkData, ChunkGuid);

	// Backup data for previous part chunk
	IsProcessingChunk = BackupProcessingChunk;
	CurrentChunkGuid = BackupChunkGuid;
	CurrentChunkBufferPos = BackupChunkBufferPos;
}

void FBuildDataChunkProcessor::FinalChunk()
{
	// If we're processing a chunk part, then we MUST be processing a chunk too
	check(IsProcessingChunkPart ? IsProcessingChunk : true);

	if (IsProcessingChunk)
	{
		// The there is no more data and the last file is processed.
		// The final chunk should be finished
		uint64 NewChunkHash = FRollingHash< FBuildPatchData::ChunkDataSize >::GetHashForDataSet(CurrentChunkBuffer);
		FGuid ChunkGuid = CurrentChunkGuid;
		FBuildDataGenerator::FindExistingChunkData(NewChunkHash, CurrentChunkBuffer, ChunkGuid);
		if (IsProcessingChunkPart)
		{
			EndNewChunkPart();
		}
		EndNewChunk(NewChunkHash, CurrentChunkBuffer, ChunkGuid);
	}

	// Wait for the chunk writer to finish
	ChunkWriter.NoMoreChunks();
	ChunkWriter.WaitForThread();
}

void FBuildDataChunkProcessor::BeginFile(const FString& FileName)
{
	check(IsProcessingFile == false);
	IsProcessingFile = true;

	// We should start a new file, which begins with the current new chunk and current chunk offset
	BuildManifest->Data->FileManifestList.Add(FFileManifestData());
	CurrentFile = &BuildManifest->Data->FileManifestList.Last();
	CurrentFile->Filename = FileName;
	CurrentFile->bIsUnixExecutable = IsUnixExecutable(*(BuildRoot / FileName));
	CurrentFile->SymlinkTarget = GetSymlinkTarget(*(BuildRoot / FileName));

	// Setup for current chunk part
	if (IsProcessingChunkPart)
	{
		CurrentFile->FileChunkParts.Add(FChunkPartData());
		FChunkPartData& NewPart = CurrentFile->FileChunkParts.Last();
		NewPart.Guid = CurrentChunkGuid;
		NewPart.Offset = CurrentChunkBufferPos;
	}
}

void FBuildDataChunkProcessor::EndFile()
{
	check(IsProcessingFile == true);
	IsProcessingFile = false;

	if (CurrentFile)
	{
		if (IsProcessingChunkPart)
		{
			// Current file should have it's last chunk part updated with the size value finalized.
			FChunkPartData* ChunkPart = &CurrentFile->FileChunkParts.Last();
			ChunkPart->Size = CurrentChunkBufferPos - ChunkPart->Offset;
		}
		// Check file size is correct from chunks
		int64 FileSystemSize = IFileManager::Get().FileSize(*(BuildRoot / CurrentFile->Filename));
		CurrentFile->Init();
		check(CurrentFile->GetFileSize() == FileSystemSize);
	}

	CurrentFile = nullptr;
}

void FBuildDataChunkProcessor::SkipKnownByte(const uint8& NextByte, const bool& bStartOfFile, const bool& bEndOfFile, const FString& Filename)
{
	// Check for start of new file
	if (bStartOfFile)
	{
		BeginFile(Filename);
		FileHash.Reset();
	}

	// Increment position between start and end of file
	++CurrentChunkBufferPos;
	FileHash.Update(&NextByte, 1);

	// Check for end of file
	if (bEndOfFile)
	{
		FileHash.Final();
		FileHash.GetHash(CurrentFile->FileHash.Hash);
		EndFile();
	}
}

void FBuildDataChunkProcessor::ProcessNewByte(const uint8& NewByte, const bool& bStartOfFile, const bool& bEndOfFile, const FString& Filename)
{
	// If we finished a chunk, we begin a new chunk and new chunk part
	if (!IsProcessingChunk)
	{
		BeginNewChunk();
		BeginNewChunkPart();
	}
	// Or if we finished a chunk part (by recognizing a chunk hash), we begin a new one
	else if (!IsProcessingChunkPart)
	{
		BeginNewChunkPart();
	}

	// Check for start of new file
	if (bStartOfFile)
	{
		BeginFile(Filename);
		FileHash.Reset();
	}

	// We add the old byte to our new chunk which will be used as part of the file it belonged to
	CurrentChunkBuffer[CurrentChunkBufferPos++] = NewByte;
	FileHash.Update(&NewByte, 1);

	// Check for end of file
	if (bEndOfFile)
	{
		FileHash.Final();
		FileHash.GetHash(CurrentFile->FileHash.Hash);
		EndFile();
	}

	// Do we have a full new chunk?
	check(CurrentChunkBufferPos <= FBuildPatchData::ChunkDataSize);
	if (CurrentChunkBufferPos == FBuildPatchData::ChunkDataSize)
	{
		uint64 NewChunkHash = FRollingHash< FBuildPatchData::ChunkDataSize >::GetHashForDataSet(CurrentChunkBuffer);
		FGuid ChunkGuid = CurrentChunkGuid;
		FBuildDataGenerator::FindExistingChunkData(NewChunkHash, CurrentChunkBuffer, ChunkGuid);
		EndNewChunkPart();
		EndNewChunk(NewChunkHash, CurrentChunkBuffer, ChunkGuid);
	}
}

void FBuildDataChunkProcessor::GetChunkStats(uint32& OutNewFiles, uint32& OutKnownFiles)
{
	OutNewFiles = NumNewChunks;
	OutKnownFiles = NumKnownChunks;
}

const TMap<FGuid, int64>& FBuildDataChunkProcessor::GetChunkFilesizes()
{
	ChunkWriter.GetChunkFilesizes(ChunkFileSizes);
	return ChunkFileSizes;
}

/* FBuildDataFileProcessor implementation
*****************************************************************************/
FBuildDataFileProcessor::FBuildDataFileProcessor(FBuildPatchAppManifestRef InBuildManifest, const FString& InBuildRoot, const FDateTime& InDataThresholdTime, const uint64 MaxFilePartSize)
	: MaxFilePartSize(MaxFilePartSize)
	, NumNewFiles(0)
	, NumKnownFiles(0)
	, BuildManifest(InBuildManifest)
	, BuildRoot(InBuildRoot)
	, CurrentFile(nullptr)
	, FileHash()
	, FileSize(0)
	, IsProcessingFile(false)
	, DataThresholdTime(InDataThresholdTime)
{
}

void FBuildDataFileProcessor::BeginFile(const FString& InFileName)
{
	check(!IsProcessingFile);
	IsProcessingFile = true;

	// Create the new file
	BuildManifest->Data->FileManifestList.Add(FFileManifestData());
	CurrentFile = &BuildManifest->Data->FileManifestList.Last();
	CurrentFile->Filename = InFileName;
	CurrentFile->bIsUnixExecutable = IsUnixExecutable(*(BuildRoot / InFileName));
	CurrentFile->SymlinkTarget = GetSymlinkTarget(*(BuildRoot / InFileName));

	// Reset the hash and size counter
	FileHash.Reset();
	FileSize = 0;

	// Init part data
	BeginFilePart();
}

void FBuildDataFileProcessor::BeginFilePart()
{
	CurrentFilePartData = FChunkPartData();
	CurrentFilePartInfo = FChunkInfoData();
	
	CurrentFilePartData.Guid = FGuid::NewGuid();
	CurrentFilePartInfo.Guid = CurrentFilePartData.Guid;

	CurrentFilePart.Empty(MaxFilePartSize);
}

void FBuildDataFileProcessor::ProcessFileData(const uint8* Data, const uint32& DataLen)
{
	check(IsProcessingFile);

	// Update the hash
	FileHash.Update(Data, DataLen);

	// Process current file part
	const uint32 NewSize = CurrentFilePartData.Size + DataLen;
	if (NewSize > MaxFilePartSize)
	{
		const uint32 RemainingSize = MaxFilePartSize - CurrentFilePartData.Size;
		check(RemainingSize < DataLen);
		FileSize += RemainingSize;
		CurrentFilePartData.Size += RemainingSize;
		CurrentFilePartInfo.Hash = FCycPoly64Hash::GetHashForDataSet(Data, RemainingSize, CurrentFilePartInfo.Hash);
		CurrentFilePart.Append(Data, RemainingSize);
		EndFilePart();
		BeginFilePart();
		const uint32 OverflowSize = DataLen - RemainingSize;
		const uint8* OverflowData = Data + RemainingSize;
		FileSize += OverflowSize;
		CurrentFilePartData.Size += OverflowSize;
		CurrentFilePartInfo.Hash = FCycPoly64Hash::GetHashForDataSet(OverflowData, OverflowSize, CurrentFilePartInfo.Hash);
		CurrentFilePart.Append(OverflowData, OverflowSize);
	}
	else
	{
		CurrentFilePartData.Size += DataLen;
		CurrentFilePartInfo.Hash = FCycPoly64Hash::GetHashForDataSet(Data, DataLen, CurrentFilePartInfo.Hash);
		CurrentFilePart.Append(Data, DataLen);
	FileSize += DataLen;
}

}

void FBuildDataFileProcessor::EndFilePart()
{
	const FString FullSourcePath = FPaths::Combine(*BuildRoot, *CurrentFile->Filename);

	bool bFoundExisting = FBuildDataGenerator::FindExistingFileData(CurrentFilePart, CurrentFilePartInfo, CurrentFilePartData, DataThresholdTime);

	// Set group number now as GUID may have changed
	CurrentFilePartInfo.GroupNumber = FCrc::MemCrc32(&CurrentFilePartInfo.Guid, sizeof(FGuid)) % 100;

	// Always call save to ensure new versions exist when recognizing old ones
	bool bFilePartSavedSuccess = FBuildDataGenerator::SaveOutFileDataPart(CurrentFilePart, CurrentFilePartInfo, CurrentFilePartData);

	// We must have successfully saved, otherwise fatal
	check(bFilePartSavedSuccess);

	// Add to arrays
	CurrentFile->FileChunkParts.Add(CurrentFilePartData);
	if (!BuildManifest->ChunkInfoLookup.Contains(CurrentFilePartInfo.Guid))
	{
		BuildManifest->Data->ChunkList.Add(CurrentFilePartInfo);
		// Lookup is used just to ensure one copy of each chunk added, we must not use the ptr as array will realloc
		BuildManifest->ChunkInfoLookup.Add(CurrentFilePartInfo.Guid, nullptr);
	}

	// Count new/known stats
	if (bFoundExisting)
	{
		++NumKnownFiles;
	}
	else
	{
		++NumNewFiles;
	}

	// Output to log for builder info
	GLog->Logf(TEXT("%s %s [%d:%d]"), *BuildManifest->GetAppName(), *BuildManifest->GetVersionString(), NumNewFiles, NumKnownFiles);
}

void FBuildDataFileProcessor::EndFile()
{
	check(IsProcessingFile);
	check(CurrentFile);
	IsProcessingFile = false;

	const FString FullSourcePath = FPaths::Combine(*BuildRoot, *CurrentFile->Filename);

	// Finalize the hash
	FileHash.Final();
	FileHash.GetHash(CurrentFile->FileHash.Hash);

	EndFilePart();

	// Init to check size
	CurrentFile->Init();
	check(CurrentFile->GetFileSize() == FileSize);
}

void FBuildDataFileProcessor::GetFileStats(uint32& OutNewFiles, uint32& OutKnownFiles)
{
	OutNewFiles = NumNewFiles;
	OutKnownFiles = NumKnownFiles;
}

/* FBuildSimpleChunkCache::FChunkReader implementation
*****************************************************************************/
FBuildGenerationChunkCache::FChunkReader::FChunkReader(const FString& InChunkFilePath, TSharedRef< FChunkFile > InChunkFile, uint32* InBytesRead, const FDateTime& InDataAgeThreshold)
	: ChunkFilePath(InChunkFilePath)
	, ChunkFileReader(NULL)
	, ChunkFile(InChunkFile)
	, FileBytesRead(InBytesRead)
	, MemoryBytesRead(0)
	, DataAgeThreshold(InDataAgeThreshold)
{
	ChunkFile->GetDataLock(&ChunkData, &ChunkHeader);
}

FBuildGenerationChunkCache::FChunkReader::~FChunkReader()
{
	ChunkFile->ReleaseDataLock();

	// Close file handle
	if (ChunkFileReader != NULL)
	{
		ChunkFileReader->Close();
		delete ChunkFileReader;
		ChunkFileReader = NULL;
	}
}

FArchive* FBuildGenerationChunkCache::FChunkReader::GetArchive()
{
	// Open file handle?
	if (ChunkFileReader == NULL)
	{
		ChunkFileReader = IFileManager::Get().CreateFileReader(*ChunkFilePath);
		if (ChunkFileReader == NULL)
		{
			// Break the magic to mark as invalid
			ChunkHeader->Magic = 0;
			GLog->Logf(TEXT("WARNING: Skipped missing chunk file %s"), *ChunkFilePath);
			return NULL;
		}
		// Read Header?
		if (ChunkHeader->Guid.IsValid() == false)
		{
			*ChunkFileReader << *ChunkHeader;
		}
		// Check the file is not too old to reuse
		if (IFileManager::Get().GetTimeStamp(*ChunkFilePath) < DataAgeThreshold)
		{
			// Break the magic to mark as invalid (this chunk is too old to reuse)
			ChunkHeader->Magic = 0;
			return ChunkFileReader;
		}
		// Check we can seek otherwise bad chunk
		const int64 ExpectedFileSize = ChunkHeader->DataSize + ChunkHeader->HeaderSize;
		const int64 ChunkFileSize = ChunkFileReader->TotalSize();
		const int64 NextByte = ChunkHeader->HeaderSize + *FileBytesRead;
		if (ChunkFileSize == ExpectedFileSize
			&& NextByte < ChunkFileSize)
		{
			// Seek to next byte
			ChunkFileReader->Seek(NextByte);
			// Break the magic to mark as invalid if archive errored, this chunk will get ignored
			if (ChunkFileReader->GetError())
			{
				ChunkHeader->Magic = 0;
			}
		}
		else
		{
			// Break the magic to mark as invalid
			ChunkHeader->Magic = 0;
		}
		// If this chunk is valid and compressed, we must read the entire file and decompress to memory now if we have not already
		// as we cannot compare to compressed data
		if (*FileBytesRead == 0 && ChunkHeader->StoredAs & FChunkHeader::STORED_COMPRESSED)
		{
			// Load the compressed chunk data
			TArray< uint8 > CompressedData;
			CompressedData.Empty(ChunkHeader->DataSize);
			CompressedData.AddUninitialized(ChunkHeader->DataSize);
			ChunkFileReader->Serialize(CompressedData.GetData(), ChunkHeader->DataSize);
			// Uncompress
			bool bSuceess = FCompression::UncompressMemory(
				static_cast<ECompressionFlags>(COMPRESS_ZLIB | COMPRESS_BiasMemory),
				ChunkData,
				FBuildPatchData::ChunkDataSize,
				CompressedData.GetData(),
				ChunkHeader->DataSize);
			// Mark that we have fully read decompressed data and update the chunkfile's data size as we are expanding it
			*FileBytesRead = FBuildPatchData::ChunkDataSize;
			ChunkHeader->DataSize = FBuildPatchData::ChunkDataSize;
			// Check uncompression was OK
			if (!bSuceess)
			{
				ChunkHeader->Magic = 0;
			}
		}
	}
	return ChunkFileReader;
}

const bool FBuildGenerationChunkCache::FChunkReader::IsValidChunk()
{
	if (ChunkHeader->Guid.IsValid() == false)
	{
		GetArchive();
	}
	// Check magic, and current support etc.
	const bool bValidHeader = ChunkHeader->IsValidMagic();
	const bool bValidChunkGuid = ChunkHeader->Guid.IsValid();
	const bool bSupportedFormat = ChunkHeader->StoredAs == FChunkHeader::STORED_RAW || ChunkHeader->StoredAs == FChunkHeader::STORED_COMPRESSED;
	return bValidHeader && bValidChunkGuid && bSupportedFormat;
}

const FGuid& FBuildGenerationChunkCache::FChunkReader::GetChunkGuid()
{
	if (ChunkHeader->Guid.IsValid() == false)
	{
		GetArchive();
	}
	return ChunkHeader->Guid;
}

void FBuildGenerationChunkCache::FChunkReader::ReadNextBytes(uint8** OutDataBuffer, const uint32& ReadLength)
{
	uint8* BufferNextByte = &ChunkData[MemoryBytesRead];
	// Do we need to load from disk?
	if ((MemoryBytesRead + ReadLength) > *FileBytesRead)
	{
		FArchive* Reader = GetArchive();
		// Do not allow incorrect usage
		check(Reader);
		check(ReadLength <= BytesLeft());
		// Read the number of bytes extra we need
		const int32 NumFileBytesRead = *FileBytesRead;
		const int32 NextMemoryBytesRead = MemoryBytesRead + ReadLength;
		const uint32 FileReadLen = FMath::Max<int32>(0, NextMemoryBytesRead - NumFileBytesRead);
		*FileBytesRead += FileReadLen;
		Reader->Serialize(BufferNextByte, FileReadLen);
		// Assert if read error, if theres some problem accessing chunks then continuing would cause bad patch
		// ratios, so it's better to hard fault.
		const bool bChunkReadOK = !Reader->GetError();
		if (!bChunkReadOK)
		{
			// Print something helpful
			GLog->Logf(TEXT("FATAL ERROR: Could not read from chunk FArchive %s"), *ChunkFilePath);
			// Check with bool variable so that output will be readable
			check(bChunkReadOK);
		}
	}
	MemoryBytesRead += ReadLength;
	(*OutDataBuffer) = BufferNextByte;
}

const uint32 FBuildGenerationChunkCache::FChunkReader::BytesLeft()
{
	if (ChunkHeader->Guid.IsValid() == false)
	{
		GetArchive();
	}
	return FBuildPatchData::ChunkDataSize - MemoryBytesRead;
}

/* FBuildGenerationChunkCache implementation
*****************************************************************************/
FBuildGenerationChunkCache::FBuildGenerationChunkCache(const FDateTime& DataAgeThreshold)
	: DataAgeThreshold(DataAgeThreshold)
{
}

TSharedRef< FBuildGenerationChunkCache::FChunkReader > FBuildGenerationChunkCache::GetChunkReader(const FString& ChunkFilePath)
{
	if (ChunkCache.Contains(ChunkFilePath) == false)
	{
		// Remove oldest access from cache?
		if (ChunkCache.Num() >= NumChunksToCache)
		{
			FString OldestAccessChunk = TEXT("");
			double OldestAccessTime = FPlatformTime::Seconds();
			for (auto ChunkCacheIt = ChunkCache.CreateConstIterator(); ChunkCacheIt; ++ChunkCacheIt)
			{
				const FString& ChunkFilePath = ChunkCacheIt.Key();
				const FChunkFile& ChunkFile = ChunkCacheIt.Value().Get();
				if (ChunkFile.GetLastAccessTime() < OldestAccessTime)
				{
					OldestAccessTime = ChunkFile.GetLastAccessTime();
					OldestAccessChunk = ChunkFilePath;
				}
			}
			ChunkCache.Remove(OldestAccessChunk);
			delete BytesReadPerChunk[OldestAccessChunk];
			BytesReadPerChunk.Remove(OldestAccessChunk);
		}
		// Add the chunk to cache
		ChunkCache.Add(ChunkFilePath, MakeShareable(new FChunkFile(1, true)));
		BytesReadPerChunk.Add(ChunkFilePath, new uint32(0));
	}
	return MakeShareable(new FChunkReader(ChunkFilePath, ChunkCache[ChunkFilePath], BytesReadPerChunk[ChunkFilePath], DataAgeThreshold));
}

void FBuildGenerationChunkCache::Cleanup()
{
	ChunkCache.Empty(0);
	for (auto BytesReadPerChunkIt = BytesReadPerChunk.CreateConstIterator(); BytesReadPerChunkIt; ++BytesReadPerChunkIt)
	{
		delete BytesReadPerChunkIt.Value();
	}
	BytesReadPerChunk.Empty(0);
}

/* FBuildGenerationChunkCache system singleton setup
*****************************************************************************/
TSharedPtr< FBuildGenerationChunkCache > FBuildGenerationChunkCache::SingletonInstance = NULL;

void FBuildGenerationChunkCache::Init(const FDateTime& DataAgeThreshold)
{
	// We won't allow misuse of these functions
	check(!SingletonInstance.IsValid());
	SingletonInstance = MakeShareable(new FBuildGenerationChunkCache(DataAgeThreshold));
}

FBuildGenerationChunkCache& FBuildGenerationChunkCache::Get()
{
	// We won't allow misuse of these functions
	check(SingletonInstance.IsValid());
	return *SingletonInstance.Get();
}

void FBuildGenerationChunkCache::Shutdown()
{
	// We won't allow misuse of these functions
	check(SingletonInstance.IsValid());
	SingletonInstance->Cleanup();
	SingletonInstance.Reset();
}

/* FBuildDataGenerator static variables
*****************************************************************************/
TMap< FGuid, FString > FBuildDataGenerator::ExistingChunkGuidInventory;
TMap< uint64, TArray< FGuid > > FBuildDataGenerator::ExistingChunkHashInventory;
bool FBuildDataGenerator::ExistingChunksEnumerated = false;
TMap< uint64, TArray< FString > > FBuildDataGenerator::ExistingFileInventory;
bool FBuildDataGenerator::ExistingFilesEnumerated = false;
FCriticalSection FBuildDataGenerator::SingleConcurrentBuildCS;

static void AddCustomFieldsToBuildManifest(const TMap<FString, FVariant>& CustomFields, IBuildManifestPtr BuildManifest)
{
	for (const auto& CustomField : CustomFields)
	{
		int32 VarType = CustomField.Value.GetType();
		if (VarType == EVariantTypes::Float || VarType == EVariantTypes::Double)
		{
			BuildManifest->SetCustomField(CustomField.Key, (double)CustomField.Value);
		}
		else if (VarType == EVariantTypes::Int8 || VarType == EVariantTypes::Int16 || VarType == EVariantTypes::Int32 || VarType == EVariantTypes::Int64 ||
			VarType == EVariantTypes::UInt8 || VarType == EVariantTypes::UInt16 || VarType == EVariantTypes::UInt32 || VarType == EVariantTypes::UInt64)
		{
			BuildManifest->SetCustomField(CustomField.Key, (int64)CustomField.Value);
		}
		else if (VarType == EVariantTypes::String)
		{
			BuildManifest->SetCustomField(CustomField.Key, CustomField.Value.GetValue<FString>());
		}
	}
}

/* FBuildDataGenerator implementation
*****************************************************************************/
bool FBuildDataGenerator::GenerateChunksManifestFromDirectory(const FBuildPatchSettings& Settings)
{
	// Output to log for builder info
	GLog->Logf(TEXT("Running Chunks Patch Generation for: %u:%s %s"), Settings.AppID, *Settings.AppName, *Settings.BuildVersion);

	// Take the build CS
	FScopeLock SingleConcurrentBuild(&SingleConcurrentBuildCS);

	// Create our chunk cache
	const FDateTime Cutoff = Settings.bShouldHonorReuseThreshold ? FDateTime::UtcNow() - FTimespan::FromDays(Settings.DataAgeThreshold) : FDateTime::MinValue();
	FBuildGenerationChunkCache::Init(Cutoff);

	// Create a manifest
	FBuildPatchAppManifestRef BuildManifest = MakeShareable(new FBuildPatchAppManifest());
	BuildManifest->Data->ManifestFileVersion = EBuildPatchAppManifestVersion::GetLatestVersion();

	// Setup custom fields
	AddCustomFieldsToBuildManifest(Settings.CustomFields, BuildManifest);

	// Reset chunk inventory
	ExistingChunksEnumerated = false;
	ExistingChunkGuidInventory.Empty();
	ExistingChunkHashInventory.Empty();

	// Declare a build processor
	FBuildDataChunkProcessor DataProcessor(BuildManifest, Settings.RootDirectory);

	// Create a build streamer
	FBuildStream* BuildStream = new FBuildStream(Settings.RootDirectory, Settings.IgnoreListFile);

	// Set the basic details
	BuildManifest->Data->bIsFileData = false;
	BuildManifest->Data->AppID = Settings.AppID;
	BuildManifest->Data->AppName = Settings.AppName;
	BuildManifest->Data->BuildVersion = Settings.BuildVersion;
	BuildManifest->Data->LaunchExe = Settings.LaunchExe;
	BuildManifest->Data->LaunchCommand = Settings.LaunchCommand;
	BuildManifest->Data->PrereqName = Settings.PrereqName;
	BuildManifest->Data->PrereqPath = Settings.PrereqPath;
	BuildManifest->Data->PrereqArgs = Settings.PrereqArgs;

	// Create a data buffer
	const uint32 DataBufferSize = FBuildPatchData::ChunkDataSize;
	uint8* DataBuffer = new uint8[DataBufferSize];

	// We'll need a rolling hash for chunking
	FRollingHash< FBuildPatchData::ChunkDataSize >* RollingHash = new FRollingHash< FBuildPatchData::ChunkDataSize >();

	// Refers to how much data has been processed (into the FBuildDataProcessor)
	uint64 ProcessPos = 0;

	// Records the current file we are processing
	FString FileName;

	// And the current file's data left to process
	uint64 FileDataCount = 0;

	// Used to store data read lengths
	uint32 ReadLen = 0;

	// The last time we logged out data processed
	double LastProgressLog = FPlatformTime::Seconds();
	const double TimeGenStarted = LastProgressLog;

	// Loop through all data
	while (!BuildStream->IsEndOfData())
	{
		// Grab some data from the build stream
		ReadLen = BuildStream->DequeueData(DataBuffer, DataBufferSize);

		// A bool says if there's no more data to come from the Build Stream
		const bool bNoMoreData = BuildStream->IsEndOfData();

		// Refers to how much data from DataBuffer has been passed into the rolling hash
		uint32 DataBufferPos = 0;

		// Count how many times we pad the rolling hash with zero
		uint32 PaddedZeros = 0;

		// Process data while we have more
		while ((DataBufferPos < ReadLen) || (bNoMoreData && PaddedZeros < RollingHash->GetWindowSize()))
		{
			// Prime the rolling hash
			if (RollingHash->GetNumDataNeeded() > 0)
			{
				if (DataBufferPos < ReadLen)
				{
					RollingHash->ConsumeByte(DataBuffer[DataBufferPos++]);
				}
				else
				{
					RollingHash->ConsumeByte(0);
					++PaddedZeros;
				}
				// Keep looping until primed
				continue;
			}

			// Check if we recognized a chunk
			FGuid ChunkGuid;
			const uint64 WindowHash = RollingHash->GetWindowHash();
			const TRingBuffer< uint8, FBuildPatchData::ChunkDataSize >& WindowData = RollingHash->GetWindowData();
			bool ChunkRecognised = FindExistingChunkData(WindowHash, WindowData, ChunkGuid);
			if (ChunkRecognised)
			{
				// Process all bytes
				DataProcessor.PushChunk();
				const uint32 WindowDataSize = RollingHash->GetWindowSize() - PaddedZeros;
				for (uint32 i = 0; i < WindowDataSize; ++i)
				{
					const bool bStartOfFile = BuildStream->GetFileSpan(ProcessPos, FileName, FileDataCount);
					const bool bEndOfFile = FileDataCount <= 1;
					check(FileDataCount > 0);// If FileDataCount is ever 0, it means this piece of data belongs to no file, so something is wrong
					DataProcessor.SkipKnownByte(WindowData[i], bStartOfFile, bEndOfFile, FileName);
					++ProcessPos;
					--FileDataCount;
				}
				uint8* SerialWindowData = new uint8[FBuildPatchData::ChunkDataSize];
				WindowData.Serialize(SerialWindowData);
				DataProcessor.PopChunk(WindowHash, SerialWindowData, ChunkGuid);
				delete[] SerialWindowData;

				// Clear
				RollingHash->Clear();
			}
			else
			{
				// Process one byte
				const bool bStartOfFile = BuildStream->GetFileSpan(ProcessPos, FileName, FileDataCount);
				const bool bEndOfFile = FileDataCount <= 1;
				DataProcessor.ProcessNewByte(WindowData.Bottom(), bStartOfFile, bEndOfFile, FileName);
				++ProcessPos;
				--FileDataCount;

				// Roll
				if (DataBufferPos < ReadLen)
				{
					RollingHash->RollForward(DataBuffer[DataBufferPos++]);
				}
				else if (bNoMoreData)
				{
					RollingHash->RollForward(0);
					++PaddedZeros;
				}
			}

			// Log processed data
			if ((FPlatformTime::Seconds() - LastProgressLog) >= 10.0)
			{
				LastProgressLog = FPlatformTime::Seconds();
				GLog->Logf(TEXT("Processed %lld bytes."), ProcessPos);
			}
		}
	}

	// The final chunk if any should be finished.
	// This also triggers the chunk writer thread to exit.
	DataProcessor.FinalChunk();

	// Handle empty files
	FSHA1 EmptyHasher;
	EmptyHasher.Final();
	const TArray< FString >& EmptyFileList = BuildStream->GetEmptyFiles();
	for (const auto& EmptyFile : EmptyFileList)
	{
		BuildManifest->Data->FileManifestList.Add(FFileManifestData());
		FFileManifestData& EmptyFileManifest = BuildManifest->Data->FileManifestList.Last();
		EmptyFileManifest.Filename = EmptyFile;
		EmptyHasher.GetHash(EmptyFileManifest.FileHash.Hash);
	}

	// Add chunk sizes
	const TMap<FGuid, int64>& ChunkFilesizes = DataProcessor.GetChunkFilesizes();
	for (FChunkInfoData& ChunkInfo : BuildManifest->Data->ChunkList)
	{
		if (ChunkFilesizes.Contains(ChunkInfo.Guid))
		{
			ChunkInfo.FileSize = ChunkFilesizes[ChunkInfo.Guid];
		}
	}

	// Fill out lookups
	BuildManifest->InitLookups();

	// Save manifest into the cloud directory
	const FString OutFilename = FBuildPatchServicesModule::GetCloudDirectory() / FDefaultValueHelper::RemoveWhitespaces(BuildManifest->Data->AppName + BuildManifest->Data->BuildVersion) + TEXT(".manifest");
	BuildManifest->SaveToFile(OutFilename, true);

	// Output to log for builder info
	GLog->Logf(TEXT("Saved manifest to %s"), *OutFilename);

	// Clean up memory
	delete[] DataBuffer;
	delete BuildStream;
	delete RollingHash;
	FBuildGenerationChunkCache::Shutdown();

	// @TODO LSwift: Detect errors and return false on failure
	return true;
}

bool FBuildDataGenerator::GenerateFilesManifestFromDirectory(const FBuildPatchSettings& Settings)
{
	// Output to log for builder info
	GLog->Logf(TEXT("Running Files Patch Generation for: %u:%s %s"), Settings.AppID, *Settings.AppName, *Settings.BuildVersion);

	// Take the build CS
	FScopeLock SingleConcurrentBuild(&SingleConcurrentBuildCS);

	// Create a manifest
	FBuildPatchAppManifestRef BuildManifest = MakeShareable(new FBuildPatchAppManifest());
	BuildManifest->Data->ManifestFileVersion = EBuildPatchAppManifestVersion::GetLatestVersion();

	// Setup custom fields
	AddCustomFieldsToBuildManifest(Settings.CustomFields, BuildManifest);

	// Reset file inventory
	ExistingFilesEnumerated = false;
	ExistingFileInventory.Empty();

	// Declare a build processor
	const FDateTime Cutoff = Settings.bShouldHonorReuseThreshold ? FDateTime::UtcNow() - FTimespan::FromDays(Settings.DataAgeThreshold) : FDateTime::MinValue();
	FBuildDataFileProcessor DataProcessor(BuildManifest, Settings.RootDirectory, Cutoff, 1024*1024*5);

	// Set the basic details
	BuildManifest->Data->bIsFileData = true;
	BuildManifest->Data->AppID = Settings.AppID;
	BuildManifest->Data->AppName = Settings.AppName;
	BuildManifest->Data->BuildVersion = Settings.BuildVersion;
	BuildManifest->Data->LaunchExe = Settings.LaunchExe;
	BuildManifest->Data->LaunchCommand = Settings.LaunchCommand;
	BuildManifest->Data->PrereqName = Settings.PrereqName;
	BuildManifest->Data->PrereqPath = Settings.PrereqPath;
	BuildManifest->Data->PrereqArgs = Settings.PrereqArgs;

	// Create a data buffer
	uint8* FileReadBuffer = new uint8[FileBufferSize];

	// Refers to how much data has been processed (into the FBuildDataFileProcessor)
	uint64 ProcessPos = 0;

	// Find all files
	TArray< FString > AllFiles;
	IFileManager::Get().FindFilesRecursive(AllFiles, *Settings.RootDirectory, TEXT("*.*"), true, false);
	AllFiles.Sort();

	// Remove the files that appear in an ignore list
	FBuildDataGenerator::StripIgnoredFiles(AllFiles, Settings.RootDirectory, Settings.IgnoreListFile);

	// Loop through all files
	for (auto FileIt = AllFiles.CreateConstIterator(); FileIt; ++FileIt)
	{
		const FString& FileName = *FileIt;
		// Read the file
		FArchive* FileReader = IFileManager::Get().CreateFileReader(*FileName);
		if (FileReader != NULL)
		{
			// Make SourceFile the format we want it in and start a new file
			FString SourceFile = FileName;
			FPaths::MakePathRelativeTo(SourceFile, *(Settings.RootDirectory + TEXT("/")));
			int64 FileSize = FileReader->TotalSize();
			if (FileSize < 0)
			{
				// Skip potential error ( INDEX_NONE == -1 )
				continue;
			}
			DataProcessor.BeginFile(SourceFile);
			while (!FileReader->AtEnd())
			{
				const int64 SizeLeft = FileSize - FileReader->Tell();
				const uint32 ReadLen = FMath::Min< int64 >(FileBufferSize, SizeLeft);
				ProcessPos += ReadLen;
				FileReader->Serialize(FileReadBuffer, ReadLen);
				// Copy into data stream
				DataProcessor.ProcessFileData(FileReadBuffer, ReadLen);
			}
			FileReader->Close();
			delete FileReader;
			DataProcessor.EndFile();
		}
		else
		{
			// @TODO LSwift: Handle File error?
		}
	}

	// Fill out lookups
	BuildManifest->InitLookups();

	// Save manifest into the cloud directory
	const FString OutFilename = FBuildPatchServicesModule::GetCloudDirectory() / FDefaultValueHelper::RemoveWhitespaces(BuildManifest->Data->AppName + BuildManifest->Data->BuildVersion) + TEXT(".manifest");
	BuildManifest->SaveToFile(OutFilename, true);

	// Output to log for builder info
	GLog->Logf(TEXT("Saved manifest to %s"), *OutFilename);

	// Clean up memory
	delete[] FileReadBuffer;

	// @TODO LSwift: Detect errors and return false on failure
	return true;
}

bool FBuildDataGenerator::FindExistingChunkData(const uint64& ChunkHash, const uint8* ChunkData, FGuid& ChunkGuid)
{
	// Quick code hack
	TRingBuffer< uint8, FBuildPatchData::ChunkDataSize > ChunkDataRing;
	const uint32 ChunkDataLen = FBuildPatchData::ChunkDataSize;
	ChunkDataRing.Enqueue(ChunkData, ChunkDataLen);
	return FindExistingChunkData(ChunkHash, ChunkDataRing, ChunkGuid);
}

bool FBuildDataGenerator::FindExistingChunkData(const uint64& ChunkHash, const TRingBuffer< uint8, FBuildPatchData::ChunkDataSize >& ChunkData, FGuid& ChunkGuid)
{
	bool bFoundMatchingChunk = false;

	// Perform an inventory on Cloud chunks if not already done
	if (ExistingChunksEnumerated == false)
	{
		IFileManager& FileManager = IFileManager::Get();

		// Find all manifest files
		const FString CloudDir = FBuildPatchServicesModule::GetCloudDirectory();
		if (FileManager.DirectoryExists(*CloudDir))
		{
			const double StartEnumerate = FPlatformTime::Seconds();
			TArray<FString> AllManifests;
			GLog->Logf(TEXT("BuildDataGenerator: Enumerating Manifests from %s"), *CloudDir);
			FileManager.FindFiles(AllManifests, *(CloudDir / TEXT("*.manifest")), true, false);
			const double EnumerateTime = FPlatformTime::Seconds() - StartEnumerate;
			GLog->Logf(TEXT("BuildDataGenerator: Found %d manifests in %.1f seconds"), AllManifests.Num(), EnumerateTime);

			// Load all manifest files
			uint64 NumChunksFound = 0;
			const double StartLoadAllManifest = FPlatformTime::Seconds();
			for (const auto& ManifestFile : AllManifests)
			{
				// Determine chunks from manifest file
				const FString ManifestFilename = CloudDir / ManifestFile;
				FBuildPatchAppManifestRef BuildManifest = MakeShareable(new FBuildPatchAppManifest());
				const double StartLoadManifest = FPlatformTime::Seconds();
				if (BuildManifest->LoadFromFile(ManifestFilename))
				{
					const double LoadManifestTime = FPlatformTime::Seconds() - StartLoadManifest;
					GLog->Logf(TEXT("BuildDataGenerator: Loaded %s in %.1f seconds"), *ManifestFile, LoadManifestTime);
					if (!BuildManifest->IsFileDataManifest())
					{
						TArray<FGuid> ChunksReferenced;
						BuildManifest->GetDataList(ChunksReferenced);
						for (const auto& ChunkGuid : ChunksReferenced)
						{
							uint64 ChunkHash;
							if (BuildManifest->GetChunkHash(ChunkGuid, ChunkHash))
							{
								if (ChunkHash != 0)
								{
									const FString SourceFile = FBuildPatchUtils::GetFileNewFilename(BuildManifest->GetManifestVersion(), CloudDir, ChunkGuid, ChunkHash);
									TArray< FGuid >& HashChunkList = ExistingChunkHashInventory.FindOrAdd(ChunkHash);
									if (!HashChunkList.Contains(ChunkGuid))
									{
										++NumChunksFound;
										HashChunkList.Add(ChunkGuid);
										ExistingChunkGuidInventory.Add(ChunkGuid, SourceFile);
									}
								}
								else
								{
									GLog->Logf(TEXT("BuildDataGenerator: WARNING: Ignored an existing chunk %s with a failed hash value of zero to avoid performance problems while chunking"), *ChunkGuid.ToString());
								}
							}
							else
							{
								GLog->Logf(TEXT("BuildDataGenerator: WARNING: Missing chunk hash for %s in manifest %s"), *ChunkGuid.ToString(), *ManifestFile);
							}
						}
					}
					else
					{
						GLog->Logf(TEXT("BuildDataGenerator: INFO: Ignoring non-chunked manifest %s"), *ManifestFilename);
					}
				}
				else
				{
					GLog->Logf(TEXT("BuildDataGenerator: WARNING: Could not read Manifest file. Data recognition will suffer (%s)"), *ManifestFilename);
				}
			}
			const double LoadAllManifestTime = FPlatformTime::Seconds() - StartLoadAllManifest;
			GLog->Logf(TEXT("BuildDataGenerator: Used %d manifests to enumerate %llu chunks in %.1f seconds"), AllManifests.Num(), NumChunksFound, LoadAllManifestTime);
		}
		else
		{
			GLog->Logf(TEXT("BuildDataGenerator: Cloud directory does not exist: %s"), *CloudDir);
		}

		ExistingChunksEnumerated = true;
	}

	// Do we have a chunk matching this data?
	if (ExistingChunkHashInventory.Num() > 0)
	{
		TArray< FGuid >* ChunkList = ExistingChunkHashInventory.Find(ChunkHash);
		if (ChunkList != NULL)
		{
			// We need to load each chunk in this list and compare data
			for (auto ChunkIt = ChunkList->CreateConstIterator(); ChunkIt && !bFoundMatchingChunk; ++ChunkIt)
			{
				FGuid Guid = *ChunkIt;
				bool bChunkIsUsable = false;
				if (ExistingChunkGuidInventory.Contains(Guid))
				{
				const FString& SourceFile = ExistingChunkGuidInventory[Guid];
				// Read the file
				uint8* TempChunkData = new uint8[FBuildPatchData::ChunkDataSize];
				ChunkData.Serialize(TempChunkData);
				if (CompareDataToChunk(SourceFile, TempChunkData, Guid, bChunkIsUsable))
				{
					// We have a chunk match!!
					bFoundMatchingChunk = true;
					ChunkGuid = Guid;
				}
					delete[] TempChunkData;
				}
				// Check if this chunk should be dumped
				if (!bChunkIsUsable)
				{
					GLog->Logf(TEXT("BuildDataGenerator: Chunk %s unusable, removed from inventory."), *ChunkGuid.ToString());
					ChunkList->Remove(Guid);
					--ChunkIt;
				}
			}
		}
	}

	return bFoundMatchingChunk;
}

bool FBuildDataGenerator::FindExistingFileData(const TArray<uint8>& Data, FChunkInfoData& FilePartInfo, FChunkPartData& FilePartData, const FDateTime& DataThresholdTime)
			{
	check(Data.Num() == FilePartData.Size);

	bool bFoundMatchingData = false;

	// Perform an inventory on Cloud files if not already done
	if (ExistingFilesEnumerated == false)
	{
		IFileManager& FileManager = IFileManager::Get();
		// Find all manifest files
		const FString CloudDir = FBuildPatchServicesModule::GetCloudDirectory();
		if (FileManager.DirectoryExists(*CloudDir))
		{
			const double StartEnumerate = FPlatformTime::Seconds();
			TArray<FString> AllManifests;
			GLog->Logf(TEXT("BuildDataGenerator: Enumerating Manifests from %s"), *CloudDir);
			FileManager.FindFiles(AllManifests, *(CloudDir / TEXT("*.manifest")), true, false);
			const double EnumerateTime = FPlatformTime::Seconds() - StartEnumerate;
			GLog->Logf(TEXT("BuildDataGenerator: Found %d manifests in %.1f seconds"), AllManifests.Num(), EnumerateTime);

			// Load all manifest files
			uint64 NumFilePartsFound = 0;
			const double StartLoadAllManifest = FPlatformTime::Seconds();
			for (const auto& ManifestFile : AllManifests)
			{
				// Determine file parts from manifest file
				const FString ManifestFilename = CloudDir / ManifestFile;
				FBuildPatchAppManifestRef BuildManifest = MakeShareable(new FBuildPatchAppManifest());
				const double StartLoadManifest = FPlatformTime::Seconds();
				if (BuildManifest->LoadFromFile(ManifestFilename))
				{
					const double LoadManifestTime = FPlatformTime::Seconds() - StartLoadManifest;
					GLog->Logf(TEXT("BuildDataGenerator: Loaded %s in %.1f seconds"), *ManifestFile, LoadManifestTime);
					if (BuildManifest->GetManifestVersion() >= EBuildPatchAppManifestVersion::FileSplittingSupport)
					{
						if (BuildManifest->IsFileDataManifest())
						{
							TArray<FGuid> FilePartsReferenced;
							BuildManifest->GetDataList(FilePartsReferenced);
							for (const auto& FilePartGuid : FilePartsReferenced)
							{
								uint64 FilePartHash;
								if (BuildManifest->GetFilePartHash(FilePartGuid, FilePartHash))
			{
				// Add to inventory
									const FString SourceFile = FBuildPatchUtils::GetFileNewFilename(BuildManifest->GetManifestVersion(), CloudDir, FilePartGuid, FilePartHash);
									TArray< FString >& FileList = ExistingFileInventory.FindOrAdd(FilePartHash);
				FileList.Add(SourceFile);
									++NumFilePartsFound;
		}
		else
		{
									GLog->Logf(TEXT("BuildDataGenerator: WARNING: Missing part hash for %s in manifest %s"), *FilePartGuid.ToString(), *ManifestFile);
								}
							}
		}
						else
				{
							GLog->Logf(TEXT("BuildDataGenerator: INFO: Ignoring chunked manifest %s"), *ManifestFilename);
				}
					}
					else
					{
						GLog->Logf(TEXT("BuildDataGenerator: INFO: Not recognising from old file manifests %s"), *ManifestFilename);
					}
				}
				else
				{
					GLog->Logf(TEXT("BuildDataGenerator: WARNING: Could not read Manifest file. Data recognition will suffer (%s)"), *ManifestFilename);
				}
			}
			const double LoadAllManifestTime = FPlatformTime::Seconds() - StartLoadAllManifest;
			GLog->Logf(TEXT("BuildDataGenerator: Used %d manifests to enumerate %llu file parts in %.1f seconds"), AllManifests.Num(), NumFilePartsFound, LoadAllManifestTime);
		}
		else
		{
			GLog->Logf(TEXT("BuildDataGenerator: Cloud directory does not exist: %s"), *CloudDir);
		}

		ExistingFilesEnumerated = true;
	}

	// Do we have a file matching this data?
	if (ExistingFileInventory.Num() > 0)
	{
		TArray< FString >* FileList = ExistingFileInventory.Find(FilePartInfo.Hash);
		if (FileList != NULL)
		{
			// We need to load each file part in this list and compare data
			for (const FString& CloudFilename : *FileList)
			{
				// Check the file date
				FDateTime ModifiedDate = IFileManager::Get().GetTimeStamp(*CloudFilename);
				if (ModifiedDate < DataThresholdTime)
				{
					// We don't want to reuse this file's Guid, as it's older than any existing files we want to consider
					continue;
				}
				// Compare the files
				TArray<uint8> FoundFileData;
				FChunkHeader FoundHeader;
				if (FFileHelper::LoadFileToArray(FoundFileData, *CloudFilename))
				{
					const int32 OriginalFileSize = FoundFileData.Num();
					if (FBuildPatchUtils::UncompressFileDataFile(FoundFileData, &FoundHeader))
					{
						check(FoundHeader.StoredAs == FChunkHeader::STORED_RAW);
						const int64 FoundFileSize = FoundFileData.Num();
						if (FoundHeader.RollingHash == FilePartInfo.Hash && Data.Num() == FoundHeader.DataSize)
						{
						// Compare
							const bool bSameData = FMemory::Memcmp(Data.GetData(), FoundFileData.GetData() + FoundHeader.HeaderSize, FoundHeader.DataSize) == 0;
						// Did we match?
							if (bSameData)
						{
							// Yes we did!
								bFoundMatchingData = true;
								FilePartInfo.Guid = FoundHeader.Guid;
								FilePartInfo.FileSize = OriginalFileSize;
								FilePartData.Guid = FoundHeader.Guid;
								break;
						}
					}
				}
				}
			}
		}
	}

	return bFoundMatchingData;
}

bool FBuildDataGenerator::SaveOutFileDataPart(TArray<uint8>& Data, FChunkInfoData& FilePartInfo, FChunkPartData& FilePartData)
{
	bool bAlreadySaved = false;
	bool bSuccess = false;
	IFileManager& FileManager = IFileManager::Get();

	const FString NewFilename = FBuildPatchUtils::GetFileNewFilename(EBuildPatchAppManifestVersion::GetLatestVersion(), FBuildPatchServicesModule::GetCloudDirectory(), FilePartInfo.Guid, FilePartInfo.Hash);
	bAlreadySaved = FPaths::FileExists( NewFilename );

	if( !bAlreadySaved )
	{
		FArchive* FileOut = FileManager.CreateFileWriter( *NewFilename );
		bSuccess = FileOut != nullptr;

		if( bSuccess )
		{
			// LSwift: No support for too large file parts
			check(Data.Num() <= 0xFFFFFFFF);

			// Setup Header
			FChunkHeader Header;
			*FileOut << Header;
			Header.HeaderSize = FileOut->Tell();
			Header.Guid = FilePartInfo.Guid;
			Header.HashType = FChunkHeader::HASH_ROLLING;
			Header.RollingHash = FilePartInfo.Hash;

			// DataSize must be uncompressed size!
			Header.DataSize = Data.Num();

			// Setup to handle compression
			bool bDataIsCompressed = true;
			uint8* DataSource = Data.GetData();
			int32 DataSourceSize = Data.Num();
			TArray< uint8 > TempCompressedData;
			TempCompressedData.Empty(DataSourceSize);
			TempCompressedData.AddUninitialized(DataSourceSize);
			int32 CompressedSize = DataSourceSize;

			// Compressed can increase in size, but the function will return as failure in that case
			// we can allow that to happen since we would not keep larger compressed data anyway.
			bDataIsCompressed = FCompression::CompressMemory(
				static_cast<ECompressionFlags>(COMPRESS_ZLIB | COMPRESS_BiasMemory),
				TempCompressedData.GetData(),
				CompressedSize,
				DataSource,
				DataSourceSize);

			// If compression succeeded, set data vars
			if (bDataIsCompressed)
			{
				DataSource = TempCompressedData.GetData();
				DataSourceSize = CompressedSize;
			}

			Header.StoredAs = bDataIsCompressed ? FChunkHeader::STORED_COMPRESSED : FChunkHeader::STORED_RAW;
			Header.HashType = FChunkHeader::HASH_ROLLING;

			// Write out files
			FileOut->Seek(0);
			*FileOut << Header;
			FileOut->Serialize(DataSource, DataSourceSize);

			FilePartInfo.FileSize = FileOut->TotalSize();

			bSuccess = !FileOut->IsError();
		}

		// Close files
		if( FileOut != NULL )
		{
			FileOut->Close();
			delete FileOut;
		}
	}

	return bSuccess || bAlreadySaved;
}

bool FBuildDataGenerator::CompareDataToChunk(const FString& ChunkFilePath, uint8* ChunkData, FGuid& ChunkGuid, bool& OutSourceChunkIsValid)
{
	bool bMatching = false;

	// Read the file
	TSharedRef< FBuildGenerationChunkCache::FChunkReader > ChunkReader = FBuildGenerationChunkCache::Get().GetChunkReader(ChunkFilePath);
	if (ChunkReader->IsValidChunk())
	{
		ChunkGuid = ChunkReader->GetChunkGuid();
		// Default true
		bMatching = true;
		// Compare per small block (for early outing!)
		const uint32 CompareSize = 64;
		uint8* ReadBuffer;
		uint32 NumCompared = 0;
		while (bMatching && ChunkReader->BytesLeft() > 0 && NumCompared < FBuildPatchData::ChunkDataSize)
		{
			const uint32 ReadLen = FMath::Min< uint32 >(CompareSize, ChunkReader->BytesLeft());
			ChunkReader->ReadNextBytes(&ReadBuffer, ReadLen);
			bMatching = FMemory::Memcmp(&ChunkData[NumCompared], ReadBuffer, ReadLen) == 0;
			NumCompared += ReadLen;
		}
	}

	// Set chunk valid state after loading in case reading discovered bad or ignorable data
	OutSourceChunkIsValid = ChunkReader->IsValidChunk();

	return bMatching;
}

void FBuildDataGenerator::StripIgnoredFiles(TArray< FString >& AllFiles, const FString& DepotDirectory, const FString& IgnoreListFile)
{
	const int32 OriginalNumFiles = AllFiles.Num();
	FString IgnoreFileList = TEXT("");
	FFileHelper::LoadFileToString(IgnoreFileList, *IgnoreListFile);
	TArray< FString > IgnoreFiles;
	IgnoreFileList.ParseIntoArray(&IgnoreFiles, TEXT("\r\n"), true);
	struct FRemoveMatchingStrings
	{
		const FString* MatchingString;
		FRemoveMatchingStrings(const FString* InMatch)
			: MatchingString(InMatch) {}

		bool operator()(const FString& RemovalCandidate) const
		{
			FString PathA = RemovalCandidate;
			FString PathB = *MatchingString;
			FPaths::NormalizeFilename(PathA);
			FPaths::NormalizeFilename(PathB);
			return PathA == PathB;
		}
	};

	for (int32 IgnoreIdx = 0; IgnoreIdx < IgnoreFiles.Num(); ++IgnoreIdx)
	{
		const FString& IgnoreFile = IgnoreFiles[IgnoreIdx];
		const FString FullIgnorePath = DepotDirectory / IgnoreFile;
		AllFiles.RemoveAll(FRemoveMatchingStrings(&FullIgnorePath));
	}
	const int32 NewNumFiles = AllFiles.Num();
	GLog->Logf(TEXT("Stripped %d ignorable file(s)"), (OriginalNumFiles - NewNumFiles));
}

#endif //WITH_BUILDPATCHGENERATION
