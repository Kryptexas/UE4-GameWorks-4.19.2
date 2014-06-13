// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DecoratedDragDropOp.h"
#include "WidgetTemplate.h"

class FWidgetTemplateDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FWidgetTemplateDragDropOp, FDecoratedDragDropOp)

	TSharedPtr<FWidgetTemplate> Template;

	static TSharedRef<FWidgetTemplateDragDropOp> New(const TSharedPtr<FWidgetTemplate>& InTemplate);

	//virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;
};
