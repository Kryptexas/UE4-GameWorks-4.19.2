// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once


/**
 * Platform specific Http implementations
 */
class CORE_API FGenericPlatformHttp
{
public:

	/**
	 * Platform initialization step
	 */
	static void Init();

	/**
	 * Creates a platform-specific HTTP manager.
	 *
	 * @return NULL if default implementation is to be used
	 */
	static class FHttpManager * CreatePlatformHttpManager()
	{
		return NULL;
	}

	/**
	 * Platform shutdown step
	 */
	static void Shutdown();

	/**
	 * Creates a new Http request instance for the current platform
	 *
	 * @return request object
	 */
	static class IHttpRequest* ConstructRequest();

	/**
	 * Returns a percent-encoded version of the passed in string
	 *
	 * @param FString& The unencoded string to convert to percent-encoding
	 * @return The percent-encoded string
	 */
	static FString UrlEncode(const FString &);
};
