// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// PlayerInput
// Object within playercontroller that manages player input.
// only spawned on client
//=============================================================================

#pragma once

#include "KeyState.h"
#include "GestureRecognizer.h"
#include "PlayerInput.generated.h"

ENGINE_API DECLARE_LOG_CATEGORY_EXTERN(LogInput, Log, All);


USTRUCT()
struct FKeyBind
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(config)
	FKey Key;

	UPROPERTY(config)
	FString Command;

	UPROPERTY(config)
	uint32 Control:1;

	UPROPERTY(config)
	uint32 Shift:1;

	UPROPERTY(config)
	uint32 Alt:1;

	/** if true, the bind will not be activated if the corresponding key is held down */
	UPROPERTY(config)
	uint32 bIgnoreCtrl:1;

	UPROPERTY(config)
	uint32 bIgnoreShift:1;

	UPROPERTY(config)
	uint32 bIgnoreAlt:1;

	FKeyBind() 
	{
	}
};

/** Configurable properties for control axes, used to turn raw input value into game-useful values. */
USTRUCT()
struct FInputAxisProperties
{
	GENERATED_USTRUCT_BODY()

	/** For control axes such as analog sticks. */
	UPROPERTY(EditAnywhere, Category="Input")
	float DeadZone;

	/** Scaling factor. */
	UPROPERTY(EditAnywhere, Category="Input")
	float Sensitivity;

	/** For applying curves to [0..1] axes, e.g. analog sticks */
	UPROPERTY(EditAnywhere, Category="Input")
	float Exponent;

	/** Inverts reported values for this axis */
	UPROPERTY(EditAnywhere, Category="Input")
	uint32 bInvert:1;

	FInputAxisProperties()
		: DeadZone(0.2f)
		, Sensitivity(1.f)
		, Exponent(1.f)
		, bInvert(false)
	{}
	
};

/** Data structure exposed to defaultproperties/inis for defining axis properties. */
USTRUCT()
struct FInputAxisConfigEntry
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, Category="Input")
	FName AxisKeyName;

	UPROPERTY(EditAnywhere, Category="Input")
	struct FInputAxisProperties AxisProperties;
};

/** Defines a mapping between an action and key */
USTRUCT()
struct FInputActionKeyMapping
{
	GENERATED_USTRUCT_BODY()

	/** Friendly name of action, e.g "jump" */
	UPROPERTY(EditAnywhere, Category="Input")
	FName ActionName;

	/** Key to bind it to. */
	UPROPERTY(EditAnywhere, Category="Input")
	FKey Key;

	/** true if one of the Shift keys must be down when the KeyEvent is received to be acknowledged */
	UPROPERTY(EditAnywhere, Category="Input")
	uint32 bShift:1;

	/** true if one of the Ctrl keys must be down when the KeyEvent is received to be acknowledged */
	UPROPERTY(EditAnywhere, Category="Input")
	uint32 bCtrl:1;

	/** true if one of the Alt keys must be down when the KeyEvent is received to be acknowledged */
	UPROPERTY(EditAnywhere, Category="Input")
	uint32 bAlt:1;

	bool operator==(const FInputActionKeyMapping& Other) const
	{
		return (   ActionName == Other.ActionName
				&& Key == Other.Key
				&& bShift == Other.bShift
				&& bCtrl == Other.bCtrl
				&& bAlt == Other.bAlt);
	}

	bool operator<(const FInputActionKeyMapping& Other) const
	{
		bool bResult = false;
		if (ActionName < Other.ActionName)
		{
			bResult = true;
		}
		else if (ActionName == Other.ActionName)
		{
			bResult = (Key < Other.Key);
		}
		return bResult;
	}

	FInputActionKeyMapping(const FName InActionName = NAME_None, const FKey InKey = EKeys::Invalid, const bool bInShift = false, const bool bInCtrl = false, const bool bInAlt = false)
		: ActionName(InActionName)
		, Key(InKey)
		, bShift(bInShift)
		, bCtrl(bInCtrl)
		, bAlt(bInAlt)
	{}
};

/** Defines a mapping between an action and key */
USTRUCT()
struct FInputAxisKeyMapping
{
	GENERATED_USTRUCT_BODY()

	/** Friendly name of axis, e.g "MoveForward" */
	UPROPERTY(EditAnywhere, Category="Input")
	FName AxisName;

	/** Key to bind it to. */
	UPROPERTY(EditAnywhere, Category="Input")
	FKey Key;

