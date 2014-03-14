// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NetworkFileServerConnection.h: Declares the FNetworkFileServerClientConnection class.
=============================================================================*/

#include "NetworkFileSystemPrivatePCH.h"
#include "PackageName.h"
#include "TargetPlatform.h"


/* FNetworkFileServerClientConnection structors
 *****************************************************************************/

FNetworkFileServerClientConnection::FNetworkFileServerClientConnection( FSocket* InSocket, const FFileRequestDelegate& InFileRequestDelegate, const FRecompileShadersDelegate& InRecompileShadersDelegate )
	: LastHandleId(0)
	, MCSocket(NULL)
	, Sandbox(NULL)
	, Socket(InSocket)
	, bNeedsToStop(false)
{
	if (InFileRequestDelegate.IsBound())
	{
		FileRequestDelegate = InFileRequestDelegate;
	}

	if (InRecompileShadersDelegate.IsBound())
	{
		RecompileShadersDelegate = InRecompileShadersDelegate;
	}

	Thread = FRunnableThread::Create(this, TEXT("FNetworkFileServerClientConnection"), false, false, 8 * 1024, TPri_AboveNormal);
}


FNetworkFileServerClientConnection::~FNetworkFileServerClientConnection( )
{
	// Kill the running thread.
	if( Thread != NULL )
	{
		Thread->Kill(true);

		delete Thread;
		Thread = NULL;
	}

	// We are done with the socket.
	Socket->Close();
	ISocketSubsystem::Get()->DestroySocket(Socket);
	Socket = NULL;
}


/* FRunnable overrides
 *****************************************************************************/

bool FNetworkFileServerClientConnection::Init( )
{
#if USE_MCSOCKET_FOR_NFS
	MCSocket = new FMultichannelTCPSocket(Socket, 64 * 1024 * 1024);
#endif
	return true;
}


uint32 FNetworkFileServerClientConnection::Run( )
{
	while (!bNeedsToStop)
	{
		// read a header and payload pair
		FArrayReader Payload; 
#if USE_MCSOCKET_FOR_NFS
		if (FNFSMessageHeader::ReceivePayload(Payload, FSimpleAbstractSocket_FMultichannelTCPSocket(MCSocket, NFS_Channels::Main)) == false)
#else
		if (FNFSMessageHeader::ReceivePayload(Payload, FSimpleAbstractSocket_FSocket(Socket)) == false)
#endif
		{
			// if we failed to receive the payload, then the client is most likely dead, so, we can kill this connection
			// @todo: Add more error codes, maybe some errors shouldn't kill the connection
			break;
		}

		// now process the contents of the payload
		ProcessPayload(Payload);
	}

	bNeedsToStop = true;
	return 0;
}


void FNetworkFileServerClientConnection::Stop( )
{
	bNeedsToStop = true;
}

void FNetworkFileServerClientConnection::Exit()
{
	// close all the files the client had opened through us when the client disconnects
	for (TMap<uint64, IFileHandle*>::TIterator It(OpenFiles); It; ++It)
	{
		delete It.Value();
	}

	delete Sandbox;
	Sandbox = NULL;	

#if	USE_MCSOCKET_FOR_NFS
	delete MCSocket;
	MCSocket = NULL;
#endif // USE_MCSOCKET_FOR_NFS	
}


/* FStreamingNetworkFileServerConnection implementation
 *****************************************************************************/
void FNetworkFileServerClientConnection::ConvertClientFilenameToServerFilename(FString& FilenameToConvert)
{
	if (FilenameToConvert.StartsWith(ConnectedEngineDir))
	{
		FilenameToConvert = FilenameToConvert.Replace(*ConnectedEngineDir, *(FPaths::EngineDir()));
	}
	else if (FilenameToConvert.StartsWith(ConnectedGameDir))
	{
		if ( FPaths::IsProjectFilePathSet() )
		{
			FilenameToConvert = FilenameToConvert.Replace(*ConnectedGameDir, *(FPaths::GetPath(FPaths::GetProjectFilePath()) + TEXT("/")));
		}
		else
		{
#if !IS_PROGRAM
			// UnrealFileServer has a GameDir of ../../../Engine/Programs/UnrealFileServer.
			// We do *not* want to replace the directory in that case.
			FilenameToConvert = FilenameToConvert.Replace(*ConnectedGameDir, *(FPaths::GameDir()));
#endif
		}
	}
}

