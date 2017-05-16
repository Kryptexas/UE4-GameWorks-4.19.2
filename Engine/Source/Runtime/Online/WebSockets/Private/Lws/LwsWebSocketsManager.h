/// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_WEBSOCKETS && WITH_LIBWEBSOCKETS

#include "IWebSocketsManager.h"
#include "LwsWebSocket.h"

#if PLATFORM_WINDOWS
#	include "AllowWindowsPlatformTypes.h"
#endif

#include "libwebsockets.h"

#if PLATFORM_WINDOWS
#	include "HideWindowsPlatformTypes.h"
#endif


class FLwsWebSocketsManager : public IWebSocketsManager, public FTickerObjectBase
{
public:
	/**
	 * Default constructor
	 */
	FLwsWebSocketsManager()
		: LwsContext(nullptr)
	{
	}


private:

	// IWebSocketsManager overrides
	virtual void InitWebSockets(TArrayView<const FString> Protocols) override;

	virtual void ShutdownWebSockets() override;

	virtual TSharedRef<IWebSocket> CreateWebSocket(const FString& Url, const TArray<FString>& Protocols, const FString& UpgradeHeader) override;
	// end IWebSocketsManager overrides

	// TickerObjectBase override
	virtual bool Tick(float DeltaTime) override;

	static void LwsLog(int Level, const char* LogLine);
	static int CallbackWrapper(lws* Connection, lws_callback_reasons Reason, void* UserData, void* Data, size_t Length);

	void DeleteLwsProtocols();

	lws_context* LwsContext;
	TArray<lws_protocols> LwsProtocols;

	static TArray<TSharedRef<IWebSocket>> Sockets;
	static TArray<TSharedRef<IWebSocket>> SocketsPendingDelete;
};

#endif // WITH_WEBSOCKETS && WITH_LIBWEBSOCKETS