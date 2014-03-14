// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ActorComponent.h"
#include "InputComponent.generated.h"

template<class DelegateType, class DynamicDelegateType>
struct TInputUnifiedDelegate
{
	TInputUnifiedDelegate() {};
	TInputUnifiedDelegate(DelegateType const& D) : FuncDelegate(D) {};
	TInputUnifiedDelegate(DynamicDelegateType const& D) : FuncDynDelegate(D) {};

	inline bool IsBound() const
	{
		return ( FuncDelegate.IsBound() || FuncDynDelegate.IsBound() );
	}

	inline bool IsBoundToObject(void const* Object) const
	{
		if (FuncDelegate.IsBound())
		{
			return FuncDelegate.IsBoundToObject(Object);
		}
		else if (FuncDynDelegate.IsBound())
		{
			return FuncDynDelegate.IsBoundToObject(Object);
		}

		return false;
	}

	template< class UserClass >
	inline void BindDelegate(UserClass* Object, typename DelegateType::template TUObjectMethodDelegate< UserClass >::FMethodPtr Func)
	{
		FuncDynDelegate.Unbind();
		FuncDelegate.BindUObject(Object, Func);
	}

	inline void BindDelegate(UObject* Object, const FName FuncName)
	{
		FuncDelegate.Unbind();
		FuncDynDelegate.SetObject(Object);
		FuncDynDelegate.SetFunctionName(FuncName);
	}

	inline void Unbind()
	{
		FuncDelegate.Unbind();
		FuncDynDelegate.Unbind();
	}

	inline bool operator==(const TInputUnifiedDelegate<DelegateType, DynamicDelegateType>& Other) const
	{
		return ( FuncDelegate.IsBound() && (FuncDelegate == Other.FuncDelegate) ) || ( FuncDynDelegate.IsBound() && (FuncDynDelegate == Other.FuncDynDelegate) );
	}

protected:
	/** Holds the delegate to call. */
	DelegateType FuncDelegate;
	/** Holds the dynamic delegate to call. */
	DynamicDelegateType FuncDynDelegate;
};

USTRUCT()
struct FInputChord
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FKey Key;

	UPROPERTY()
	uint32 bShift:1;

	UPROPERTY()
	uint32 bCtrl:1;

	UPROPERTY()
	uint32 bAlt:1;

	enum RelationshipType
	{
		None,
		Same,
		Masked,
		Masks
	};

	RelationshipType GetRelationship(const FInputChord& OtherChord) const;

	FInputChord()
		: bShift(false)
		, bCtrl(false)
		, bAlt(false)
	{
	}

	FInputChord(const FKey InKey, const bool bInShift, const bool bInCtrl, const bool bInAlt)
		: Key(InKey)
		, bShift(bInShift)
		, bCtrl(bInCtrl)
		, bAlt(bInAlt)
	{
	}

	bool operator==(const FInputChord& Other) const
	{
		return (   Key == Other.Key
				&& bShift == Other.bShift
				&& bCtrl == Other.bCtrl
				&& bAlt == Other.bAlt);
	}
};

/** Delegate signature for action handlers. */
DECLARE_DELEGATE( FInputActionHandlerSignature );
DECLARE_DYNAMIC_DELEGATE( FInputActionHandlerDynamicSignature );

struct FInputActionUnifiedDelegate : public TInputUnifiedDelegate<FInputActionHandlerSignature, FInputActionHandlerDynamicSignature>
{
	inline void Execute() const
	{
		if (FuncDelegate.IsBound())
		{
			FuncDelegate.Execute();
		}
		else if (FuncDynDelegate.IsBound())
		{
			FuncDynDelegate.Execute();
		}
	}
};

/** Binds a game-defined action. */
struct FInputActionBinding
{
	/** Friendly name of action, e.g "jump" */
	FName ActionName;

	/** Key event to bind it to, e.g. pressed, released, dblclick */
	TEnumAsByte<EInputEvent> KeyEvent;

	/** Whether the binding should consume the input or allow it to pass to another component */
	uint32 bConsumeInput:1;

	/** Whether the binding should execute while paused */
	uint32 bExecuteWhenPaused:1;

	/** Whether the binding is part of a paired (both pressed and released events bound) action */
	uint32 bPaired:1;

	FInputActionUnifiedDelegate ActionDelegate;

	FInputActionBinding()
		: ActionName(NAME_None)
		, KeyEvent(EInputEvent::IE_Pressed)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
		, bPaired(false)
	{}