	/** Multiplier to use for the mapping when accumulating the axis value */
	UPROPERTY(EditAnywhere, Category="Input")
	float Scale;

	bool operator==(const FInputAxisKeyMapping& Other) const
	{
		return (   AxisName == Other.AxisName
				&& Key == Other.Key
				&& Scale == Other.Scale);
	}

	bool operator<(const FInputAxisKeyMapping& Other) const
	{
		bool bResult = false;
		if (AxisName < Other.AxisName)
		{
			bResult = true;
		}
		else if (AxisName == Other.AxisName)
		{
			if (Key < Other.Key)
			{
				bResult = true;
			}
			else if (Key == Other.Key)
			{
				bResult = (Scale < Other.Scale);

			}
		}
		return bResult;
	}

	FInputAxisKeyMapping(const FName InAxisName = NAME_None, const FKey InKey = EKeys::Invalid, const float InScale = 1.f)
		: AxisName(InAxisName)
		, Key(InKey)
		, Scale(InScale)
	{}
};

struct FActionKeyDetails
{
	TArray<FInputActionKeyMapping> Actions;
	FInputChord CapturingChord;
};

struct FAxisKeyDetails
{
	TArray<FInputAxisKeyMapping> KeyMappings;
	uint32 bInverted:1;

	FAxisKeyDetails()
		: bInverted(false)
	{
	}
};

UCLASS(Within=PlayerController, config=Input, dependsOn=UEngineTypes, transient, HeaderGroup=UserInterface)
class ENGINE_API UPlayerInput : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	// NOTE: These touch vectors are calculated and set directly, they do not go through the .ini Bindings
	// Touch locations, from 0..1 (0,0 is top left, 1,1 is bottom right), the Z component is > 0 if the touch is currently held down
	// @todo: We have 10 touches to match the number of Touch* entries in EKeys (not easy to make this an enum or define or anything)
	FVector Touches[EKeys::NUM_TOUCH_KEYS];

	// Zoom Scaling
	UPROPERTY()
	uint32 bEnableFOVScaling:1;

	// Mouse smoothing sample data
	float ZeroTime[2];    /** How long received mouse movement has been zero. */
	float SmoothedMouse[2];    /** Current average mouse movement/sample */
	int32 MouseSamples;    /** Number of mouse samples since mouse movement has been zero */
	float MouseSamplingTotal;    /** DirectInput's mouse sampling total time */


protected:
	TEnumAsByte<EInputEvent> CurrentEvent;

public:
	/** Generic bindings of keys to Exec()-compatible strings for development purposes only */
	UPROPERTY(config)
	TArray<struct FKeyBind> DebugExecBindings;

	/** This player's version of the Action Mappings */
	TArray<struct FInputActionKeyMapping> ActionMappings;

	/** This player's version of Axis Mappings */
	TArray<struct FInputAxisKeyMapping> AxisMappings;

	UPROPERTY(config)
	TArray<FName> InvertedAxis;

	//=============================================================================
	// Input related functions.
	UFUNCTION(exec)
	void SetMouseSensitivity(float F);

	UFUNCTION(exec)
	void SetMouseSensitivityToDefault();

#if !UE_BUILD_SHIPPING
	UFUNCTION(exec)
	void SetBind(FName BindName, const FString& Command);
#endif

	/** Returns the mouse sensitivity along the X-axis, or the Y-axis, or 1.0 if none are known. */
	float GetMouseSensitivity();

	bool GetInvertAxisKey(const FKey AxisKey);
	bool GetInvertAxis(const FName AxisName);

	UFUNCTION(exec)
	void InvertAxisKey(const FKey AxisKey);

	// Backwards compatibility exec function for people used to it instead of using InvertAxisKey
	UFUNCTION(exec)
	void InvertMouse();

	UFUNCTION(exec)
	void InvertAxis(const FName AxisName);

	//*************************************************************************************
	// Normal gameplay execs
	// Type the name of the exec function at the console to execute it

	// Mouse smoothing
	UFUNCTION(exec)
	void ClearSmoothing();

	void AddActionMapping(const FInputActionKeyMapping& KeyMapping);
	void RemoveActionMapping(const FInputActionKeyMapping& KeyMapping);

	void AddAxisMapping(const FInputAxisKeyMapping& KeyMapping);
	void RemoveAxisMapping(const FInputAxisKeyMapping& KeyMapping);

	static void AddEngineDefinedActionMapping(const FInputActionKeyMapping& ActionMapping);
	static void AddEngineDefinedAxisMapping(const FInputAxisKeyMapping& AxisMapping);

	void ForceRebuildingKeyMaps(const bool bRestoreDefaults = false);