void FNetworkFileServerClientConnection::ConvertServerFilenameToClientFilename(FString& FilenameToConvert)
{
	if (FilenameToConvert.StartsWith(FPaths::EngineDir()))
	{
		FilenameToConvert = FilenameToConvert.Replace(*(FPaths::EngineDir()), *ConnectedEngineDir);
	}
	else if (FPaths::IsProjectFilePathSet())
	{
		if (FilenameToConvert.StartsWith(FPaths::GetPath(FPaths::GetProjectFilePath())))
		{
			FilenameToConvert = FilenameToConvert.Replace(*(FPaths::GetPath(FPaths::GetProjectFilePath()) + TEXT("/")), *ConnectedGameDir);
		}
	}
#if !IS_PROGRAM
	else if (FilenameToConvert.StartsWith(FPaths::GameDir()))
	{
			// UnrealFileServer has a GameDir of ../../../Engine/Programs/UnrealFileServer.
			// We do *not* want to replace the directory in that case.
			FilenameToConvert = FilenameToConvert.Replace(*(FPaths::GameDir()), *ConnectedGameDir);
	}
#endif
}

void FNetworkFileServerClientConnection::ProcessPayload( FArchive& Ar )
{
	// first part of the payload is always the command
	uint32 Cmd;
	Ar << Cmd;

	UE_LOG(LogFileServer, Verbose, TEXT("Processing payload with Cmd %d"), Cmd);

	// what type of message is this?
	NFS_Messages::Type Msg = NFS_Messages::Type(Cmd);

	// allocate an archive for the commands to write to
	FBufferArchive Out;

	// make sure the first thing is GetFileList which initializes the game/platform
	checkf(Msg == NFS_Messages::GetFileList || Sandbox != NULL, TEXT("The first client message MUST be GetFileList, not %d"), (int32)Msg);

	// process the message!
	bool bSendUnsolicitedFiles = false;

	switch (Msg)
	{
	case NFS_Messages::OpenRead:
		ProcessOpenFile(Ar, Out, false);
		break;

	case NFS_Messages::OpenWrite:
		ProcessOpenFile(Ar, Out, true);
		break;

	case NFS_Messages::Read:
		ProcessReadFile(Ar, Out);
		break;

	case NFS_Messages::Write:
		ProcessWriteFile(Ar, Out);
		break;

	case NFS_Messages::Seek:
		ProcessSeekFile(Ar, Out);
		break;

	case NFS_Messages::Close:
		ProcessCloseFile(Ar, Out);
		break;

	case NFS_Messages::MoveFile:
		ProcessMoveFile(Ar, Out);
		break;

	case NFS_Messages::DeleteFile:
		ProcessDeleteFile(Ar, Out);
		break;

	case NFS_Messages::GetFileInfo:
		ProcessGetFileInfo(Ar, Out);
		break;

	case NFS_Messages::CopyFile:
		ProcessCopyFile(Ar, Out);
		break;

	case NFS_Messages::SetTimeStamp:
		ProcessSetTimeStamp(Ar, Out);
		break;

	case NFS_Messages::SetReadOnly:
		ProcessSetReadOnly(Ar, Out);
		break;

	case NFS_Messages::CreateDirectory:
		ProcessCreateDirectory(Ar, Out);
		break;

	case NFS_Messages::DeleteDirectory:
		ProcessDeleteDirectory(Ar, Out);
		break;

	case NFS_Messages::DeleteDirectoryRecursively:
		ProcessDeleteDirectoryRecursively(Ar, Out);
		break;

	case NFS_Messages::ToAbsolutePathForRead:
		ProcessToAbsolutePathForRead(Ar, Out);
		break;

	case NFS_Messages::ToAbsolutePathForWrite:
		ProcessToAbsolutePathForWrite(Ar, Out);
		break;

	case NFS_Messages::ReportLocalFiles:
		ProcessReportLocalFiles(Ar, Out);
		break;

	case NFS_Messages::GetFileList:
		ProcessGetFileList(Ar, Out);
		break;

	case NFS_Messages::Heartbeat:
		ProcessHeartbeat(Ar, Out);
		break;

	case NFS_Messages::SyncFile:
		ProcessSyncFile(Ar, Out);
		bSendUnsolicitedFiles = true;
		break;

	case NFS_Messages::RecompileShaders:
		ProcessRecompileShaders(Ar, Out);
		break;

	default:

		UE_LOG(LogFileServer, Error, TEXT("Bad incomming message tag (%d)."), (int32)Msg);
	}

	// send back a reply if the command wrote anything back out
	if (Out.Num())
	{
		int32 NumUnsolictedFiles = UnsolictedFiles.Num();
		if (bSendUnsolicitedFiles)
		{
			Out << NumUnsolictedFiles;
		}

		UE_LOG(LogFileServer, Verbose, TEXT("Returning payload with %d bytes"), Out.Num());

		// send back a reply
#if USE_MCSOCKET_FOR_NFS
		FNFSMessageHeader::WrapAndSendPayload(Out, FSimpleAbstractSocket_FMultichannelTCPSocket(MCSocket, NFS_Channels::Main));
#else
		FNFSMessageHeader::WrapAndSendPayload(Out, FSimpleAbstractSocket_FSocket(Socket));
#endif

		if (bSendUnsolicitedFiles)
		{
			for (int32 Index = 0; Index < NumUnsolictedFiles; Index++)
			{
				FBufferArchive OutUnsolicitedFile;
				PackageFile(UnsolictedFiles[Index], OutUnsolicitedFile);

				UE_LOG(LogFileServer, Display, TEXT("Returning unsolicited file %s with %d bytes"), *UnsolictedFiles[Index], OutUnsolicitedFile.Num());

#if USE_MCSOCKET_FOR_NFS
				FNFSMessageHeader::WrapAndSendPayload(OutUnsolicitedFile, FSimpleAbstractSocket_FMultichannelTCPSocket(MCSocket, NFS_Channels::Main));
#else
				FNFSMessageHeader::WrapAndSendPayload(OutUnsolicitedFile, FSimpleAbstractSocket_FSocket(Socket));
#endif
			}

			UnsolictedFiles.Empty();
		}
	}
}


