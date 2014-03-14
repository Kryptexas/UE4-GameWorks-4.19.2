// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealPak.h"
#include "RequiredProgramMainCPPInclude.h"
#include "IPlatformFilePak.h"
#include "SecureHash.h"
#include "BigInt.h"
#include "SignedArchiveWriter.h"
#include "KeyGenerator.h"

IMPLEMENT_APPLICATION(UnrealPak, "UnrealPak");

struct FPakEntryPair
{
	FString Filename;
	FPakEntry Info;
};

struct FPakInputPair
{
	FString Source;
	FString Dest;

	FPakInputPair()
	{}

	FPakInputPair(const FString& InSource, const FString& InDest)
		: Source(InSource)
		, Dest(InDest)
	{}

	FORCEINLINE bool operator==(const FPakInputPair& Other) const
	{
		return Source == Other.Source;
	}
};

FString GetLongestPath(TArray<FPakInputPair>& FilesToAdd)
{
	FString LongestPath;
	int32 MaxNumDirectories = 0;

	for (int32 FileIndex = 0; FileIndex < FilesToAdd.Num(); FileIndex++)
	{
		FString& Filename = FilesToAdd[FileIndex].Dest;
		int32 NumDirectories = 0;
		for (int32 Index = 0; Index < Filename.Len(); Index++)
		{
			if (Filename[Index] == '/')
			{
				NumDirectories++;
			}
		}
		if (NumDirectories > MaxNumDirectories)
		{
			LongestPath = Filename;
			MaxNumDirectories = NumDirectories;
		}
	}
	return FPaths::GetPath(LongestPath) + TEXT("/");
}

FString GetCommonRootPath(TArray<FPakInputPair>& FilesToAdd)
{
	FString Root = GetLongestPath(FilesToAdd);
	for (int32 FileIndex = 0; FileIndex < FilesToAdd.Num() && Root.Len(); FileIndex++)
	{
		FString Filename(FilesToAdd[FileIndex].Dest);
		FString Path = FPaths::GetPath(Filename) + TEXT("/");
		int32 CommonSeparatorIndex = -1;
		int32 SeparatorIndex = Path.Find(TEXT("/"), ESearchCase::CaseSensitive);
		while (SeparatorIndex >= 0)
		{
			if (FCString::Strnicmp(*Root, *Path, SeparatorIndex + 1) != 0)
			{
				break;
			}
			CommonSeparatorIndex = SeparatorIndex;
			if (CommonSeparatorIndex + 1 < Path.Len())
			{
				SeparatorIndex = Path.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, CommonSeparatorIndex + 1);
			}
			else
			{
				break;
			}
		}
		if ((CommonSeparatorIndex + 1) < Root.Len())
		{
			Root = Root.Mid(0, CommonSeparatorIndex + 1);
		}
	}
	return Root;
}


bool CopyFileToPak(FArchive& InPak, const FString& InMountPoint, const FPakInputPair& InFile, uint8*& InOutPersistentBuffer, int64& InOutBufferSize, FPakEntryPair& OutNewEntry)
{	
	TAutoPtr<FArchive> FileHandle(IFileManager::Get().CreateFileReader(*InFile.Source));
	bool bFileExists = FileHandle.IsValid();
	if (bFileExists)
	{
		const int64 FileSize = FileHandle->TotalSize();
		OutNewEntry.Filename = InFile.Dest.Mid(InMountPoint.Len());
		OutNewEntry.Info.Offset = 0; // Don't serialize offsets here.
		OutNewEntry.Info.Size = FileSize;
		OutNewEntry.Info.UncompressedSize = FileSize;
		OutNewEntry.Info.CompressionMethod = 0;

		if (InOutBufferSize < FileSize)
		{
			InOutPersistentBuffer = (uint8*)FMemory::Realloc(InOutPersistentBuffer, FileSize);
			InOutBufferSize = FileSize;
		}

		// Load to buffer
		FileHandle->Serialize(InOutPersistentBuffer, FileSize);
		// Calculate the buffer hash value
		FSHA1::HashBuffer(InOutPersistentBuffer, FileSize, OutNewEntry.Info.Hash);

		// Write to file
		OutNewEntry.Info.Serialize(InPak, FPakInfo::PakFile_Version_Latest);
		InPak.Serialize(InOutPersistentBuffer, FileSize);
	}
	return bFileExists;
}

