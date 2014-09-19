// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetTemplateClass.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "UMGEditor"

FWidgetTemplateClass::FWidgetTemplateClass()
	: WidgetClass(nullptr)
{
	// register for any objects replaced
	GEditor->OnObjectsReplaced().AddRaw(this, &FWidgetTemplateClass::OnObjectsReplaced);
}

FWidgetTemplateClass::FWidgetTemplateClass(TSubclassOf<UWidget> InWidgetClass)
	: WidgetClass(InWidgetClass)
{
	Name = WidgetClass->GetDisplayNameText();

	// register for any objects replaced
	GEditor->OnObjectsReplaced().AddRaw(this, &FWidgetTemplateClass::OnObjectsReplaced);
}

FWidgetTemplateClass::~FWidgetTemplateClass()
{
	GEditor->OnObjectsReplaced().RemoveAll(this);
}

FText FWidgetTemplateClass::GetCategory() const
{
	auto DefaultWidget = WidgetClass->GetDefaultObject<UWidget>();
	return DefaultWidget->GetPaletteCategory();
}

UWidget* FWidgetTemplateClass::Create(UWidgetTree* Tree)
{
	return Tree->ConstructWidget<UWidget>(WidgetClass.Get());
}

const FSlateBrush* FWidgetTemplateClass::GetIcon() const
{
	UWidget* DefaultWidget = WidgetClass->GetDefaultObject<UWidget>();
	return DefaultWidget->GetEditorIcon();
}

TSharedRef<IToolTip> FWidgetTemplateClass::GetToolTip() const
{
	return IDocumentation::Get()->CreateToolTip(FText::FromString(WidgetClass->GetDescription()), nullptr, FString(TEXT("Shared/Types/")) + WidgetClass->GetName(), TEXT("Class"));
}

void FWidgetTemplateClass::OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	UObject* const* NewObject = ReplacementMap.Find(WidgetClass.Get());
	if (NewObject)
	{
		WidgetClass = CastChecked<UClass>(*NewObject);
	}
}

#undef LOCTEXT_NAMESPACE
