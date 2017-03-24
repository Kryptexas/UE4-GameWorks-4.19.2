// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FPaintModePainter;
class UPaintModeSettings;
class IDetailsView;
class SWrapBox;

/** Widget representing the state / functionality and settings for PaintModePainter*/
class SPaintModeWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPaintModeWidget) {}
	SLATE_END_ARGS()

	/** Slate widget construction */
	void Construct(const FArguments& InArgs, FPaintModePainter* InPainter);

protected:
	/** Creates and sets up details view */
	void CreateDetailsView();
	
	/** Returns a widget comprising special UI elements  for vertex color painting */
	TSharedPtr<SWidget> CreateVertexPaintWidget();

	/** Returns a widget comprising UI elements for texture painting */
	TSharedPtr<SWidget> CreateTexturePaintWidget();	

	/** Returns the toolbar widget instance */
	TSharedPtr<SWidget> CreateToolBarWidget();

	/** Getters for whether or not a specific mode is visible */
	EVisibility IsVertexPaintModeVisible() const;
	EVisibility IsTexturePaintModeVisible() const;

	/** Helper function to create SButtons for each entry inside of Enum together with the enabled check and click action */
	void CreateActionEnumButtons(TSharedPtr<SWrapBox> WrapBox, const UEnum* Enum, TFunction<bool (int32)> EnabledFunc, TFunction<FReply (int32)> ClickFunc);
protected:	
	/** Objects displayed in the details view */
	TArray<UObject*> SettingsObjects;
	/** Details view for brush and paint settings */
	TSharedPtr<IDetailsView> SettingsDetailsView;
	/** Ptr to painter for which this widget is the ui representation */
	FPaintModePainter* MeshPainter;
	/** Paint settings instance */
	UPaintModeSettings* PaintModeSettings;
};
