// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "HTTPOnlineTitleFile.h"

#include "HTTPChunkInstallerLog.h"
#include "CoreMinimal.h"
#include "OnlineSubsystemTypes.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "HAL/FileManager.h"
#include "Async/AsyncWork.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/EngineVersion.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Serialization/Archive.h"

#if PLATFORM_ANDROID
#include "OpenGLDrv.h"
#endif

#define LOCTEXT_NAMESPACE "HTTPChunkInstaller"


FOnlineTitleFileHttp::FOnlineTitleFileHttp(const FString& InBaseUrl, const FString& InManifestData)
	: BaseUrl(InBaseUrl)
	, EnumerateFilesUrl(TEXT(""))
	, ErrorMsg()
	, Manifest(TEXT("Master.manifest"))
	, CompatibleClientVersion(0)
{
	GConfig->GetString(TEXT("HTTPOnlineTitleFile"), TEXT("ManifestName"), Manifest, GEngineIni);
	GConfig->GetString(TEXT("HTTPOnlineTitleFile"), TEXT("EnumerateFilesUrl"), EnumerateFilesUrl, GEngineIni);
	bCacheFiles = true;
	bPlatformSupportsSHA256 = false;
	ManifestData = InManifestData;

#if PLATFORM_ANDROID
	if (FOpenGL::SupportsASTC())
	{
		EnumerateFilesUrl = EnumerateFilesUrl.Replace(TEXT("^FORMAT"), TEXT("ASTC"));
		Manifest = Manifest.Replace(TEXT("^FORMAT"), TEXT("astc"));
	}
	else if (FOpenGL::SupportsATITC())
	{
		EnumerateFilesUrl = EnumerateFilesUrl.Replace(TEXT("^FORMAT"), TEXT("ATC"));
		Manifest = Manifest.Replace(TEXT("^FORMAT"), TEXT("atc"));
	}
	else if (FOpenGL::SupportsDXT())
	{
		EnumerateFilesUrl = EnumerateFilesUrl.Replace(TEXT("^FORMAT"), TEXT("DXT"));
		Manifest = Manifest.Replace(TEXT("^FORMAT"), TEXT("dxt"));
	}
	else if (FOpenGL::SupportsPVRTC())
	{
		EnumerateFilesUrl = EnumerateFilesUrl.Replace(TEXT("^FORMAT"), TEXT("PVRTC"));
		Manifest = Manifest.Replace(TEXT("^FORMAT"), TEXT("pvrtc"));
	}
	else if (FOpenGL::SupportsETC2())
	{
		EnumerateFilesUrl = EnumerateFilesUrl.Replace(TEXT("^FORMAT"), TEXT("ETC2"));
		Manifest = Manifest.Replace(TEXT("^FORMAT"), TEXT("etc2"));
	}
	else
	{
		EnumerateFilesUrl = EnumerateFilesUrl.Replace(TEXT("^FORMAT"), TEXT("ETC1"));
		Manifest = Manifest.Replace(TEXT("^FORMAT"), TEXT("etc1"));
	}
#endif
}

bool FOnlineTitleFileHttp::GetFileContents(const FString& FileName, TArray<uint8>& FileContents)
{
	const TArray<FCloudFile>* FilesPtr = &Files;
	if (FilesPtr != NULL)
	{
		for (TArray<FCloudFile>::TConstIterator It(*FilesPtr); It; ++It)
		{
			if (It->FileName == FileName)
			{
				FileContents = It->Data;
				return true;
			}
		}
	}
	return false;
}

bool FOnlineTitleFileHttp::ClearFiles()
{
	for (int Idx = 0; Idx < Files.Num(); Idx++)
	{
		if (Files[Idx].AsyncState == EOnlineAsyncTaskState::InProgress)
		{
			UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("Cant clear files. Pending file op for %s"), *Files[Idx].FileName);
			return false;
		}
	}
	// remove all cached file entries
	Files.Empty();
	return true;
}

