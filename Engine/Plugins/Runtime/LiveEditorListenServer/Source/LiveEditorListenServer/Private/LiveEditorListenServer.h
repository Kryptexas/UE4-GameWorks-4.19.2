// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LiveEditorListenServerObjectCache.h"

class FLiveEditorListenServer : public ILiveEditorListenServer
{
public:
	FLiveEditorListenServer();

	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	void Tick( float DeltaTime );

	void OnObjectCreation( const class UObjectBase *NewObject );
	void OnObjectDeletion( const class UObjectBase *NewObject );

	bool HandleListenerConnectionAccepted( class FSocket* ClientSocket, const FIPv4Endpoint& ClientEndpoint );

	void OnPreLoadMap();
	void OnPostLoadMap();

private:
	void InstallHooks();
	void RemoveHooks();
	void ReplicateChanges( const struct nLiveEditorListenServer::FNetworkMessage &Message );

private:
	nLiveEditorListenServer::FObjectCache ObjectCache;
	nLiveEditorListenServer::FCreateListener *ObjectCreateListener;
	nLiveEditorListenServer::FDeleteListener *ObjectDeleteListener;
	nLiveEditorListenServer::FTickObject *TickObject;
	TSharedPtr<nLiveEditorListenServer::FMapLoadObserver> MapLoadObserver;

	class FTcpListener *Listener;
	TQueue<class FSocket*, EQueueMode::Mpsc> PendingClients;
	TArray<class FSocket*> Clients;
};
