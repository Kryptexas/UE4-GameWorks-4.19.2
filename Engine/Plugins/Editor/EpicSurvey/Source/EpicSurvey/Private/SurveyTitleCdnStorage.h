// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "OnlineTitleFileInterface.h"

#include "IHttpBase.h"
#include "IHttpRequest.h"
#include "IHttpResponse.h"

class FSurveyTitleCdnStorage : public IOnlineTitleFile
{
public:
	static IOnlineTitleFilePtr Create( const FString& IndexUrl );

public:

	virtual bool GetFileContents(const FString& DLName, TArray<uint8>& FileContents) OVERRIDE;

	virtual bool ClearFiles() OVERRIDE;

	virtual bool ClearFile(const FString& DLName) OVERRIDE;

	virtual bool EnumerateFiles() OVERRIDE;

	virtual void GetFileList(TArray<FCloudFileHeader>& InFileHeaders) OVERRIDE;

	virtual bool ReadFile(const FString& DLName) OVERRIDE;

private:
	
	/**
	 * Delegate called when a Http request completes for enumerating list of file headers
	 */
	void EnumerateFiles_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	/**
	 * Delegate called when a Http request completes for reading a cloud file
	 */
	void ReadFile_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	/**
	 * Find/create cloud file entry 
	 *
	 * @param FileName cached file entry to find
	 * @param bCreateIfMissing create the file entry if not found
	 *
	 * @return cached cloud file entry
	 */
	FCloudFile* GetCloudFile(const FString& FileName, bool bCreateIfMissing);

	/**
	 * Find cloud file header entry 
	 *
	 * @param FileName cached file entry to find
	 *
	 * @return cached cloud file header entry
	 */
	FCloudFileHeader* GetCloudFileHeader( const FString& FileName);

	/**
	 * Converts filename into a local file cache path
	 *
	 * @param FileName name of file being loaded
	 *
	 * @return unreal file path to be used by file manager
	 */
	FString GetLocalFilePath(const FString& FileName);

	/**
	 * Save a file from a given user to disk
	 *
	 * @param FileName name of file being saved
	 * @param Data data to write to disk
	 */
	void SaveCloudFileToDisk(const FString& Filename, const TArray<uint8>& Data);

	/**
	 * Should use the initialization constructor instead
	 */
	FSurveyTitleCdnStorage( const FString& InIndexUrl );

	/** Config based url for enumerating list of cloud files*/
	FString EnumerateFilesUrl;
	/** Config based url for downloading/updating a single cloud file entry */
	FString FileUrl;

	/** List of pending Http requests for enumerating files */
	TQueue<IHttpRequest*> EnumerateFilesRequests;
	
	/** Info used to send request for a file */
	struct FPendingFileRequest
	{
		/**
		 * Constructor
		 */
		FPendingFileRequest(const FString& InFileName=FString(TEXT("")))
			:  FileName(InFileName)
		{

		}

		/**
		 * Equality op
		 */
		inline bool operator==(const FPendingFileRequest& Other) const
		{
			return FileName == Other.FileName;
		}

		/** File being operated on by the pending request */
		FString FileName;
	};

	/** List of pending Http requests for reading files */
	TMap<class IHttpRequest*, FPendingFileRequest> FileRequests;

	TArray<FCloudFileHeader> FileHeaders;
	TArray<FCloudFile> Files;

	FString IndexUrl;
};