bool FOnlineTitleFileHttp::ClearFile(const FString& FileName)
{
	for (int Idx = 0; Idx < Files.Num(); Idx++)
	{
		if (Files[Idx].FileName == FileName)
		{
			if (Files[Idx].AsyncState == EOnlineAsyncTaskState::InProgress)
			{
				UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("Cant clear file. Pending file op for %s"), *Files[Idx].FileName);
				return false;
			}
			else
			{
				Files.RemoveAt(Idx);
				return true;
			}
		}
	}
	return false;
}

void FOnlineTitleFileHttp::DeleteCachedFiles(bool bSkipEnumerated)
{
	TArray<FString> CachedFiles;
	IFileManager::Get().FindFiles(CachedFiles, *(GetLocalCachePath() / TEXT("*")), true, false);

	for (auto CachedFile : CachedFiles)
	{
		bool bSkip = bSkipEnumerated && GetCloudFileHeader(CachedFile);
		if (!bSkip)
		{
			IFileManager::Get().Delete(*GetLocalFilePath(CachedFile), false, true);
		}
	}
}

bool FOnlineTitleFileHttp::EnumerateFiles(const FPagedQuery& Page)
{
	static const FText ErrorGeneric = LOCTEXT("EnumerateFilesError_Generic", "Enumerate Files request failed.");

	// clear the compatible client version when we start an enumeration
	CompatibleClientVersion = 0;

	// Make sure an enumeration request  is not currently pending
	if (EnumerateFilesRequests.Num() > 0)
	{
		UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("EnumerateFiles request already in progress"));
		ErrorMsg = ErrorGeneric;
		TriggerOnEnumerateFilesCompleteDelegates(false);
		return false;
	}

	if (!ManifestData.IsEmpty())
	{
		// parse the manifest
		if (!ParseManifest(ManifestData))
		{
			UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("EnumerateFiles failed to parse manifest: %s"), *ErrorMsg.ToString());
			TriggerOnEnumerateFilesCompleteDelegates(false);
			return false;
		}

		// make sure the chunks in this manifest are compatible with our current client
		if (IsClientCompatible() != Compatible::ClientIsCompatible)
		{
			UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("EnumerateFiles failed due to update required. Current CL=%u, but manifest requires %u"), FEngineVersion::Current().GetChangelist(), CompatibleClientVersion);
			ErrorMsg = LOCTEXT("EnumerateFilesError_UpdateRequired", "This game requires an update in order to continue playing.");
			TriggerOnEnumerateFilesCompleteDelegates(false);
			return false;
		}

		// Everything went ok, so we can remove any cached files that are not in the current list
		ErrorMsg = FText::GetEmpty();
		DeleteCachedFiles(true);
		TriggerOnEnumerateFilesCompleteDelegates(true);
	}
	else
	{
		// Create the Http request and add to pending request list
		TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
		EnumerateFilesRequests.Add(HttpRequest, Page);

		FString EnumerateUrl = GetBaseUrl() + EnumerateFilesUrl + TEXT("/") + Manifest;
		HttpRequest->SetURL(EnumerateUrl);
		HttpRequest->SetVerb(TEXT("GET"));
		HttpRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineTitleFileHttp::EnumerateFiles_HttpRequestComplete);
		if (!HttpRequest->ProcessRequest())
		{
			UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("EnumerateFiles unable to queue HTTP request"));
			ErrorMsg = ErrorGeneric;
			TriggerOnEnumerateFilesCompleteDelegates(false);
			return false;
		}
	}

	return true;
}

void FOnlineTitleFileHttp::GetFileList(TArray<FCloudFileHeader>& OutFiles)
{
	TArray<FCloudFileHeader>* FilesPtr = &FileHeaders;
	if (FilesPtr != NULL)
	{
		OutFiles = *FilesPtr;
	}
	else
	{
		OutFiles.Empty();
	}
}

