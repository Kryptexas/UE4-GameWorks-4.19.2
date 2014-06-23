// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FWidgetBlueprintEditorUtils

class FWidgetBlueprintEditorUtils
{
public:
	static bool RenameWidget(TSharedRef<class FWidgetBlueprintEditor> BlueprintEditor, const FName& OldName, const FName& NewName);

	static void CreateWidgetContextMenu(FMenuBuilder& MenuBuilder, TSharedRef<FWidgetBlueprintEditor> BlueprintEditor, FVector2D TargetLocation);

	static void CopyWidgets(UWidgetBlueprint* BP, TSet<FWidgetReference> Widgets);

	static void PasteWidgets(UWidgetBlueprint* BP, FWidgetReference ParentWidget, FVector2D PasteLocation);

	static void DeleteWidgets(UWidgetBlueprint* BP, TSet<FWidgetReference> Widgets);

public:
	static void ExportWidgetsToText(TSet<UWidget*> WidgetsToExport, /*out*/ FString& ExportedText);

	static void ImportWidgetsFromText(UWidgetBlueprint* BP, const FString& TextToImport, /*out*/ TSet<UWidget*>& ImportedWidgetSet);

private:
	static void BuildWrapWithMenu(FMenuBuilder& Menu, UWidgetBlueprint* BP, TSet<FWidgetReference> Widgets);

	static void WrapWidgets(UWidgetBlueprint* BP, TSet<FWidgetReference> Widgets, UClass* WidgetClass);
};