void ProcessCommandLine(int32 ArgC, ANSICHAR* ArgV[], TArray<FPakInputPair>& Entries)
{
	// List of all items to add to pak file
	FString ResponseFile;

	if (FParse::Value(FCommandLine::Get(), TEXT("-create="), ResponseFile))
	{
		FString Text;
		UE_LOG(LogPakFile, Display, TEXT("Loading response file %s"), *ResponseFile);
		if (FFileHelper::LoadFileToString(Text, *ResponseFile))
		{
			// Remove all carriage return characters.
			Text.ReplaceInline(TEXT("\r"), TEXT(""));
			// Read all lines
			TArray<FString> Lines;
			Text.ParseIntoArray(&Lines, TEXT("\n"), true);
			for (int32 EntryIndex = 0; EntryIndex < Lines.Num(); EntryIndex++)
			{
				TArray<FString> SourceAndDest;
				int32 Index = Lines[EntryIndex].Find("\" \"");
				if (Index > 0 && Lines[EntryIndex].StartsWith("\"") && Lines[EntryIndex].EndsWith("\""))
				{
					SourceAndDest.Add(Lines[EntryIndex].Mid(1, Index - 1));
					SourceAndDest.Add(Lines[EntryIndex].Mid(Index + 3, Lines[EntryIndex].Len() - Index - 4));
					if (SourceAndDest[1].Len() < 1)
					{
						SourceAndDest.RemoveAt(1);
					}
				}
				else
				{
					Lines[EntryIndex].ParseIntoArrayWS(&SourceAndDest);
				}
				FPakInputPair Input;
				Input.Source = SourceAndDest[0];
				FPaths::NormalizeFilename(Input.Source);
				if (SourceAndDest.Num() > 1)
				{
					Input.Dest = FPaths::GetPath(SourceAndDest[1]);
				}
				else
				{
					Input.Dest = FPaths::GetPath(Input.Source);
				}
				FPaths::NormalizeFilename(Input.Dest);
				FPakFile::MakeDirectoryFromPath(Input.Dest);
				Entries.Add(Input);
			}			
		}
		else
		{
			UE_LOG(LogPakFile, Error, TEXT("Failed to load %s"), *ResponseFile);
		}
	}
	else
	{
		// Override destination path.
		FString MountPoint;
		FParse::Value(FCommandLine::Get(), TEXT("-dest="), MountPoint);
		FPaths::NormalizeFilename(MountPoint);
		FPakFile::MakeDirectoryFromPath(MountPoint);

		// Parse comand line params. The first param after the program name is the created pak name
		for (int32 Index = 2; Index < ArgC; Index++)
		{
			// Skip switches and add everything else to the Entries array
			ANSICHAR* Param = ArgV[Index];
			if (Param[0] != '-')
			{
				FPakInputPair Input;
				Input.Source = Param;
				FPaths::NormalizeFilename(Input.Source);
				if (MountPoint.Len() > 0)
				{
					FString SourceDirectory( FPaths::GetPath(Input.Source) );
					FPakFile::MakeDirectoryFromPath(SourceDirectory);
					Input.Dest = Input.Source.Replace(*SourceDirectory, *MountPoint, ESearchCase::IgnoreCase);
				}
				else
				{
					Input.Dest = FPaths::GetPath(Input.Source);
					FPakFile::MakeDirectoryFromPath(Input.Dest);
				}
				FPaths::NormalizeFilename(Input.Dest);
				Entries.Add(Input);
			}
		}
	}
	UE_LOG(LogPakFile, Display, TEXT("Added %d entries to add to pak file."), Entries.Num());
}

