// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VisualLog.h"

/** Actual implementation of LogVisualizer, private inside module */
class FLogVisualizer : public ILogVisualizer
{
public:
	virtual void SummonUI(UWorld* InWorld) OVERRIDE;
	virtual void CloseUI(UWorld* InWorld) OVERRIDE;

	virtual bool IsRecording();
	// End ILogVisualizer interface

	/** Change the current recording state */
	void SetIsRecording(bool bNewRecording);

	FLogVisualizer()
	{
	}

	virtual ~FLogVisualizer() 
	{
		CleanUp();
	}
	
	DECLARE_EVENT( FLogVisualizer, FVisLogsChangedEvent );
	FVisLogsChangedEvent& OnLogAdded() { return LogAddedEvent; }

	void CleanUp();

	void PullDataFromVisualLog(FVisualLog* VisualLog);

	int32 GetLogIndexForActor(const class AActor* Actor);

	void AddLoadedLog(TSharedPtr<FActorsVisLog> Log);

	UWorld* GetWorld() { return World.Get(); }

protected:
	void OnNewLog(const class AActor* Actor, TSharedPtr<FActorsVisLog> Log);
	
public:
	/** All hosted logs */
	TArray<TSharedPtr<FActorsVisLog> > Logs;

private:
	/** Event called when a new log is created */
	FVisLogsChangedEvent	LogAddedEvent;
	TWeakObjectPtr<UWorld>	World;
	TWeakPtr<SWindow>		LogWindow;
};