void FNetworkFileServerClientConnection::ProcessOpenFile( FArchive& In, FArchive& Out, bool bIsWriting )
{
	// Get filename
	FString Filename;
	In << Filename;

	bool bAppend = false;
	bool bAllowRead = false;

	if (bIsWriting)
	{
		In << bAppend;
		In << bAllowRead;
	}

	ConvertClientFilenameToServerFilename(Filename);

	if (bIsWriting)
	{
		// Make sure the directory exists...
		Sandbox->CreateDirectoryTree(*(FPaths::GetPath(Filename)));
	}

	TArray<FString> NewUnsolictedFiles;
	FileRequestDelegate.ExecuteIfBound(Filename, NewUnsolictedFiles);

	FDateTime ServerTimeStamp = Sandbox->GetTimeStamp(*Filename);
	int64 ServerFileSize = 0;
	IFileHandle* File = bIsWriting ? Sandbox->OpenWrite(*Filename, bAppend, bAllowRead) : Sandbox->OpenRead(*Filename);
	if (!File)
	{
		UE_LOG(LogFileServer, Display, TEXT("Open request for %s failed for file %s."), bIsWriting ? TEXT("Writing") : TEXT("Reading"), *Filename);
		ServerTimeStamp = FDateTime::MinValue(); // if this was a directory, this will make sure it is not confused with a zero byte file
	}
	else
	{
		ServerFileSize = File->Size();
	}

	uint64 HandleId = ++LastHandleId;
	OpenFiles.Add( HandleId, File );

	Out << HandleId;
	Out << ServerTimeStamp;
	Out << ServerFileSize;
}


void FNetworkFileServerClientConnection::ProcessReadFile( FArchive& In, FArchive& Out )
{
	// Get Handle ID
	uint64 HandleId = 0;
	In << HandleId;

	int64 BytesToRead = 0;
	In << BytesToRead;

	int64 BytesRead = 0;
	IFileHandle* File = FindOpenFile(HandleId);

	if (File)
	{
		uint8* Dest = (uint8*)FMemory::Malloc(BytesToRead);		

		if (File->Read(Dest, BytesToRead))
		{
			BytesRead = BytesToRead;
			Out << BytesRead;
			Out.Serialize(Dest, BytesRead);
		}
		else
		{
			Out << BytesRead;
		}

		FMemory::Free(Dest);
	}
	else
	{
		Out << BytesRead;
	}
}