void CollectFilesToAdd(TArray<FPakInputPair>& OutFilesToAdd, const TArray<FPakInputPair>& InEntries)
{
	UE_LOG(LogPakFile, Display, TEXT("Collecting files to add to pak file..."));
	const double StartTime = FPlatformTime::Seconds();

	// Start collecting files
	TSet<FString> AddedFiles;	
	for (int32 Index = 0; Index < InEntries.Num(); Index++)
	{
		const FPakInputPair& Input = InEntries[Index];

		FString Filename = FPaths::GetCleanFilename(Input.Source);
		FString Directory = FPaths::GetPath(Input.Source);
		FPaths::MakeStandardFilename(Directory);
		FPakFile::MakeDirectoryFromPath(Directory);

		if (Filename.IsEmpty())
		{
			Filename = TEXT("*.*");
		}
		if ( Filename.Contains(TEXT("*")) )
		{
			// Add multiple files
			TArray<FString> FoundFiles;
			IFileManager::Get().FindFilesRecursive(FoundFiles, *Directory, *Filename, true, false);

			for (int32 FileIndex = 0; FileIndex < FoundFiles.Num(); FileIndex++)
			{
				FPakInputPair FileInput;
				FileInput.Source = FoundFiles[FileIndex];
				FPaths::MakeStandardFilename(FileInput.Source);
				FileInput.Dest = FileInput.Source.Replace(*Directory, *Input.Dest, ESearchCase::IgnoreCase);
				if (!AddedFiles.Contains(FileInput.Source))
				{
					OutFilesToAdd.Add(FileInput);
					AddedFiles.Add(FileInput.Source);
				}
			}
		}
		else
		{
			// Add single file
			FPakInputPair FileInput;
			FileInput.Source = Input.Source;
			FPaths::MakeStandardFilename(FileInput.Source);
			FileInput.Dest = FileInput.Source.Replace(*Directory, *Input.Dest, ESearchCase::IgnoreCase);
			OutFilesToAdd.AddUnique(FileInput);
		}
	}
	// Sort alphabetically
	struct FInputPairSort
	{
		FORCEINLINE bool operator()(const FPakInputPair& A, const FPakInputPair& B) const
		{
			return A.Dest < B.Dest;
		}
	};
	OutFilesToAdd.Sort(FInputPairSort());
	UE_LOG(LogPakFile, Display, TEXT("Collected %d files in %.2lfs."), OutFilesToAdd.Num(), FPlatformTime::Seconds() - StartTime);
}

bool BufferedCopyFile(FArchive& Dest, FArchive& Source, const int64 FileSize, void* Buffer, const int64 BufferSize)
{	
	int64 RemainingSizeToCopy = FileSize;
	while (RemainingSizeToCopy > 0)
	{
		const int64 SizeToCopy = FMath::Min(BufferSize, RemainingSizeToCopy);
		Source.Serialize(Buffer, SizeToCopy);
		Dest.Serialize(Buffer, SizeToCopy);
		RemainingSizeToCopy -= SizeToCopy;
	}
	return true;
}

/**
 * Creates a pak file writer. This can be a signed writer if the encryption keys are specified in the command line
 */