protected:
	/** Internal structure for storing axis config data. */
	static TMap<FKey,FInputAxisProperties> AxisProperties;

	TMap<FName,FActionKeyDetails> ActionKeyMap;
	TMap<FName,FAxisKeyDetails> AxisKeyMap;

	TMap<FKey,FKeyState> KeyStateMap;

	struct FDelegateDispatchDetails
	{
		uint32 EventIndex;
		uint32 bTouchDelegate:1;

		FInputActionUnifiedDelegate ActionDelegate;
		const FInputActionBinding* SourceAction;
		FInputChord Chord;
		TEnumAsByte<EInputEvent> KeyEvent;

		FInputTouchUnifiedDelegate TouchDelegate;
		FVector TouchLocation;
		ETouchIndex::Type FingerIndex;

		FInputGestureUnifiedDelegate GestureDelegate;
		float GestureValue;

		FDelegateDispatchDetails(const uint32 InEventIndex, const FInputChord& InChord, const FInputActionUnifiedDelegate& InDelegate, const EInputEvent InKeyEvent, const FInputActionBinding* InSourceAction = NULL)
			: EventIndex(InEventIndex)
			, bTouchDelegate(false)
			, ActionDelegate(InDelegate)
			, SourceAction(InSourceAction)
			, Chord(InChord)
			, KeyEvent(InKeyEvent)
		{}

		FDelegateDispatchDetails(const uint32 InEventIndex, const FInputTouchUnifiedDelegate& InDelegate, const FVector InLocation, const ETouchIndex::Type InFingerIndex)
			: EventIndex(InEventIndex)
			, bTouchDelegate(true)
			, TouchDelegate(InDelegate)
			, TouchLocation(InLocation)
			, FingerIndex(InFingerIndex)
		{}

		FDelegateDispatchDetails(const FInputGestureUnifiedDelegate& InDelegate, const float InValue)
			: GestureDelegate(InDelegate)
			, GestureValue(InValue)
		{}
	};

public:
	
	virtual void PostInitProperties() OVERRIDE;

	void FlushPressedKeys();
	bool InputKey(FKey Key, enum EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = false );
	bool InputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples=1, bool bGamepad = false);
	bool InputTouch(uint32 Handle, ETouchType::Type Type, const FVector2D& TouchLocation, FDateTime DeviceTimestamp, uint32 TouchpadIndex);
	bool InputMotion(const FVector& Tilt, const FVector& RotationRate, const FVector& Gravity, const FVector& Acceleration);
	bool InputGesture(const FKey Gesture, const EInputEvent Event, const float Value);
	void Tick(float DeltaTime);

	/** Process the player's input. */
	void ProcessInputStack(const TArray<UInputComponent*>& InputComponentStack, const float DeltaTime, const bool bGamePaused);

	/** Rather than processing input, consume it and discard without doing anything useful with it.  Like calling PlayerInput() and ignoring all results. */
	void DiscardPlayerInput();

	/** 
	Smooth mouse movement, because mouse sampling doesn't match up with tick time.
	 * @note: if we got sample event for zero mouse samples (so we
				didn't have to guess whether a 0 was caused by no sample occuring during the tick (at high frame rates) or because the mouse actually stopped)
	 * @param: aMouse is the mouse axis movement received from DirectInput
	 * @param: DeltaTime is the tick time
	 * @param: SampleCount is the number of mouse samples received from DirectInput
	 * @param: Index is 0 for X axis, 1 for Y axis
	 * @return the smoothed mouse axis movement
	 */
	float SmoothMouse(float aMouse, float DeltaTime, uint8& SampleCount, int32 Index);

	/** Hook to do mouse acceleration if desired. */
	float AccelMouse(FKey Key, float RawValue, float DeltaTime);

	/**
	 * Draw important PlayerInput variables on canvas.  HUD will call DisplayDebug() on the current ViewTarget when the ShowDebug exec is used
	 *
	 * @param Canvas - Canvas to draw on
	 * @param DebugDisplay - List of names specifying what debug info to display
	 * @param YL - Height of the current font
	 * @param YPos - Y position on Canvas. YPos += YL, gives position to draw text for next debug line.
	 */
	void DisplayDebug(class UCanvas* Canvas, const TArray<FName>& DebugDisplay, float& YL, float& YPos);

	bool IsPressed( FKey InKey ) const;
	
	/** @return true if InKey went from up to down since player input was last processed. */
	bool WasJustPressed( FKey InKey ) const;

	/** return true if InKey went from down to up since player input was last processed. */
	bool WasJustReleased( FKey InKey ) const;

	/** @return how long the key has been held down, or 0.f if not down. */
	float GetTimeDown( FKey InKey ) const;

	/** @return current state of the InKey */
	float GetKeyValue( FKey InKey ) const;

	/** @return current state of the InKey */
	float GetRawKeyValue( FKey InKey ) const;

	/** @return current state of the InKey */
	FVector GetVectorKeyValue( FKey InKey ) const;

	/** @return true if alt key is pressed */
	bool IsAltPressed() const;

	/** @return true if ctrl key is pressed */
	bool IsCtrlPressed() const;

	/** @return true if shift key is pressed */
	bool IsShiftPressed() const;

