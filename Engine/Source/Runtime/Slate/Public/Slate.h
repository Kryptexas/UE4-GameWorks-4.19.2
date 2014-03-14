// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#ifndef _INC_SLATE
#define _INC_SLATE


#include "Core.h"
#include "CoreUObject.h"
#include "SlateCore.h"

// Enables or disables more slate stats.  These stats are usually per widget and can be gathered hundreds or thousands of times per frame
// enabling these constantly causes a bit of a performance drop due to stat gathering overhead so these should only be enabled for profiling
#define SLATE_HD_STATS 0

SLATE_API DECLARE_LOG_CATEGORY_EXTERN(LogSlate, Log, All);
SLATE_API DECLARE_LOG_CATEGORY_EXTERN(LogSlateStyles, Log, All);

#define BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION PRAGMA_DISABLE_OPTIMIZATION
#define END_SLATE_FUNCTION_BUILD_OPTIMIZATION   PRAGMA_ENABLE_OPTIMIZATION

/** Stat tracking for Slate */
DECLARE_CYCLE_STAT_EXTERN( TEXT("Total Slate Tick Time"), STAT_SlateTickTime, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("CacheDesiredSize"), STAT_SlateCacheDesiredSize, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("STextBlock"), STAT_SlateCacheDesiredSize_STextBlock, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("TickWidgets"), STAT_SlateTickWidgets, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("DrawWindows"), STAT_SlateDrawWindowTime, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("OnPaint"), STAT_SlateOnPaint, STATGROUP_Slate , );
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
DECLARE_CYCLE_STAT_EXTERN( TEXT("Slate Rendering GT Time"), STAT_SlateRenderingGTTime, STATGROUP_Slate , SLATE_API);
DECLARE_CYCLE_STAT_EXTERN( TEXT("Font Caching"), STAT_SlateFontCachingTime, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("Build Elements"), STAT_SlateBuildElements, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("Find Batch"), STAT_SlateFindBatchTime, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("Build Lines"), STAT_SlateBuildLines, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("Build Splines"), STAT_SlateBuildSplines, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("Build Gradients"), STAT_SlateBuildGradients, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("Build Text"), STAT_SlateBuildText, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("Build Border"), STAT_SlateBuildBorders, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("Build Boxes"), STAT_SlateBuildBoxes, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("Build Viewports"), STAT_SlateBuildViewports, STATGROUP_Slate , );
DECLARE_CYCLE_STAT_EXTERN( TEXT("Update Buffers GT"), STAT_SlateUpdateBufferGTTime, STATGROUP_Slate , );


DECLARE_CYCLE_STAT_EXTERN( TEXT("Slate Rendering RT Time"), STAT_SlateRenderingRTTime, STATGROUP_Slate , SLATE_API);
DECLARE_CYCLE_STAT_EXTERN( TEXT("Update Buffers RT"), STAT_SlateUpdateBufferRTTime, STATGROUP_Slate , SLATE_API);
DECLARE_CYCLE_STAT_EXTERN( TEXT("Draw Time"), STAT_SlateDrawTime, STATGROUP_Slate , SLATE_API);

DECLARE_DWORD_COUNTER_STAT_EXTERN( TEXT("Num Layers"), STAT_SlateNumLayers, STATGROUP_Slate , );
DECLARE_DWORD_COUNTER_STAT_EXTERN( TEXT("Num Batches"), STAT_SlateNumBatches, STATGROUP_Slate , );
DECLARE_DWORD_COUNTER_STAT_EXTERN( TEXT("Num Vertices"), STAT_SlateVertexCount, STATGROUP_Slate , );

DECLARE_CYCLE_STAT_EXTERN( TEXT("Measure String"), STAT_SlateMeasureStringTime, STATGROUP_Slate , );

DECLARE_CYCLE_STAT_EXTERN( TEXT("Slate Misc Time"), STAT_SlateMiscTime, STATGROUP_Slate , );

DECLARE_MEMORY_STAT_EXTERN( TEXT("Batch Vertex Memory"), STAT_SlateVertexBatchMemory, STATGROUP_SlateMemory , );
DECLARE_MEMORY_STAT_EXTERN( TEXT("Batch Index Memory"), STAT_SlateIndexBatchMemory, STATGROUP_SlateMemory , );
DECLARE_MEMORY_STAT_EXTERN( TEXT("Vertex Buffer Memory"), STAT_SlateVertexBufferMemory, STATGROUP_SlateMemory , SLATE_API);
DECLARE_MEMORY_STAT_EXTERN( TEXT("Index Buffer Memory"), STAT_SlateIndexBufferMemory, STATGROUP_SlateMemory , SLATE_API);