FArchive* CreatePakWriter(const TCHAR* Filename)
{
	FArchive* Writer = IFileManager::Get().CreateFileWriter(Filename);
	FString KeyFilename;
	if (Writer && FParse::Value(FCommandLine::Get(), TEXT("sign="), KeyFilename, false))
	{
		FKeyPair Pair;
		if (KeyFilename.StartsWith(TEXT("0x")))
		{
			TArray<FString> KeyValueText;
			if (KeyFilename.ParseIntoArray(&KeyValueText, TEXT("+"), true) == 3)
			{
				Pair.PrivateKey.Exponent.Parse(KeyValueText[0]);
				Pair.PrivateKey.Modulus.Parse(KeyValueText[1]);
				Pair.PublicKey.Exponent.Parse(KeyValueText[2]);
				Pair.PublicKey.Modulus = Pair.PrivateKey.Modulus;
				UE_LOG(LogPakFile, Display, TEXT("Parsed signature keys from command line."));
			}
			else
			{
				UE_LOG(LogPakFile, Error, TEXT("Expected 3 values, got %d, when parsing %s"), KeyValueText.Num(), *KeyFilename);
				Pair.PrivateKey.Exponent.Zero();
			}
		}
		else if (!ReadKeysFromFile(*KeyFilename, Pair))
		{
			UE_LOG(LogPakFile, Error, TEXT("Unable to load signature keys %s."), *KeyFilename);
			Pair.PrivateKey.Exponent.Zero();
		}

		if (!Pair.PrivateKey.Exponent.IsZero())
		{
			UE_LOG(LogPakFile, Display, TEXT("Creating signed pak %s."), Filename);
			Writer = new FSignedArchiveWriter(*Writer, Pair.PublicKey, Pair.PrivateKey);
		}
		else
		{
			UE_LOG(LogPakFile, Error, TEXT("Unable to create a signed pak writer."));
			delete Writer;
			Writer = NULL;
		}		
	}
	return Writer;
}

bool CreatePakFile(const TCHAR* Filename, TArray<FPakInputPair>& FilesToAdd)
{	
	const double StartTime = FPlatformTime::Seconds();

	// Create Pak
	TAutoPtr<FArchive> PakFileHandle(CreatePakWriter(Filename));
	if (!PakFileHandle.IsValid())
	{
		UE_LOG(LogPakFile, Error, TEXT("Unable to create pak file \"%s\"."), Filename);
		return false;
	}

	FPakInfo Info;
	TArray<FPakEntryPair> Index;
	FString MountPoint = GetCommonRootPath(FilesToAdd);
	uint8* ReadBuffer = NULL;
	int64 BufferSize = 0;

	for (int32 FileIndex = 0; FileIndex < FilesToAdd.Num(); FileIndex++)
	{
		//  Remember the offset but don't serialize it with the entry header.
		const int64 NewEntryOffset = PakFileHandle->Tell();
		FPakEntryPair NewEntry;
		if (CopyFileToPak(*PakFileHandle, MountPoint, FilesToAdd[FileIndex], ReadBuffer, BufferSize, NewEntry) == true)
		{
			// Update offset now and store it in the index (and only in index)
			NewEntry.Info.Offset = NewEntryOffset;
			Index.Add(NewEntry);
			UE_LOG(LogPakFile, Display, TEXT("Added file \"%s\", %lld bytes."), *NewEntry.Filename, NewEntry.Info.Size);
		}
		else
		{
			UE_LOG(LogPakFile, Warning, TEXT("Missing file \"%s\" will not be added to PAK file."), *FilesToAdd[FileIndex].Source);
		}
	}

	FMemory::Free(ReadBuffer);
	ReadBuffer = NULL;

	// Remember IndexOffset
	Info.IndexOffset = PakFileHandle->Tell();

	// Serialize Pak Index at the end of Pak File
	TArray<uint8> IndexData;
	FMemoryWriter IndexWriter(IndexData);
	IndexWriter.SetByteSwapping(PakFileHandle->ForceByteSwapping());
	int32 NumEntries = Index.Num();
	IndexWriter << MountPoint;
	IndexWriter << NumEntries;
	for (int32 EntryIndex = 0; EntryIndex < Index.Num(); EntryIndex++)
	{
		FPakEntryPair& Entry = Index[EntryIndex];
		IndexWriter << Entry.Filename;
		Entry.Info.Serialize(IndexWriter, Info.Version);
	}
	PakFileHandle->Serialize(IndexData.GetData(), IndexData.Num());

	FSHA1::HashBuffer(IndexData.GetData(), IndexData.Num(), Info.IndexHash);
	Info.IndexSize = IndexData.Num();

	// Save trailer (offset, size, hash value)
	Info.Serialize(*PakFileHandle);

	UE_LOG(LogPakFile, Display, TEXT("Added %d files, %lld bytes total, time %.2lfs."), Index.Num(), PakFileHandle->TotalSize(), FPlatformTime::Seconds() - StartTime);

	PakFileHandle->Close();
	PakFileHandle.Reset();

	return true;
}

