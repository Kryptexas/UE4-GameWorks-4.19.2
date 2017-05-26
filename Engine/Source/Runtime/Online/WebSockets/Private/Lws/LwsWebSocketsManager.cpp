// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "LwsWebSocketsManager.h"

#if WITH_WEBSOCKETS && WITH_LIBWEBSOCKETS

#include "Ssl.h"
#include "WebSocketsLog.h"

void FLwsWebSocketsManager::InitWebSockets(TArrayView<const FString> Protocols)
{
	// Subscribe to log events.  Everything except LLL_PARSER
	lws_set_log_level(LLL_ERR|LLL_WARN|LLL_NOTICE|LLL_INFO|LLL_DEBUG|LLL_HEADER|LLL_EXT|LLL_CLIENT|LLL_LATENCY, &FLwsWebSocketsManager::LwsLog);

	check(!LwsContext && LwsProtocols.Num() == 0);

	LwsProtocols.Reserve(Protocols.Num() + 1);
	for (const FString& Protocol: Protocols)
	{
		FTCHARToUTF8 ConvertName(*Protocol);

		// We need to hold on to the converted strings
		ANSICHAR *Converted = static_cast<ANSICHAR*>(FMemory::Malloc(ConvertName.Length()+1));
		FCStringAnsi::Strcpy(Converted, ConvertName.Length(), ConvertName.Get());
		LwsProtocols.Add({Converted, &FLwsWebSocketsManager::CallbackWrapper, 0, 65536});
	}

	// LWS requires a zero terminator (we don't pass the length)
	LwsProtocols.Add({nullptr, nullptr, 0, 0});

	struct lws_context_creation_info ContextInfo = {};

	ContextInfo.port = CONTEXT_PORT_NO_LISTEN;
	ContextInfo.protocols = LwsProtocols.GetData();
	ContextInfo.uid = -1;
	ContextInfo.gid = -1;
	ContextInfo.options |= LWS_SERVER_OPTION_PEER_CERT_NOT_REQUIRED | LWS_SERVER_OPTION_DISABLE_OS_CA_CERTS;
	ContextInfo.ssl_cipher_list = nullptr;
	LwsContext = lws_create_context(&ContextInfo);
}

void FLwsWebSocketsManager::ShutdownWebSockets()
{
	if (LwsContext)
	{
		lws_context_destroy(LwsContext);
		LwsContext = nullptr;
	}
	DeleteLwsProtocols();
}

static inline bool LwsLogLevelIsWarning(int Level)
{
	return Level == LLL_ERR ||
		Level == LLL_WARN;
}

static inline const TCHAR* LwsLogLevelToString(int Level)
{
	switch (Level)
	{
	case LLL_ERR: return TEXT("Error");
	case LLL_WARN: return TEXT("Warning");
	case LLL_NOTICE: return TEXT("Notice");
	case LLL_INFO: return TEXT("Info");
	case LLL_DEBUG: return TEXT("Debug");
	case LLL_PARSER: return TEXT("Parser");
	case LLL_HEADER: return TEXT("Header");
	case LLL_EXT: return TEXT("Ext");
	case LLL_CLIENT: return TEXT("Client");
	case LLL_LATENCY: return TEXT("Latency");
	}
	return TEXT("Invalid");
}

void FLwsWebSocketsManager::LwsLog(int Level, const char* LogLine)
{
	bool bIsWarning = LwsLogLevelIsWarning(Level);
	if (bIsWarning || UE_LOG_ACTIVE(LogWebSockets, Verbose))
	{
		FUTF8ToTCHAR Converter(LogLine);
		// Trim trailing newline
		FString ConvertedLogLine(Converter.Get());
		if (ConvertedLogLine.EndsWith(TEXT("\n")))
		{
			ConvertedLogLine[ConvertedLogLine.Len() - 1] = TEXT(' ');
		}
		if (bIsWarning)
		{
			UE_LOG(LogWebSockets, Warning, TEXT("Lws(%s): %s"), LwsLogLevelToString(Level), *ConvertedLogLine);
		}
		else
		{
			UE_LOG(LogWebSockets, Verbose, TEXT("Lws(%s): %s"), LwsLogLevelToString(Level), *ConvertedLogLine);
		}
	}
}

int FLwsWebSocketsManager::CallbackWrapper(lws* Connection, lws_callback_reasons Reason, void* UserData, void* Data, size_t Length)
{
	FLwsWebSocket* Socket = static_cast<FLwsWebSocket*>(UserData);

	switch (Reason)
	{
	case LWS_CALLBACK_RECEIVE_PONG:
		return 0;
	case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
	case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS:
	{
		FSslModule::Get().GetCertificateManager().AddCertificatesToSslContext(static_cast<SSL_CTX*>(UserData));
		return 0;
	}
	case LWS_CALLBACK_WSI_DESTROY:
	{
		TSharedRef<FLwsWebSocket> SharedSocket = Socket->AsShared();
		Sockets.RemoveSwap(SharedSocket);
		SocketsPendingDelete.Emplace(SharedSocket);
		break;
	}
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
	case LWS_CALLBACK_CLIENT_RECEIVE:
	case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
	case LWS_CALLBACK_CLOSED:
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
	case LWS_CALLBACK_CLIENT_WRITEABLE:
	case LWS_CALLBACK_SERVER_WRITEABLE:
	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
		break;
	default:
		return 0;
	}

	return Socket->LwsCallback(Connection, Reason, Data, Length);
}

void FLwsWebSocketsManager::DeleteLwsProtocols()
{
	for (const lws_protocols& Protocol: LwsProtocols)
	{
		FMemory::Free(const_cast<ANSICHAR*>(Protocol.name));
	}
	LwsProtocols.Reset();
}

bool FLwsWebSocketsManager::Tick(float DeltaTime)
{
	if (LwsContext)
	{
		lws_service(LwsContext, 0);
	}
	SocketsPendingDelete.Reset();
	return true;
}

TSharedRef<IWebSocket> FLwsWebSocketsManager::CreateWebSocket(const FString& Url, const TArray<FString>& Protocols, const FString& UpgradeHeader)
{
	check(LwsContext);
	TSharedRef<IWebSocket> Socket = MakeShareable(new FLwsWebSocket(Url, Protocols, LwsContext, UpgradeHeader));
	Sockets.Emplace(Socket);
	return Socket;
}

TArray<TSharedRef<IWebSocket>> FLwsWebSocketsManager::Sockets;
TArray<TSharedRef<IWebSocket>> FLwsWebSocketsManager::SocketsPendingDelete;

#endif // #if WITH_WEBSOCKETS