bool FOnlineTitleFileHttp::ReadFile(const FString& FileName)
{
	bool bStarted = false;

	FCloudFileHeader* CloudFileHeader = GetCloudFileHeader(FileName);

	// Make sure valid filename was specified3
	if (FileName.IsEmpty() || FileName.Contains(TEXT(" ")))
	{
		UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("Invalid filename filename=%s"), *FileName);
		ErrorMsg = FText::Format(LOCTEXT("ReadFile_FileNotFound", "File not found: {0}"), FText::FromString(FileName));
		TriggerOnReadFileCompleteDelegates(false, FileName);
		return false;
	}

	// Make sure a file request for this file is not currently pending
	for (const auto& Pair : FileRequests)
	{
		if (Pair.Value == FPendingFileRequest(FileName))
		{
			UE_LOG(LogHTTPChunkInstaller, Verbose, TEXT("ReadFileRemote is already in progress for (%s)"), *FileName);
			return true;
		}
	}

	FCloudFile* CloudFile = GetCloudFile(FileName, true);
	if (CloudFile->AsyncState == EOnlineAsyncTaskState::InProgress)
	{
		UE_LOG(LogHTTPChunkInstaller, Verbose, TEXT("ReadFile is already in progress for (%s)"), *FileName);
		return true;
	}

	if (bCacheFiles)
	{
		// Try to read this from the cache if possible
		bStarted = StartReadFileLocal(FileName);
	}
	if (!bStarted)
	{
		// Failed locally (means not on disk) so fetch from server
		bStarted = ReadFileRemote(FileName);
	}

	if (!bStarted || CloudFile->AsyncState == EOnlineAsyncTaskState::Failed)
	{
		UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("ReadFile request failed for file (%s)"), *FileName);
		ErrorMsg = FText::Format(LOCTEXT("ReadFile_RequestFailed", "Read request failed for file ({0})"), FText::FromString(FileName));
		TriggerOnReadFileCompleteDelegates(false, FileName);
	}
	else if (CloudFile->AsyncState == EOnlineAsyncTaskState::Done)
	{
		ErrorMsg = FText::GetEmpty();
		TriggerOnReadFileCompleteDelegates(true, FileName);
	}
	return bStarted;
}

void FOnlineTitleFileHttp::Tick(float DeltaTime)
{
	TArray<int32> ItemsToRemove;
	ItemsToRemove.Reserve(AsyncLocalReads.Num());

	// Check for any completed tasks
	for (int32 TaskIdx = 0; TaskIdx < AsyncLocalReads.Num(); TaskIdx++)
	{
		FTitleAsyncReadData& Task = AsyncLocalReads[TaskIdx];
		if (Task.AsyncTask->IsDone())
		{
			FinishReadFileLocal(Task.AsyncTask->GetTask());
			ItemsToRemove.Add(TaskIdx);
			UE_LOG(LogHTTPChunkInstaller, VeryVerbose, TEXT("Title Task Complete: %s"), *Task.Filename);
		}
		else
		{
			const int64 NewValue = Task.BytesRead.GetValue();
			if (NewValue != Task.LastBytesRead)
			{
				Task.LastBytesRead = NewValue;
				TriggerOnReadFileProgressDelegates(Task.Filename, NewValue);
			}
		}
	}

	// Clean up any tasks that were completed
	for (int32 ItemIdx = ItemsToRemove.Num() - 1; ItemIdx >= 0; ItemIdx--)
	{
		const int32 TaskIdx = ItemsToRemove[ItemIdx];
		FTitleAsyncReadData& TaskToDelete = AsyncLocalReads[TaskIdx];
		UE_LOG(LogHTTPChunkInstaller, VeryVerbose, TEXT("Title Task Removal: %s read: %d"), *TaskToDelete.Filename, TaskToDelete.BytesRead.GetValue());
		delete TaskToDelete.AsyncTask;
		AsyncLocalReads.RemoveAtSwap(TaskIdx);
	}
}

