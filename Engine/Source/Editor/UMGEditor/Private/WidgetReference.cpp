// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

FWidgetHandle::FWidgetHandle(UWidget* Widget)
	: Widget(Widget)
{

}

// FWidgetReference

FWidgetReference::FWidgetReference()
	: WidgetEditor()
	, TemplateHandle()
{

}

FWidgetReference::FWidgetReference(TSharedPtr<FWidgetBlueprintEditor> WidgetEditor, TSharedPtr< FWidgetHandle > TemplateHandle)
	: WidgetEditor(WidgetEditor)
	, TemplateHandle(TemplateHandle)
{
	
}

bool FWidgetReference::IsValid() const
{
	if ( TemplateHandle.IsValid() )
	{
		return TemplateHandle->Widget.Get() != NULL && GetPreview();
	}

	return false;
}

UWidget* FWidgetReference::GetTemplate() const
{
	if ( TemplateHandle.IsValid() )
	{
		return TemplateHandle->Widget.Get();
	}

	return NULL;
}

UWidget* FWidgetReference::GetPreview() const
{
	if ( WidgetEditor.IsValid() && TemplateHandle.IsValid() )
	{
		UUserWidget* PreviewRoot = WidgetEditor.Pin()->GetPreview();

		if ( PreviewRoot && TemplateHandle->Widget.Get() )
		{
			UWidget* PreviewWidget = PreviewRoot->GetHandleFromName(TemplateHandle->Widget.Get()->GetName());
			return PreviewWidget;
		}
	}

	return NULL;
}

#undef LOCTEXT_NAMESPACE
