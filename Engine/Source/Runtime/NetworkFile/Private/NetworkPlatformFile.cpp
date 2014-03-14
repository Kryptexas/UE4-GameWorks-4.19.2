// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "NetworkFilePrivatePCH.h"
#include "NetworkPlatformFile.h"
#include "Sockets.h"
#include "MultiChannelTcp.h"
#include "DerivedDataCacheInterface.h"
#include "PackageName.h"

#if WITH_UNREAL_DEVELOPER_TOOLS
	#include "Developer/PackageDependencyInfo/Public/PackageDependencyInfo.h"
#endif	//WITH_UNREAL_DEVELOPER_TOOLS

#include "IPlatformFileModule.h"


DEFINE_LOG_CATEGORY(LogNetworkPlatformFile);


FNetworkPlatformFile::FNetworkPlatformFile()
	: bHasLoadedDDCDirectories(false)
	, InnerPlatformFile(NULL)
	, bIsUsable(false)
	, FileServerPort(DEFAULT_FILE_SERVING_PORT)
	, FileSocket(NULL)
	, MCSocket(NULL)
	, FinishedAsyncReadUnsolicitedFiles(NULL)
	, FinishedAsyncWriteUnsolicitedFiles(NULL)
{
}

bool FNetworkPlatformFile::ShouldBeUsed(IPlatformFile* Inner, const TCHAR* CmdLine) const
{
	FString HostIp;
	return FParse::Value(CmdLine, TEXT("-FileHostIP="), HostIp);
}

bool FNetworkPlatformFile::Initialize(IPlatformFile* Inner, const TCHAR* CmdLine)
{
	bool bResult = false;
	FString HostIpString;	
	if (FParse::Value(CmdLine, TEXT("-FileHostIP="), HostIpString))
	{
		TArray<FString> HostIpList;
		if (HostIpString.ParseIntoArray(&HostIpList, TEXT("+"), true) > 0)
		{
			// Try to initialize with each of the IP addresses found in the command line until we 
			// get a working one.
			for (int32 HostIpIndex = 0; !bResult && HostIpIndex < HostIpList.Num(); ++HostIpIndex)
			{
				bResult = InitializeInternal(Inner, *HostIpList[HostIpIndex]);				
			}
		}
	}
	return bResult;
}

bool FNetworkPlatformFile::InitializeInternal(IPlatformFile* Inner, const TCHAR* HostIP)
{
	// This platform file requires an inner.
	check(Inner != NULL);
	InnerPlatformFile = Inner;
	if (HostIP == NULL)
	{
		UE_LOG(LogNetworkPlatformFile, Error, TEXT("No Host IP specified in the commandline."));
		bIsUsable = false;
		return false;
	}

	// Save and Intermediate directories are always local
	LocalDirectories.Add(FPaths::EngineDir() / TEXT("Binaries"));
	LocalDirectories.Add(FPaths::EngineIntermediateDir());
	LocalDirectories.Add(FPaths::GameDir() / TEXT("Binaries"));
	LocalDirectories.Add(FPaths::GameIntermediateDir());
	LocalDirectories.Add(FPaths::GameSavedDir() / TEXT("Backup"));
	LocalDirectories.Add(FPaths::GameSavedDir() / TEXT("Config"));
	LocalDirectories.Add(FPaths::GameSavedDir() / TEXT("Logs"));
	LocalDirectories.Add(FPaths::GameSavedDir() / TEXT("Sandboxes"));

	ISocketSubsystem* SSS = ISocketSubsystem::Get();

	// convert the string to a ip addr structure
	TSharedRef<FInternetAddr> Addr = SSS->CreateInternetAddr(0, FileServerPort);
	bool bIsValid;

	Addr->SetIp(HostIP, bIsValid);

	if (bIsValid)
	{
		// create the socket
		FileSocket = SSS->CreateSocket(NAME_Stream, TEXT("FNetworkPlatformFile tcp"));
		
		// try to connect to the server
		if (FileSocket->Connect(*Addr) == false)
		{
			// on failure, shut it all down
			SSS->DestroySocket(FileSocket);
			FileSocket = NULL;
			UE_LOG(LogNetworkPlatformFile, Error, TEXT("Failed to connect to file server at %s:%d."), HostIP, (int32)DEFAULT_FILE_SERVING_PORT);
		}
	}

	// was the socket opened?
	bIsUsable = FileSocket != NULL;
	if (bIsUsable)
	{
		FCommandLine::AddToSubprocessCommandline( *FString::Printf( TEXT("-FileHostIP=%s"), HostIP ) );
	}
	return bIsUsable;
}

