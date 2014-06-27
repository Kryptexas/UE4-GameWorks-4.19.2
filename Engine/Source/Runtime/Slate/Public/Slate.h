// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "CoreUObject.h"
#include "Json.h"
#include "SlateCore.h"

#include "SlateClasses.h"


// Enables or disables more slate stats.  These stats are usually per widget and can be gathered hundreds or thousands of times per frame
// enabling these constantly causes a bit of a performance drop due to stat gathering overhead so these should only be enabled for profiling
#define SLATE_HD_STATS 0

#define BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION PRAGMA_DISABLE_OPTIMIZATION
#define END_SLATE_FUNCTION_BUILD_OPTIMIZATION   PRAGMA_ENABLE_OPTIMIZATION

/** Stat tracking for Slate */
DECLARE_CYCLE_STAT_EXTERN( TEXT("Total Slate Tick Time"), STAT_SlateTickTime, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("CacheDesiredSize"), STAT_SlateCacheDesiredSize, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("STextBlock"), STAT_SlateCacheDesiredSize_STextBlock, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("TickWidgets"), STAT_SlateTickWidgets, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("DrawWindows"), STAT_SlateDrawWindowTime, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint SViewport"), STAT_SlateOnPaint_SViewport, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint SImage"), STAT_SlateOnPaint_SImage, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint SBox"), STAT_SlateOnPaint_SBox, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint SCompoundWidget"), STAT_SlateOnPaint_SCompoundWidget, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint SBorder"), STAT_SlateOnPaint_SBorder, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint SPanel"), STAT_SlateOnPaint_SPanel, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint SOverlay"), STAT_SlateOnPaint_SOverlay, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint SEditableText"), STAT_SlateOnPaint_SEditableText, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint STextBlock"), STAT_SlateOnPaint_STextBlock, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint SRichTextBlock"), STAT_SlateOnPaint_SRichTextBlock, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint SSlider"), STAT_SlateOnPaint_SSlider, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint SGraphPanel"), STAT_SlateOnPaint_SGraphPanel, STATGROUP_Slate , SLATE_API);
DECLARE_CYCLE_STAT_EXTERN( TEXT("Slate Misc Time"), STAT_SlateMiscTime, STATGROUP_Slate , );


#if PLATFORM_IOS
	#include "IOS/IOSPlatformTextField.h"
#else
	#include "GenericPlatformTextField.h"
#endif

#include "InputCore.h"

#include "SlateDelegates.h"

// Application
#include "MenuStack.h"
#include "IPlatformTextField.h"
#include "IWidgetReflector.h"
#include "SlateApplication.h"
#include "SlateIcon.h"

// Commands
#include "InputGesture.h"
#include "UIAction.h"
#include "UICommandInfo.h"
#include "InputBindingManager.h"
#include "Commands.h"
#include "UICommandList.h"

// Legacy
#include "SMissingWidget.h"
#include "SlateScrollHelper.h"
#include "SlateStyleRegistry.h"
#include "UICommandDragDropOp.h"
#include "InertialScrollManager.h"
#include "GenericCommands.h"
#include "SWeakWidget.h"
#include "TextLayoutEngine.h"
#include "SPanel.h"
#include "SCompoundWidget.h"
#include "SFxWidget.h"
#include "SBorder.h"
#include "SSeparator.h"
#include "SSpacer.h"
#include "SWrapBox.h"
#include "SImage.h"
#include "SSpinningImage.h"
#include "SProgressBar.h"
#include "SCanvas.h"
#include "STextBlock.h"
#include "TextDecorators.h"
#include "SRichTextBlock.h"
#include "SBox.h"
#include "SHeader.h"
#include "SGridPanel.h"
#include "SUniformGridPanel.h"
#include "SMenuAnchor.h"
#include "MultiBoxDefs.h"
////
#include "MultiBox.h"
////
#include "MultiBoxBuilder.h"
#include "MultiBoxExtender.h"
#include "SMultiLineEditableText.h"
#include "SMultiLineEditableTextBox.h"
#include "SEditableText.h"
#include "SEditableTextBox.h"
#include "SSearchBox.h"
#include "SButton.h"
#include "SToolTip.h"
#include "SScrollBar.h"
#include "SScrollBorder.h"
#include "SErrorText.h"
#include "SErrorHint.h"
#include "SPopUpErrorText.h"
#include "SSplitter.h"
#include "TableViewTypeTraits.h"
#include "SExpanderArrow.h"
#include "ITypedTableView.h"
#include "STableViewBase.h"
#include "SHeaderRow.h"
#include "STableRow.h"
#include "SListView.h"
#include "STileView.h"
#include "STreeView.h"
#include "SScrollBox.h"
#include "SViewport.h"
#include "SColorBlock.h"
#include "SCheckBox.h"
#include "SSpinBox.h"
#include "SSlider.h"
#include "SVolumeControl.h"
#include "SColorSpectrum.h"
#include "SColorWheel.h"
#include "SColorThemes.h"
#include "SColorPicker.h"
///
#include "SToolBarButtonBlock.h"
#include "SToolBarComboButtonBlock.h"
///
#include "SComboButton.h"
#include "SComboBox.h"
#include "SHyperlink.h"
#include "SRichTextHyperlink.h"
#include "SThrobber.h"
#include "STextEntryPopup.h"
#include "STextComboPopup.h"
#include "SExpandableButton.h"
#include "SExpandableArea.h"
#include "SNotificationList.h"
#include "SWidgetSwitcher.h"
#include "SSuggestionTextBox.h"
#include "SBreadcrumbTrail.h"
#include "SWizard.h"
#include "STextComboBox.h"
#include "SNumericEntryBox.h"
#include "SEditableComboBox.h"
#include "NotificationManager.h"
#include "SDPIScaler.h"
#include "SInlineEditableTextBlock.h"
#include "SVirtualKeyboardEntry.h"
#include "STutorialWrapper.h"
#include "ScrollyZoomy.h"
#include "SSafeZone.h"
#include "MarqueeRect.h"
#include "SRotatorInputBox.h"
#include "SVectorInputBox.h"
#include "SVirtualJoystick.h"

// Docking Framework
#include "WorkspaceItem.h"
#include "TabManager.h"
#include "SDockTab.h"
#include "LayoutService.h"

// Old docking
#include "SDockableTab.h"
#include "SDockTabStack.h"

// Test suite
#include "STestSuite.h"
