// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITransaction.h: Declares the ITransaction interface.
=============================================================================*/

#pragma once

// Class for handling undo/redo transactions among objects.
typedef void( *STRUCT_AR )( class FArchive& Ar, void* TPtr );
typedef void( *STRUCT_DTOR )( void* TPtr );

/**
 * Interface for transactions.
 */
class ITransaction
{
public:

	/**
	 * Applies the transaction.
	 */
	virtual void Apply( ) = 0;

	/**
	 * Saves an array to the transaction.
	 *
	 * @param Object - The object that owns the array.
	 * @param Array - The array to save.
	 * @param Index - 
	 * @param Count - 
	 * @param Oper -
	 * @param ElementSize -
	 * @param Serializer -
	 * @param Destructor -
	 */
	virtual void SaveArray( UObject* Object, class FScriptArray* Array, int32 Index, int32 Count, int32 Oper, int32 ElementSize, STRUCT_AR Serializer, STRUCT_DTOR Destructor ) = 0;

	/**
	 * Saves an UObject to the transaction.
	 *
	 * @param Object - The object to save.
	 */
	virtual void SaveObject( UObject* Object ) = 0;

	/**
	 * Sets the transaction's primary object.
	 *
	 * @param Object - The primary object to set.
	 */
	virtual void SetPrimaryObject( UObject* Object ) = 0;
};
