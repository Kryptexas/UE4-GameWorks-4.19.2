// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Commands.h"

class FBTCommonCommands : public TCommands<FBTCommonCommands>
{
public:
	FBTCommonCommands();

	TSharedPtr<FUICommandInfo> SearchBT;

	/** Initialize commands */
	virtual void RegisterCommands() override;
};

class FBTDebuggerCommands : public TCommands<FBTDebuggerCommands>
{
public:
	FBTDebuggerCommands();

	TSharedPtr<FUICommandInfo> BackInto;
	TSharedPtr<FUICommandInfo> BackOver;
	TSharedPtr<FUICommandInfo> ForwardInto;
	TSharedPtr<FUICommandInfo> ForwardOver;
	TSharedPtr<FUICommandInfo> StepOut;

	TSharedPtr<FUICommandInfo> PausePlaySession;
	TSharedPtr<FUICommandInfo> ResumePlaySession;
	TSharedPtr<FUICommandInfo> StopPlaySession;

	/** Initialize commands */
	virtual void RegisterCommands() override;
};
