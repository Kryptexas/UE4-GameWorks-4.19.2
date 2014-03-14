// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// Console - A quick little command line console that accepts most commands.
//=============================================================================
#pragma once

#include "Console.generated.h"

USTRUCT()
struct FAutoCompleteCommand
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString Command;

	UPROPERTY()
	FString Desc;

};

/** Node for storing an auto-complete tree based on each char in the command */
USTRUCT()
struct FAutoCompleteNode
{
	GENERATED_USTRUCT_BODY()

	/** Char for node in the tree */
	UPROPERTY()
	int32 IndexChar;

	/** Indicies into AutoCompleteList for commands that match to this level */
	UPROPERTY()
	TArray<int32> AutoCompleteListIndices;



		/** Children for further matching */
		TArray<FAutoCompleteNode*> ChildNodes;

		FAutoCompleteNode()
		{
			IndexChar = INDEX_NONE;
		}
		FAutoCompleteNode(int32 NewChar)
		{
			IndexChar = NewChar;
		}
		~FAutoCompleteNode()
		{
			for (int32 Idx = 0; Idx < ChildNodes.Num(); Idx++)
			{
				FAutoCompleteNode *Node = ChildNodes[Idx];
				delete Node;
			}
			ChildNodes.Empty();
		}
	
};

UCLASS(Within=GameViewportClient, transient, config=Input, HeaderGroup=UserInterface)
class ENGINE_API UConsole : public UObject, public FOutputDevice
{
	GENERATED_UCLASS_BODY()

	/** The player which the next console command should be executed in the context of.  If NULL, execute in the viewport. */
	UPROPERTY()
	class ULocalPlayer* ConsoleTargetPlayer;

	UPROPERTY()
	class UTexture2D* DefaultTexture_Black;

	UPROPERTY()
	class UTexture2D* DefaultTexture_White;

	/**  Visible Console stuff */
	UPROPERTY(globalconfig, EditAnywhere, Category=General)
	int32 MaxScrollbackSize;

	/** Holds the scrollback buffer */
	TArray<FString> Scrollback;

	/**  Where in the scrollback buffer are we */
	int32 SBHead;

	int32 SBPos;

	/** index into the History array for the latest command that was entered */
	UPROPERTY(config)
	int32 HistoryTop;

	/** index into the History array for the earliest command that was entered */
	UPROPERTY(config)
	int32 HistoryBot;

	/** the index of the current position in the History array */
	UPROPERTY(config)
	int32 HistoryCur;

	/** tracks previously entered console commands */
	UPROPERTY(config)
	FString History[16];    /*MAX_HISTORY_ENTRIES @fixmeconst*/

	/** tracks whether the user is using arrows keys to navigate the history, so that auto-complete doesn't override */
	uint32 bNavigatingHistory:1;

	/** The command the user is currently typing. */
	FString TypedStr;

	int32 TypedStrPos;    //Current position in TypedStr

	/**
	 * Indicates that InputChar events should be captured to prevent them from being passed on to other interactions.  Reset
	 * when the another keydown event is received.
	 */
	uint32 bCaptureKeyInput:1;

	/** True while a control key is pressed. */
	uint32 bCtrl:1;

	/** Manual list of auto-complete commands and info specified in BaseInput.ini */
	UPROPERTY(config, EditAnywhere, Category=AutoComplete)
	TArray<struct FAutoCompleteCommand> ManualAutoCompleteList;

	/** List of relative paths (e.g. Content/Maps) to search for map names for auto-complete usage. Specified in BaseInput.ini. */
	UPROPERTY(config, EditAnywhere, Category=AutoComplete)
	TArray<FString> AutoCompleteMapPaths;

	/** Full list of auto-complete commands and info */
	TArray<struct FAutoCompleteCommand> AutoCompleteList;

	/** Is the current auto-complete selection locked */
	uint32 bAutoCompleteLocked:1;

	/** Currently selected auto complete index */
	int32 AutoCompleteIndex;

	/** Do we need to rebuild auto complete? */
	uint32 bIsRuntimeAutoCompleteUpToDate:1;

	FName ConsoleState;

	FAutoCompleteNode AutoCompleteTree;

	/** Current list of matching commands for auto-complete, @see UpdateCompleteIndices() */
	TArray<int32> AutoCompleteIndices;


	/** Max number of command history entries */
	static const int32 MAX_HISTORY_ENTRIES = 16;

	~UConsole();

	// UObject Interface
	virtual void PostInitProperties() OVERRIDE;

	/** Set the input to text */
	virtual void SetInputText( const FString& Text );

	/** Set cursor position for typing text */
	virtual void SetCursorPos( int32 Position );
	
	/**
	 * Executes a console command.
	 * @param Command - The command to execute.
	 */
	virtual void ConsoleCommand(const FString& Command);
	
	/**
	 * Clears the console output buffer.
	 */
	virtual void ClearOutput();
	
	/**
	 * Prints a (potentially multi-line) FString of text to the console.
	 * The text is split into separate lines and passed to OutputTextLine.
	 * @param Text - Text to display on the console.
	 */
	virtual void OutputText(const FString& Text);
	