	FInputActionBinding(const FName InActionName, const enum EInputEvent InKeyEvent)
		: ActionName(InActionName)
		, KeyEvent(InKeyEvent)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
		, bPaired(false)
	{}
};

struct FInputKeyBinding
{
	/** Raw key to bind to */
	FInputChord Chord;

	/** Key event to bind it to, e.g. pressed, released, dblclick */
	TEnumAsByte<EInputEvent> KeyEvent;

	uint32 bConsumeInput:1;

	uint32 bExecuteWhenPaused:1;

	FInputActionUnifiedDelegate KeyDelegate;

	FInputKeyBinding()
		: KeyEvent(EInputEvent::IE_Pressed)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
	{}

	FInputKeyBinding(const FInputChord InChord, const enum EInputEvent InKeyEvent)
		: Chord(InChord)
		, KeyEvent(InKeyEvent)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
	{}
};

/** 
 * Delegate signature for touch handlers. 
 * @FigerIndex: Which finger touched
 * @Location: The 2D screen location that was touched
 */
DECLARE_DELEGATE_TwoParams( FInputTouchHandlerSignature, ETouchIndex::Type, FVector );
DECLARE_DYNAMIC_DELEGATE_TwoParams( FInputTouchHandlerDynamicSignature, ETouchIndex::Type, FingerIndex, FVector, Location );

struct FInputTouchUnifiedDelegate : public TInputUnifiedDelegate<FInputTouchHandlerSignature, FInputTouchHandlerDynamicSignature>
{
	inline void Execute(const ETouchIndex::Type FingerIndex, const FVector Location) const
	{
		if (FuncDelegate.IsBound())
		{
			FuncDelegate.Execute(FingerIndex, Location);
		}
		else if (FuncDynDelegate.IsBound())
		{
			FuncDynDelegate.Execute(FingerIndex, Location);
		}
	}
};


struct FInputTouchBinding
{
	/** Key event to bind it to, e.g. pressed, released, dblclick */
	TEnumAsByte<EInputEvent> KeyEvent;

	uint32 bConsumeInput:1;

	uint32 bExecuteWhenPaused:1;

	FInputTouchUnifiedDelegate TouchDelegate;

	FInputTouchBinding()
		: KeyEvent(EInputEvent::IE_Pressed)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
	{}

	FInputTouchBinding(const enum EInputEvent InKeyEvent)
		: KeyEvent(InKeyEvent)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
	{}
};

/** 
 * Delegate signature for axis handlers. 
 * @AxisValue: "Value" to pass to the axis.  This value will be the device-dependent, so a mouse will report absolute change since the last update, 
 *		a joystick will report total displacement from the center, etc.  It is up to the handler to interpret this data as it sees fit, i.e. treating 
 *		joystick values as a rate of change would require scaling by frametime to get an absolute delta.
 */
DECLARE_DELEGATE_OneParam( FInputAxisHandlerSignature, float );
DECLARE_DYNAMIC_DELEGATE_OneParam( FInputAxisHandlerDynamicSignature, float, AxisValue );

struct FInputAxisUnifiedDelegate : public TInputUnifiedDelegate<FInputAxisHandlerSignature, FInputAxisHandlerDynamicSignature>
{
	inline void Execute(const float AxisValue) const
	{
		if (FuncDelegate.IsBound())
		{
			FuncDelegate.Execute(AxisValue);
		}
		else if (FuncDynDelegate.IsBound())
		{
			FuncDynDelegate.Execute(AxisValue);
		}
	}
};


/** Binds a game-defined axis to a function */
struct FInputAxisBinding
{
	FName AxisName;

	FInputAxisUnifiedDelegate AxisDelegate;

	uint32 bConsumeInput:1;

	uint32 bExecuteWhenPaused:1;

	float AxisValue;

	FInputAxisBinding()
		: AxisName(NAME_None)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
		, AxisValue(0.f)
	{}

	FInputAxisBinding(const FName InAxisName)
		: AxisName(InAxisName)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
		, AxisValue(0.f)
	{}
};

/** Binds a raw axis to a function */
struct FInputAxisKeyBinding
{
	FKey AxisKey;

	FInputAxisUnifiedDelegate AxisDelegate;

	uint32 bConsumeInput : 1;

	uint32 bExecuteWhenPaused : 1;

	float AxisValue;

	FInputAxisKeyBinding()
		: bConsumeInput(true)
		, bExecuteWhenPaused(false)
		, AxisValue(0.f)
	{}