DECLARE_MEMORY_STAT_EXTERN( TEXT("Texture Data CPU Memory"), STAT_SlateTextureDataMemory, STATGROUP_SlateMemory, SLATE_API );
DECLARE_MEMORY_STAT_EXTERN( TEXT("Texture Atlas Memory"), STAT_SlateTextureAtlasMemory, STATGROUP_SlateMemory, );
DECLARE_MEMORY_STAT_EXTERN( TEXT("Font Kerning Table Memory"), STAT_SlateFontKerningTableMemory, STATGROUP_SlateMemory, );
DECLARE_MEMORY_STAT_EXTERN( TEXT("Font Measure Memory"), STAT_SlateFontMeasureCacheMemory, STATGROUP_SlateMemory, );

DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN( TEXT("Num Texture Atlases"), STAT_SlateNumTextureAtlases, STATGROUP_SlateMemory, SLATE_API );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN( TEXT("Num Font Atlases"), STAT_SlateNumFontAtlases, STATGROUP_SlateMemory,  );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN( TEXT("Num Non-Atlased Textures"), STAT_SlateNumNonAtlasedTextures, STATGROUP_SlateMemory,SLATE_API );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN( TEXT("Num Dynamic Textures"), STAT_SlateNumDynamicTextures, STATGROUP_SlateMemory, SLATE_API);


#if PLATFORM_IOS
#include "IOS/IOSPlatformTextField.h"
#else
#include "GenericPlatformTextField.h"
#endif

#include "StyleDefaults.h"
#include "CoreStyle.h"
#include "InputCore.h"
#include "ISlateSoundDevice.h"

#include "WidgetPath.h"
#include "SlateSprings.h"
#include "SlateScrollHelper.h"
#include "ISlateStyle.h"
#include "SlateStyleRegistry.h"
#include "Events.h"
#include "CursorReply.h"
#include "DragAndDrop.h"
#include "Reply.h"
#include "SlateDelegates.h"
#include "SNullWidget.h"
#include "DeclarativeSyntaxSupport.h"
#include "RenderingCommon.h"
#include "ThrottleManager.h"
#include "InertialScrollManager.h"
#include "MenuStack.h"
#include "EventLogger.h"
#include "SlateApplication.h"
#include "DragAndDrop.inl"
#include "InputBinding.h"
#include "Commands.h"
#include "GenericCommands.h"
#include "Renderer.h"
#include "DrawElements.h"
#include "Children.h"
#include "SWidget.h"
#include "WidgetPath.inl"
#include "SLeafWidget.h"
#include "SWeakWidget.h"
#include "TextLayoutEngine.h"
#include "SPanel.h"
#include "SOverlay.h"
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
#include "SBoxPanel.h"
#include "SHeader.h"
#include "SGridPanel.h"
#include "SUniformGridPanel.h"
#include "SMultiLineEditableText.h"
#include "SEditableText.h"
#include "SEditableTextBox.h"
#include "SSearchBox.h"
#include "SButton.h"
#include "SToolTip.h"
#include "SScrollBar.h"
#include "SMenuAnchor.h"
#include "SErrorText.h"
#include "SErrorHint.h"
#include "SPopupErrorText.h"
#include "SSplitter.h"
#include "TableView/TableViewTypeTraits.h"
#include "TableView/SExpanderArrow.h"
#include "TableView/ITypedTableView.h"
#include "TableView/STableViewBase.h"
#include "TableView/SHeaderRow.h"
#include "TableView/STableRow.h"
#include "TableView/SListView.h"
#include "TableView/STileView.h"
#include "TableView/STreeView.h"
#include "SScrollBox.h"
#include "SWindow.h"
#include "RenderingPolicy.h"
#include "ElementBatcher.h"
#include "Renderer.h"
#include "TextureAtlas.h"
#include "FontTypes.h"
#include "FontMeasure.h"
#include "TextureManager.h"
#include "SViewport.h"
#include "SColorBlock.h"
#include "SCheckBox.h"
#include "SSpinBox.h"
#include "SSlider.h"
#include "SVolumeControl.h"
#include "SToolTip.h"
#include "SColorSpectrum.h"
#include "SColorWheel.h"
#include "SColorThemes.h"
#include "SColorPicker.h"
#include "MultiBox/MultiBoxBuilder.h"
#include "MultiBox/MultiBoxExtender.h"
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

// Docking Framework
#include "DockingFramework/WorkspaceItem.h"
#include "DockingFramework/TabManager.h"
#include "DockingFramework/SDockTab.h"
#include "DockingFramework/LayoutService.h"


// Old docking
#include "Docking/SDockableTab.h"
#include "Docking/SDockTabStack.h"

// Test suite
#include "Testing/STestSuite.h"

#endif // _INC_SLATE
