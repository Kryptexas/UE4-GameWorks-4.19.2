// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LevelSequenceEditorPCH.h"
#include "CinematicLevelViewportLayout.h"
#include "LevelViewportLayout.h"
#include "SLevelViewport.h"
#include "CinematicViewport/SCinematicLevelViewport.h"
#include "LevelViewportActions.h"

class FCinematicViewportLayoutEntity : public IViewportLayoutEntity
{
public:
	FCinematicViewportLayoutEntity(TSharedRef<SWidget> InWidget)
		: Widget(InWidget)
	{}

	virtual TSharedRef<SWidget> AsWidget() const override { return Widget; }

	bool IsPlayInEditorViewportActive() const { return false; }
	void RegisterGameViewportIfPIE(){}
	void SetKeyboardFocus(){ FSlateApplication::Get().SetKeyboardFocus(Widget); }
	void OnLayoutDestroyed(){}
	void SaveConfig(const FString&){}

protected:

	/** This entity's widget */
	TSharedRef<SWidget> Widget;
};

TSharedRef<SWidget> FCinematicLevelViewportLayout_OnePane::MakeViewportLayout(const FString& LayoutString)
{
	FString SpecificLayoutString = GetTypeSpecificLayoutString(LayoutString);

	TSharedPtr<SCinematicLevelViewport> ViewportWidget;

	FName CinematicKey = "Cinematic";

	WidgetSlot = SNew(SBox)
	[
		SAssignNew(ViewportWidget, SCinematicLevelViewport)
		.ParentLayout(AsShared())
		.LayoutName(CinematicKey)
		.RevertToLayoutName(LevelViewportConfigurationNames::OnePane)
		.bShowMaximize(false)
	];

	Viewports.Add( CinematicKey, MakeShareable( new FCinematicViewportLayoutEntity(ViewportWidget.ToSharedRef()) ) );

	// Make newly-created perspective viewports active by default
	GCurrentLevelEditingViewportClient = ViewportWidget->GetViewportClient().Get();

	return WidgetSlot.ToSharedRef();
}

void FCinematicLevelViewportLayout_OnePane::SaveLayoutString(const FString& LayoutString) const
{
}

const FName& FCinematicLevelViewportLayout_OnePane::GetLayoutTypeName() const
{
	static FName LayoutTypeName("OnePaneCinematic");
	return LayoutTypeName;
}

void FCinematicLevelViewportLayout_OnePane::ReplaceWidget( TSharedRef<SWidget> Source, TSharedRef<SWidget> Replacement)
{
	checkf(Source == WidgetSlot->GetChildren()->GetChildAt(0), TEXT("Source widget should have already been a content widget for the splitter"));
	WidgetSlot->SetContent(Replacement);
}

TSharedRef<SWidget> FCinematicLevelViewportLayout_TwoPane::MakeViewportLayout(const FString& LayoutString)
{
	FString SpecificLayoutString = GetTypeSpecificLayoutString(LayoutString);

	TSharedPtr<SLevelViewport> ViewportWidget0;
	TSharedPtr<SCinematicLevelViewport> ViewportWidget1;

	FString ViewportKey0;
	float SplitterPercentage = 0.5f;

	if (!SpecificLayoutString.IsEmpty())
	{
		// The Layout String only holds the unique ID of the Additional Layout Configs to use
		const FString& IniSection = FLayoutSaveRestore::GetAdditionalLayoutConfigIni();

		ViewportKey0 = SpecificLayoutString + TEXT(".Viewport0");

		FString PercentageString;
		if (GConfig->GetString(*IniSection, *(SpecificLayoutString + TEXT(".Percentage")), PercentageString, GEditorPerProjectIni))
		{
			TTypeFromString<float>::FromString(SplitterPercentage, *PercentageString);
		}
	}

	FName CinematicKey = "Cinematic";

	SplitterWidget = 
		SNew( SSplitter )
		.Orientation(EOrientation::Orient_Horizontal)

		+SSplitter::Slot()
		.Value(SplitterPercentage)
		[
			SAssignNew(ViewportWidget0, SLevelViewport)
			.ViewportType(LVT_Perspective)
			.ParentLayout(AsShared())
			.ParentLevelEditor(ParentLevelEditor)
			.ConfigKey(ViewportKey0)
		]

		+SSplitter::Slot()
		.Value(1.0f - SplitterPercentage)
		[
			SAssignNew(ViewportWidget1, SCinematicLevelViewport)
			.ParentLayout(AsShared())
			.LayoutName(CinematicKey)
			.RevertToLayoutName(LevelViewportConfigurationNames::TwoPanesHoriz)
		];

	ViewportWidget0->GetLevelViewportClient().SetAllowCinematicPreview(false);

	Viewports.Add( *ViewportKey0, MakeShareable( new FLevelViewportLayoutEntity(ViewportWidget0.ToSharedRef()) ) );
	Viewports.Add( CinematicKey, MakeShareable( new FCinematicViewportLayoutEntity(ViewportWidget1.ToSharedRef()) ) );

	// Make newly-created perspective viewports active by default
	GCurrentLevelEditingViewportClient = &ViewportWidget0->GetLevelViewportClient();

	InitCommonLayoutFromString(SpecificLayoutString);

	return SplitterWidget.ToSharedRef();
}

void FCinematicLevelViewportLayout_TwoPane::SaveLayoutString(const FString& LayoutString) const
{
	if (!bIsTransitioning)
	{
		FString SpecificLayoutString = GetTypeSpecificLayoutString(LayoutString);

		const FString& IniSection = FLayoutSaveRestore::GetAdditionalLayoutConfigIni();

		float Percentage = SplitterWidget->SlotAt(0).SizeValue.Get();
		GConfig->SetString(*IniSection, *(SpecificLayoutString + TEXT(".Percentage")), *TTypeToString<float>::ToString(Percentage), GEditorPerProjectIni);

		SaveCommonLayoutString(SpecificLayoutString);
	}
}

const FName& FCinematicLevelViewportLayout_TwoPane::GetLayoutTypeName() const
{
	static FName LayoutTypeName("TwoPaneCinematic");
	return LayoutTypeName;
}

void FCinematicLevelViewportLayout_TwoPane::ReplaceWidget( TSharedRef<SWidget> Source, TSharedRef<SWidget> Replacement)
{
	for (int32 SlotIdx = 0; SlotIdx < SplitterWidget->GetChildren()->Num(); SlotIdx++)
	{
		if  (SplitterWidget->GetChildren()->GetChildAt(SlotIdx) == Source)
		{
			SplitterWidget->SlotAt(SlotIdx)
			[
				Replacement
			];
			return;
		}
	}

	checkf(false, TEXT("Source widget should have already been a content widget for the splitter"));
}