void FNetworkFileServerClientConnection::ProcessWriteFile( FArchive& In, FArchive& Out )
{
	// Get Handle ID
	uint64 HandleId = 0;
	In << HandleId;

	int64 BytesWritten = 0;
	IFileHandle* File = FindOpenFile(HandleId);

	if (File)
	{
		int64 BytesToWrite = 0;
		In << BytesToWrite;

		uint8* Source = (uint8*)FMemory::Malloc(BytesToWrite);
		In.Serialize(Source, BytesToWrite);

		if (File->Write(Source, BytesToWrite))
		{
			BytesWritten = BytesToWrite;
		}

		FMemory::Free(Source); 
	}
		
	Out << BytesWritten;
}


void FNetworkFileServerClientConnection::ProcessSeekFile( FArchive& In, FArchive& Out )
{
	// Get Handle ID
	uint64 HandleId = 0;
	In << HandleId;

	int64 NewPosition;
	In << NewPosition;

	int64 SetPosition = -1;
	IFileHandle* File = FindOpenFile(HandleId);

	if (File && File->Seek(NewPosition))
	{
		SetPosition = File->Tell();
	}

	Out << SetPosition;
}


void FNetworkFileServerClientConnection::ProcessCloseFile( FArchive& In, FArchive& Out )
{
	// Get Handle ID
	uint64 HandleId = 0;
	In << HandleId;

	uint32 Closed = 0;
	IFileHandle* File = FindOpenFile(HandleId);

	if (File)
	{
		Closed = 1;
		OpenFiles.Remove(HandleId);

		delete File;
	}
		
	Out << Closed;
}


void FNetworkFileServerClientConnection::ProcessGetFileInfo( FArchive& In, FArchive& Out )
{
	// Get filename
	FString Filename;
	In << Filename;

	ConvertClientFilenameToServerFilename(Filename);

	FFileInfo Info;
	Info.FileExists = Sandbox->FileExists(*Filename);

	// if the file exists, cook it if necessary (the FileExists flag won't change value based on this callback)
	// without this, the server can return the uncooked file size, which can cause reads off the end
	if (Info.FileExists)
	{
		TArray<FString> NewUnsolictedFiles;
		FileRequestDelegate.ExecuteIfBound(Filename, NewUnsolictedFiles);
	}

	// get the rest of the info
	Info.ReadOnly = Sandbox->IsReadOnly(*Filename);
	Info.Size = Sandbox->FileSize(*Filename);
	Info.TimeStamp = Sandbox->GetTimeStamp(*Filename);
	Info.AccessTimeStamp = Sandbox->GetAccessTimeStamp(*Filename);

	Out << Info.FileExists;
	Out << Info.ReadOnly;
	Out << Info.Size;
	Out << Info.TimeStamp;
	Out << Info.AccessTimeStamp;
}


void FNetworkFileServerClientConnection::ProcessMoveFile( FArchive& In, FArchive& Out )
{
	FString From;
	In << From;
	FString To;
	In << To;

	ConvertClientFilenameToServerFilename(From);
	ConvertClientFilenameToServerFilename(To);

	uint32 Success = Sandbox->MoveFile(*To, *From);
	Out << Success;
}


void FNetworkFileServerClientConnection::ProcessDeleteFile( FArchive& In, FArchive& Out )
{
	FString Filename;
	In << Filename;

	ConvertClientFilenameToServerFilename(Filename);

	uint32 Success = Sandbox->DeleteFile(*Filename);
	Out << Success;
}


