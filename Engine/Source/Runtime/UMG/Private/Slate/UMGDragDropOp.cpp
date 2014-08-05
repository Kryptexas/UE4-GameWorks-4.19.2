// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "UMGDragDropOp.h"

//////////////////////////////////////////////////////////////////////////
// FUMGDragDropOp
void FUMGDragDropOp::OnDrop( bool bDropWasHandled, const FPointerEvent& MouseEvent )
{
	//if ( bDropWasHandled == false )
	//{
	//	if (OriginalTrackNode.IsValid())
	//	{
	//		//OriginalTrackNode.Pin()->OnDropCancelled(MouseEvent);
	//	}
	//}
	
	FDragDropOperation::OnDrop(bDropWasHandled, MouseEvent);
}

void FUMGDragDropOp::OnDragged( const class FDragDropEvent& DragDropEvent )
{
	//TSharedPtr<STrackNode> Node = OriginalTrackNode.Pin();
	//if (Node.IsValid())
	//{
	//	Node->OnDragged(DragDropEvent);
	//}

	FVector2D pos;
	pos.X = (DragDropEvent.GetScreenSpacePosition() + Offset).X;
	pos.Y = StartingScreenPos.Y;

	CursorDecoratorWindow->MoveWindowTo(pos);
}

TSharedPtr<SWidget> FUMGDragDropOp::GetDefaultDecorator() const
{
	return DecoratorWidget;
}

TSharedRef<FUMGDragDropOp> FUMGDragDropOp::New(UObject* Payload, UUserWidget* Decorator, const FVector2D &CursorPosition, const FVector2D &ScreenPositionOfNode)
{
	TSharedRef<FUMGDragDropOp> Operation = MakeShareable(new FUMGDragDropOp);
	Operation->Offset = ScreenPositionOfNode - CursorPosition;
	Operation->StartingScreenPos = ScreenPositionOfNode;

	Operation->Payload = Payload;
	Operation->DecoratorWidget = Decorator->TakeWidget();

	Operation->Construct();

	return Operation;
}