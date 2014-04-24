// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DecoratedDragDropOp.h"
#include "WidgetTemplateDescriptor.h"

class FWidgetTemplateDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FWidgetTemplateDragDropOp, FDecoratedDragDropOp)

	TSharedPtr<FWidgetTemplateDescriptor> Template;

	static TSharedRef<FWidgetTemplateDragDropOp> New(const TSharedPtr<FWidgetTemplateDescriptor>& InTemplate);

	//virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE;
};