bool TestPakFile(const TCHAR* Filename)
{	
	FPakFile PakFile(Filename, FParse::Param(FCommandLine::Get(), TEXT("signed")));
	if (PakFile.IsValid())
	{
		UE_LOG(LogPakFile, Display, TEXT("Checking pak file \"%s\". This may take a while..."), Filename);
		FArchive& PakReader = *PakFile.GetSharedReader(NULL);
		int32 ErrorCount = 0;
		int32 FileCount = 0;

		for (FPakFile::FFileIterator It(PakFile); It; ++It, ++FileCount)
		{
			const FPakEntry& Entry = It.Info();
			void* FileContents = FMemory::Malloc(Entry.Size);
			PakReader.Seek(Entry.Offset);
			uint32 SerializedCrcTest = 0;
			FPakEntry EntryInfo;
			EntryInfo.Serialize(PakReader, PakFile.GetInfo().Version);
			if (EntryInfo != Entry)
			{
				UE_LOG(LogPakFile, Error, TEXT("Serialized hash mismatch for \"%s\"."), *It.Filename());
				ErrorCount++;
			}
			PakReader.Serialize(FileContents, Entry.Size);
		
			uint8 TestHash[20];
			FSHA1::HashBuffer(FileContents, Entry.Size, TestHash);
			if (FMemory::Memcmp(TestHash, Entry.Hash, sizeof(TestHash)) != 0)
			{
				UE_LOG(LogPakFile, Error, TEXT("Hash mismatch for \"%s\"."), *It.Filename());
				ErrorCount++;
			}
			else
			{
				UE_LOG(LogPakFile, Display, TEXT("\"%s\" OK."), *It.Filename());
			}
			FMemory::Free(FileContents);
		}
		if (ErrorCount == 0)
		{
			UE_LOG(LogPakFile, Display, TEXT("Pak file \"%s\" healthy, %d files checked."), Filename, FileCount);
		}
		else
		{
			UE_LOG(LogPakFile, Display, TEXT("Pak file \"%s\" corrupted (%d errors ouf of %d files checked.)."), Filename, ErrorCount, FileCount);
		}
		return (ErrorCount == 0);
	}
	else
	{
		UE_LOG(LogPakFile, Error, TEXT("Unable to open pak file \"%s\"."), Filename);
		return false;
	}
}

bool ExtractFilesFromPak(const TCHAR* InPakFilename, const TCHAR* InDestPath)
{
	FPakFile PakFile(InPakFilename, FParse::Param(FCommandLine::Get(), TEXT("signed")));
	if (PakFile.IsValid())
	{
		FString DestPath(InDestPath);
		FArchive& PakReader = *PakFile.GetSharedReader(NULL);
		const int64 BufferSize = 8 * 1024 * 1024; // 8MB buffer for extracting
		void* Buffer = FMemory::Malloc(BufferSize);
		int32 ErrorCount = 0;
		int32 FileCount = 0;

		for (FPakFile::FFileIterator It(PakFile); It; ++It, ++FileCount)
		{
			const FPakEntry& Entry = It.Info();
			PakReader.Seek(Entry.Offset);
			uint32 SerializedCrcTest = 0;
			FPakEntry EntryInfo;
			EntryInfo.Serialize(PakReader, PakFile.GetInfo().Version);
			if (EntryInfo == Entry)
			{
				FString DestFilename(DestPath / It.Filename());
				TAutoPtr<FArchive> FileHandle(IFileManager::Get().CreateFileWriter(*DestFilename));
				if (FileHandle.IsValid())
				{
					BufferedCopyFile(*FileHandle, PakReader, Entry.Size, Buffer, BufferSize);
					UE_LOG(LogPakFile, Display, TEXT("Extracted \"%s\" to \"%s\"."), *It.Filename(), *DestFilename);
				}
				else
				{
					UE_LOG(LogPakFile, Error, TEXT("Unable to create file \"%s\"."), *DestFilename);
					ErrorCount++;
				}
			}
			else
			{
				UE_LOG(LogPakFile, Error, TEXT("Serialized hash mismatch for \"%s\"."), *It.Filename());
				ErrorCount++;
			}
		}
		FMemory::Free(Buffer);

		UE_LOG(LogPakFile, Error, TEXT("Finished extracting %d files (including %d errors)."), FileCount, ErrorCount);

		return true;
	}
	else
	{
		UE_LOG(LogPakFile, Error, TEXT("Unable to open pak file \"%s\"."), InPakFilename);
		return false;
	}
}