void FNetworkPlatformFile::InitializeAfterSetActive()
{
	double NetworkFileStartupTime = 0.0;
	{
		SCOPE_SECONDS_COUNTER(NetworkFileStartupTime);

#if USE_MCSOCKET_FOR_NFS
		MCSocket = new FMultichannelTcpSocket(FileSocket, 64 * 1024 * 1024);
#endif

		// send the filenames and timestamps to the server
		FNetworkFileArchive Payload(NFS_Messages::GetFileList);
		FillGetFileList(Payload, false);

		// send the directories over, and wait for a response
		FArrayReader Response;
		if (!SendPayloadAndReceiveResponse(Payload, Response))
		{
			// on failure, shut it all down
			delete MCSocket;
			MCSocket = NULL;
			ISocketSubsystem::Get()->DestroySocket(FileSocket);
			FileSocket = NULL;
		}
		else
		{
			// receive the cooked version information
			int32 ServerPackageVersion = 0;
			int32 ServerPackageLicenseeVersion = 0;
			ProcessServerInitialResponse(Response, ServerPackageVersion, ServerPackageLicenseeVersion);

			// receive a list of the cache files and their timestamps
			TMap<FString, FDateTime> ServerCachedFiles;
			Response << ServerCachedFiles;

			bool bDeleteAllFiles = true;
			// Check the stored cooked version
			FString CookedVersionFile = FPaths::GeneratedConfigDir() / TEXT("CookedVersion.txt");

			if (InnerPlatformFile->FileExists(*CookedVersionFile) == true)
			{
				IFileHandle* FileHandle = InnerPlatformFile->OpenRead(*CookedVersionFile);
				if (FileHandle != NULL)
				{
					int32 StoredPackageCookedVersion;
					int32 StoredPackageCookedLicenseeVersion;
					if (FileHandle->Read((uint8*)&StoredPackageCookedVersion, sizeof(int32)) == true)
					{
						if (FileHandle->Read((uint8*)&StoredPackageCookedLicenseeVersion, sizeof(int32)) == true)
						{
							if ((ServerPackageVersion == StoredPackageCookedVersion) &&
								(ServerPackageLicenseeVersion == StoredPackageCookedLicenseeVersion))
							{
								bDeleteAllFiles = false;
							}
							else
							{
								UE_LOG(LogNetworkPlatformFile, Display, 
									TEXT("Engine version mismatch: Server %d.%d, Stored %d.%d\n"), 
									ServerPackageVersion, ServerPackageLicenseeVersion,
									StoredPackageCookedVersion, StoredPackageCookedLicenseeVersion);
							}
						}
					}

					delete FileHandle;
				}
			}
			else
			{
				UE_LOG(LogNetworkPlatformFile, Display, TEXT("Cooked version file missing: %s\n"), *CookedVersionFile);
			}

			if (bDeleteAllFiles == true)
			{
				// Make sure the config file exists...
				InnerPlatformFile->CreateDirectoryTree(*(FPaths::GeneratedConfigDir()));
				// Update the cooked version file
				IFileHandle* FileHandle = InnerPlatformFile->OpenWrite(*CookedVersionFile);
				if (FileHandle != NULL)
				{
					FileHandle->Write((const uint8*)&ServerPackageVersion, sizeof(int32));
					FileHandle->Write((const uint8*)&ServerPackageLicenseeVersion, sizeof(int32));
					delete FileHandle;
				}
			}

			// list of directories to skip
			TArray<FString> DirectoriesToSkip;
			TArray<FString> DirectoriesToNotRecurse;
			// use the timestamp grabbing visitor to get all the content times
			FLocalTimestampDirectoryVisitor Visitor(*InnerPlatformFile, DirectoriesToSkip, DirectoriesToNotRecurse, false);

			TArray<FString> RootContentPaths;
			FPackageName::QueryRootContentPaths( RootContentPaths );
			for( TArray<FString>::TConstIterator RootPathIt( RootContentPaths ); RootPathIt; ++RootPathIt )
			{
				const FString& RootPath = *RootPathIt;
				const FString& ContentFolder = FPackageName::LongPackageNameToFilename(RootPath);

				InnerPlatformFile->IterateDirectory( *ContentFolder, Visitor);
			}

			// delete out of date files using the server cached files
			for (TMap<FString, FDateTime>::TIterator It(ServerCachedFiles); It; ++It)
			{
				bool bDeleteFile = bDeleteAllFiles;
				FString ServerFile = It.Key();

				// Convert the filename to the client version
				ConvertServerFilenameToClientFilename(ServerFile);

				// Set it in the visitor file times list
				Visitor.FileTimes.Add(ServerFile, FDateTime::MinValue());

				if (bDeleteFile == false)
				{
					// Check the time stamps...
					// get local time
					FDateTime LocalTime = InnerPlatformFile->GetTimeStamp(*ServerFile);
					// If local time == MinValue than the file does not exist in the cache.
					if (LocalTime != FDateTime::MinValue())
					{
						FDateTime ServerTime = It.Value();
						// delete if out of date
						// We will use 1.0 second as the tolerance to cover any platform differences in resolution
						FTimespan TimeDiff = LocalTime - ServerTime;
						double TimeDiffInSeconds = TimeDiff.GetTotalSeconds();
						bDeleteFile = (TimeDiffInSeconds > 1.0) || (TimeDiffInSeconds < -1.0);
						if (bDeleteFile == true)
						{
							if (InnerPlatformFile->FileExists(*ServerFile) == true)
							{
								UE_LOG(LogNetworkPlatformFile, Display, TEXT("Deleting cached file: TimeDiff %5.3f, %s"), TimeDiffInSeconds, *It.Key());
							}
							else
							{
								// It's a directory
								bDeleteFile = false;
							}
						}
					}
				}
				if (bDeleteFile == true)
				{
					InnerPlatformFile->DeleteFile(*ServerFile);
				}
			}

			// Any content files we have locally that were not cached, delete them
			for (TMap<FString, FDateTime>::TIterator It(Visitor.FileTimes); It; ++It)
			{
				if (It.Value() != FDateTime::MinValue())
				{
					// This was *not* found in the server file list... delete it
					UE_LOG(LogNetworkPlatformFile, Display, TEXT("Deleting cached file: %s"), *It.Key());
					InnerPlatformFile->DeleteFile(*It.Key());
				}
			}

			// make sure we can sync a file
			FString TestSyncFile = FPaths::Combine(*(FPaths::EngineDir()), TEXT("Config/BaseEngine.ini"));

			InnerPlatformFile->SetReadOnly(*TestSyncFile, false);
			InnerPlatformFile->DeleteFile(*TestSyncFile);
			if (InnerPlatformFile->FileExists(*TestSyncFile))
			{
				UE_LOG(LogNetworkPlatformFile, Fatal, TEXT("Could not delete file sync test file %s."), *TestSyncFile);
			}

			EnsureFileIsLocal(TestSyncFile);

			if (!InnerPlatformFile->FileExists(*TestSyncFile) || InnerPlatformFile->FileSize(*TestSyncFile) < 1)
			{
				UE_LOG(LogNetworkPlatformFile, Fatal, TEXT("Could not sync test file %s."), *TestSyncFile);
			}
		}
	}

	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Network file startup time: %5.3f seconds\n"), NetworkFileStartupTime);

}

