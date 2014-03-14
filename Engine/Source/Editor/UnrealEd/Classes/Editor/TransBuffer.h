// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Transaction tracking system, manages the undo and redo buffer.
 */

#pragma once
#include "TransBuffer.generated.h"

UCLASS(transient)
class UNREALED_API UTransBuffer : public UTransactor
{
    GENERATED_UCLASS_BODY()
	// Variables.
	/** The queue of transaction records */
	TArray<FTransaction> UndoBuffer;

	/** Number of transactions that have been undone, and are eligible to be redone */
	int32 UndoCount;

	/** Text describing the reason that the undo buffer is empty */
	FText ResetReason;

	/** Number of actions in the current transaction */
	int32 ActiveCount;

	/** Maximum number of bytes the transaction buffer is allowed to occupy */
	SIZE_T MaxMemory;

private:
	/** Controls whether the transaction buffer is allowed to serialize object references */
	int32 DisallowObjectSerialization;

public:

	// Constructor.
	UTransBuffer( const class FPostConstructInitializeProperties& PCIP,SIZE_T InMaxMemory );

	// Begin UObject interface.
	virtual void Serialize( FArchive& Ar ) OVERRIDE;
	virtual void FinishDestroy() OVERRIDE;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject interface.

	// Begin UTransactor interface.
	virtual int32 Begin( const TCHAR* SessionContext, const FText& Description ) OVERRIDE;
	virtual int32 End() OVERRIDE;
	virtual void Cancel( int32 StartIndex = 0 ) OVERRIDE;
	virtual void Reset( const FText& Reason ) OVERRIDE;
	virtual bool CanUndo( FText* Text=NULL ) OVERRIDE;
	virtual bool CanRedo( FText* Text=NULL ) OVERRIDE;
	virtual FUndoSessionContext GetUndoContext( bool bCheckWhetherUndoPossible = true ) OVERRIDE;
	virtual FUndoSessionContext GetRedoContext() OVERRIDE;
	virtual bool Undo() OVERRIDE;
	virtual bool Redo() OVERRIDE;
	virtual bool EnableObjectSerialization() OVERRIDE;
	virtual bool DisableObjectSerialization() OVERRIDE;
	virtual bool IsObjectSerializationEnabled() OVERRIDE { return DisallowObjectSerialization == 0; }
	virtual void SetPrimaryUndoObject( UObject* Object ) OVERRIDE;
	virtual ITransaction* CreateInternalTransaction() OVERRIDE;
	virtual bool IsActive() OVERRIDE
	{
		return ActiveCount > 0;
	}
	// End UTransactor interface.

	/**
	 * Determines the amount of data currently stored by the transaction buffer.
	 *
	 * @return	number of bytes stored in the undo buffer
	 */
	SIZE_T GetUndoSize() const;

	/**
	 * Validates the state of the transaction buffer.
	 */
	void CheckState() const;

};
