// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MatineeTransBuffer.generated.h"

/**
 * Special transaction buffer for Matinee undo/redo
 * Will be capped at InMaxMemory.
 * 
 */
UCLASS(transient)
class UMatineeTransBuffer : public UTransBuffer
{
    GENERATED_UCLASS_BODY()
	UMatineeTransBuffer(const class FPostConstructInitializeProperties& PCIP, SIZE_T InMaxMemory)
		:	UTransBuffer( PCIP, InMaxMemory )
	{}

	// Begin UTransactor Interface
	virtual int32 Begin( const TCHAR* SessionContext, const FText& Description ) OVERRIDE
	{
		return 0;
	}

	virtual int32 End() OVERRIDE
	{
		return 1;
	}

	virtual void Cancel(int32 StartIndex = 0) OVERRIDE {}
	// End UTransactor Interface

	/**  
	 * Begin a Matinee specific transaction
	 * @param	Description		The description for transaction event
	 */
	virtual void BeginSpecial(const FText& Description);

	/** End a Matinee specific transaction */
	virtual void EndSpecial();
};
