// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "UMGBlueprintEditorExtensionHook.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

#include "BlueprintEditor.h"
#include "BlueprintEditorModes.h"
#include "BlueprintEditorTabs.h"

#include "SUMGEditorTree.h"
#include "SUMGDesigner.h"
#include "SUMGEditorWidgetTemplates.h"
#include "SKismetInspector.h"
#include "IDetailsView.h"

#include "DetailCustomizations.h"
#include "CanvasSlotCustomization.h"

#include "WidgetBlueprintEditor.h"

#include "MovieScene.h"
#include "ISequencerModule.h"

#define LOCTEXT_NAMESPACE "UMG_EXTENSION"

/////////////////////////////////////////////////////
// FSlatePreviewSummoner

static const FName SlatePreviewTabID(TEXT("SlatePreview"));

struct FSlatePreviewSummoner : public FWorkflowTabFactory
{
protected:
	TWeakPtr<class FWidgetBlueprintEditor> BlueprintEditor;
public:
	FSlatePreviewSummoner(TSharedPtr<class FWidgetBlueprintEditor> InBlueprintEditor)
		: FWorkflowTabFactory(SlatePreviewTabID, InBlueprintEditor)
		, BlueprintEditor(InBlueprintEditor)
	{
		TabLabel = LOCTEXT("SlatePreviewTabLabel", "UI Preview");
		//TabIcon = FEditorStyle::GetBrush("Kismet.Tabs.Components");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("SlatePreview_ViewMenu_Desc", "UI Preview");
		ViewMenuTooltip = LOCTEXT("SlatePreview_ViewMenu_ToolTip", "Show the UI preview");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		//TSharedPtr<SWidget> Result;
		//if ( BlueprintEditorPtr->CanAccessComponentsMode() )
		//{
		//	Result = BlueprintEditorPtr->GetSCSViewport();
		//}

		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SNew(SUMGDesigner, BlueprintEditor.Pin())
			];
	}
};

/////////////////////////////////////////////////////
// FSlateTreeSummoner

static const FName SlateHierarchyTabID(TEXT("SlateHierarchy"));

struct FSlateTreeSummoner : public FWorkflowTabFactory
{
protected:
	TWeakPtr<class FWidgetBlueprintEditor> BlueprintEditor;
public:
	FSlateTreeSummoner(TSharedPtr<class FWidgetBlueprintEditor> InBlueprintEditor)
		: FWorkflowTabFactory(SlateHierarchyTabID, InBlueprintEditor)
		, BlueprintEditor(InBlueprintEditor)
	{
		TabLabel = LOCTEXT("SlateHierarchyTabLabel", "Hierarchy");
		//TabIcon = FEditorStyle::GetBrush("Kismet.Tabs.Components");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("SlateHierarchy_ViewMenu_Desc", "Hierarchy");
		ViewMenuTooltip = LOCTEXT("SlateHierarchy_ViewMenu_ToolTip", "Show the Hierarchy");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		return SNew(SUMGEditorTree, BlueprintEditor.Pin(), BlueprintEditor.Pin()->GetBlueprintObj()->SimpleConstructionScript);
	}
};


/////////////////////////////////////////////////////
// FWidgetTemplatesSummoner

static const FName UMGWidgetTemplatesTabID(TEXT("WidgetTemplates"));

struct FWidgetTemplatesSummoner : public FWorkflowTabFactory
{
protected:
	TWeakPtr<class FWidgetBlueprintEditor> BlueprintEditor;
public:
	FWidgetTemplatesSummoner(TSharedPtr<class FWidgetBlueprintEditor> InBlueprintEditor)
		: FWorkflowTabFactory(UMGWidgetTemplatesTabID, InBlueprintEditor)
		, BlueprintEditor(InBlueprintEditor)
	{
		TabLabel = LOCTEXT("WidgetTemplatesTabLabel", "Templates");
		//TabIcon = FEditorStyle::GetBrush("Kismet.Tabs.Components");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("WidgetTemplates_ViewMenu_Desc", "Templates");
		ViewMenuTooltip = LOCTEXT("WidgetTemplates_ViewMenu_ToolTip", "Show the Templates");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		return SNew(SUMGEditorWidgetTemplates, BlueprintEditor.Pin(), BlueprintEditor.Pin()->GetBlueprintObj()->SimpleConstructionScript);
	}
};

/////////////////////////////////////////////////////
// FSequencerSummoner

static const FName FSequencerSummonerTabID(TEXT("Sequencer"));

