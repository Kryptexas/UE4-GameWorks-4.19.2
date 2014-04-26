// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	InputGesture.h: Declares the FInputGesture class.
=============================================================================*/

#pragma once


/**
 * Enumerates available modifier keys for input gestures.
 */
namespace EModifierKey
{
	typedef uint8 Type;

	/** No key. */
	const Type None	= 0;

	/** Ctrl key. */
	const Type Control = 1 << 0;

	/** Alt key. */
	const Type Alt = 1 << 1;

	/** Shift key. */
	const Type Shift = 1 << 2;
};


/**
 * Raw input gesture that defines input that must be valid when           
 */
struct SLATE_API FInputGesture
{
	/** Key that must be pressed */
	FKey Key;

	/** True if control must be pressed */
	uint32 bCtrl:1;
	
	/** True if alt must be pressed */
	uint32 bAlt:1;
	
	/** True if shift must be pressed */
	uint32 bShift:1;

public:

	/**
	 * Default constructor.
	 */
	FInputGesture( )
	{
		bCtrl = bAlt = bShift = 0;
	}

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InModifierKeys
	 * @param InKey
	 */
	FInputGesture( EModifierKey::Type InModifierKeys, const FKey InKey )
		: Key(InKey)
	{
		bCtrl = (InModifierKeys & EModifierKey::Control) != 0;
		bAlt = (InModifierKeys & EModifierKey::Alt) != 0;
		bShift = (InModifierKeys & EModifierKey::Shift) != 0;
	}

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InKey
	 */
	FInputGesture( const FKey InKey )
		: Key(InKey)
	{
		bCtrl = bAlt = bShift = 0;
	}

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InKey
	 * @param InModifierKeys
	 */
	FInputGesture( const FKey InKey, EModifierKey::Type InModifierKeys )
		: Key(InKey)
	{
		bCtrl = (InModifierKeys & EModifierKey::Control) != 0;
		bAlt = (InModifierKeys & EModifierKey::Alt) != 0;
		bShift = (InModifierKeys & EModifierKey::Shift) != 0;
	}

	/**
	 * Copy constructor.
	 *
	 * @param Other
	 */
	FInputGesture( const FInputGesture& Other )
		: Key(Other.Key)
		, bCtrl(Other.bCtrl)
		, bAlt(Other.bAlt)
		, bShift(Other.bShift)
	{ }

public:

	/**
	 * Compares this input gesture with another for equality.
	 *
	 * @param Other The other gesture to compare with.
	 *
	 * @return true if the gestures are equal, false otherwise.
	 */
	bool operator!=( const FInputGesture& Other ) const
	{
		return (Key != Other.Key) || (bCtrl != Other.bCtrl) || (bAlt != Other.bAlt) || (bShift != Other.bShift);
	}

	/**
	 * Compares this input gesture with another for inequality.
	 *
	 * @param Other The other gesture to compare with.
	 *
	 * @return true if the gestures are not equal, false otherwise.
	 */
	bool operator==( const FInputGesture& Other ) const
	{
		return (Key == Other.Key) && (bCtrl == Other.bCtrl) && (bAlt == Other.bAlt) && (bShift == Other.bShift);
	}

public:

	/** 
	 * Gets a localized string that represents the gesture.
	 *
	 * @return A localized string.
	 */
	FText GetInputText( ) const;
	
	/**
	 * Gets the key represented as a localized string.
	 *
	 * @return A localized string.
	 */
	FText GetKeyText( ) const;

	/**
	 * Checks whether this gesture requires an modifier keys to be pressed.
	 *
	 * @return true if modifier keys must be pressed, false otherwise.
	 */
	bool HasAnyModifierKeys( ) const
	{
		return bCtrl || bAlt || bShift;
	}

	/**
	 * Determines if this gesture is valid.  A gesture is valid if it has a non modifier key that must be pressed
	 * and zero or more modifier keys that must be pressed
	 *
	 * @return true if the gesture is valid
	 */
	bool IsValidGesture( ) const
	{
		return (Key.IsValid() && !Key.IsModifierKey());
	}

	/**
	 * Sets this gesture to a new key and modifier state based on the provided template
	 * Should not be called directly.  Only used by the key binding editor to set user defined keys
	 */
	void Set( FInputGesture InTemplate )
	{
		Key = InTemplate.Key;
		bCtrl = InTemplate.bCtrl;
		bAlt = InTemplate.bAlt;
		bShift = InTemplate.bShift;
	}

public:

	/**
	 * Gets a type hash value for the specified gesture.
	 *
	 * @param Gesture The input gesture to get the hash value for.
	 *
	 * @return The hash value.
	 */
	friend uint32 GetTypeHash( const FInputGesture& Gesture )
	{
		return GetTypeHash(Gesture.Key) ^ ((Gesture.bCtrl << 2) | (Gesture.bShift << 1) | Gesture.bAlt);
	}
};