	FInputAxisKeyBinding(const FKey InAxisKey)
		: AxisKey(InAxisKey)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
		, AxisValue(0.f)
	{}
};


/** 
 * Delegate signature for gesture handlers. 
 * @Value: "Value" to pass to the axis.  Note that by convention this is assumed to be a framerate-independent "delta" value, i.e. absolute change for this frame
 *				so the handler need not scale by frametime.
 */
DECLARE_DELEGATE_OneParam( FInputGestureHandlerSignature, float );
DECLARE_DYNAMIC_DELEGATE_OneParam( FInputGestureHandlerDynamicSignature, float, Value );

struct FInputGestureUnifiedDelegate : public TInputUnifiedDelegate<FInputGestureHandlerSignature, FInputGestureHandlerDynamicSignature>
{
	inline void Execute(const float Value) const
	{
		if (FuncDelegate.IsBound())
		{
			FuncDelegate.Execute(Value);
		}
		else if (FuncDynDelegate.IsBound())
		{
			FuncDynDelegate.Execute(Value);
		}
	}
};

/** Binds a gesture to a function. */
struct FInputGestureBinding
{
	FKey GestureKey;

	FInputGestureUnifiedDelegate GestureDelegate;

	uint32 bConsumeInput : 1;

	uint32 bExecuteWhenPaused : 1;

	/** Value parameter, meaning is dependent on the gesture. */
	float GestureValue;

	FInputGestureBinding()
		: bConsumeInput(true)
		, bExecuteWhenPaused(false)
		, GestureValue(0.f)
	{}

	FInputGestureBinding(const FKey InGestureKey)
		: GestureKey(InGestureKey)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
		, GestureValue(0.f)

	{}
};

// -----------------------------------------------------------------
// Helper macros to make setting up axis bindings more readable in native code. 
//

/** Creates and adds a new axis binding. */
#define BIND_AXIS(Comp, AxisName, Delegate)	\
	Comp->BindAxis(AxisName, this, Delegate)

/** Creates and adds a new action binding. */
#define BIND_ACTION(Comp, ActionName, KeyEvent, Delegate)	\
	Comp->BindAction(ActionName, KeyEvent, this, Delegate);

/** Creates and adds a new axis binding. */
#define BIND_AXIS_OBJ(Comp, AxisName, Delegate, DelegateObj)	\
	Comp->BindAxis(AxisName, DelegateObj, Delegate)

/** Same as BIND_ACTION, except can be used to bind to a delegate in a specific object. */
#define BIND_ACTION_OBJ(Comp, ActionName, KeyEvent, Delegate, DelegateObj)	\
	Comp->BindAction(ActionName, KeyEvent, DelegateObj, Delegate);


UENUM()
namespace EControllerAnalogStick
{
	enum Type
	{
		CAS_LeftStick,
		CAS_RightStick,
		CAS_MAX
	};
}

UCLASS(transient, HeaderGroup=Component, config=Input, dependson=UPlayerInput, hidecategories=(Activation, "Components|Activation"))
class ENGINE_API UInputComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

private:
	TArray<struct FInputActionBinding> ActionBindings;