void FNetworkFileServerClientConnection::ProcessReportLocalFiles( FArchive& In, FArchive& Out )
{
	// get the list of files on the other end
	TMap<FString, FDateTime> ClientFileTimes;
	In << ClientFileTimes;

	// go over them and compare times to this side
	TArray<FString> OutOfDateFiles;

	for (TMap<FString, FDateTime>::TIterator It(ClientFileTimes); It; ++It)
	{
		FString ClientFile = It.Key();
		ConvertClientFilenameToServerFilename(ClientFile);
		// get the local timestamp
		FDateTime Timestamp = Sandbox->GetTimeStamp(*ClientFile);

		// if it's newer than the client/remote timestamp, it's newer here, so tell the other side it's out of date
		if (Timestamp > It.Value())
		{
			OutOfDateFiles.Add(ClientFile);
		}
	}

	UE_LOG(LogFileServer, Display, TEXT("There were %d out of date files"), OutOfDateFiles.Num());
}


/** Copies file. */
void FNetworkFileServerClientConnection::ProcessCopyFile( FArchive& In, FArchive& Out )
{
	FString To;
	FString From;
	In << To;	
	In << From;

	ConvertClientFilenameToServerFilename(To);
	ConvertClientFilenameToServerFilename(From);

	bool Success = Sandbox->CopyFile(*To, *From);
	Out << Success;
}


void FNetworkFileServerClientConnection::ProcessSetTimeStamp( FArchive& In, FArchive& Out )
{
	FString Filename;
	FDateTime Timestamp;
	In << Filename;	
	In << Timestamp;

	ConvertClientFilenameToServerFilename(Filename);

	Sandbox->SetTimeStamp(*Filename, Timestamp);

	// Need to sends something back otherwise the response won't get sent at all.
	bool Success = true;
	Out << Success;
}


void FNetworkFileServerClientConnection::ProcessSetReadOnly( FArchive& In, FArchive& Out )
{
	FString Filename;
	bool bReadOnly;
	In << Filename;	
	In << bReadOnly;

	ConvertClientFilenameToServerFilename(Filename);

	bool Success = Sandbox->SetReadOnly(*Filename, bReadOnly);
	Out << Success;
}


void FNetworkFileServerClientConnection::ProcessCreateDirectory( FArchive& In, FArchive& Out ) 
{
	FString Directory;
	In << Directory;

	ConvertClientFilenameToServerFilename(Directory);

	bool bSuccess = Sandbox->CreateDirectory(*Directory);
	Out << bSuccess;
}


void FNetworkFileServerClientConnection::ProcessDeleteDirectory( FArchive& In, FArchive& Out )
{
	FString Directory;
	In << Directory;

	ConvertClientFilenameToServerFilename(Directory);

	bool bSuccess = Sandbox->DeleteDirectory(*Directory);
	Out << bSuccess;
}


void FNetworkFileServerClientConnection::ProcessDeleteDirectoryRecursively( FArchive& In, FArchive& Out )
{
	FString Directory;
	In << Directory;

	ConvertClientFilenameToServerFilename(Directory);

	bool bSuccess = Sandbox->DeleteDirectoryRecursively(*Directory);
	Out << bSuccess;
}


void FNetworkFileServerClientConnection::ProcessToAbsolutePathForRead( FArchive& In, FArchive& Out )
{
	FString Filename;
	In << Filename;

	ConvertClientFilenameToServerFilename(Filename);

	Filename = Sandbox->ConvertToAbsolutePathForExternalAppForRead(*Filename);
	Out << Filename;
}


void FNetworkFileServerClientConnection::ProcessToAbsolutePathForWrite( FArchive& In, FArchive& Out )
{
	FString Filename;
	In << Filename;

	ConvertClientFilenameToServerFilename(Filename);

	Filename = Sandbox->ConvertToAbsolutePathForExternalAppForWrite(*Filename);
	Out << Filename;
}


