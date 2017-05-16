// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Private/Lws/LwsWebSocket.h"

#if WITH_WEBSOCKETS && WITH_LIBWEBSOCKETS

#include "WebSocketsLog.h"
#include "Ssl.h"

namespace {
	static const struct lws_extension LwsExtensions[] = {
		{
			"permessage-deflate",
			lws_extension_callback_pm_deflate,
			"permessage-deflate; client_max_window_bits"
		},
		{
			"deflate-frame",
			lws_extension_callback_pm_deflate,
			"deflate_frame"
		},
		// zero terminated:
		{ nullptr, nullptr, nullptr }
	};
}

FLwsWebSocket::FLwsWebSocket(const FString& InUrl, const TArray<FString>& InProtocols, lws_context* Context, const FString& InUpgradeHeader)
	: LwsContext(Context)
	, LwsConnection(nullptr)
	, Url(InUrl)
	, Protocols(InProtocols)
	, UpgradeHeader(InUpgradeHeader)
	, ReceiveBuffer()
	, CloseCode(0)
	, CloseReason()
	, bIsConnecting(false)
	, bIsConnected(false)
	, bClientInitiatedClose(false)
{
}

FLwsWebSocket::~FLwsWebSocket()
{
	Close(LWS_CLOSE_STATUS_GOINGAWAY, TEXT("Bye"));
}

void FLwsWebSocket::DelayConnectionError(const FString& Error)
{
	// report connection error on the next tick, so we don't invoke event handlers on the current stack frame
	FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=](float) -> bool
	{
		bIsConnecting = false;
		OnConnectionError().Broadcast(Error);
		return false;
	}));
}

void FLwsWebSocket::Connect()
{
	bIsConnecting = true;
	if (LwsContext == nullptr)
	{
		DelayConnectionError(TEXT("Invalid context"));
		return;
	}
	if (LwsConnection != nullptr)
	{
		DelayConnectionError(TEXT("Already connected"));
		return;
	}

	struct lws_client_connect_info ConnectInfo = {};
	ConnectInfo.context = LwsContext;
	FTCHARToUTF8 UrlUtf8(*Url);
	const char *UrlProtocol, *TmpUrlPath;
	char UrlPath[300];

	if (lws_parse_uri((char*) UrlUtf8.Get(), &UrlProtocol, &ConnectInfo.address, &ConnectInfo.port, &TmpUrlPath))
	{
		DelayConnectionError(TEXT("Bad URL"));
		return;
	}
	UrlPath[0] = '/';
	FCStringAnsi::Strncpy(UrlPath+1, TmpUrlPath, sizeof(UrlPath)-2);
	UrlPath[sizeof(UrlPath)-1]='\0';
	ConnectInfo.path = UrlPath;
	ConnectInfo.host = ConnectInfo.address;
	ConnectInfo.origin = ConnectInfo.address;
	ConnectInfo.ietf_version_or_minus_one = -1;
	ConnectInfo.client_exts = LwsExtensions;

	// Use SSL and require a valid cerver cert
	if (FCStringAnsi::Stricmp(UrlProtocol, "wss") == 0 )
	{
		ConnectInfo.ssl_connection = 1;
	}
	// Use SSL, and allow self-signed certs
	else if (FCStringAnsi::Stricmp(UrlProtocol, "wss+insecure") == 0 )
	{
		ConnectInfo.ssl_connection = 2;
	}
	// No encryption
	else if (FCStringAnsi::Stricmp(UrlProtocol, "ws") == 0 )
	{
		ConnectInfo.ssl_connection = 0;
	}
	// Else return an error
	else
	{
		DelayConnectionError(FString::Printf(TEXT("Bad protocol '%s'. Use either 'ws', 'wss', or 'wss+insecure'"), UTF8_TO_TCHAR(UrlProtocol)));
		return;
	}

	FString Combined = FString::Join(Protocols, TEXT(","));
	FTCHARToUTF8 CombinedUTF8(*Combined);
	ConnectInfo.protocol = CombinedUTF8.Get();
	ConnectInfo.userdata = this;

	if (lws_client_connect_via_info(&ConnectInfo) == nullptr)
	{
		DelayConnectionError(TEXT("Could not initialize connection"));
	}

}

void FLwsWebSocket::Close(int32 Code, const FString& Reason)
{
	CloseCode = Code;
	CloseReason = Reason;
	if (LwsConnection != nullptr && SendQueue.IsEmpty())
	{
		lws_callback_on_writable(LwsConnection);
	}
}

void FLwsWebSocket::Send(const void* Data, SIZE_T Size, bool bIsBinary)
{
	bool bQueueWasEmpty = SendQueue.IsEmpty();
	SendQueue.Enqueue(MakeShareable(new FLwsSendBuffer((const uint8*) Data, Size, bIsBinary)));
	if (LwsConnection != nullptr && bQueueWasEmpty)
	{
		lws_callback_on_writable(LwsConnection);
	}
}

void FLwsWebSocket::Send(const FString& Data)
{
	FTCHARToUTF8 Converted(*Data);
	Send((uint8*)Converted.Get(), Converted.Length(), false);
}

void FLwsWebSocket::FlushQueues()
{
	TSharedPtr<FLwsSendBuffer> Buffer;
	while (SendQueue.Dequeue(Buffer)); // Dequeue all elements until empty
	ReceiveBuffer.Empty(0); // Also clear temporary receive buffer
}

