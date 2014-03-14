// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** @return the content being dragged if it matched the 'OperationType'; invalid Ptr otherwise. */
template<typename OperationType>
TSharedPtr<OperationType> FDragDropEvent::GetOperationAs() const
{
	if ( FSlateApplication::GetDragDropReflector().CheckEquivalence<OperationType>(TWeakPtr<FDragDropOperation>(Content)) )
	{
		return StaticCastSharedPtr<OperationType>(Content);
	}
	else
	{
		return TSharedPtr<OperationType>();
	}
}

// DragAndDrop functions that depend on classes defined after DragAndDrop.h is included in Slate.h

namespace DragDrop
{
	/** See if this dragdrop operation matches another dragdrop operation */
	template <typename OperatorType>
	bool IsTypeMatch(const TSharedPtr<FDragDropOperation> Operation)
	{
		return Operation.IsValid() && FSlateApplication::GetDragDropReflector().CheckEquivalence<OperatorType>(TWeakPtr<FDragDropOperation>(Operation));
	}
}
