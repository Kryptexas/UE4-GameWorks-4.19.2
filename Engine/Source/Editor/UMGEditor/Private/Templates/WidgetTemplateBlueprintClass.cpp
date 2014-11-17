// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetTemplateBlueprintClass.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "UMGEditor"

FWidgetTemplateBlueprintClass::FWidgetTemplateBlueprintClass(const FAssetData& InWidgetAssetData, TSubclassOf<UUserWidget> InUserWidgetClass)
	: FWidgetTemplateClass(), WidgetAssetData(InWidgetAssetData)
{
	if (InUserWidgetClass)
	{
		WidgetClass = CastChecked<UClass>(InUserWidgetClass);
		Name = WidgetClass->GetDisplayNameText();
	}
	else
	{
		Name = FText::FromString(FName::NameToDisplayString(WidgetAssetData.AssetName.ToString(), false));
	}
}

FWidgetTemplateBlueprintClass::~FWidgetTemplateBlueprintClass()
{
}

FText FWidgetTemplateBlueprintClass::GetCategory() const
{
	auto DefaultUserWidget = UUserWidget::StaticClass()->GetDefaultObject<UUserWidget>();
	return DefaultUserWidget->GetPaletteCategory();
}

UWidget* FWidgetTemplateBlueprintClass::Create(UWidgetTree* Tree)
{
	// Load the blueprint asset if needed
	if (!WidgetClass.Get())
	{
		FString AssetPath = WidgetAssetData.ObjectPath.ToString();
		UWidgetBlueprint* LoadedWidget = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
		WidgetClass = CastChecked<UClass>(LoadedWidget->GeneratedClass);
	}

	return FWidgetTemplateClass::Create(Tree);
}

const FSlateBrush* FWidgetTemplateBlueprintClass::GetIcon() const
{
	auto DefaultUserWidget = UUserWidget::StaticClass()->GetDefaultObject<UUserWidget>();
	return DefaultUserWidget->GetEditorIcon();
}

TSharedRef<IToolTip> FWidgetTemplateBlueprintClass::GetToolTip() const
{
	if (WidgetClass.Get())
	{
		return FWidgetTemplateClass::GetToolTip();
	}

	return IDocumentation::Get()->CreateToolTip(Name, nullptr, FString(TEXT("Shared/Types/")) + Name.ToString(), TEXT("Class"));
}

#undef LOCTEXT_NAMESPACE
