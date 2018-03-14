// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Modules/ModuleInterface.h"
#include "Widgets/SWindow.h"

class FConsoleCommandExecutor;
class SMultiLineEditableTextBox;

/** Style of the debug console */
namespace EDebugConsoleStyle
{
	enum Type
	{
		/** Shows the debug console input line with tab completion only */
		Compact,

		/** Shows a scrollable log window with the input line on the bottom */
		WithLog,
	};
};

struct FDebugConsoleDelegates
{
	FSimpleDelegate OnFocusLost;
	FSimpleDelegate OnConsoleCommandExecuted;
	FSimpleDelegate OnCloseConsole;
};

class FOutputLogModule : public IModuleInterface
{
public:
	virtual void StartupModule();
	virtual void ShutdownModule();

	/** Generates a console input box widget.  Remember, this widget will become invalid if the
	    output log DLL is unloaded on the fly. */
	virtual TSharedRef< SWidget > MakeConsoleInputBox( TSharedPtr< SMultiLineEditableTextBox >& OutExposedEditableTextBox ) const;

	/** Opens a debug console in the specified window, if not already open */
	virtual void ToggleDebugConsoleForWindow( const TSharedRef< SWindow >& Window, const EDebugConsoleStyle::Type InStyle, const FDebugConsoleDelegates& DebugConsoleDelegates );

	/** Closes the debug console for the specified window */
	virtual void CloseDebugConsole();


private:

	/** Weak pointer to a debug console that's currently open, if any */
	TWeakPtr< SWidget > DebugConsole;

	/** Pointer to the classic "Cmd" executor */
	TSharedPtr<FConsoleCommandExecutor> CmdExec;
};