	/**
	 * Opens the typing bar with text already entered.
	 * @param Text - The text to enter in the typing bar.
	 */
	virtual void StartTyping(const FString& Text);
	
	/**
	 * Clears out all pressed keys from the player's input object.
	 */
	virtual void FlushPlayerInput();
	
	/** looks for Control key presses and the copy/paste combination that apply to both the console bar and the full open console */
	virtual bool ProcessControlKey(FKey Key, EInputEvent Event);
	
	/** appends the specified text to the string of typed text */
	virtual void AppendInputText(const FString& Text);
	
	/** Build the list of auto complete console commands */
	virtual void BuildRuntimeAutoCompleteList(bool bForce = false);
	
	/** Update the auto complete indices from the typed string */
	virtual void UpdateCompleteIndices();
	
	/**
	 * Process a character input event (typing) routed through unrealscript from another object. This method is assigned as the value for the
	 * OnRecievedNativeInputKey delegate so that native input events are routed to this unrealscript function.
	 *
	 * @param	ControllerId	the controller that generated this character input event
	 * @param	Unicode			the character that was typed
	 *
	 * @return	True to consume the character, false to pass it on.
	 */
	virtual bool InputChar_Typing( int32 ControllerId, const FString& Unicode );
	
	/**
	 * Process an input key event routed through unrealscript from another object. This method is assigned as the value for the
	 * OnRecievedNativeInputKey delegate so that native input events are routed to this unrealscript function.
	 *
	 * @param	ControllerId	the controller that generated this input key event
	 * @param	Key				the key which for an event occurred for (EKeys::Up, EKeys::Down, etc.)
	 * @param	EventType		the type of event which occurred (pressed, released, etc.)
	 * @param	AmountDepressed	for analog keys, the depression percent.
	 *
	 * @return	true to consume the key event, false to pass it on.
	 */
	virtual bool InputKey_Typing( int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = false );
	
	/** perform rendering of the console on the canvas */
	virtual void PostRender_Console_Typing(class UCanvas* Canvas);
	
	/** Perform actions on transition to Typing state */
	virtual void BeginState_Typing(FName PreviousStateName);

	/** Perform actions on transition from Typing state */
	virtual void EndState_Typing( FName NextStateName );
	
	/**
	 * This state is used when the console is open.
	 */
	virtual bool InputChar_Open( int32 ControllerId, const FString& Unicode );
	
	/**
	 * Process an input key event routed through unrealscript from another object. This method is assigned as the value for the
	 * OnRecievedNativeInputKey delegate so that native input events are routed to this unrealscript function.
	 *
	 * @param	ControllerId	the controller that generated this input key event
	 * @param	Key				the name of the key which an event occured for (KEY_Up, KEY_Down, etc.)
	 * @param	EventType		the type of event which occured (pressed, released, etc.)
	 * @param	AmountDepressed	for analog keys, the depression percent.
	 *
	 * @return	true to consume the key event, false to pass it on.
	 */
	virtual bool InputKey_Open( int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = false );
	
	/** perform rendering of the console on the canvas */
	virtual void PostRender_Console_Open(class UCanvas* Canvas);


	/** Perform actions on transition to the Open state */
	virtual void BeginState_Open(FName PreviousStateName);

	virtual bool InputChar(int32 ControllerId, const FString& Unicode);
	virtual bool InputKey(int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed=1.f, bool bGamepad=false);
	virtual bool InputAxis(int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples=1, bool bGamepad=false) { return false; };
	virtual bool InputTouch(int32 ControllerId, uint32 Handle, ETouchType::Type Type, const FVector2D& TouchLocation, FDateTime DeviceTimestamp, uint32 TouchpadIndex) { return false; }

	/**
	 * Process an input key event routed through unrealscript from another object. This method is assigned as the value for the
	 * OnRecievedNativeInputKey delegate so that native input events are routed to this unrealscript function.
	 *
	 * @param	ControllerId	the controller that generated this input key event
	 * @param	Key				the name of the key which an event occured for (KEY_Up, KEY_Down, etc.)
	 * @param	EventType		the type of event which occured (pressed, released, etc.)
	 * @param	AmountDepressed	for analog keys, the depression percent.
	 *
	 * @return	true to consume the key event, false to pass it on.
	 */
	virtual bool InputKey_Global( int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = false );

	/** render to the canvas based on the console state */
	virtual void PostRender_Console(class UCanvas* Canvas);

	/** controls state transitions for the console */
	virtual void FakeGotoState(FName NextStateName);

	virtual bool ConsoleActive() const;

private:

	// interface FOutputDevice
	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category );
	// expose the base class other Serialize function (clang will give "error : 'UConsole::Serialize' hides overloaded virtual function" without this)
	using UObject::Serialize; 

	/**
	 * Prints a single line of text to the console.
	 * @param Text - A line of text to display on the console.
	 */
	void OutputTextLine(const FString& Text);
	
	/**
	* Searches console command history and removes any entries matching the specified command.
	* @param Command - The command to search for and purge from the history.
	*/
	virtual void PurgeCommandFromHistory(const FString& Command);
};