FNetworkPlatformFile::~FNetworkPlatformFile()
{
	if (FileSocket != NULL 
		&& !GIsRequestingExit) // the socket subsystem is probably already gone, so it will crash if we clean up
	{
		delete FinishedAsyncReadUnsolicitedFiles; // wait here for any async unsolicited files to finish reading being read from the network 
		FinishedAsyncReadUnsolicitedFiles = NULL;
		delete FinishedAsyncWriteUnsolicitedFiles; // wait here for any async unsolicited files to finish writing 
		FinishedAsyncWriteUnsolicitedFiles = NULL;
		// kill the socket
		delete MCSocket;
		MCSocket = NULL;
		ISocketSubsystem::Get()->DestroySocket(FileSocket);
	}
}

bool FNetworkPlatformFile::DeleteFile(const TCHAR* Filename)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	// make and send payload (this is how we would do for sending all commands over the network)
//	FNetworkFileArchive Payload(NFS_Messages::DeleteFile);
//	Payload << Filename;
//	return FNFSMessageHeader::WrapAndSendPayload(Payload, FileSocket);

	// perform a local operation
	return InnerPlatformFile->DeleteFile(Filename);
}

bool FNetworkPlatformFile::MoveFile(const TCHAR* To, const TCHAR* From)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	// make and send payload (this is how we would do for sending all commands over the network)
//	FNetworkFileArchive Payload(NFS_Messages::MoveFile);
//	Payload << To << From;
//	return FNFSMessageHeader::WrapAndSendPayload(Payload, FileSocket);

	FString RelativeFrom = From;
	FPaths::MakeStandardFilename(RelativeFrom);

	// don't copy files in local directories
	if (!IsInLocalDirectory(RelativeFrom))
	{
		// make sure the source file exists here
		EnsureFileIsLocal(RelativeFrom);
	}

	// perform a local operation
	return InnerPlatformFile->MoveFile(To, From);
}

bool FNetworkPlatformFile::SetReadOnly(const TCHAR* Filename, bool bNewReadOnlyValue)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	// perform a local operation
	return InnerPlatformFile->SetReadOnly(Filename, bNewReadOnlyValue);
}

void FNetworkPlatformFile::SetTimeStamp(const TCHAR* Filename, FDateTime DateTime)
{
	// perform a local operation
	InnerPlatformFile->SetTimeStamp(Filename, DateTime);
}

IFileHandle* FNetworkPlatformFile::OpenRead(const TCHAR* Filename)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	FString RelativeFilename = Filename;
	FPaths::MakeStandardFilename(RelativeFilename);
	// don't copy files in local directories
	if (!IsInLocalDirectory(RelativeFilename))
	{
		EnsureFileIsLocal(RelativeFilename);
	}

	double StartTime;
	float ThisTime;

	StartTime = FPlatformTime::Seconds();
	IFileHandle* Result = InnerPlatformFile->OpenRead(Filename);

	ThisTime = 1000.0f * float(FPlatformTime::Seconds() - StartTime);
	//UE_LOG(LogNetworkPlatformFile, Display, TEXT("Open local file %6.2fms"), ThisTime);
	return Result;
}

IFileHandle* FNetworkPlatformFile::OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	// just let the physical file interface write the file (we don't write over the network)
	return InnerPlatformFile->OpenWrite(Filename, bAppend, bAllowRead);
}

bool FNetworkPlatformFile::CreateDirectoryTree(const TCHAR* Directory)
{
	//	FScopeLock ScopeLock(&SynchronizationObject);

	// perform a local operation
	return InnerPlatformFile->CreateDirectoryTree(Directory);
}

bool FNetworkPlatformFile::CreateDirectory(const TCHAR* Directory)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	// perform a local operation
	return InnerPlatformFile->CreateDirectory(Directory);
}

bool FNetworkPlatformFile::DeleteDirectory(const TCHAR* Directory)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	// perform a local operation
	return InnerPlatformFile->DeleteDirectory(Directory);
}

bool FNetworkPlatformFile::IterateDirectory(const TCHAR* InDirectory, IPlatformFile::FDirectoryVisitor& Visitor)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	// for .dll, etc searches that don't specify a path, we need to strip off the path
	// before we send it to the visitor
	bool bHadNoPath = InDirectory[0] == 0;

	// local files go right to the source
	FString RelativeDirectory = InDirectory;
	FPaths::MakeStandardFilename(RelativeDirectory);
	if (IsInLocalDirectory(RelativeDirectory))
	{
		return InnerPlatformFile->IterateDirectory(InDirectory, Visitor);
	}

	// we loop until this is false
	bool RetVal = true;

	FServerTOC::FDirectory* ServerDirectory = ServerFiles.FindDirectory(RelativeDirectory);
	if (ServerDirectory != NULL)
	{
		// loop over the server files and look if they are in this exact directory
		for (FServerTOC::FDirectory::TIterator It(*ServerDirectory); It && RetVal == true; ++It)
		{
			if (FPaths::GetPath(It.Key()) == RelativeDirectory)
			{
				// timestamps of 0 mean directories
				bool bIsDirectory = It.Value() == 0;
			
				// visit (stripping off the path if needed)
				RetVal = Visitor.Visit(bHadNoPath ? *FPaths::GetCleanFilename(It.Key()) : *It.Key(), bIsDirectory);
			}
		}
	}

	return RetVal;
}