bool FOnlineTitleFileHttp::StartReadFileLocal(const FString& FileName)
{
	UE_LOG(LogHTTPChunkInstaller, VeryVerbose, TEXT("StartReadFile %s"), *FileName);
	bool bStarted = false;
	FCloudFileHeader* CloudFileHeader = GetCloudFileHeader(FileName);
	if (CloudFileHeader != nullptr)
	{
		// Mark file entry as in progress
		FCloudFile* CloudFile = GetCloudFile(FileName, true);
		CloudFile->AsyncState = EOnlineAsyncTaskState::InProgress;
		if (CloudFileHeader->Hash.IsEmpty())
		{
			UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("Requested file (%s) is missing a hash, so can't be verified"), *FileName);
		}
		FTitleAsyncReadData* NewItem = new FTitleAsyncReadData();
		NewItem->Filename = FileName;

		// Create the async task and start it
		NewItem->AsyncTask = new FAsyncTask<FTitleFileHttpAsyncLoadAndVerify>(FileName,
			GetLocalFilePath(FileName), CloudFileHeader->Hash, CloudFileHeader->HashType, &NewItem->BytesRead);

		AsyncLocalReads.Add(NewItem);
		NewItem->AsyncTask->StartBackgroundTask();
		bStarted = true;
	}
	return bStarted;
}

void FOnlineTitleFileHttp::FinishReadFileLocal(FTitleFileHttpAsyncLoadAndVerify& AsyncLoad)
{
	UE_LOG(LogHTTPChunkInstaller, VeryVerbose, TEXT("FinishReadFileLocal %s"), *AsyncLoad.OriginalFileName);
	FCloudFileHeader* CloudFileHeader = GetCloudFileHeader(AsyncLoad.OriginalFileName);
	FCloudFile* CloudFile = GetCloudFile(AsyncLoad.OriginalFileName, true);
	if (CloudFileHeader != nullptr && CloudFile != nullptr)
	{
		// if hash matches then just use the local file
		if (AsyncLoad.bHashesMatched)
		{
			UE_LOG(LogHTTPChunkInstaller, Verbose, TEXT("Local file hash matches cloud header. No need to download for filename=%s"), *AsyncLoad.OriginalFileName);
			CloudFile->Data = AsyncLoad.FileData;
			CloudFile->AsyncState = EOnlineAsyncTaskState::Done;
			ErrorMsg = FText::GetEmpty();
			TriggerOnReadFileProgressDelegates(AsyncLoad.OriginalFileName, static_cast<uint64>(AsyncLoad.BytesRead->GetValue()));
			TriggerOnReadFileCompleteDelegates(true, AsyncLoad.OriginalFileName);
		}
		else
		{
			// Request it from server
			ReadFileRemote(AsyncLoad.OriginalFileName);
		}
	}
	else
	{
		UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("ReadFile request failed for file (%s)"), *AsyncLoad.OriginalFileName);
		ErrorMsg = FText::Format(LOCTEXT("ReadFile_RequestFailed", "Read request failed for file ({0})"), FText::FromString(AsyncLoad.OriginalFileName));
		TriggerOnReadFileCompleteDelegates(false, AsyncLoad.OriginalFileName);
	}
}

