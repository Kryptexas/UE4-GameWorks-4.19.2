// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "WebSocketsPrivatePCH.h"
#include "WebSocketsModule.h"
#include "PlatformWebSocket.h"

DEFINE_LOG_CATEGORY(LogWebSockets);

// FWebSocketsModule
IMPLEMENT_MODULE(FWebSocketsModule, WebSockets);

FWebSocketsModule* FWebSocketsModule::Singleton = nullptr;

void FWebSocketsModule::StartupModule()
{
	Singleton = this;
}

void FWebSocketsModule::ShutdownModule()
{
	
	Singleton = nullptr;
}

FWebSocketsModule& FWebSocketsModule::Get()
{
	return *Singleton;
}

TSharedRef<IWebSocket> FWebSocketsModule::CreateWebSocket(const FString& Url, const TArray<FString>& Protocols)
{
	return MakeShareable(new FPlatformWebSocket(Url, Protocols));
}

TSharedRef<IWebSocket> FWebSocketsModule::CreateWebSocket(const FString& Url, const FString& Protocol)
{
	TArray<FString> Protocols;
	Protocols.Add(Protocol);
	return MakeShareable(new FPlatformWebSocket(Url, Protocols));
}