bool FNetworkPlatformFile::IterateDirectoryRecursively(const TCHAR* InDirectory, IPlatformFile::FDirectoryVisitor& Visitor)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	// local files go right to the source
	FString RelativeDirectory = InDirectory;
	FPaths::MakeStandardFilename(RelativeDirectory);

	if (IsInLocalDirectory(RelativeDirectory))
	{
		return InnerPlatformFile->IterateDirectoryRecursively(InDirectory, Visitor);
	}

	// we loop until this is false
	bool RetVal = true;

	for (TMap<FString, FServerTOC::FDirectory*>::TIterator DirIt(ServerFiles.Directories); DirIt && RetVal == true; ++DirIt)
	{
		if (DirIt.Key().StartsWith(RelativeDirectory))
		{
			FServerTOC::FDirectory& ServerDirectory = *DirIt.Value();
		
			// loop over the server files and look if they are in this exact directory
			for (FServerTOC::FDirectory::TIterator It(ServerDirectory); It && RetVal == true; ++It)
			{
				// timestamps of 0 mean directories
				bool bIsDirectory = It.Value() == 0;

				// visit!
				RetVal = Visitor.Visit(*It.Key(), bIsDirectory);
			}
		}
	}

	return RetVal;
}

bool FNetworkPlatformFile::DeleteDirectoryRecursively(const TCHAR* Directory)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	// perform a local operation
	return InnerPlatformFile->DeleteDirectory(Directory);
}

bool FNetworkPlatformFile::CopyFile(const TCHAR* To, const TCHAR* From)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	FString RelativeFrom = From;
	FPaths::MakeStandardFilename(RelativeFrom);

	// don't copy files in local directories
	if (!IsInLocalDirectory(RelativeFrom))
	{
		// make sure the source file exists here
		EnsureFileIsLocal(RelativeFrom);
	}

	// perform a local operation
	return InnerPlatformFile->CopyFile(To, From);
}

FString FNetworkPlatformFile::ConvertToAbsolutePathForExternalAppForRead( const TCHAR* Filename )
{
	FString RelativeFrom = Filename;
	FPaths::MakeStandardFilename(RelativeFrom);
	
	if (!IsInLocalDirectory(RelativeFrom))
	{
		EnsureFileIsLocal(RelativeFrom);
	}
	return InnerPlatformFile->ConvertToAbsolutePathForExternalAppForRead(Filename);
}

FString FNetworkPlatformFile::ConvertToAbsolutePathForExternalAppForWrite( const TCHAR* Filename )
{
	FString RelativeFrom = Filename;
	FPaths::MakeStandardFilename(RelativeFrom);

	if (!IsInLocalDirectory(RelativeFrom))
	{
		EnsureFileIsLocal(RelativeFrom);
	}
	return InnerPlatformFile->ConvertToAbsolutePathForExternalAppForWrite(Filename);
}

bool FNetworkPlatformFile::DirectoryExists(const TCHAR* Directory)
{
	if (InnerPlatformFile->DirectoryExists(Directory))
	{
		return true;
	}
	// If there are any syncable files in this directory, consider it existing
	FString RelativeDirectory = Directory;
	FPaths::MakeStandardFilename(RelativeDirectory);
	FServerTOC::FDirectory* ServerDirectory = ServerFiles.FindDirectory(RelativeDirectory);
	return ServerDirectory != NULL;
}

void FNetworkPlatformFile::GetFileInfo(const TCHAR* Filename, FFileInfo& Info)
{
	FString RelativeFilename = Filename;
	FPaths::MakeStandardFilename(RelativeFilename);

	// don't copy files in local directories
	if (!IsInLocalDirectory(RelativeFilename))
	{
		EnsureFileIsLocal(RelativeFilename);
	}

	// @todo: This is pretty inefficient. If GetFileInfo was a first class function, this could be avoided
	Info.FileExists = InnerPlatformFile->FileExists(Filename);
	Info.ReadOnly = InnerPlatformFile->IsReadOnly(Filename);
	Info.Size = InnerPlatformFile->FileSize(Filename);
	Info.TimeStamp = InnerPlatformFile->GetTimeStamp(Filename);
	Info.AccessTimeStamp = InnerPlatformFile->GetAccessTimeStamp(Filename);
}

void FNetworkPlatformFile::ConvertServerFilenameToClientFilename(FString& FilenameToConvert)
{
	FNetworkPlatformFile::ConvertServerFilenameToClientFilename(FilenameToConvert, ServerEngineDir, ServerGameDir);
}

void FNetworkPlatformFile::FillGetFileList(FNetworkFileArchive& Payload, bool bInStreamingFileRequest)
{
	TArray<FString> TargetPlatformNames;
	FPlatformMisc::GetValidTargetPlatforms(TargetPlatformNames);
	FString GameName = FApp::GetGameName();
	if (FPaths::IsProjectFilePathSet())
	{
		GameName = FPaths::GetProjectFilePath();
	}

	FString EngineRelPath = FPaths::EngineDir();
	FString GameRelPath = FPaths::GameDir();
	TArray<FString> Directories;
	Directories.Add(EngineRelPath);
	Directories.Add(GameRelPath);

	Payload << TargetPlatformNames;
	Payload << GameName;
	Payload << EngineRelPath;
	Payload << GameRelPath;
	Payload << Directories;
	Payload << bInStreamingFileRequest;
}

