// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"

TSharedRef<FExternalDragOperation> FExternalDragOperation::NewText( const FString& InText )
{
	TSharedRef<FExternalDragOperation> Operation = MakeShareable(new FExternalDragOperation);
	FSlateApplication::GetDragDropReflector().RegisterOperation<FExternalDragOperation>(Operation);
	Operation->DragType = DragText;
	Operation->DraggedText = InText;
	Operation->Construct();
	return Operation;
}

TSharedRef<FExternalDragOperation> FExternalDragOperation::NewFiles( const TArray<FString>& InFileNames )
{
	TSharedRef<FExternalDragOperation> Operation = MakeShareable(new FExternalDragOperation);
	FSlateApplication::GetDragDropReflector().RegisterOperation<FExternalDragOperation>(Operation);
	Operation->DragType = DragFiles;
	Operation->DraggedFileNames = InFileNames;
	Operation->Construct();
	return Operation;
}

FDragDropOperation::~FDragDropOperation()
{
	DestroyCursorDecoratorWindow();
}

void FDragDropOperation::OnDrop( bool bDropWasHandled, const FPointerEvent& MouseEvent )
{
	DestroyCursorDecoratorWindow();
}

void FDragDropOperation::OnDragged( const class FDragDropEvent& DragDropEvent )
{
	if (CursorDecoratorWindow.IsValid())
	{
		CursorDecoratorWindow->MoveWindowTo(DragDropEvent.GetScreenSpacePosition() + FSlateApplication::Get().GetCursorSize());
	}
}

FCursorReply FDragDropOperation::OnCursorQuery()
{
	if ( MouseCursorOverride.IsSet() )
	{
		return FCursorReply::Cursor( MouseCursorOverride.GetValue() );
	}

	if ( MouseCursor.IsSet() )
	{
		return FCursorReply::Cursor(MouseCursor.GetValue());
	}

	return FCursorReply::Unhandled();
}

void FDragDropOperation::SetDecoratorVisibility(bool bVisible)
{
	if (CursorDecoratorWindow.IsValid())
	{
		if (bVisible)
		{
			CursorDecoratorWindow->ShowWindow();
		}
		else
		{
			CursorDecoratorWindow->HideWindow();
		}
	}
}

void FDragDropOperation::SetCursorOverride( TOptional<EMouseCursor::Type> CursorType )
{
	MouseCursorOverride = CursorType;
}

void FDragDropOperation::Construct()
{
	TSharedPtr<SWidget> DecoratorToUse = GetDefaultDecorator();

	if (DecoratorToUse.IsValid())
	{
		// Create window for info
		CursorDecoratorWindow = SWindow::MakeCursorDecorator();
		FSlateApplication::Get().AddWindow(CursorDecoratorWindow.ToSharedRef(), true);

		// Create hover widget
		CursorDecoratorWindow->SetContent(DecoratorToUse.ToSharedRef());
	}
}

void FDragDropOperation::DestroyCursorDecoratorWindow()
{
	if (CursorDecoratorWindow.IsValid())
	{
		CursorDecoratorWindow->RequestDestroyWindow();
		CursorDecoratorWindow.Reset();
	}
}