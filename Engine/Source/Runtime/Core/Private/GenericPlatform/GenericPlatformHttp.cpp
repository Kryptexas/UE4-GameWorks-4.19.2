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
	virtual FString GetURL() override { return TEXT(""); }
	virtual FString GetURLParameter(const FString& ParameterName) override { return TEXT(""); }
	virtual FString GetHeader(const FString& HeaderName) override { return TEXT(""); }
	virtual TArray<FString> GetAllHeaders() override { return TArray<FString>(); }
	virtual FString GetContentType() override { return TEXT(""); }
	virtual int32 GetContentLength() override { return 0; }
	virtual const TArray<uint8>& GetContent() { static TArray<uint8> Temp; return Temp; }

	// IHttpRequest
	virtual FString GetVerb() override { return TEXT(""); }
	virtual void SetVerb(const FString& Verb) override {}
	virtual void SetURL(const FString& URL) override {}
	virtual void SetContent(const TArray<uint8>& ContentPayload) override {}
	virtual void SetContentAsString(const FString& ContentString) override {}
	virtual void SetHeader(const FString& HeaderName, const FString& HeaderValue) override {}
	virtual bool ProcessRequest() override { return false; }
	virtual FHttpRequestCompleteDelegate& OnProcessRequestComplete() override { static FHttpRequestCompleteDelegate RequestCompleteDelegate; return RequestCompleteDelegate; }
	virtual FHttpRequestProgressDelegate& OnRequestProgress() override { static FHttpRequestProgressDelegate RequestProgressDelegate; return RequestProgressDelegate; }
	virtual void CancelRequest() override {}
	virtual EHttpRequestStatus::Type GetStatus() override { return EHttpRequestStatus::NotStarted; }
	virtual void Tick(float DeltaSeconds) override {}
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