#if !UE_BUILD_SHIPPING
	/**
	 * Exec handler
	 *
	 * @param	InWorld	World context
	 * @param	Cmd		Command to parse
	 * @param	Ar		Output device to log to
	 *
	 * @return	true if command was handled, false otherwise
	 */
	bool Exec(UWorld* UInWorld, const TCHAR* Cmd,FOutputDevice& Ar);

	/** @todo document */
	FString GetBind(FKey Key);

	/** 
	* Get the Exec key binding for the given command. 
	* @param	ExecCommand		Command to find the key binding for
	*
	* @return	The key binding for the command.
	*/
	FKeyBind GetExecBind(FString const& ExecCommand);

	/** Execute input commands. */
	void ExecInputCommands( UWorld* InWorld, const TCHAR* Cmd,class FOutputDevice& Ar);
#endif

	const TArray<FInputActionKeyMapping>& GetKeysForAction(const FName ActionName);

	class UWorld* GetWorld() const;

protected:
	/** 
	 * Given raw keystate value, returns the "massaged" value. Override for any custom behavior,
	 * such as input changes dependent on a particular game state.
 	 */
	float MassageAxisInput(FKey Key, float RawValue, float DeltaTime);

	/** Process non-axes keystates */
	void ProcessNonAxesKeys(FKey Inkey, FKeyState* KeyState, float DeltaTime);
	
	// finished processing input for this frame, clean up for next update
	void FinishProcessingPlayerInput();

	/** key event processing
	 * @param Key - name of key causing event
	 * @param Event - types of event, includes IE_Pressed
	 * @return true if just pressed
	 */
	bool KeyEventOccurred(FKey Key, EInputEvent Event, TArray<uint32>& EventIndices) const;

	/* Collects the chords and the delegates they invoke
	 * @param ActionBinding - the action to determine whether it occurred
	 * @param FoundChords - the list of chord/delegate pairs to add to
	 * @param KeysToConsume - optional array to collect the keys associated with this binding if the action occurred
	 */
	void GetChordsForAction(const FInputActionBinding& ActionBinding, const bool bGamePaused, TArray<FDelegateDispatchDetails>& FoundChords, TArray<FKey>& KeysToConsume);

	void GetChordForKey(const FInputKeyBinding& KeyBinding, const bool bGamePaused, TArray<FDelegateDispatchDetails>& FoundChords, TArray<FKey>& KeysToConsume);

	/* Returns the summed values of all the components of this axis this frame
	 * @param AxisBinding - the action to determine if it ocurred
	 * @param KeysToConsume - optional array to collect the keys associated with this binding
	 */
	float DetermineAxisValue(const FInputAxisBinding& AxisBinding, const bool bGamePaused, TArray<FKey>& KeysToConsume);

	void ConditionalBuildKeyMappings();

	/** Set Key to be ignored */
	void ConsumeKey(FKey Key);

	/** @return true if InKey is being consumed */
	bool IsKeyConsumed(FKey Key) const;

	/** Initialized axis properties (i.e deadzone values) if needed */
	void ConditionalInitAxisProperties();

	/** @return True if a key is handled by an action binding */
	bool IsKeyHandledByAction( FKey Key ) const;

	/** Gesture recognizer object */
	// @todo: Move this up to Slate?
	FGestureRecognizer GestureRecognizer;
	friend FGestureRecognizer;

private:
	static const TArray<FInputActionKeyMapping> NoKeyMappings;

	static TArray<FInputActionKeyMapping> EngineDefinedActionMappings;
	static TArray<FInputAxisKeyMapping> EngineDefinedAxisMappings;

	// A counter used to track the order in which events occurred since the last time the input stack was processed
	uint32 EventCount;
};