bool FOnlineTitleFileHttp::ReadFileRemote(const FString& FileName)
{
	UE_LOG(LogHTTPChunkInstaller, VeryVerbose, TEXT("ReadFileRemote %s"), *FileName);

	bool bStarted = false;
	FCloudFileHeader* CloudFileHeader = GetCloudFileHeader(FileName);
	if (CloudFileHeader != nullptr)
	{
		FCloudFile* CloudFile = GetCloudFile(FileName, true);
		CloudFile->Data.Empty();
		CloudFile->AsyncState = EOnlineAsyncTaskState::InProgress;

		// Create the Http request and add to pending request list
		TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
		FileRequests.Add(HttpRequest, FPendingFileRequest(FileName));
		FileProgressRequestsMap.Add(HttpRequest, FPendingFileRequest(FileName));

		HttpRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineTitleFileHttp::ReadFile_HttpRequestComplete);
		HttpRequest->OnRequestProgress().BindRaw(this, &FOnlineTitleFileHttp::ReadFile_HttpRequestProgress);
		FString RequestUrl;
		// Grab the file from the specified URL if that was set, otherwise use the old method that hits the game service
		if (!CloudFileHeader->URL.IsEmpty())
		{
			RequestUrl = CloudFileHeader->URL;
		}
		else
		{
			RequestUrl = GetBaseUrl() + FileName;
		}
		HttpRequest->SetURL(RequestUrl);
		HttpRequest->SetVerb(TEXT("GET"));
		bStarted = HttpRequest->ProcessRequest();

		if (!bStarted)
		{
			UE_LOG(LogHTTPChunkInstaller, Error, TEXT("Unable to start the HTTP request to fetch file (%s)"), *FileName);
		}
	}
	else
	{
		UE_LOG(LogHTTPChunkInstaller, Error, TEXT("No cloud file header entry for filename=%s."), *FileName);
	}
	return bStarted;
}

void FOnlineTitleFileHttp::EnumerateFiles_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	const FPagedQuery& PendingOp = EnumerateFilesRequests.FindRef(HttpRequest);
	EnumerateFilesRequests.Remove(HttpRequest);

	bool bResult = false;
	FString ResponseStr;

	if (HttpResponse.IsValid() && (EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode()) || HttpResponse->GetResponseCode() == EHttpResponseCodes::NotModified))
	{
		ResponseStr = HttpResponse->GetContentAsString();
		UE_LOG(LogHTTPChunkInstaller, Verbose, TEXT("EnumerateFiles request complete. url=%s code=%d response=%s"),
			*HttpRequest->GetURL(), HttpResponse->GetResponseCode(), *ResponseStr);

		if (PendingOp.Start == 0)
		{
			FileHeaders.Empty();
		}

		// parse the html for the file list
		if (ResponseStr.StartsWith(TEXT("<!DOCTYPE")))
		{
			TArray<FString> Lines;
			ResponseStr.ParseIntoArrayLines(Lines);
			for (int Index = 0; Index < Lines.Num(); ++Index)
			{
				if (Lines[Index].StartsWith(TEXT("<li>")))
				{
					TArray<FString> Elements;
					Lines[Index].ParseIntoArray(Elements, TEXT(">"));
					if (!Elements[2].StartsWith(TEXT("Chunks")))
					{
						FString File = Elements[2].Replace(TEXT("</a"), TEXT(""));
						FCloudFileHeader FileHeader;
						FileHeader.DLName = File;
						FileHeader.FileName = File;
						FileHeader.URL = GetBaseUrl() + File;
						FileHeader.Hash.Empty();
						FileHeader.FileSize = 0;
						FileHeaders.Add(FileHeader);
					}
				}
			}
			bResult = true;
		}
		else
		{
			ErrorMsg = FText::GetEmpty();
			bResult = ParseManifest(ResponseStr);
		}
	}
	else
	{
		if (HttpResponse.IsValid())
		{
			switch (HttpResponse->GetResponseCode())
			{
			case 403:
			case 404:
				ErrorMsg = LOCTEXT("MissingFile", "File not found.");
				break;

			default:
				ErrorMsg = FText::Format(LOCTEXT("HttpResponse", "HTTP {0} response"),
					FText::AsNumber(HttpResponse->GetResponseCode()));
				break;
			}
		}
		else
		{
			ErrorMsg = LOCTEXT("ConnectionFailure", "Connection to the server failed.");
		}
	}

	if (!ErrorMsg.IsEmpty())
	{
		UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("EnumerateFiles request failed. %s"), *ErrorMsg.ToString());
	}
	else
	{
		// Everything went ok, so we can remove any cached files that are not in the current list
		DeleteCachedFiles(true);
	}

	TriggerOnEnumerateFilesCompleteDelegates(bResult);
}