void FNetworkPlatformFile::ProcessServerInitialResponse(FArrayReader& InResponse, int32 OutServerPackageVersion, int32 OutServerPackageLicenseeVersion)
{
	// Receive the cooked version information.
	InResponse << OutServerPackageVersion;
	InResponse << OutServerPackageLicenseeVersion;

	// receive the server engine and game dir
	InResponse << ServerEngineDir;
	InResponse << ServerGameDir;

	UE_LOG(LogNetworkPlatformFile, Display, TEXT("    Server EngineDir = %s"), *ServerEngineDir);
	UE_LOG(LogNetworkPlatformFile, Display, TEXT("     Local EngineDir = %s"), *FPaths::EngineDir());
	UE_LOG(LogNetworkPlatformFile, Display, TEXT("    Server GameDir   = %s"), *ServerGameDir);
	UE_LOG(LogNetworkPlatformFile, Display, TEXT("     Local GameDir   = %s"), *FPaths::GameDir());

	// Receive a list of files and their timestamps.
	TMap<FString, FDateTime> ServerFileMap;
	InResponse << ServerFileMap;
	for (TMap<FString, FDateTime>::TIterator It(ServerFileMap); It; ++It)
	{
		FString ServerFile = It.Key();
		ConvertServerFilenameToClientFilename(ServerFile);
		ServerFiles.AddFileOrDirectory(ServerFile, It.Value());
	}
}

bool FNetworkPlatformFile::SendReadMessage(uint8* Destination, int64 BytesToRead)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	return true;
}

bool FNetworkPlatformFile::SendWriteMessage(const uint8* Source, int64 BytesToWrite)
{
//	FScopeLock ScopeLock(&SynchronizationObject);

	return true;
}

bool FNetworkPlatformFile::SendMessageToServer(const TCHAR* Message, IPlatformFile::IFileServerMessageHandler* Handler)
{
	// handle the recompile shaders message
	// @todo: Maybe we should just send the string message to the server, but then we'd have to 
	// handle the return from the server in a generic way
	if (FCString::Stricmp(Message, TEXT("RecompileShaders")) == 0)
	{
		FNetworkFileArchive Payload(NFS_Messages::RecompileShaders);

		// let the handler fill out the object
		Handler->FillPayload(Payload);

		FArrayReader Response;
#if USE_MCSOCKET_FOR_NFS
		if (FNFSMessageHeader::SendPayloadAndReceiveResponse(Payload, Response, FSimpleAbstractSocket_FMultichannelTCPSocket(MCSocket, NFS_Channels::Main)) == false)
#else
		if (FNFSMessageHeader::SendPayloadAndReceiveResponse(Payload, Response, FSimpleAbstractSocket_FSocket(FileSocket)) == false)
#endif
		{
			return false;
		}

		// locally delete any files that were modified on the server, so that any read will recache the file
		// this has to be done in this class, not in the Handler (which can't access these members)
		TArray<FString> ModifiedFiles;
		Response << ModifiedFiles;

		if( InnerPlatformFile != NULL )
		{
			for (int32 Index = 0; Index < ModifiedFiles.Num(); Index++)
			{
				InnerPlatformFile->DeleteFile(*ModifiedFiles[Index]);
				CachedLocalFiles.Remove(ModifiedFiles[Index]);
				ServerFiles.AddFileOrDirectory(ModifiedFiles[Index], FDateTime::UtcNow());
			}
		}


		// let the handler process the response directly
		Handler->ProcessResponse(Response);
	}

	return true;
}


//@todo, this should really be a member of FNetworkPlatformFile
static FThreadSafeCounter OutstandingAsyncWrites;

class FAsyncNetworkWriteWorker : public FNonAbandonableTask
{
public:
	/** Filename To write to**/
	FString							Filename;
	/** An archive to read the file contents from */
	FArchive* FileArchive;
	/** timestamp for the file **/
	FDateTime ServerTimeStamp;
	IPlatformFile& InnerPlatformFile;
	FScopedEvent* Event;

	uint8 Buffer[128 * 1024];

	/** Constructor
	*/
	FAsyncNetworkWriteWorker(const TCHAR* InFilename, FArchive* InArchive, FDateTime InServerTimeStamp, IPlatformFile* InInnerPlatformFile, FScopedEvent* InEvent)
		: Filename(InFilename)
		, FileArchive(InArchive)
		, ServerTimeStamp(InServerTimeStamp)
		, InnerPlatformFile(*InInnerPlatformFile)
		, Event(InEvent)
	{
	}
		
