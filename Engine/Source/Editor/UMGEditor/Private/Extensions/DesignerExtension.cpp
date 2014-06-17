// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetBlueprintEditor.h"

#define LOCTEXT_NAMESPACE "UMG"

// FWidgetReference

FWidgetReference::FWidgetReference()
	: WidgetEditor()
	, TemplateWidget(NULL)
{
}

FWidgetReference::FWidgetReference(TSharedPtr<FWidgetBlueprintEditor> WidgetEditor, UWidget* TemplateWidget)
	: WidgetEditor(WidgetEditor)
	, TemplateWidget(TemplateWidget)
{
}

FWidgetReference FWidgetReference::FromTemplate(TSharedPtr<FWidgetBlueprintEditor> WidgetEditor, UWidget* TemplateWidget)
{
	return FWidgetReference(WidgetEditor, TemplateWidget);
}

FWidgetReference FWidgetReference::FromPreview(TSharedPtr<FWidgetBlueprintEditor> WidgetEditor, UWidget* PreviewWidget)
{
	if ( WidgetEditor.IsValid() )
	{
		UUserWidget* PreviewRoot = WidgetEditor->GetPreview();
		if ( PreviewRoot )
		{
			UWidgetBlueprint* Blueprint = WidgetEditor->GetWidgetBlueprintObj();

			if ( PreviewWidget )
			{
				FString Name = PreviewWidget->GetName();
				return FromTemplate(WidgetEditor, Blueprint->WidgetTree->FindWidget(Name));
			}
		}
	}

	return FWidgetReference();
}

bool FWidgetReference::IsValid() const
{
	return TemplateWidget != NULL && GetPreview();
}

UWidget* FWidgetReference::GetTemplate() const
{
	return TemplateWidget;
}

UWidget* FWidgetReference::GetPreview() const
{
	if ( WidgetEditor.IsValid() )
	{
		UUserWidget* PreviewRoot = WidgetEditor->GetPreview();

		if ( PreviewRoot && TemplateWidget )
		{
			UWidget* PreviewWidget = PreviewRoot->GetHandleFromName(TemplateWidget->GetName());
			return PreviewWidget;
		}
	}

	return NULL;
}

// Designer Extension

FDesignerExtension::FDesignerExtension()
	: ScopedTransaction(NULL)
{

}

void FDesignerExtension::Initialize(UWidgetBlueprint* InBlueprint)
{
	Blueprint = InBlueprint;
}

FName FDesignerExtension::GetExtensionId() const
{
	return ExtensionId;
}

void FDesignerExtension::BeginTransaction(const FText& SessionName)
{
	if ( ScopedTransaction == NULL )
	{
		ScopedTransaction = new FScopedTransaction(SessionName);
	}

	for ( FWidgetReference& Selection : SelectionCache )
	{
		Selection.GetPreview()->Modify();
		Selection.GetTemplate()->Modify();
	}
}

void FDesignerExtension::EndTransaction()
{
	if ( ScopedTransaction != NULL )
	{
		delete ScopedTransaction;
		ScopedTransaction = NULL;
	}
}

#undef LOCTEXT_NAMESPACE