FString FOnlineTitleFileHttp::GetBaseUrl() const
{
	return BaseUrl + TEXT("/") + BuildUrl + TEXT("/");
}

FOnlineTitleFileHttp::Compatible FOnlineTitleFileHttp::IsClientCompatible() const
{
	if (CompatibleClientVersion == 0)
	{
		// no manifest downloaded yet?
		return Compatible::UnknownCompatibility;
	}
	uint32 BuildCl = FEngineVersion::Current().GetChangelist();
	if (BuildCl == 0)
	{
		// programmer build?
		return Compatible::ClientIsCompatible;
	}

	if (CompatibleClientVersion == BuildCl)
	{
		return Compatible::ClientIsCompatible;
	}
	return BuildCl < CompatibleClientVersion ? Compatible::ClientUpdateRequired : Compatible::ClientIsNewerThanMcp;
}

bool FOnlineTitleFileHttp::ParseManifest(FString& ResponseStr)
{
	// Create the Json parser
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(ResponseStr);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
	{
		ErrorMsg = LOCTEXT("ParseManifestError_Json", "Unable to parse manifest JSON.");
		return false;
	}

	// get the data location, just the same as the build version
	BuildUrl = JsonObject->GetStringField(TEXT("BuildUrl"));
	TSharedPtr<FJsonValue> ClientVersionValue = JsonObject->GetField<EJson::None>(TEXT("ClientVersion"));
	if (ClientVersionValue->Type == EJson::Number)
	{
		// load from JSON directly
		CompatibleClientVersion = (uint32)ClientVersionValue->AsNumber();
	}
	else
	{
		// parse the old CL_%u format
		FString StringValue = ClientVersionValue->AsString();
		CompatibleClientVersion = FCString::Atoi(*StringValue.RightChop(3));
	}

	// Parse the array of file headers
	TArray<TSharedPtr<FJsonValue> > JsonFileHeaders = JsonObject->GetArrayField(TEXT("files"));
	for (TArray<TSharedPtr<FJsonValue> >::TConstIterator It(JsonFileHeaders); It; ++It)
	{
		TSharedPtr<FJsonObject> JsonFileHeader = (*It)->AsObject();
		if (JsonFileHeader.IsValid())
		{
			FCloudFileHeader FileHeader;
			if (JsonFileHeader->HasField(TEXT("hash")))
			{
				FileHeader.Hash = JsonFileHeader->GetStringField(TEXT("hash"));
				FileHeader.HashType = FileHeader.Hash.IsEmpty() ? NAME_None : NAME_SHA1;
			}
			// This one takes priority over the old SHA1 hash if present (requires platform support)
			if (bPlatformSupportsSHA256 && JsonFileHeader->HasField(TEXT("hash256")))
			{
				FString Hash256 = JsonFileHeader->GetStringField(TEXT("hash256"));
				if (!Hash256.IsEmpty())
				{
					FileHeader.Hash = Hash256;
					FileHeader.HashType = FileHeader.Hash.IsEmpty() ? NAME_None : NAME_SHA256;
				}
			}
			if (JsonFileHeader->HasField(TEXT("uniqueFilename")))
			{
				FileHeader.DLName = JsonFileHeader->GetStringField(TEXT("uniqueFilename"));
			}
			if (JsonFileHeader->HasField(TEXT("filename")))
			{
				FileHeader.FileName = JsonFileHeader->GetStringField(TEXT("filename"));
			}
			if (JsonFileHeader->HasField(TEXT("length")))
			{
				FileHeader.FileSize = FMath::TruncToInt(JsonFileHeader->GetNumberField(TEXT("length")));
			}
			if (JsonFileHeader->HasField(TEXT("URL")))
			{
				FileHeader.URL = GetBaseUrl() + JsonFileHeader->GetStringField(TEXT("URL"));
			}

			if (FileHeader.FileName.IsEmpty())
			{
				FileHeader.FileName = FileHeader.DLName;
			}

			// for now parse out the chunk ID
			// TODO: move this in to the manifest itself
			int startIndex = FileHeader.FileName.Find(TEXT("pakchunk")) + 8;
			int endIndex = FileHeader.FileName.Find(TEXT("CL_"));
			FString SubStr = FileHeader.FileName.Mid(startIndex, (endIndex - startIndex));
			FileHeader.ChunkID = FCString::Atoi(*SubStr);

			if (FileHeader.Hash.IsEmpty() ||
				(FileHeader.DLName.IsEmpty() && FileHeader.URL.IsEmpty()) ||
				FileHeader.HashType == NAME_None)
			{
				UE_LOG(LogHTTPChunkInstaller, Error, TEXT("Invalid file entry hash=%s hashType=%s dlname=%s filename=%s URL=%s"),
					*FileHeader.Hash,
					*FileHeader.HashType.ToString(),
					*FileHeader.DLName,
					*FileHeader.FileName,
					*FileHeader.URL);
				ErrorMsg = LOCTEXT("ParseManifestError_BadFileEntry", "Bad manifest (invalid file entry).");
				return false;
			}

			int32 FoundIdx = INDEX_NONE;
			for (int32 Idx = 0; Idx < FileHeaders.Num(); Idx++)
			{
				const FCloudFileHeader& ExistingFile = FileHeaders[Idx];
				if (ExistingFile.DLName == FileHeader.DLName)
				{
					FoundIdx = Idx;
					break;
				}
			}
			if (FoundIdx != INDEX_NONE)
			{
				FileHeaders[FoundIdx] = FileHeader;
			}
			else
			{
				FileHeaders.Add(FileHeader);
			}
		}
	}

	return true;
}