	/** Write the file  */
	void DoWork()
	{
		if (InnerPlatformFile.FileExists(*Filename))
		{
			InnerPlatformFile.SetReadOnly(*Filename, false);
			InnerPlatformFile.DeleteFile(*Filename);
		}
		// Read FileSize first so that the correct amount of data is read from the archive
		// before exiting this worker.
		uint64 FileSize;
		*FileArchive << FileSize;
		if (ServerTimeStamp != FDateTime::MinValue())  // if the file didn't actually exist on the server, don't create a zero byte file
		{
			FString TempFilename = Filename + TEXT(".tmp");
			InnerPlatformFile.CreateDirectoryTree(*FPaths::GetPath(Filename));
			{
				TAutoPtr<IFileHandle> FileHandle;
				FileHandle = InnerPlatformFile.OpenWrite(*TempFilename);

				if (!FileHandle)
				{
					UE_LOG(LogNetworkPlatformFile, Fatal, TEXT("Could not open file for writing '%s'."), *TempFilename);
				}

				// now write the file from bytes pulled from the archive
				// read/write a chunk at a time
				uint64 RemainingData = FileSize;
				while (RemainingData)
				{
					// read next chunk from archive
					uint32 LocalSize = FPlatformMath::Min<uint32>(ARRAY_COUNT(Buffer), RemainingData);
					FileArchive->Serialize(Buffer, LocalSize);
					// write it out
					if (!FileHandle->Write(Buffer, LocalSize))
					{
						UE_LOG(LogNetworkPlatformFile, Fatal, TEXT("Could not write '%s'."), *TempFilename);
					}

					// decrement how much is left
					RemainingData -= LocalSize;
				}

				// delete async write archives
				if (Event)
				{
					delete FileArchive;
				}

				if (InnerPlatformFile.FileSize(*TempFilename) != FileSize)
				{
					UE_LOG(LogNetworkPlatformFile, Fatal, TEXT("Did not write '%s'."), *TempFilename);
				}
			}

			// rename from temp filename to real filename
			InnerPlatformFile.MoveFile(*Filename, *TempFilename);

			// now set the server's timestamp on the local file (so we can make valid comparisons)
			InnerPlatformFile.SetTimeStamp(*Filename, ServerTimeStamp);

			FDateTime CheckTime = InnerPlatformFile.GetTimeStamp(*Filename);
			if (CheckTime < ServerTimeStamp)
			{
				UE_LOG(LogNetworkPlatformFile, Fatal, TEXT("Could Not Set Timestamp '%s'."), *Filename);
			}
		}
		if (Event)
		{
			if (!OutstandingAsyncWrites.Decrement())
			{
				Event->Trigger(); // last file, fire trigger
			}
		}
	}
	/** Give the name for external event viewers
	* @return	the name to display in external event viewers
	*/
	static const TCHAR *Name()
	{
		return TEXT("FAsyncNetworkWriteWorker");
	}
};

/**
 * Write a file async or sync, with the data coming from a TArray or an FArchive/Filesize
 */
void SyncWriteFile(FArchive* Archive, const FString& Filename, FDateTime ServerTimeStamp, IPlatformFile& InnerPlatformFile)
{
	FScopedEvent* NullEvent = NULL;
	(new FAutoDeleteAsyncTask<FAsyncNetworkWriteWorker>(*Filename, Archive, ServerTimeStamp, &InnerPlatformFile, NullEvent))->StartSynchronousTask();
}

void AsyncWriteFile(FArchive* Archive, const FString& Filename, FDateTime ServerTimeStamp, IPlatformFile& InnerPlatformFile, FScopedEvent* Event = NULL)
{
	(new FAutoDeleteAsyncTask<FAsyncNetworkWriteWorker>(*Filename, Archive, ServerTimeStamp, &InnerPlatformFile, Event))->StartBackgroundTask();
}

void AsyncReadUnsolicitedFile(int32 InNumUnsolictedFiles, FNetworkPlatformFile& InNetworkFile, FScopedEvent* InEvent, IPlatformFile& InInnerPlatformFile, 
							  FScopedEvent* InAllDoneEvent, FString& InServerEngineDir, FString& InServerGameDir)
{
	class FAsyncReadUnsolicitedFile : public FNonAbandonableTask
	{
	public:
		int32 NumUnsolictedFiles;
		FNetworkPlatformFile& NetworkFile;
		FScopedEvent* Event;
		IPlatformFile& InnerPlatformFile;
		FScopedEvent* AllDoneEvent;
		FString ServerEngineDir;
		FString ServerGameDir;

		FAsyncReadUnsolicitedFile(int32 In_NumUnsolictedFiles, FNetworkPlatformFile* In_NetworkFile, FScopedEvent* In_Event, IPlatformFile* In_InnerPlatformFile, 
			FScopedEvent* In_AllDoneEvent, FString& In_ServerEngineDir, FString& In_ServerGameDir)
			: NumUnsolictedFiles(In_NumUnsolictedFiles)
			, NetworkFile(*In_NetworkFile)
			, Event(In_Event)
			, InnerPlatformFile(*In_InnerPlatformFile)
			, AllDoneEvent(In_AllDoneEvent)
			, ServerEngineDir(In_ServerEngineDir)
			, ServerGameDir(In_ServerGameDir)
		{
			check(Event);
			check(AllDoneEvent);
		}
		
		/** Write the file  */
		void DoWork()
		{
			check(!OutstandingAsyncWrites.GetValue());
			OutstandingAsyncWrites.Add(NumUnsolictedFiles);
			for (int32 Index = 0; Index < NumUnsolictedFiles; Index++)
			{
				FArrayReader* UnsolictedResponse = new FArrayReader;
				if (!NetworkFile.ReceivePayload(*UnsolictedResponse))
				{
					UE_LOG(LogNetworkPlatformFile, Fatal, TEXT("Receive failure!"));
					return;
				}
				FString UnsolictedReplyFile;
				*UnsolictedResponse << UnsolictedReplyFile;
				FNetworkPlatformFile::ConvertServerFilenameToClientFilename(UnsolictedReplyFile, ServerEngineDir, ServerGameDir);

				// get the server file timestamp
				FDateTime UnsolictedServerTimeStamp;
				*UnsolictedResponse << UnsolictedServerTimeStamp;

				// write the file by pulling out of the FArrayReader
				AsyncWriteFile(UnsolictedResponse, UnsolictedReplyFile, UnsolictedServerTimeStamp, InnerPlatformFile, AllDoneEvent);
			}
			Event->Trigger();
		}
		/** Give the name for external event viewers
		* @return	the name to display in external event viewers
		*/
		static const TCHAR *Name()
		{
			return TEXT("FAsyncReadUnsolicitedFile");
		}
	};
	(new FAutoDeleteAsyncTask<FAsyncReadUnsolicitedFile>(InNumUnsolictedFiles, &InNetworkFile, InEvent, &InInnerPlatformFile, InAllDoneEvent, InServerEngineDir, InServerGameDir))->StartBackgroundTask();
}