void FLwsWebSocket::SendFromQueue()
{
	if (LwsConnection == nullptr)
	{
		return;
	}

	TSharedPtr<FLwsSendBuffer> CurrentBuffer;
	if (SendQueue.Peek(CurrentBuffer))
	{
		bool FinishedSending = CurrentBuffer->Write(LwsConnection);
		if (FinishedSending)
		{
			SendQueue.Dequeue(CurrentBuffer);
		}
	}

	// If we still have data to send, ask for a notification when ready to send more
	if (!SendQueue.IsEmpty())
	{
		lws_callback_on_writable(LwsConnection);
	}
}

bool FLwsSendBuffer::Write(struct lws* LwsConnection)
{
	enum lws_write_protocol WriteProtocol;
	if (BytesWritten > 0)
	{
		WriteProtocol = LWS_WRITE_CONTINUATION;
	}
	else
	{
		WriteProtocol = bIsBinary?LWS_WRITE_BINARY:LWS_WRITE_TEXT;
	}

	int32 Offset = LWS_PRE + BytesWritten;
	int32 CurrentBytesWritten = lws_write(LwsConnection, Payload.GetData() + Offset, Payload.Num() - Offset, WriteProtocol);

	if (CurrentBytesWritten > 0)
	{
		BytesWritten += CurrentBytesWritten;
	}
	else
	{
		// @TODO: indicate error
		return true;
	}

	return BytesWritten + (int32)(LWS_PRE) >= Payload.Num();
}

int FLwsWebSocket::LwsCallback(lws* Instance, lws_callback_reasons Reason, void* Data, size_t Length)
{
	switch (Reason)
	{
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
	{
		bIsConnecting = false;
		bIsConnected = true;
		LwsConnection = Instance;
		if (!SendQueue.IsEmpty())
		{
			lws_callback_on_writable(LwsConnection);
		}
		OnConnected().Broadcast();
		break;
	}
	case LWS_CALLBACK_CLIENT_RECEIVE:
	{
		SIZE_T BytesLeft = lws_remaining_packet_payload(Instance);
		if(OnMessage().IsBound())
		{
			FUTF8ToTCHAR Convert((const ANSICHAR*)Data, Length);
			ReceiveBuffer.Append(Convert.Get(), Convert.Length());
			if (BytesLeft == 0)
			{
				OnMessage().Broadcast(ReceiveBuffer);
				ReceiveBuffer.Empty();
			}
		}
		OnRawMessage().Broadcast(Data, Length, BytesLeft);
		break;
	}
	case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
	{
		bClientInitiatedClose = true;
		bIsConnected = false;
		FlushQueues();
		if (OnClosed().IsBound())
		{
			uint16 CloseStatus = *((uint16*)Data);
#if PLATFORM_LITTLE_ENDIAN
			// The status is the first two bytes of the message in network byte order
			CloseStatus = BYTESWAP_ORDER16(CloseStatus);
#endif
			FUTF8ToTCHAR Convert((const ANSICHAR*)Data + sizeof(uint16), Length - sizeof(uint16));
			OnClosed().Broadcast(CloseStatus, Convert.Get(), true);
			return 1; // Close the connection without logging if the user handles Close events
		}
		break;
	}
	case LWS_CALLBACK_WSI_DESTROY:
	{
		// Getting a WSI_DESTROY before a connection has been established and no errors reported usually means there was a timeout establishing a connection
		if (bIsConnecting)
		{
			OnConnectionError().Broadcast(TEXT("Connection timed out"));
			bIsConnecting = false;
		}
		LwsConnection = nullptr;
		break;
	}
	case LWS_CALLBACK_CLOSED:
	{
		bIsConnected = false;
		OnClosed().Broadcast(LWS_CLOSE_STATUS_NORMAL, bClientInitiatedClose ? TEXT("Successfully closed connection to server") : TEXT("Connection closed by server"), bClientInitiatedClose);
		FlushQueues();
		break;
	}
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
	{
		FUTF8ToTCHAR Convert((const ANSICHAR*)Data, Length);
		OnConnectionError().Broadcast(Convert.Get());
		FlushQueues();
		return -1;
	}
	case LWS_CALLBACK_RECEIVE_PONG:
		break;
	case LWS_CALLBACK_CLIENT_WRITEABLE:
	case LWS_CALLBACK_SERVER_WRITEABLE:
	{
		if (CloseCode != 0)
		{
			FTCHARToUTF8 Convert(*CloseReason);
			// This only sets the reason for closing the connection:
			lws_close_reason(Instance, (enum lws_close_status)CloseCode, (unsigned char*)Convert.Get(), (size_t)Convert.Length());
			CloseCode = 0;
			CloseReason = FString();
			FlushQueues();
			return -1; // Returning non-zero will close the current connection
		}
		else
		{
			SendFromQueue();
		}
		break;
	}
	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
	{
		if (!UpgradeHeader.IsEmpty())
		{
			// FIXME: Should probably use strcat but libws suggests snprintf (https://libwebsockets.org/lws-api-doc-master/html/)
			char** WriteableString = reinterpret_cast<char**>(Data);
			*WriteableString += FCStringAnsi::Snprintf(*WriteableString, Length, TCHAR_TO_ANSI(*UpgradeHeader));
		}
		break;
	}
	default:
		break;
	}
	return 0;
}

#endif // #if WITH_WEBSOCKETS
