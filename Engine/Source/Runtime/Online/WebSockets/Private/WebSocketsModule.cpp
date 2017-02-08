// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "WebSocketsModule.h"
#include "WebSocketsLog.h"

#if WITH_WEBSOCKETS
#include "PlatformWebSocket.h"
#endif // #if WITH_WEBSOCKETS

DEFINE_LOG_CATEGORY(LogWebSockets);

// FWebSocketsModule
IMPLEMENT_MODULE(FWebSocketsModule, WebSockets);

FWebSocketsModule* FWebSocketsModule::Singleton = nullptr;

void FWebSocketsModule::StartupModule()
{
	Singleton = this;

#if WITH_WEBSOCKETS
	const FString Protocols[] = {TEXT("v10.stomp"), TEXT("v11.stomp"), TEXT("v12.stomp")};

	WebSocketsManager = new FPlatformWebSocketsManager;
	WebSocketsManager->InitWebSockets(Protocols);
#endif
}

void FWebSocketsModule::ShutdownModule()
{
#if WITH_WEBSOCKETS
	if (WebSocketsManager)
	{
		WebSocketsManager->ShutdownWebSockets();
		WebSocketsManager = nullptr;
	}
#endif

	Singleton = nullptr;
}

FWebSocketsModule& FWebSocketsModule::Get()
{
	return *Singleton;
}

#if WITH_WEBSOCKETS
TSharedRef<IWebSocket> FWebSocketsModule::CreateWebSocket(const FString& Url, const TArray<FString>& Protocols)
{
	check(WebSocketsManager);
	return WebSocketsManager->CreateWebSocket(Url, Protocols);
}

TSharedRef<IWebSocket> FWebSocketsModule::CreateWebSocket(const FString& Url, const FString& Protocol)
{
	check(WebSocketsManager);

	TArray<FString> Protocols;
	Protocols.Add(Protocol);
	return WebSocketsManager->CreateWebSocket(Url, Protocols);
}
#endif // #if WITH_WEBSOCKETS