/**
 * Given a filename, make sure the file exists on the local filesystem
 */

void FNetworkPlatformFile::EnsureFileIsLocal(const FString& Filename)
{
	double StartTime;
	float ThisTime;
	StartTime = FPlatformTime::Seconds();

	{
		FScopeLock ScopeLock(&SynchronizationObject);
		// have we already cached this file? 
		if (CachedLocalFiles.Find(Filename) != NULL)
		{
			return;
		}
	}

	delete FinishedAsyncReadUnsolicitedFiles; // wait here for any async unsolicited files to finish reading being read from the network 
	FinishedAsyncReadUnsolicitedFiles = NULL;
	delete FinishedAsyncWriteUnsolicitedFiles; // wait here for any async unsolicited files to finish writing 
	FinishedAsyncWriteUnsolicitedFiles = NULL;

	FScopeLock ScopeLock(&SynchronizationObject);

	ThisTime = 1000.0f * float(FPlatformTime::Seconds() - StartTime);
	//UE_LOG(LogNetworkPlatformFile, Display, TEXT("Lock and wait for old async writes %6.2fms"), ThisTime);

	// have we already cached this file? (test again, since some other thread might have done this between waits)
	if (CachedLocalFiles.Find(Filename) != NULL)
	{
		return;
	}
	// even if an error occurs later, we still want to remember not to try again
	CachedLocalFiles.Add(Filename);

	StartTime = FPlatformTime::Seconds();

	// no need to read it if it already exists 
	// @todo: Handshake with server to delete files that are out of date
	if (InnerPlatformFile->FileExists(*Filename))
	{
		return;
	}

	ThisTime = 1000.0f * float(FPlatformTime::Seconds() - StartTime);
	//UE_LOG(LogNetworkPlatformFile, Display, TEXT("Check for local file %6.2fms - %s"), ThisTime, *Filename);

	// this is a bit of a waste if we aren't doing cook on the fly, but we assume missing asset files are relatively rare
	bool bIsCookable = GConfig && GConfig->IsReadyForUse() && FPackageName::IsPackageExtension(*FPaths::GetExtension(Filename, true));

	// we only copy files that actually exist on the server, can greatly reduce network traffic for, say,
	// the INT file each package tries to load
	if (!bIsCookable && ServerFiles.FindFile(Filename) == NULL)
	{
		// Uncomment this to have the server file list dumped
		// the first time a file requested is not found.
#if 0
		static bool sb_DumpedServer = false;
		if (sb_DumpedServer == false)
		{
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Dumping server files... %s not found\n"), *Filename);
			for (TMap<FString, FServerTOC::FDirectory*>::TIterator ServerDumpIt(ServerFiles.Directories); ServerDumpIt; ++ServerDumpIt)
			{
				FServerTOC::FDirectory& Directory = *ServerDumpIt.Value();
				for (FServerTOC::FDirectory::TIterator DirDumpIt(Directory); DirDumpIt; ++DirDumpIt)
				{
					FPlatformMisc::LowLevelOutputDebugStringf(TEXT("%10s - %s\n"), *(DirDumpIt.Value().ToString()), *(DirDumpIt.Key()));
				}
			}
			sb_DumpedServer = true;
		}
#endif
		return;
	}

	// send the filename over (cast away const here because we know this << will not modify the string)
	FNetworkFileArchive Payload(NFS_Messages::SyncFile);
	Payload << (FString&)Filename;

	StartTime = FPlatformTime::Seconds();

	// allocate array reader on the heap, because the SyncWriteFile function will delete it
	FArrayReader Response;
	if (!SendPayloadAndReceiveResponse(Payload, Response))
	{
		UE_LOG(LogNetworkPlatformFile, Fatal, TEXT("Receive failure!"));
		return;
	}
	ThisTime = 1000.0f * float(FPlatformTime::Seconds() - StartTime);
	//UE_LOG(LogNetworkPlatformFile, Display, TEXT("Send and receive %6.2fms"), ThisTime);

	StartTime = FPlatformTime::Seconds();

	FString ReplyFile;
	Response << ReplyFile;
	ConvertServerFilenameToClientFilename(ReplyFile);
	check(ReplyFile == Filename);

	// get the server file timestamp
	FDateTime ServerTimeStamp;
	Response << ServerTimeStamp;

	// write the file in chunks, synchronously
	SyncWriteFile(&Response, ReplyFile, ServerTimeStamp, *InnerPlatformFile);

	int32 NumUnsolictedFiles;
	Response << NumUnsolictedFiles;

	if (NumUnsolictedFiles)
	{
		FinishedAsyncReadUnsolicitedFiles = new FScopedEvent;
		FinishedAsyncWriteUnsolicitedFiles = new FScopedEvent;
		AsyncReadUnsolicitedFile(NumUnsolictedFiles, *this, FinishedAsyncReadUnsolicitedFiles, *InnerPlatformFile, FinishedAsyncWriteUnsolicitedFiles, ServerEngineDir, ServerGameDir);
	}

	ThisTime = 1000.0f * float(FPlatformTime::Seconds() - StartTime);
	//UE_LOG(LogNetworkPlatformFile, Display, TEXT("Write file to local %6.2fms"), ThisTime);
}