struct FSequencerSummoner : public FWorkflowTabFactory
{
protected:
	TWeakPtr<class FWidgetBlueprintEditor> BlueprintEditor;
public:
	FSequencerSummoner(TSharedPtr<class FWidgetBlueprintEditor> InBlueprintEditor)
		: FWorkflowTabFactory(FSequencerSummonerTabID, InBlueprintEditor)
		, BlueprintEditor(InBlueprintEditor)
	{
		TabLabel = LOCTEXT("SequencerLabel", "Timeline");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("Sequencer_ViewMenu_Desc", "Timeline");
		ViewMenuTooltip = LOCTEXT("Sequencer_ViewMenu_ToolTip", "Show the Animation editor");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		TSharedPtr<FWidgetBlueprintEditor> BlueprintEditorPinned = BlueprintEditor.Pin();

		return BlueprintEditorPinned->GetSequencer()->GetSequencerWidget();
	}
};


//////////////////////////////////////////////////////////////////////////
// FComponentsEditorModeOverride

class FComponentsEditorModeOverride : public FBlueprintDefaultsApplicationMode
{
public:
	FComponentsEditorModeOverride(TSharedPtr<class FBlueprintEditor> InBlueprintEditor)
		: FBlueprintDefaultsApplicationMode(InBlueprintEditor)
	{
		//BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FConstructionScriptEditorSummoner(InBlueprintEditor)));
		//BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FSCSViewportSummoner(InBlueprintEditor)));
		//BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FSelectionDetailsSummoner(InBlueprintEditor)));
		//BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FDefaultsEditorSummoner(InBlueprintEditor)));
		//BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FFindResultsSummoner(InBlueprintEditor)));

		//BlueprintComponentsTabFactories.UnregisterFactory(FBlueprintEditorTabs::ConstructionScriptEditorID);
		//BlueprintDefaultsTabFactories.UnregisterFactory(FBlueprintEditorTabs::SCSViewportID);

		TSharedPtr<FWidgetBlueprintEditor> WidgetBlueprintEditor = StaticCastSharedPtr<FWidgetBlueprintEditor>(InBlueprintEditor);


		BlueprintDefaultsTabFactories.RegisterFactory(MakeShareable(new FSelectionDetailsSummoner(WidgetBlueprintEditor)));
		BlueprintDefaultsTabFactories.RegisterFactory(MakeShareable(new FSlatePreviewSummoner(WidgetBlueprintEditor)));
		BlueprintDefaultsTabFactories.RegisterFactory(MakeShareable(new FSlateTreeSummoner(WidgetBlueprintEditor)));
		BlueprintDefaultsTabFactories.RegisterFactory(MakeShareable(new FWidgetTemplatesSummoner(WidgetBlueprintEditor)));
		BlueprintDefaultsTabFactories.RegisterFactory(MakeShareable(new FSequencerSummoner(WidgetBlueprintEditor)));

		TabLayout = FTabManager::NewLayout( "Standalone_UMGEditor_Layout_v2" )
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient( 0.2f )
				->SetHideTabWell(true)
				->AddTab( InBlueprintEditor->GetToolbarTabId(), ETabState::OpenedTab )
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient( 0.15f )
					->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient( 0.5f )
						->AddTab( UMGWidgetTemplatesTabID, ETabState::OpenedTab )
					)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient( 0.15f )
					->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient( 0.5f )
						->AddTab( SlateHierarchyTabID, ETabState::OpenedTab )
					)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient( 0.45f )
					->SetOrientation(Orient_Horizontal)
					->Split
					(
						FTabManager::NewStack()
						->SetHideTabWell(true)
						->AddTab( SlatePreviewTabID, ETabState::OpenedTab )
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient( 0.5f )
						->AddTab( FBlueprintEditorTabs::DetailsID, ETabState::OpenedTab )
					)
				)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.2f)
				->SetHideTabWell(true)
				->AddTab( FSequencerSummonerTabID, ETabState::OpenedTab )
			)
		);
	}

	virtual void PreDeactivateMode() OVERRIDE
	{
		FBlueprintDefaultsApplicationMode::PreDeactivateMode();

		GetBlueprintEditor()->GetInspector()->EnableComponentDetailsCustomization(false);

		static FName PropertyEditor("PropertyEditor");
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
		PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("CanvasPanelSlot"));
	}

	virtual void PostActivateMode() OVERRIDE
	{
		FBlueprintDefaultsApplicationMode::PostActivateMode();

		GetBlueprintEditor()->GetInspector()->EnableComponentDetailsCustomization(false);

		TSharedRef<class SKismetInspector> Inspector = MyBlueprintEditor.Pin()->GetInspector();
		FOnGetDetailCustomizationInstance LayoutDelegateDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FBlueprintWidgetCustomization::MakeInstance, MyBlueprintEditor.Pin()->GetBlueprintObj());
		Inspector->GetPropertyView()->RegisterInstancedCustomPropertyLayout(UWidget::StaticClass(), LayoutDelegateDetails);

		FOnGetPropertyTypeCustomizationInstance CanvasSlotCustomization = FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCanvasSlotCustomization::MakeInstance, MyBlueprintEditor.Pin()->GetBlueprintObj());
		
		static FName PropertyEditor("PropertyEditor");
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
		PropertyModule.RegisterCustomPropertyTypeLayout(TEXT("PanelSlot"), CanvasSlotCustomization);
	}

	UWidgetBlueprint* GetBlueprint() const
	{
		if (FBlueprintEditor* Editor = MyBlueprintEditor.Pin().Get())
		{
			return Cast<UWidgetBlueprint>(Editor->GetBlueprintObj());
		}
		else
		{
			return NULL;
		}
	}	
	
	TSharedPtr<FBlueprintEditor> GetBlueprintEditor() const
	{
		return MyBlueprintEditor.Pin();
	}
};

