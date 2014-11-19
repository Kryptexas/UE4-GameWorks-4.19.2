// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "LogVisualizer.h"
#include "LogVisualizerModule.h"
#include "LogVisualizerStyle.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( FPaths::EngineContentDir() / "Editor/Slate"/ RelativePath + TEXT(".png"), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( FPaths::EngineContentDir() / "Editor/Slate"/ RelativePath + TEXT(".png"), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( FPaths::EngineContentDir() / "Editor/Slate"/ RelativePath + TEXT(".png"), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( FPaths::EngineContentDir() / "Editor/Slate"/ RelativePath + TEXT(".ttf"), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( FPaths::EngineContentDir() / "Editor/Slate"/ RelativePath + TEXT(".otf"), __VA_ARGS__ )
#define TTF_CORE_FONT( RelativePath, ... ) FSlateFontInfo( FPaths::EngineContentDir()  / "Slate"/ RelativePath + TEXT(".ttf"), __VA_ARGS__ )

TSharedPtr< FSlateStyleSet > FLogVisualizerStyle::StyleInstance = nullptr;

void FLogVisualizerStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FLogVisualizerStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FLogVisualizerStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("LogVisualizerStyle"));
	return StyleSetName;
}

TSharedRef< FSlateStyleSet > FLogVisualizerStyle::Create()
{
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);
	const FVector2D Icon48x48(48.0f, 48.0f);

	TSharedRef<FSlateStyleSet> StyleRef = MakeShareable(new FSlateStyleSet(FLogVisualizerStyle::GetStyleSetName()));
	StyleRef->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	StyleRef->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	FSlateStyleSet& Style = StyleRef.Get();
	// Generic styles
	{
		Style.Set("LogVisualizer.LogBar.Background", new BOX_BRUSH("Common/ProgressBar_Background", FMargin(5.f / 12.f)));
		Style.Set("LogVisualizer.LogBar.Selected", new BOX_BRUSH("Common/TaskGraph_Selected", FMargin(5.f / 12.f)));
		Style.Set("LogVisualizer.LogBar.EntryDefault", new BOX_BRUSH("Common/TaskGraph_Mono", FMargin(5.f / 12.f)));
		Style.Set("LogVisualizer.LogBar.TimeMark", new BOX_BRUSH("Icons/LV_BarMark", FMargin(5.f / 12.f)));

		//Style.Set("ToolPanel.GroupBorder", FEditorStyle::Get().GetBrush("ToolPanel.GroupBorder"));
		Style.Set("ToolPanel.GroupBorder", new BOX_BRUSH("Common/GroupBorder", FMargin(4.0f / 16.0f)));
		//Style.Set("NoBorder", FEditorStyle::Get().GetBrush("NoBorder"));
		Style.Set("NoBorder", new FSlateNoResource());
		Style.Set("ToolBar.Button.Normal", new FSlateNoResource());
		Style.Set("ToolBar.Button.Hovered", new BOX_BRUSH("Common/RoundedSelection_16x", 4.0f / 16.0f, FLinearColor(0.728f, 0.364f, 0.003f)));

	}

	// Toolbar
	{
		Style.Set("Toolbar.Pause", new IMAGE_BRUSH("Icons/icon_pause_40x", Icon40x40));
		Style.Set("Toolbar.Resume", new IMAGE_BRUSH("Icons/icon_play_40x", Icon40x40));
		Style.Set("Toolbar.Record", new IMAGE_BRUSH("Icons/LV_Record", Icon40x40));
		Style.Set("Toolbar.Stop", new IMAGE_BRUSH("Icons/LV_Stop", Icon40x40));
		Style.Set("Toolbar.Camera", new IMAGE_BRUSH("Icons/LV_Camera", Icon40x40));
		Style.Set("Toolbar.Save", new IMAGE_BRUSH("Icons/LV_Save", Icon40x40));
		Style.Set("Toolbar.Load", new IMAGE_BRUSH("Icons/LV_Load", Icon40x40));
		Style.Set("Toolbar.Remove", new IMAGE_BRUSH("Icons/LV_Remove", Icon40x40));
	}

	// Filters
	{
		Style.Set("Filters.FilterIcon", new IMAGE_BRUSH("Icons/Profiler/Profiler_Filter_Events_16x", Icon16x16));
		Style.Set("Filters.Style", FEditorStyle::Get().GetWidgetStyle<FComboButtonStyle>("ToolbarComboButton"));
		Style.Set("ContentBrowser.FilterButtonBorder", new BOX_BRUSH("Common/RoundedSelection_16x", FMargin(4.0f / 16.0f)));
		//FSlateBrush *FilterButtonBorder = FEditorStyle::Get().GetBrush("ToolbarComboButton");
		//Style.Set("ContentBrowser.FilterButtonBorder", FilterButtonBorder);
		
		//Style.Set("ContentBrowser.FilterButton", FEditorStyle::Get().GetWidgetStyle<FCheckBoxStyle>("ContentBrowser.FilterButton"));
		const FCheckBoxStyle ContentBrowserFilterButtonCheckBoxStyle = FCheckBoxStyle()
			.SetUncheckedImage(IMAGE_BRUSH("ContentBrowser/FilterUnchecked", FVector2D(10.0f, 14.0f)))
			.SetUncheckedHoveredImage(IMAGE_BRUSH("ContentBrowser/FilterUnchecked", FVector2D(10.0f, 14.0f), FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)))
			.SetUncheckedPressedImage(IMAGE_BRUSH("ContentBrowser/FilterUnchecked", FVector2D(10.0f, 14.0f), FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)))
			.SetCheckedImage(IMAGE_BRUSH("ContentBrowser/FilterChecked", FVector2D(10.0f, 14.0f)))
			.SetCheckedHoveredImage(IMAGE_BRUSH("ContentBrowser/FilterChecked", FVector2D(10.0f, 14.0f), FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)))
			.SetCheckedPressedImage(IMAGE_BRUSH("ContentBrowser/FilterChecked", FVector2D(10.0f, 14.0f), FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)));
		/* ... and add the new style */
		Style.Set("ContentBrowser.FilterButton", ContentBrowserFilterButtonCheckBoxStyle);

		Style.Set("ContentBrowser.FilterNameFont", TTF_CORE_FONT("Fonts/Roboto-Regular", 9));
		//Style.Set("ContentBrowser.FilterNameFont", FEditorStyle::Get().GetFontStyle("ContentBrowser.FilterNameFont"));

		FTextBlockStyle FiltersText = FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("ContentBrowser.Filters.Text");
		Style.Set("Filters.Text", FiltersText);
	}

	//Sequencer
	{
		Style.Set("Sequencer.ItemTitle.Normal", new BOX_BRUSH("Common/Button/simple_round_normal", FMargin(4 / 16.0f), FLinearColor(1, 1, 1, 1)));
		Style.Set("Sequencer.ItemTitle.Hover", new BOX_BRUSH("Common/Button/simple_round_hovered", FMargin(4 / 16.0f), FLinearColor(1, 1, 1, 1)));
		Style.Set("Sequencer.SectionArea.Background", new FSlateColorBrush(FColor::White));
	}

	return StyleRef;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

const ISlateStyle& FLogVisualizerStyle::Get()
{
	return *StyleInstance;
}