bool FNetworkPlatformFile::IsInLocalDirectoryUnGuarded(const FString& Filename)
{
	// cache the directory of the input file
	FString Directory = FPaths::GetPath(Filename);

	// look if the file is in a local directory
	for (int32 DirIndex = 0; DirIndex < LocalDirectories.Num(); DirIndex++)
	{
		if (Directory.StartsWith(LocalDirectories[DirIndex]))
		{
			return true;
		}
	}

	// if not local, talk to the server
	return false;
}

bool FNetworkPlatformFile::IsInLocalDirectory(const FString& Filename)
{
	if (!bHasLoadedDDCDirectories)
	{
		// need to be careful here to avoid initializing the DDC from the wrong thread or using LocalDirectories while it is being initialized
		FScopeLock ScopeLock(&LocalDirectoriesCriticalSection);

		if (IsInGameThread() && GConfig && GConfig->IsReadyForUse())
		{
			// one time DDC directory initialization
			// add any DDC directories to our list of local directories (local = inner platform file, it may
			// actually live on a server, but it will use the platform's file system)
			if (GetDerivedDataCache())
			{
				TArray<FString> DdcDirectories;

				GetDerivedDataCacheRef().GetDirectories(DdcDirectories);

				LocalDirectories.Append(DdcDirectories);
			}

			FPlatformMisc::MemoryBarrier();
			bHasLoadedDDCDirectories = true;
		}

		return IsInLocalDirectoryUnGuarded(Filename);
	}

	// once the DDC is initialized, we don't need to lock a critical section anymore
	return IsInLocalDirectoryUnGuarded(Filename);
}

void FNetworkPlatformFile::PerformHeartbeat()
{
	// send the filename over (cast away const here because we know this << will not modify the string)
	FNetworkFileArchive Payload(NFS_Messages::Heartbeat);

	// send the filename over
	FArrayReader Response;
	if (!SendPayloadAndReceiveResponse(Payload, Response))
	{
		return;
	}

	// get any files that have been modified on the server - 
	TArray<FString> UpdatedFiles;
	Response << UpdatedFiles;

	// delete any outdated files from the client
	// @todo: This may need a critical section around all calls to LowLevel in the other functions
	// because we don't want to delete files while other threads are using them!
	for (int32 FileIndex = 0; FileIndex < UpdatedFiles.Num(); FileIndex++)
	{
		UE_LOG(LogNetworkPlatformFile, Log, TEXT("Server updated file '%s', deleting local copy"), *UpdatedFiles[FileIndex]);
		if (InnerPlatformFile->DeleteFile(*UpdatedFiles[FileIndex]) == false)
		{
			UE_LOG(LogNetworkPlatformFile, Error, TEXT("Failed to delete %s, someone is probably accessing without FNetworkPlatformFile, or we need better thread protection"), *UpdatedFiles[FileIndex]);
		}
	}
}

bool FNetworkPlatformFile::ReceivePayload(FArrayReader& OutPayload)
{
	if (MCSocket)
	{
		return FNFSMessageHeader::ReceivePayload(OutPayload, FSimpleAbstractSocket_FMultichannelTCPSocket(MCSocket, NFS_Channels::Main));
	}
	return FNFSMessageHeader::ReceivePayload(OutPayload, FSimpleAbstractSocket_FSocket(FileSocket));
}

bool FNetworkPlatformFile::WrapAndSendPayload(const TArray<uint8>& Payload)
{
	if (MCSocket)
	{
		return FNFSMessageHeader::WrapAndSendPayload(Payload, FSimpleAbstractSocket_FMultichannelTCPSocket(MCSocket, NFS_Channels::Main));
	}
	return FNFSMessageHeader::WrapAndSendPayload(Payload, FSimpleAbstractSocket_FSocket(FileSocket));
}

bool FNetworkPlatformFile::SendPayloadAndReceiveResponse(const TArray<uint8>& Payload, class FArrayReader& Response)
{
	if (!WrapAndSendPayload(Payload))
	{
		return false;
	}
	delete FinishedAsyncReadUnsolicitedFiles; // wait here for any async unsolicited files to finish being read from the network before we start to read again
	FinishedAsyncReadUnsolicitedFiles = NULL;
	return ReceivePayload(Response);
}

void FNetworkPlatformFile::ConvertServerFilenameToClientFilename(FString& FilenameToConvert, const FString& InServerEngineDir, const FString& InServerGameDir)
{
	if (FilenameToConvert.StartsWith(InServerEngineDir))
	{
		FilenameToConvert = FilenameToConvert.Replace(*InServerEngineDir, *(FPaths::EngineDir()));
	}
	else if (FilenameToConvert.StartsWith(InServerGameDir))
	{
		FilenameToConvert = FilenameToConvert.Replace(*InServerGameDir, *(FPaths::GameDir()));
	}
}

/**
 * Module for the network file
 */
class FNetworkFileModule : public IPlatformFileModule
{
public:
	virtual IPlatformFile* GetPlatformFile() OVERRIDE
	{
		static TScopedPointer<IPlatformFile> AutoDestroySingleton(new FNetworkPlatformFile());
		return AutoDestroySingleton.GetOwnedPointer();
	}
};
IMPLEMENT_MODULE(FNetworkFileModule, NetworkFile);