//////////////////////////////////////////////////////////////////////////
// FGraphEditorModeOverride

class FGraphEditorModeOverride : public FBlueprintEditorApplicationMode
{
public:
	FGraphEditorModeOverride(TSharedPtr<class FBlueprintEditor> InBlueprintEditor, FName InModeName, const bool bRegisterViewport = true, const bool bRegisterDefaultsTab = true)
		: FBlueprintEditorApplicationMode(InBlueprintEditor, InModeName, bRegisterViewport, bRegisterDefaultsTab)
	{}

	virtual void PreDeactivateMode() OVERRIDE
	{
		FBlueprintEditorApplicationMode::PreDeactivateMode();
	}

	virtual void PostActivateMode() OVERRIDE
	{
		FBlueprintEditorApplicationMode::PostActivateMode();
	}

	UWidgetBlueprint* GetBlueprint() const
	{
		if ( FBlueprintEditor* Editor = MyBlueprintEditor.Pin().Get() )
		{
			return Cast<UWidgetBlueprint>(Editor->GetBlueprintObj());
		}
		else
		{
			return NULL;
		}
	}

	TSharedPtr<FBlueprintEditor> GetBlueprintEditor() const
	{
		return MyBlueprintEditor.Pin();
	}
};

//////////////////////////////////////////////////////////////////////////
// FBlueprintEditorExtenderForSlatePreview

class FBlueprintEditorExtenderForSlatePreview
{
public:
	static TSharedRef<FApplicationMode> OnModeCreated(const FName ModeName, TSharedRef<FApplicationMode> InMode)
	{
		if (ModeName == FBlueprintEditorApplicationModes::BlueprintDefaultsMode)
		{
			//@TODO: Bit of a lie - push GetBlueprint up, or pass in editor!
			auto LieMode = StaticCastSharedRef<FComponentsEditorModeOverride>(InMode);

			if (UBlueprint* BP = LieMode->GetBlueprint())
			{
				if (BP->ParentClass->IsChildOf(UUserWidget::StaticClass()))
				{
					return MakeShareable(new FComponentsEditorModeOverride(LieMode->GetBlueprintEditor()));
				}
			}
		}
		else if ( ModeName == FBlueprintEditorApplicationModes::StandardBlueprintEditorMode )
		{
			//@TODO: Bit of a lie - push GetBlueprint up, or pass in editor!
			auto LieMode = StaticCastSharedRef<FGraphEditorModeOverride>(InMode);

			if ( UBlueprint* BP = LieMode->GetBlueprint() )
			{
				if ( BP->ParentClass->IsChildOf(UUserWidget::StaticClass()) )
				{
					return MakeShareable(new FGraphEditorModeOverride(LieMode->GetBlueprintEditor(), ModeName));
				}
			}
		}

		return InMode;
	}
};

//////////////////////////////////////////////////////////////////////////
// FUMGBlueprintEditorExtensionHook

FWorkflowApplicationModeExtender BlueprintEditorExtenderDelegate;

void FUMGBlueprintEditorExtensionHook::InstallHooks()
{
	BlueprintEditorExtenderDelegate = FWorkflowApplicationModeExtender::CreateStatic(&FBlueprintEditorExtenderForSlatePreview::OnModeCreated);
	FWorkflowCentricApplication::GetModeExtenderList().Add(BlueprintEditorExtenderDelegate);
}

void FUMGBlueprintEditorExtensionHook::RemoveHooks()
{
	FWorkflowCentricApplication::GetModeExtenderList().Remove(BlueprintEditorExtenderDelegate);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE