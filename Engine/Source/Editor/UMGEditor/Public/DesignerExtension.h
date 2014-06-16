// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class UWidget;
class SWidget;
class UWidgetBlueprint;

/**
 * The Selected widget is a useful way to hold onto the selection in a way that allows for up to date access to the current preview object.
 * Because the designer could end up rebuilding the preview, it's best to hold onto an FSelectedWidget.
 */
struct FSelectedWidget
{
public:
	static FSelectedWidget FromTemplate(TSharedPtr<class FWidgetBlueprintEditor> WidgetEditor, UWidget* TemplateWidget);
	static FSelectedWidget FromPreview(TSharedPtr<class FWidgetBlueprintEditor> WidgetEditor, UWidget* PreviewWidget);

	FSelectedWidget();

	bool IsValid() const;

	UWidget* GetTemplate() const;
	UWidget* GetPreview() const;

private:
	FSelectedWidget(TSharedPtr<class FWidgetBlueprintEditor> WidgetEditor, UWidget* TemplateWidget);

private:

	TSharedPtr<class FWidgetBlueprintEditor> WidgetEditor;
	UWidget* TemplateWidget;
};

class FDesignerExtension : public TSharedFromThis<FDesignerExtension>
{
public:
	FDesignerExtension();

	virtual void Initialize(UWidgetBlueprint* InBlueprint);

	virtual void BuildWidgetsForSelection(const TArray< FSelectedWidget >& Selection, TArray< TSharedRef<SWidget> >& Widgets) = 0;

	FName GetExtensionId() const;

protected:
	void BeginTransaction(const FText& SessionName);

	void EndTransaction();

protected:
	FName ExtensionId;
	UWidgetBlueprint* Blueprint;

	TArray< FSelectedWidget > SelectionCache;

private:
	FScopedTransaction* ScopedTransaction;
};