void FNetworkFileServerClientConnection::ProcessGetFileList( FArchive& In, FArchive& Out )
{
	// get the list of directories to process
	TArray<FString> TargetPlatformNames;
	FString GameName;
	FString EngineRelativePath;
	FString GameRelativePath;
	TArray<FString> RootDirectories;
	bool bIsStreamingRequest = false;

	In << TargetPlatformNames;
	In << GameName;
	In << EngineRelativePath;
	In << GameRelativePath;
	In << RootDirectories;
	In << bIsStreamingRequest;

	ConnectedPlatformName = TEXT("");
	// figure out the best matching target platform for the set of valid ones
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	static const TArray<ITargetPlatform*>& ActiveTargetPlatforms =  TPM.GetActiveTargetPlatforms();
	for (int32 TPIndex = 0; TPIndex < TargetPlatformNames.Num(); TPIndex++)
	{
		UE_LOG(LogFileServer, Display, TEXT("    Possible Target Platform from client: %s"), *TargetPlatformNames[TPIndex]);

		// look for a matching target platform
		for (int32 ActiveTPIndex = 0; ActiveTPIndex < ActiveTargetPlatforms.Num(); ActiveTPIndex++)
		{
			if (ActiveTargetPlatforms[ActiveTPIndex]->PlatformName() == TargetPlatformNames[TPIndex])
			{
				ConnectedPlatformName = ActiveTargetPlatforms[ActiveTPIndex]->PlatformName();
				break;
			}
		}
	}
	// @todo: if there were some platforms in ActiveTPs, but none matched, we should reject this client

	// if we didn't find one, then just use what was sent
	if (ConnectedPlatformName == TEXT(""))
	{
		ConnectedPlatformName = TargetPlatformNames[0];
	}
	ConnectedEngineDir = EngineRelativePath;
	ConnectedGameDir = GameRelativePath;


	FString LocalEngineDir = FPaths::EngineDir();
	FString LocalGameDir = FPaths::GameDir();
	if ( FPaths::IsProjectFilePathSet() )
	{
		LocalGameDir = FPaths::GetPath(FPaths::GetProjectFilePath()) + TEXT("/");
	}

	UE_LOG(LogFileServer, Display, TEXT("    Connected EngineDir = %s"), *ConnectedEngineDir);
	UE_LOG(LogFileServer, Display, TEXT("        Local EngineDir = %s"), *LocalEngineDir);
	UE_LOG(LogFileServer, Display, TEXT("    Connected GameDir   = %s"), *ConnectedGameDir);
	UE_LOG(LogFileServer, Display, TEXT("        Local GameDir   = %s"), *LocalGameDir);

	// Remap the root directories requested...
	for (int32 RootDirIdx = 0; RootDirIdx < RootDirectories.Num(); RootDirIdx++)
	{
		FString CheckRootDir = RootDirectories[RootDirIdx];
		ConvertClientFilenameToServerFilename(CheckRootDir);
		RootDirectories[RootDirIdx] = CheckRootDir;
	}

	// figure out the sandbox directory
	// @todo: This should use FPlatformMisc::SavedDirectory(GameName)
	FString SandboxDirectory;
	if ( FPaths::IsProjectFilePathSet() )
	{
		FString ProjectDir = FPaths::GetPath(FPaths::GetProjectFilePath());
		SandboxDirectory = FPaths::Combine(*ProjectDir, TEXT("Saved"), TEXT("Sandboxes"), *(FString("Cooked-") + ConnectedPlatformName));
		if( bIsStreamingRequest )
		{
			RootDirectories.Add(ProjectDir);
		}
	}
	else
	{
		if (FPaths::GetExtension(GameName) == IProjectManager::GetProjectFileExtension())
		{
			SandboxDirectory = FString::Printf(TEXT("%s/Saved/Sandboxes/Cooked-%s"), *FPaths::GetPath(GameName), *ConnectedPlatformName);
		}
		else
		{
			//@todo: This assumes the game is located in the UE4 Root directory
			FString SandboxFolder = FString::Printf(TEXT("Cooked-%s"), *ConnectedPlatformName);
			SandboxDirectory = FPaths::Combine(*FPaths::GetRelativePathToRoot(), *GameName, TEXT("Saved/Sandboxes"), *SandboxFolder);
		}
	}

	// delete any existing one first, in case game name somehow changed and client is re-asking for files (highly unlikely)
	delete Sandbox;
	Sandbox = new FSandboxPlatformFile(false);
	Sandbox->Initialize(&FPlatformFileManager::Get().GetPlatformFile(), *FString::Printf(TEXT("-sandbox=\"%s\""), *SandboxDirectory));

	// make sure the global shaders are up to date before letting the client read any shaders
	// @todo: This will probably add about 1/2 second to the boot-up time of the client while the server does this
	// @note: We assume the delegate will write to the proper sandbox directory, should we pass in SandboxDirectory, or Sandbox?
	FShaderRecompileData RecompileData;
	RecompileData.PlatformName = ConnectedPlatformName;
	// All target platforms
	RecompileData.ShaderPlatform = -1;
	RecompileData.ModifiedFiles = NULL;
	RecompileData.MeshMaterialMaps = NULL;
	RecompileShadersDelegate.ExecuteIfBound(RecompileData);

	UE_LOG(LogFileServer, Display, TEXT("Getting files for %d directories, game = %s, platform = %s"), RootDirectories.Num(), *GameName, *ConnectedPlatformName);
	UE_LOG(LogFileServer, Display, TEXT("    Sandbox dir = %s"), *SandboxDirectory);

	for (int32 DumpIdx = 0; DumpIdx < RootDirectories.Num(); DumpIdx++)
	{
		UE_LOG(LogFileServer, Display, TEXT("\t%s"), *(RootDirectories[DumpIdx]));
	}

	// list of directories to skip
	TArray<FString> DirectoriesToSkip;
	TArray<FString> DirectoriesToNotRecurse;
	// @todo: This should really be FPlatformMisc::GetSavedDirForGame(ClientGameName), etc
	for (int32 DirIndex = 0; DirIndex < RootDirectories.Num(); DirIndex++)
	{
		DirectoriesToSkip.Add(FString(RootDirectories[DirIndex] / TEXT("Saved/Backup")));
		DirectoriesToSkip.Add(FString(RootDirectories[DirIndex] / TEXT("Saved/Config")));
		DirectoriesToSkip.Add(FString(RootDirectories[DirIndex] / TEXT("Saved/Logs")));
		DirectoriesToSkip.Add(FString(RootDirectories[DirIndex] / TEXT("Saved/Sandboxes")));
		DirectoriesToSkip.Add(FString(RootDirectories[DirIndex] / TEXT("Intermediate")));
		DirectoriesToSkip.Add(FString(RootDirectories[DirIndex] / TEXT("Documentation")));
		DirectoriesToSkip.Add(FString(RootDirectories[DirIndex] / TEXT("Extras")));
		DirectoriesToSkip.Add(FString(RootDirectories[DirIndex] / TEXT("Binaries")));
		DirectoriesToSkip.Add(FString(RootDirectories[DirIndex] / TEXT("Source")));
		DirectoriesToNotRecurse.Add(FString(RootDirectories[DirIndex] / TEXT("DerivedDataCache")));
	}

	// use the timestamp grabbing visitor (include directories)
	FLocalTimestampDirectoryVisitor Visitor(*Sandbox, DirectoriesToSkip, DirectoriesToNotRecurse, true);
	for (int32 DirIndex = 0; DirIndex < RootDirectories.Num(); DirIndex++)
	{
		Sandbox->IterateDirectory(*RootDirectories[DirIndex], Visitor);
	}

	// report the package version information
	// The downside of this is that ALL cooked data will get tossed on package version changes
	Out << GPackageFileUE4Version;
	Out << GPackageFileLicenseeUE4Version;

	// Send *our* engine and game dirs
	Out << LocalEngineDir;
	Out << LocalGameDir;

	// return the files and their timestamps
	Out << Visitor.FileTimes;

	// Do it again, preventing access to non-cooked files
	if( bIsStreamingRequest == false )
	{
		// Do it again, preventing access to non-cooked files
		const int32 NUM_EXCLUSION_WILDCARDS = 2;
		FString ExclusionWildcard[NUM_EXCLUSION_WILDCARDS];
		ExclusionWildcard[0] = FString(TEXT("*")) + FPackageName::GetAssetPackageExtension(); 
		ExclusionWildcard[1] = FString(TEXT("*")) + FPackageName::GetMapPackageExtension();

		for (int32 i=0; i < NUM_EXCLUSION_WILDCARDS; ++i)
		{
			Sandbox->AddExclusion(*ExclusionWildcard[i]);
			UE_LOG(LogFileServer, Display, TEXT("Excluding %s from non-sandboxed directories"), 
				   *ExclusionWildcard[i]);
		}
	
		FLocalTimestampDirectoryVisitor VisitorForCacheDates(*Sandbox, DirectoriesToSkip, DirectoriesToNotRecurse, true);

		for (int32 DirIndex = 0; DirIndex < RootDirectories.Num(); DirIndex++)
		{
			Sandbox->IterateDirectory(*RootDirectories[DirIndex], VisitorForCacheDates);
		}
	
		// return the cached files and their timestamps
		Out << VisitorForCacheDates.FileTimes;
	}
}


