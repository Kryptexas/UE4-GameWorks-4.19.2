// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A base class for a DragAndDrop operation which supports reflection.
 * Drag and Drop is inherently quite stateful.
 * Implementing a custom DragDropOperation allows for dedicated handling of
 * a drag drop operation which keeps a decorator window (optionally)
 */
class SLATE_API FDragDropOperation
{
public:
	FDragDropOperation()
	{
	}

	virtual ~FDragDropOperation();

	/**
	 * Invoked when the drag and drop operation has ended.
	 * 
	 * @param bDropWasHandled   true when the drop was handled by some widget; false otherwise
	 * @param MouseEvent        The mouse event which caused the on drop to be called.
	 */
	virtual void OnDrop( bool bDropWasHandled, const FPointerEvent& MouseEvent );

	/** 
	 * Called when the mouse was moved during a drag and drop operation
	 *
	 * @param DragDropEvent    The event that describes this drag drop operation.
	 */
	virtual void OnDragged( const class FDragDropEvent& DragDropEvent );
	
	/** Allows drag/drop operations to override the current cursor */
	virtual FCursorReply OnCursorQuery();

	/** Gets the widget that will serve as the decorator unless overridden. If you do not override, you will have no decorator */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const {return TSharedPtr<SWidget>();}
	
	/** Alters the visibility of the window */
	void SetDecoratorVisibility(bool bVisible);

	/** Is this drag Drop operation going to interact with applications outside of Slate */
	virtual bool IsExternalOperation() const { return false; }

	/** 
	 * Sets the cursor to override the drag drop operations cursor with so that a 
	 * control can give temporary feedback, for example - a slashed circle telling 
	 * the user 'this can't be dropped here'.
	 */
	void SetCursorOverride( TOptional<EMouseCursor::Type> CursorType );

protected:
	/** Constructs the window and widget if applicable */
	virtual void Construct();

protected:

	/** Destroy the cursor decorator */
	void DestroyCursorDecoratorWindow();

protected:
	/** The window that owns the decorator widget */
	TSharedPtr<SWindow> CursorDecoratorWindow;

	/** Mouse cursor used by the drag drop operation */
	TOptional<EMouseCursor::Type> MouseCursor;

	/** Mouse cursor used to temporarily replace the drag drops cursor */
	TOptional<EMouseCursor::Type> MouseCursorOverride;
};

/** Like a mouse event but with content */
class FDragDropEvent : public FPointerEvent
{
public:
	 /**
	  * Construct a DragDropEvent.
	  *
	  * @param InMouseEvent  The mouse event that is causing this drag and drop event to occur.
	  * @param InContent     The content being dragged.
	  */
	 FDragDropEvent( const FPointerEvent& InMouseEvent, const TSharedPtr<FDragDropOperation> InContent )
		: FPointerEvent( InMouseEvent )
		, Content( InContent )
	{
	}

	/** @return the content being dragged */
	TSharedPtr<FDragDropOperation> GetOperation() const
	{
		return Content;
	}

	/** @return the content being dragged if it matched the 'OperationType'; invalid Ptr otherwise. */
	template<typename OperationType>
	TSharedPtr<OperationType> GetOperationAs() const;

private:
	/** the content being dragged */
	TSharedPtr<FDragDropOperation> Content;
};


/**
 * Invoked when a drag and drop is finished.
 * This allows the widget that started the drag/drop to respond to the end of the operation.
 *
 * @param bWasDropHandled   True when the drag and drop operation was handled by some widget;
 *                          False when no widget handled the drop.
 * @param DragDropEvent     The Drop event that terminated the whole DragDrop operation
 */
DECLARE_DELEGATE_TwoParams( FOnDragDropEnded,
	bool /* bWasDropHandled */,
	const FDragDropEvent& /* DragDropEvent */
)

/**
 * A delegate invoked on the initiator of the DragDrop operation.
 * This delegate is invoked periodically during the DragDrop, and
 * gives the initiator an opportunity to respond to the current state of
 * the process. e.g. Create and update a custom cursor.
 */
DECLARE_DELEGATE_OneParam( FOnDragDropUpdate,
	const FDragDropEvent& /* DragDropEvent */
)



/**
 * An external drag and drop operation that originates outside of slate.
 * E.g. an OLE drag and drop.
 */
class SLATE_API FExternalDragOperation : public FDragDropOperation
{
private:
	/** A private constructor to ensure that the appropriate "New" factory method is used below */
	FExternalDragOperation(){}

	virtual bool IsExternalOperation() const OVERRIDE { return true; }

public:
	static FString GetTypeId() {static FString Type = TEXT("FExternalDragOperation"); return Type;}

	/** Creates a new external text drag operation */
	static TSharedRef<FExternalDragOperation> NewText( const FString& InText );
	/** Creates a new external file drag operation */
	static TSharedRef<FExternalDragOperation> NewFiles( const TArray<FString>& InFileNames );

	/** @return true if this is a text drag operation */
	bool HasText() const {return DragType == DragText;}
	/** @return true if this is a file drag operation */
	bool HasFiles() const {return DragType == DragFiles;}
	
	/** @return the text from this drag operation */
	const FString& GetText() const { ensure(HasText()); return DraggedText;}
	/** @return the filenames from this drag operation */
	const TArray<FString>& GetFiles() const { ensure(HasFiles()); return DraggedFileNames;}

private:
	FString DraggedText;
	TArray<FString> DraggedFileNames;

	enum ExternalDragType
	{
		DragText,
		DragFiles
	} DragType;
};



/**
 * The Drag Drop Reflector handles reflection for all drag drop objects.
 * It could easily be expanded to handle reflection of any object.
 *
 * To make a drag drop operation reflective, you will need this line in your class:
 * static FString GetTypeId() {static FString Type = TEXT("..."); return Type;}
 *
 * You will also need to Register the class during it's construction
 * However, FDecoratedDragDrop already handles the second part for you if you use it
 */
class SLATE_API FDragDropReflector
{
public:
	typedef TMap<TWeakPtr<FDragDropOperation>, FString> FOperationMap;

	/** Registers a class instance with the class type's TypeId */
	template <typename Operation>
	void RegisterOperation(TWeakPtr<FDragDropOperation> Op)
	{
		CleanupPointerMap(RegisteredOperations);

		RegisteredOperations.Add(Op, Operation::GetTypeId());
	}

	/** Checks to see if two reflective classes have the same class type */
	template <typename Operation>
	bool CheckEquivalence(TWeakPtr<FDragDropOperation> Op)
	{
		FString* RegisteredOperation = RegisteredOperations.Find(Op);
		return RegisteredOperation && Operation::GetTypeId() == *RegisteredOperation;
	}

private:
	/** A map of pointers to all registered classes to their TypeIds */
	FOperationMap RegisteredOperations;
};



namespace DragDrop
{
	/** See if this dragdrop operation matches another dragdrop operation */
	template <typename OperatorType>
	bool IsTypeMatch(const TSharedPtr<FDragDropOperation> Operation);
}