public:
	TArray<struct FInputKeyBinding> KeyBindings;
	TArray<struct FInputTouchBinding> TouchBindings;
	TArray<struct FInputAxisBinding> AxisBindings;
	TArray<struct FInputAxisKeyBinding> AxisKeyBindings;
	TArray<struct FInputGestureBinding> GestureBindings;

	// Whether any components lower on the input stack should be allowed to receive input
	uint32 bBlockInput:1;

	float GetAxisValue(const FName AxisName) const;
	float GetAxisKeyValue(const FKey AxisKey) const;

	bool HasBindings() const;

	template<class UserClass>
	FInputActionBinding& BindAction(const FName ActionName, const EInputEvent KeyEvent, UserClass* Object, typename FInputActionHandlerSignature::TUObjectMethodDelegate< UserClass >::FMethodPtr Func)
	{
		FInputActionBinding AB( ActionName, KeyEvent );
		AB.ActionDelegate.BindDelegate(Object, Func);
		return AddActionBinding(AB);
	}

	template<class UserClass>
	FInputAxisBinding& BindAxis(const FName AxisName, UserClass* Object, typename FInputAxisHandlerSignature::TUObjectMethodDelegate< UserClass >::FMethodPtr Func)
	{
		FInputAxisBinding AB( AxisName );
		AB.AxisDelegate.BindDelegate(Object, Func);
		AxisBindings.Add(AB);
		return AxisBindings.Last();
	}

	template<class UserClass>
	FInputAxisKeyBinding& BindAxisKey(const FKey AxisKey, UserClass* Object, typename FInputAxisHandlerSignature::TUObjectMethodDelegate< UserClass >::FMethodPtr Func)
	{
		FInputAxisKeyBinding AB(AxisKey);
		AB.AxisDelegate.BindDelegate(Object, Func);
		AxisKeyBindings.Add(AB);
		return AxisKeyBindings.Last();
	}

	template<class UserClass>
	FInputKeyBinding& BindKey(const FInputChord Chord, const EInputEvent KeyEvent, UserClass* Object, typename FInputActionHandlerSignature::TUObjectMethodDelegate< UserClass >::FMethodPtr Func)
	{
		FInputKeyBinding KB(Chord, KeyEvent);
		KB.KeyDelegate.BindDelegate(Object, Func);
		KeyBindings.Add(KB);
		return KeyBindings.Last();
	}

	template<class UserClass>
	FInputKeyBinding& BindKey(const FKey Key, const EInputEvent KeyEvent, UserClass* Object, typename FInputActionHandlerSignature::TUObjectMethodDelegate< UserClass >::FMethodPtr Func)
	{
		return BindKey(FInputChord(Key, false, false, false), KeyEvent, Object, Func);
	}

	template<class UserClass>
	FInputTouchBinding& BindTouch(const EInputEvent KeyEvent, UserClass* Object, typename FInputTouchHandlerSignature::TUObjectMethodDelegate< UserClass >::FMethodPtr Func)
	{
		FInputTouchBinding TB(KeyEvent);
		TB.TouchDelegate.BindDelegate(Object, Func);
		TouchBindings.Add(TB);
		return TouchBindings.Last();
	}

	template<class UserClass>
	FInputGestureBinding& BindGesture(const FKey GestureKey, UserClass* Object, typename FInputGestureHandlerSignature::TUObjectMethodDelegate< UserClass >::FMethodPtr Func)
	{
		FInputGestureBinding GB(GestureKey);
		GB.GestureDelegate.BindDelegate(Object, Func);
		GestureBindings.Add(GB);
		return GestureBindings.Last();
	}

	FInputActionBinding& AddActionBinding(const FInputActionBinding& Binding);
	void RemoveActionBinding(const int32 BindingIndex);
	int32 GetNumActionBindings() const;
	FInputActionBinding& GetActionBinding(const int32 BindingIndex);

private:
	/** Returns true if the given key/button is pressed on the input of the controller (if present) */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage="Use PlayerController.IsInputKeyDown instead."))
	bool IsControllerKeyDown(FKey Key) const;

	/** Returns true if the given key/button was up last frame and down this frame. */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage="Use PlayerController.WasInputKeyJustPressed instead."))
	bool WasControllerKeyJustPressed(FKey Key) const;

	/** Returns true if the given key/button was down last frame and up this frame. */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage="Use PlayerController.WasInputKeyJustReleased instead."))
	bool WasControllerKeyJustReleased(FKey Key) const;

	/** Returns the analog value for the given key/button.  If analog isn't supported, returns 1 for down and 0 for up. */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage="Use PlayerController.GetInputAnalogKeyState instead."))
	float GetControllerAnalogKeyState(FKey Key) const;

	/** Returns the vector value for the given key/button. */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage="Use PlayerController.GetInputVectorKeyState instead."))
	FVector GetControllerVectorKeyState(FKey Key) const;

	/** Returns the location of a touch, and if it's held down */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage="Use PlayerController.GetInputTouchState instead."))
	void GetTouchState(int32 FingerIndex, float& LocationX, float& LocationY, bool& bIsCurrentlyPressed) const;

	/** Returns how long the given key/button has been down.  Returns 0 if it's up or it just went down this frame. */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage="Use PlayerController.GetInputKeyTimeDown instead."))
	float GetControllerKeyTimeDown(FKey Key) const;

	/** Retrieves how far the mouse moved this frame. */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage="Use PlayerController.GetInputMouseDelta instead."))
	void GetControllerMouseDelta(float& DeltaX, float& DeltaY) const;

	/** Retrieves the X and Y displacement of the given analog stick.  For WhickStick, 0 = left, 1 = right. */
	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage="Use PlayerController.GetInputAnalogStickState instead."))
	void GetControllerAnalogStickState(EControllerAnalogStick::Type WhichStick, float& StickX, float& StickY) const;
};