void FNetworkFileServerClientConnection::ProcessHeartbeat( FArchive& In, FArchive& Out )
{
	// Protect the array
	FScopeLock Lock(&ModifiedFilesSection);

	// return the list of modified files
	Out << ModifiedFiles;

	// @todo: note the last received time, and toss clients that don't heartbeat enough!

	// @todo: Right now, there is no directory watcher adding to ModifiedFiles. It had to be pulled from this thread (well, the ModuleManager part)
	// We should have a single directory watcher that pushes the changes to all the connections - or possibly pass in a shared DirectoryWatcher
	// and have each connection set up a delegate (see p4 history for HandleDirectoryWatcherDirectoryChanged)
}


/* FStreamingNetworkFileServerConnection callbacks
 *****************************************************************************/

void FNetworkFileServerClientConnection::PackageFile( FString& Filename, FArchive& Out )
{
	// get file timestamp and send it to client
	FDateTime ServerTimeStamp = Sandbox->GetTimeStamp(*Filename);

	TArray<uint8> Contents;
	// open file
	IFileHandle* File = Sandbox->OpenRead(*Filename);

	if (!File)
	{
		ServerTimeStamp = FDateTime::MinValue(); // if this was a directory, this will make sure it is not confused with a zero byte file

		UE_LOG(LogFileServer, Warning, TEXT("Request for missing file %s."), *Filename);
	}
	else
	{
		if (!File->Size())
		{
			UE_LOG(LogFileServer, Error, TEXT("Sending empty file %s...."), *Filename);
		}
		else
		{
			// read it
			Contents.AddUninitialized(File->Size());
			File->Read(Contents.GetTypedData(), Contents.Num());
		}

		// close it
		delete File;

		UE_LOG(LogFileServer, Display, TEXT("Read %s, %d bytes"), *Filename, Contents.Num());
	}

	Out << Filename;
	Out << ServerTimeStamp;
	uint64 FileSize = Contents.Num();
	Out << FileSize;
	Out.Serialize(Contents.GetData(), FileSize);
}


