// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
// Precompiled header include. Can't add anything above this line.

#include "MapPakDownloaderModulePrivatePCH.h"
#include "MapPakDownloader.h"
#include "emscripten.h"
#include "Misc/Guid.h"

DECLARE_DELEGATE_OneParam(FDelegateFString, FString)
DECLARE_DELEGATE_OneParam(FDelegateInt32, int32)

// avoid IHttpModule
class FEmscriptenHttpFileRequest
{
public:
	typedef int32 Handle;
private:

	FString FileName;
	FString Url;

	FDelegateFString OnLoadCallBack;
	FDelegateInt32 OnErrorCallBack;
	FDelegateInt32 OnProgressCallBack;

public:

	FEmscriptenHttpFileRequest()
	{}
	
	static void OnLoad(uint32 Var, void* Arg, const ANSICHAR* FileName)
	{
		FEmscriptenHttpFileRequest* Request = reinterpret_cast<FEmscriptenHttpFileRequest*>(Arg);
		Request->OnLoadCallBack.ExecuteIfBound(FString(ANSI_TO_TCHAR(FileName)));
	}
	
	static void OnError(uint32 Var,void* Arg, int32 ErrorCode)
	{
		FEmscriptenHttpFileRequest* Request = reinterpret_cast<FEmscriptenHttpFileRequest*>(Arg);
		Request->OnErrorCallBack.ExecuteIfBound(ErrorCode);
	}

	static void OnProgress(uint32 Var,void* Arg, int32 Progress)
	{
		FEmscriptenHttpFileRequest* Request = reinterpret_cast<FEmscriptenHttpFileRequest*>(Arg);
		Request->OnProgressCallBack.ExecuteIfBound(Progress);
	}

	void Process()
	{
		UE_LOG(LogMapPakDownloader, Warning, TEXT("Starting Download for %s"), *FileName);

		emscripten_async_wget2(
			TCHAR_TO_ANSI(*(Url + FString(TEXT("?rand=")) + FGuid::NewGuid().ToString())),
			TCHAR_TO_ANSI(*FileName),
			TCHAR_TO_ANSI(TEXT("GET")),
			TCHAR_TO_ANSI(TEXT("")),
			this,
			&FEmscriptenHttpFileRequest::OnLoad,
			&FEmscriptenHttpFileRequest::OnError,
			&FEmscriptenHttpFileRequest::OnProgress
			);
	}

	void SetFileName(const FString& InFileName) { FileName = InFileName; }
	FString GetFileName() { return FileName;  }
	
	void SetUrl(const FString& InUrl) { Url = InUrl; }
	void SetOnLoadCallBack(const FDelegateFString& CallBack) { OnLoadCallBack = CallBack; }
	void SetOnErrorCallBack(const FDelegateInt32& CallBack) { OnErrorCallBack = CallBack; }
	void SetOnProgressCallBack(const FDelegateInt32& CallBack) { OnProgressCallBack = CallBack; }

};

FMapPakDownloader::FMapPakDownloader()
{}

bool FMapPakDownloader::Init()
{
	//  figure out where we are hosted 
	ANSICHAR *LocationString = (ANSICHAR*)EM_ASM_INT_V({

		var hoststring = "http://" + location.host; 
		var buffer = Module._malloc(hoststring.length);
		Module.writeAsciiToMemory(hoststring, buffer);
		return buffer;

	});

	HostName = FString(ANSI_TO_TCHAR(LocationString));

	PakLocation = FString(FApp::GetGameName()) / FString(TEXT("Content")) / FString(TEXT("Paks")); 

	// Create directory. 
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectory(*PakLocation);

	return true;
}

void FMapPakDownloader::CachePak()
{
	UE_LOG(LogMapPakDownloader, Warning, TEXT("Caching Dependencies for %s"), *MapToCache);

	FString PakName = FPackageName::GetShortName(FName(*MapToCache)) + ".pak";
	FString DeltaPakName = FPackageName::GetShortName(FName(*LastMap)) + TEXT("_") + FPackageName::GetShortName(FName(*MapToCache)) + ".pak";

	FEmscriptenHttpFileRequest* PakRequest = new FEmscriptenHttpFileRequest; // can't use shared ptrs. 


	FDelegateFString OnFileDownloaded = FDelegateFString::CreateLambda([=](FString Name){

											UE_LOG(LogMapPakDownloader, Warning, TEXT("%s download complete!"), *PakRequest->GetFileName());
											UE_LOG(LogMapPakDownloader, Warning, TEXT("Mounting..."), *Name);
											FCoreDelegates::OnMountPak.Execute(Name, 0);
											UE_LOG(LogMapPakDownloader, Warning, TEXT("%s Mounted!"), *Name);

											// Get hold of the world.
											TObjectIterator<UWorld> It;
											UWorld* ItWorld = *It;

											UE_LOG(LogMapPakDownloader, Warning, TEXT("Travel to %s"), *MapToCache);
											// Make engine Travel to the cached Map. 
											GEngine->SetClientTravel(ItWorld, *MapToCache, TRAVEL_Absolute);

											// delete the HTTP request. 
											delete PakRequest; 
										});

	FDelegateInt32 OnFileDownloadProgress = FDelegateInt32::CreateLambda([=](int32 Progress){
												UE_LOG(LogMapPakDownloader, Warning, TEXT(" %s %d%% downloaded"), *PakRequest->GetFileName(), Progress);
											});

	PakRequest->SetFileName(PakLocation / DeltaPakName);
	PakRequest->SetUrl(HostName / PakLocation / DeltaPakName);
	PakRequest->SetOnLoadCallBack(OnFileDownloaded);
	PakRequest->SetOnProgressCallBack(OnFileDownloadProgress);
	PakRequest->SetOnErrorCallBack(FDelegateInt32::CreateLambda([=](int32){

											UE_LOG(LogMapPakDownloader, Warning, TEXT("Could not download %s"), *PakRequest->GetFileName());
											// lets try again with regular map pak
											PakRequest->SetFileName(PakLocation/PakName);
											PakRequest->SetUrl(HostName/PakLocation/PakName);
											PakRequest->SetOnErrorCallBack( 
												FDelegateInt32::CreateLambda([=](int32){
												// we can't even find regular maps. fatal. 
												UE_LOG(LogMapPakDownloader, Fatal, TEXT("Could not find any Map Paks, exiting"), *PakRequest->GetFileName());
												}
											));
											PakRequest->Process();

									}));

	PakRequest->Process();
}

void FMapPakDownloader::Cache(FString& Map, FString& InLastMap, void* InDynData)
{
	MapToCache = Map;
	LastMap = InLastMap;

	if (!FPackageName::DoesPackageExist(Map))
	{
		CachePak();
		Map = TEXT("/Engine/Maps/Loading");
	}
}