/**
 * Application entry point
 * Params:
 *   -Test test if the pak file is healthy
 *   -Extract extracts pak file contents (followed by a path, i.e.: -extract D:\ExtractedPak)
 *   -Create=filename response file to create a pak file with
 *   -Sign=filename use the key pair in filename to sign a pak file, or: -sign=key_hex_values_separated_with_+, i.e: -sign=0x123456789abcdef+0x1234567+0x12345abc
 *    where the first number is the private key exponend, the second one is modulus and the third one is the public key exponent.
 *   -Signed use with -extract and -test to let the code know this is a signed pak
 *   -GenerateKeys=filename generates encryption key pair for signing a pak file
 *   -P=prime will use a predefined prime number for generating encryption key file
 *   -Q=prime same as above, P != Q, GCD(P, Q) = 1 (which is always true if they're both prime)
 *   -GeneratePrimeTable=filename generates a prime table for faster prime number generation (.inl file)
 *   -TableMax=number maximum prime number in the generated table (default is 10000)
 *
 * @param	ArgC	Command-line argument count
 * @param	ArgV	Argument strings
 */
int32 main(int32 ArgC, ANSICHAR* ArgV[])
{
	// start up the main loop
	GEngineLoop.PreInit(ArgC, ArgV);
	
	if (ArgC < 2)
	{
		UE_LOG(LogPakFile, Error, TEXT("No pak file name specified."));
		return 1;
	}

	int32 Result = 0;
	FString KeyFilename;
	if (FParse::Value(FCommandLine::Get(), TEXT("GenerateKeys="), KeyFilename, false))
	{
		Result = GenerateKeys(*KeyFilename) ? 0 : 1;
	}
	else if (FParse::Value(FCommandLine::Get(), TEXT("GeneratePrimeTable="), KeyFilename, false))
	{
		int64 MaxPrimeValue = 10000;
		FParse::Value(FCommandLine::Get(), TEXT("TableMax="), MaxPrimeValue);
		GeneratePrimeNumberTable(MaxPrimeValue, *KeyFilename);
	}
	else 
	{
		FString PakFilename(ArgV[1]);
		FPaths::MakeStandardFilename(PakFilename);

		if (FParse::Param(FCommandLine::Get(), TEXT("Test")))
		{
			Result = TestPakFile(*PakFilename) ? 0 : 1;
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("Extract")))
		{
			if (ArgC < 4)
			{
				UE_LOG(LogPakFile, Error, TEXT("No extraction path specified."));
				Result = 1;
			}
			else
			{
				FString DestPath = (ArgV[2][0] == '-') ? ArgV[3] : ArgV[2];
				Result = ExtractFilesFromPak(*PakFilename, *DestPath) ? 0 : 1;
			}
		}
		else
		{
			// List of all items to add to pak file
			TArray<FPakInputPair> Entries;
			ProcessCommandLine(ArgC, ArgV, Entries);

			if (Entries.Num() == 0)
			{
				UE_LOG(LogPakFile, Error, TEXT("No files specified to add to pak file."));
				Result = 1;
			}
			else
			{
				// Start collecting files
				TArray<FPakInputPair> FilesToAdd;
				CollectFilesToAdd(FilesToAdd, Entries);

				Result = CreatePakFile(*PakFilename, FilesToAdd) ? 0 : 1;
			}
		}
	}

	return Result;
}