void FNetworkFileServerClientConnection::ProcessRecompileShaders( FArchive& In, FArchive& Out )
{
	TArray<FString> RecompileModifiedFiles;
	TArray<uint8> MeshMaterialMaps;
	FShaderRecompileData RecompileData;
	RecompileData.PlatformName = ConnectedPlatformName;
	RecompileData.ModifiedFiles = &RecompileModifiedFiles;
	RecompileData.MeshMaterialMaps = &MeshMaterialMaps;

	// tell other side all the materials to load, by pathname
	In << RecompileData.MaterialsToLoad;
	In << RecompileData.ShaderPlatform;
	In << RecompileData.SerializedShaderResources;

	RecompileShadersDelegate.ExecuteIfBound(RecompileData);

	// tell other side what to do!
	Out << RecompileModifiedFiles;
	Out << MeshMaterialMaps;
}


void FNetworkFileServerClientConnection::ProcessSyncFile( FArchive& In, FArchive& Out )
{
	// get filename
	FString Filename;
	In << Filename;

	ConvertClientFilenameToServerFilename(Filename);

	//FString AbsFile(FString(*Sandbox->ConvertToAbsolutePathForExternalApp(*Filename)).MakeStandardFilename());
	// ^^ we probably in general want that filename, but for cook on the fly, we want the un-sandboxed name

	TArray<FString> NewUnsolictedFiles;

	FileRequestDelegate.ExecuteIfBound(Filename, NewUnsolictedFiles);

	for (int32 Index = 0; Index < NewUnsolictedFiles.Num(); Index++)
	{
		if (NewUnsolictedFiles[Index] != Filename)
		{
			UnsolictedFiles.AddUnique(NewUnsolictedFiles[Index]);
		}
	}

	PackageFile(Filename, Out);
}