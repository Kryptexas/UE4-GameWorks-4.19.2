// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "UMGDragDropOp.h"

//////////////////////////////////////////////////////////////////////////
// FUMGDragDropOp
void FUMGDragDropOp::OnDrop( bool bDropWasHandled, const FPointerEvent& MouseEvent )
{
	UDragDropOperation* OperationObject = DragOperation.Get();
	if ( OperationObject )
	{
		if ( bDropWasHandled )
		{
			OperationObject->Drop(MouseEvent);
		}
		else
		{
			OperationObject->DropCanceled(MouseEvent);
		}
	}
	
	FDragDropOperation::OnDrop(bDropWasHandled, MouseEvent);
}

void FUMGDragDropOp::OnDragged( const class FDragDropEvent& DragDropEvent )
{
	UDragDropOperation* OperationObject = DragOperation.Get();
	if ( OperationObject )
	{
		OperationObject->Dragged(DragDropEvent);
	}

	FVector2D Position = DragDropEvent.GetScreenSpacePosition() + Offset;

	CursorDecoratorWindow->MoveWindowTo(Position);
}

TSharedPtr<SWidget> FUMGDragDropOp::GetDefaultDecorator() const
{
	return DecoratorWidget;
}

TSharedRef<FUMGDragDropOp> FUMGDragDropOp::New(UDragDropOperation* InOperation, const FVector2D &CursorPosition, const FVector2D &ScreenPositionOfDragee)
{
	TSharedRef<FUMGDragDropOp> Operation = MakeShareable(new FUMGDragDropOp);
	Operation->Offset = ScreenPositionOfDragee - CursorPosition;
	Operation->StartingScreenPos = ScreenPositionOfDragee;

	Operation->DragOperation = InOperation;

	if ( InOperation->DefaultDragVisual == nullptr )
	{
		Operation->DecoratorWidget = SNew(STextBlock)
			.Text(FText::FromString(InOperation->Tag));
	}
	else
	{
		Operation->DecoratorWidget = InOperation->DefaultDragVisual->TakeWidget();
	}

	Operation->DecoratorWidget->SlatePrepass();

	Operation->Construct();

	return Operation;
}