void FOnlineTitleFileHttp::ReadFile_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	bool bResult = false;
	FString ResponseStr;

	// should have a pending Http request
	FPendingFileRequest PendingRequest = FileRequests.FindChecked(HttpRequest);
	FileRequests.Remove(HttpRequest);
	// remove from progress updates
	FileProgressRequestsMap.Remove(HttpRequest);
	HttpRequest->OnRequestProgress().Unbind();

	// Cloud file being operated on
	FCloudFile* CloudFile = GetCloudFile(PendingRequest.FileName, true);
	CloudFile->AsyncState = EOnlineAsyncTaskState::Failed;
	CloudFile->Data.Empty();

	if (HttpResponse.IsValid() && EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode()))
	{
		UE_LOG(LogHTTPChunkInstaller, Verbose, TEXT("ReadFile request complete. url=%s code=%d"),
			*HttpRequest->GetURL(), HttpResponse->GetResponseCode());

		// update the memory copy of the file with data that was just downloaded
		CloudFile->AsyncState = EOnlineAsyncTaskState::Done;
		CloudFile->Data = HttpResponse->GetContent();

		if (bCacheFiles)
		{
			// cache to disk on successful download
			SaveCloudFileToDisk(CloudFile->FileName, CloudFile->Data);
		}

		bResult = true;
		ErrorMsg = FText::GetEmpty();
	}
	else
	{
		if (HttpResponse.IsValid())
		{
			ErrorMsg = FText::Format(LOCTEXT("HttpResponseFrom", "HTTP {0} response from {1}"),
				FText::AsNumber(HttpResponse->GetResponseCode()),
				FText::FromString(HttpResponse->GetURL()));
		}
		else
		{
			ErrorMsg = FText::Format(LOCTEXT("HttpResponseTo", "Connection to {0} failed"), FText::FromString(HttpRequest->GetURL()));
		}
	}

	if (!ErrorMsg.IsEmpty())
	{
		UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("EnumerateFiles request failed. %s"), *ErrorMsg.ToString());
	}
	TriggerOnReadFileCompleteDelegates(bResult, PendingRequest.FileName);
}

