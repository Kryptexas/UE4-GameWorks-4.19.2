// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetBlueprintEditor.h"

#define LOCTEXT_NAMESPACE "UMG"

// FSelectedWidget

FSelectedWidget::FSelectedWidget()
	: WidgetEditor()
	, TemplateWidget(NULL)
{
}

FSelectedWidget::FSelectedWidget(TSharedPtr<FWidgetBlueprintEditor> WidgetEditor, UWidget* TemplateWidget)
	: WidgetEditor(WidgetEditor)
	, TemplateWidget(TemplateWidget)
{
}

FSelectedWidget FSelectedWidget::FromTemplate(TSharedPtr<FWidgetBlueprintEditor> WidgetEditor, UWidget* TemplateWidget)
{
	return FSelectedWidget(WidgetEditor, TemplateWidget);
}

FSelectedWidget FSelectedWidget::FromPreview(TSharedPtr<FWidgetBlueprintEditor> WidgetEditor, UWidget* PreviewWidget)
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

	return FSelectedWidget();
}

bool FSelectedWidget::IsValid() const
{
	return TemplateWidget != NULL && GetPreview();
}

UWidget* FSelectedWidget::GetTemplate() const
{
	return TemplateWidget;
}

UWidget* FSelectedWidget::GetPreview() const
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

	for ( FSelectedWidget& Selection : SelectionCache )
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
