// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

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
	// register for any objects replaced
	GEditor->OnObjectsReplaced().AddRaw(this, &FWidgetReference::OnObjectsReplaced);
}

FWidgetReference::~FWidgetReference()
{
	GEditor->OnObjectsReplaced().RemoveAll(this);
}

FWidgetReference FWidgetReference::FromTemplate(TSharedPtr<FWidgetBlueprintEditor> InWidgetEditor, UWidget* TemplateWidget)
{
	return FWidgetReference(InWidgetEditor, TemplateWidget);
}

FWidgetReference FWidgetReference::FromPreview(TSharedPtr<FWidgetBlueprintEditor> InWidgetEditor, UWidget* PreviewWidget)
{
	if ( InWidgetEditor.IsValid() )
	{
		UUserWidget* PreviewRoot = InWidgetEditor->GetPreview();
		if ( PreviewRoot )
		{
			UWidgetBlueprint* Blueprint = InWidgetEditor->GetWidgetBlueprintObj();

			if ( PreviewWidget )
			{
				FString Name = PreviewWidget->GetName();
				return FromTemplate(InWidgetEditor, Blueprint->WidgetTree->FindWidget(Name));
			}
		}
	}

	return FWidgetReference();
}

bool FWidgetReference::IsValid() const
{
	return TemplateWidget.Get() != NULL && GetPreview();
}

UWidget* FWidgetReference::GetTemplate() const
{
	return TemplateWidget.Get();
}

UWidget* FWidgetReference::GetPreview() const
{
	if ( WidgetEditor.IsValid() )
	{
		UUserWidget* PreviewRoot = WidgetEditor.Pin()->GetPreview();

		if ( PreviewRoot && TemplateWidget.Get() )
		{
			UWidget* PreviewWidget = PreviewRoot->GetHandleFromName(TemplateWidget.Get()->GetName());
			return PreviewWidget;
		}
	}

	return NULL;
}

void FWidgetReference::OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	UObject* const* NewObject = ReplacementMap.Find(TemplateWidget.Get());
	if ( NewObject )
	{
		TemplateWidget = Cast<UWidget>(*NewObject);
	}
}

#undef LOCTEXT_NAMESPACE
