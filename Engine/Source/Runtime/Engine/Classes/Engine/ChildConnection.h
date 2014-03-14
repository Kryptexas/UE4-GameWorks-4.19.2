// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/** represents a secondary splitscreen connection that reroutes calls to the parent connection */

#pragma once
#include "ChildConnection.generated.h"

UCLASS(HeaderGroup=Network, MinimalAPI,transient,config=Engine)
class UChildConnection : public UNetConnection
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(transient)
	class UNetConnection* Parent;

	// Begin UNetConnection interface.
	virtual UChildConnection* GetUChildConnection() OVERRIDE
	{
		return this;
	}

	virtual FString LowLevelGetRemoteAddress(bool bAppendPort=false) OVERRIDE
	{
		return Parent->LowLevelGetRemoteAddress(bAppendPort);
	}

	virtual FString LowLevelDescribe() OVERRIDE
	{
		return Parent->LowLevelDescribe();
	}

		virtual void LowLevelSend( void* Data, int32 Count ) OVERRIDE
	{
	}

	virtual void InitOut() OVERRIDE
	{
		Parent->InitOut();
	}

	virtual void AssertValid() OVERRIDE
	{
		Parent->AssertValid();
	}

	virtual void SendAck( int32 PacketId, bool FirstTime=1, bool bHavePingAckData=0, uint32 PingAckData=0 ) OVERRIDE
	{
	}

	virtual void FlushNet(bool bIgnoreSimulation = false) OVERRIDE
	{
		Parent->FlushNet(bIgnoreSimulation);
	}

	virtual int32 IsNetReady(bool Saturate) OVERRIDE
	{
		return Parent->IsNetReady(Saturate);
	}

	virtual void Tick() OVERRIDE
	{
		State = Parent->State;
	}

	virtual void HandleClientPlayer(class APlayerController* PC, class UNetConnection* NetConnection) OVERRIDE;
	
	virtual void CleanUp() OVERRIDE;

	// End UNetConnection interface.

};
