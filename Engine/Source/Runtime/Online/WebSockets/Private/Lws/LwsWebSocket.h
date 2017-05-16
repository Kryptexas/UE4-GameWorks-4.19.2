// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Ticker.h"
#include "Containers/Queue.h"

#if WITH_WEBSOCKETS && WITH_LIBWEBSOCKETS

#include "IWebSocket.h"

#if PLATFORM_WINDOWS
#   include "WindowsHWrapper.h"
#	include "AllowWindowsPlatformTypes.h"
#endif

THIRD_PARTY_INCLUDES_START
#include "libwebsockets.h"
THIRD_PARTY_INCLUDES_END

#if PLATFORM_WINDOWS
#	include "HideWindowsPlatformTypes.h"
#endif

struct FLwsSendBuffer
{
	bool bIsBinary;
	int32 BytesWritten;
	TArray<uint8> Payload;

	FLwsSendBuffer(const uint8* Data, SIZE_T Size, bool InIsBinary)
		: bIsBinary(InIsBinary)
		, BytesWritten(0)
	{
		Payload.AddDefaulted(LWS_PRE); // Reserve space for WS header data
		Payload.Append(Data, Size);
	}

	bool Write(struct lws* LwsConnection);

};


class FLwsWebSocket
	: public IWebSocket
	, public TSharedFromThis<FLwsWebSocket>
{

public:

	virtual ~FLwsWebSocket();

	// IWebSocket
	virtual void Connect() override;
	virtual void Close(int32 Code = 1000, const FString& Reason = FString()) override;

	virtual bool IsConnected() override
	{
		return bIsConnected;
	}

	virtual void Send(const FString& Data);
	virtual void Send(const void* Data, SIZE_T Size, bool bIsBinary) override;

	/**
	 * Delegate called when a web socket connection has been established
	 *
	 */
	DECLARE_DERIVED_EVENT(FLwsWebSocket, IWebSocket::FWebSocketConnectedEvent, FWebSocketConnectedEvent);
	virtual FWebSocketConnectedEvent& OnConnected() override
	{
		return ConnectedEvent;
	}

	/**
	 * Delegate called when a web socket connection has been established
	 *
	 */
	DECLARE_DERIVED_EVENT(FLwsWebSocket, IWebSocket::FWebSocketConnectionErrorEvent, FWebSocketConnectionErrorEvent);
	virtual FWebSocketConnectionErrorEvent& OnConnectionError() override
	{
		return ConnectionErrorEvent;
	}

	/**
	 * Delegate called when a web socket connection has been closed
	 *
	 */
	DECLARE_DERIVED_EVENT(FLwsWebSocket, IWebSocket::FWebSocketClosedEvent, FWebSocketClosedEvent);
	virtual FWebSocketClosedEvent& OnClosed() override
	{
		return ClosedEvent;
	}

	/**
	 * Delegate called when a web socket message has been received
	 *
	 */
	DECLARE_DERIVED_EVENT(FLwsWebSocket, IWebSocket::FWebSocketMessageEvent, FWebSocketMessageEvent);
	virtual FWebSocketMessageEvent& OnMessage() override
	{
		return MessageEvent;
	}

	/**
	 * Delegate called when a raw web socket message has been received
	 *
	 */
	DECLARE_DERIVED_EVENT(FLwsWebSocket, IWebSocket::FWebSocketRawMessageEvent, FWebSocketRawMessageEvent);
	virtual FWebSocketRawMessageEvent& OnRawMessage() override
	{
		return RawMessageEvent;
	}

	int LwsCallback(lws* Instance, lws_callback_reasons Reason, void* Data, size_t Length);
private:

	FLwsWebSocket(const FString& Url, const TArray<FString>& Protocols, lws_context* Context, const FString& UpgradeHeader);
	void SendFromQueue();
	void FlushQueues();
	void DelayConnectionError(const FString& Error);

	FWebSocketConnectedEvent ConnectedEvent;
	FWebSocketConnectionErrorEvent ConnectionErrorEvent;
	FWebSocketClosedEvent ClosedEvent;
	FWebSocketMessageEvent MessageEvent;
	FWebSocketRawMessageEvent RawMessageEvent;

	struct lws_context *LwsContext;
	struct lws *LwsConnection;
	FString Url;
	TArray<FString> Protocols;
	FString UpgradeHeader;

	FString ReceiveBuffer;
	TQueue<TSharedPtr<FLwsSendBuffer>, EQueueMode::Spsc> SendQueue;
	int32 CloseCode;
	FString CloseReason;
	bool bIsConnecting;
	bool bIsConnected;
	bool bClientInitiatedClose;

	friend class FLwsWebSocketsManager;
};

#endif
