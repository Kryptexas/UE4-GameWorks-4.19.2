// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Mac implementation of an Http request
 */
class FMacHttpRequest : public IHttpRequest
{
private:
	// This is the NSMutableURLRequest, all our Mac functionality will deal with this.
	NSMutableURLRequest* Request;

	// This is the connection our request is sent along.
	NSURLConnection* Connection;


public:
	// implementation friends
	friend class FMacHttpResponse;


	// Begin IHttpBase interface
	virtual FString GetURL() OVERRIDE;
	virtual FString GetURLParameter(const FString& ParameterName) OVERRIDE;
	virtual FString GetHeader(const FString& HeaderName) OVERRIDE;
	virtual TArray<FString> GetAllHeaders() OVERRIDE;	
	virtual FString GetContentType() OVERRIDE;
	virtual int32 GetContentLength() OVERRIDE;
	virtual const TArray<uint8>& GetContent() OVERRIDE;
	// End IHttpBase interface

	// Begin IHttpRequest interface
	virtual FString GetVerb() OVERRIDE;
	virtual void SetVerb(const FString& Verb) OVERRIDE;
	virtual void SetURL(const FString& URL) OVERRIDE;
	virtual void SetContent(const TArray<uint8>& ContentPayload) OVERRIDE;
	virtual void SetContentAsString(const FString& ContentString) OVERRIDE;
	virtual void SetHeader(const FString& HeaderName, const FString& HeaderValue) OVERRIDE;
	virtual bool ProcessRequest() OVERRIDE;
	virtual FHttpRequestCompleteDelegate& OnProcessRequestComplete() OVERRIDE;
	virtual FHttpRequestProgressDelegate& OnRequestProgress() OVERRIDE;
	virtual void CancelRequest() OVERRIDE;
	virtual EHttpRequestStatus::Type GetStatus() OVERRIDE;
	virtual void Tick(float DeltaSeconds) OVERRIDE;
	// End IHttpRequest interface


	/**
	 * Constructor
	 */
	FMacHttpRequest();

	/**
	 * Destructor. Clean up any connection/request handles
	 */
	virtual ~FMacHttpRequest();


private:

	/**
	 * Create the session connection and initiate the web request
	 *
	 * @return true if the request was started
	 */
	bool StartRequest();

	/**
	 * Process state for a finished request that no longer needs to be ticked
	 * Calls the completion delegate
	 */
	void FinishedRequest();

	/**
	 * Close session/request handles and unregister callbacks
	 */
	void CleanupRequest();


private:
	/** The response object which we will use to pair with this request */
	TSharedPtr<class FMacHttpResponse,ESPMode::ThreadSafe> Response;

	/** BYTE array payload to use with the request. Typically for a POST */
	TArray<uint8> RequestPayload;

	/** Delegate that will get called once request completes or on any error */
	FHttpRequestCompleteDelegate RequestCompleteDelegate;

	/** Delegate that will get called once per tick with bytes downloaded so far */
	FHttpRequestProgressDelegate RequestProgressDelegate;

	/** Current status of request being processed */
	EHttpRequestStatus::Type CompletionStatus;
};



/**
 * Mac Response Wrapper which will be used for it's delegates to receive responses.
 */
@interface FHttpResponseMacWrapper : NSObject
{
};
/** A handle for the response */
@property(retain) NSHTTPURLResponse* Response;
/** Holds the payload as we receive it. */
@property(retain) NSMutableData* Payload;
/** Flag whether the response is ready */
@property BOOL bIsReady;
/** When the response is complete, indicates whether the response was received without error. */
@property BOOL bHadError;

/** Delegate called with we receive a response. See Mac docs for when/how this should be used. */
-(void) connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response;
/** Delegate called with we receive data. See Mac docs for when/how this should be used. */
-(void) connection:(NSURLConnection *)connection didReceiveData:(NSData *)data;
/** Delegate called with we complete with an error. See Mac docs for when/how this should be used. */
-(void) connection:(NSURLConnection *)connection didFailWithError:(NSError *)error;
/** Delegate called with we complete successfully. See Mac docs for when/how this should be used. */
-(void) connectionDidFinishLoading:(NSURLConnection *)connection;
@end


/**
 * Mac implementation of an Http response
 */
class FMacHttpResponse : public IHttpResponse
{
private:
	// This is the NSHTTPURLResponse, all our functionality will deal with.
	FHttpResponseMacWrapper* ResponseWrapper;

	/** Request that owns this response */
	const FMacHttpRequest& Request;


public:
	// implementation friends
	friend class FMacHttpRequest;


	// Begin IHttpBase interface
	virtual FString GetURL() OVERRIDE;
	virtual FString GetURLParameter(const FString& ParameterName) OVERRIDE;
	virtual FString GetHeader(const FString& HeaderName) OVERRIDE;
	virtual TArray<FString> GetAllHeaders() OVERRIDE;	
	virtual FString GetContentType() OVERRIDE;
	virtual int32 GetContentLength() OVERRIDE;
	virtual const TArray<uint8>& GetContent() OVERRIDE;
	// End IHttpBase interface

	// Begin IHttpResponse interface
	virtual int32 GetResponseCode() OVERRIDE;
	virtual FString GetContentAsString() OVERRIDE;
	// End IHttpResponse interface

	NSHTTPURLResponse* GetResponseObj();

	/**
	 * Check whether a response is ready or not.
	 */
	bool IsReady();
	
	/**
	 * Check whether a response had an error.
	 */
	bool HadError();

	/**
	 * Constructor
	 *
	 * @param InRequest - original request that created this response
	 */
	FMacHttpResponse(const FMacHttpRequest& InRequest);

	/**
	 * Destructor
	 */
	virtual ~FMacHttpResponse();


private:

	/** BYTE array to fill in as the response is read via didReceiveData */
	TArray<uint8> Payload;
};