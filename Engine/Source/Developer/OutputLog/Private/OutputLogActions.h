// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FOutputLogCommandsImpl : public TCommands<FOutputLogCommandsImpl>
{

public:
	FOutputLogCommandsImpl() : TCommands<FOutputLogCommandsImpl>
	(
		"OutputLog", // Context name for fast lookup
		NSLOCTEXT("Contexts", "OutputLog", "Output Log"), // Localized context name for displaying
		NAME_None, // Parent
		FEditorStyle::GetStyleSetName() // Icon Style Set
	)
	{
	}
	
	/**
	 * Initialize commands
	 */
	virtual void RegisterCommands() OVERRIDE;

public:
	/** Copy command */
	TSharedPtr< FUICommandInfo > CopyOutputLog;

	/** SelectAll command */
	TSharedPtr< FUICommandInfo > SelectAllInOutputLog;

	/** SelectNone command */
	TSharedPtr< FUICommandInfo > SelectNoneInOutputLog;

	/** DeleteAll command */
	TSharedPtr< FUICommandInfo > ClearOutputLog;

};


class OUTPUTLOG_API FOutputLogCommands
{
public:
	static void Register();

	static const FOutputLogCommandsImpl& Get();

	static void Unregister();
};