void FOnlineTitleFileHttp::ReadFile_HttpRequestProgress(FHttpRequestPtr HttpRequest, int32 BytesSent, int32 BytesReceived)
{
	FPendingFileRequest PendingRequest = FileProgressRequestsMap.FindChecked(HttpRequest);
	// Just forward this to anyone that is listening
	TriggerOnReadFileProgressDelegates(PendingRequest.FileName, BytesReceived);
}

FCloudFile* FOnlineTitleFileHttp::GetCloudFile(const FString& FileName, bool bCreateIfMissing)
{
	FCloudFile* CloudFile = NULL;
	for (int Idx = 0; Idx < Files.Num(); Idx++)
	{
		if (Files[Idx].FileName == FileName)
		{
			CloudFile = &Files[Idx];
			break;
		}
	}
	if (CloudFile == NULL && bCreateIfMissing)
	{
		CloudFile = new(Files)FCloudFile(FileName);
	}
	return CloudFile;
}

FCloudFileHeader* FOnlineTitleFileHttp::GetCloudFileHeader(const FString& FileName)
{
	FCloudFileHeader* CloudFileHeader = NULL;

	for (int Idx = 0; Idx < FileHeaders.Num(); Idx++)
	{
		if (FileHeaders[Idx].DLName == FileName)
		{
			CloudFileHeader = &FileHeaders[Idx];
			break;
		}
	}

	return CloudFileHeader;
}

void FOnlineTitleFileHttp::SaveCloudFileToDisk(const FString& Filename, const TArray<uint8>& Data)
{
	// save local disk copy as well
	FString LocalFilePath = GetLocalFilePath(Filename);
	bool bSavedLocal = FFileHelper::SaveArrayToFile(Data, *LocalFilePath);
	if (bSavedLocal)
	{
		UE_LOG(LogHTTPChunkInstaller, Verbose, TEXT("WriteUserFile request complete. Local file cache updated =%s"),
			*LocalFilePath);
	}
	else
	{
		UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("WriteUserFile request complete. Local file cache failed to update =%s"),
			*LocalFilePath);
	}
}

void FTitleFileHttpAsyncLoadAndVerify::DoWork()
{
	// load file from disk
	bool bLoadedFile = false;

	FArchive* Reader = IFileManager::Get().CreateFileReader(*FileName, FILEREAD_Silent);
	if (Reader)
	{
		int64 SizeToRead = Reader->TotalSize();
		FileData.Reset(SizeToRead);
		FileData.AddUninitialized(SizeToRead);

		uint8* FileDataPtr = FileData.GetData();

		static const int64 ChunkSize = 100 * 1024;

		int64 TotalBytesRead = 0;
		while (SizeToRead > 0)
		{
			int64 Val = FMath::Min(SizeToRead, ChunkSize);
			Reader->Serialize(FileDataPtr + TotalBytesRead, Val);
			BytesRead->Add(Val);
			TotalBytesRead += Val;
			SizeToRead -= Val;
		}

		ensure(SizeToRead == 0 && Reader->TotalSize() == TotalBytesRead);
		bLoadedFile = Reader->Close();
		delete Reader;
	}

	// verify hash of file if it exists
	if (bLoadedFile)
	{
		UE_LOG(LogHTTPChunkInstaller, Verbose, TEXT("ReadFile request. Local file read from cache =%s"), *FileName);
		if (HashType == NAME_SHA1)
		{
			bHashesMatched = IsValidSHA1(ExpectedHash, FileData);
		}
		else if (HashType == NAME_SHA256)
		{
			bHashesMatched = IsValidSHA256(ExpectedHash, FileData);
		}
	}
	else
	{
		UE_LOG(LogHTTPChunkInstaller, Verbose, TEXT("Local file (%s) not cached locally"), *FileName);
	}
	if (!bHashesMatched)
	{
		// Empty local that was loaded
		FileData.Empty();
	}
}

#undef LOCTEXT_NAMESPACE
