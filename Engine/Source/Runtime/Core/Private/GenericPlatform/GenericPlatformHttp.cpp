// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "CorePrivate.h"
#include "Runtime/Online/HTTP/Public/Interfaces/IHttpBase.h"
#include "Runtime/Online/HTTP/Public/Interfaces/IHttpRequest.h"
#include "Runtime/Online/HTTP/Public/Interfaces/IHttpResponse.h"

/**
 * A generic http request
 */
class FGenericPlatformHttpRequest : public IHttpRequest
{
public:
	// IHttpBase
	virtual FString GetURL() OVERRIDE { return TEXT(""); }
	virtual FString GetURLParameter(const FString& ParameterName) OVERRIDE { return TEXT(""); }
	virtual FString GetHeader(const FString& HeaderName) OVERRIDE { return TEXT(""); }
	virtual TArray<FString> GetAllHeaders() OVERRIDE { return TArray<FString>(); }
	virtual FString GetContentType() OVERRIDE { return TEXT(""); }
	virtual int32 GetContentLength() OVERRIDE { return 0; }
	virtual const TArray<uint8>& GetContent() { static TArray<uint8> Temp; return Temp; }

	// IHttpRequest
	virtual FString GetVerb() OVERRIDE { return TEXT(""); }
	virtual void SetVerb(const FString& Verb) OVERRIDE {}
	virtual void SetURL(const FString& URL) OVERRIDE {}
	virtual void SetContent(const TArray<uint8>& ContentPayload) OVERRIDE {}
	virtual void SetContentAsString(const FString& ContentString) OVERRIDE {}
	virtual void SetHeader(const FString& HeaderName, const FString& HeaderValue) OVERRIDE {}
	virtual bool ProcessRequest() OVERRIDE { return false; }
	virtual FHttpRequestCompleteDelegate& OnProcessRequestComplete() OVERRIDE { static FHttpRequestCompleteDelegate RequestCompleteDelegate; return RequestCompleteDelegate; }
	virtual FHttpRequestProgressDelegate& OnRequestProgress() OVERRIDE { static FHttpRequestProgressDelegate RequestProgressDelegate; return RequestProgressDelegate; }
	virtual void CancelRequest() OVERRIDE {}
	virtual EHttpRequestStatus::Type GetStatus() OVERRIDE { return EHttpRequestStatus::NotStarted; }
	virtual void Tick(float DeltaSeconds) OVERRIDE {}
};


void FGenericPlatformHttp::Init()
{
}


void FGenericPlatformHttp::Shutdown()
{
}


IHttpRequest* FGenericPlatformHttp::ConstructRequest()
{
	return new FGenericPlatformHttpRequest();
}


FString FGenericPlatformHttp::UrlEncode(const FString &UnencodedString)
{
	FString EncodedString = TEXT("");
	const FString UnreservedChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";
	for (auto EncodeIt = UnencodedString.CreateConstIterator(); EncodeIt; ++EncodeIt)
	{
		if (UnreservedChars.Contains(FString::Chr(*EncodeIt)))
		{
			EncodedString += *EncodeIt;
		}
		else if (*EncodeIt != '\0')
		{
			EncodedString += TEXT("%");
			EncodedString += FString::Printf(TEXT("%.2X"), *EncodeIt);
		}
	}
	return EncodedString;
}
