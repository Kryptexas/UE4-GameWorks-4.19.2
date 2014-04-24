// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "UMGBlueprintEditorExtensionHook.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

#include "BlueprintEditor.h"
#include "BlueprintEditorModes.h"
#include "BlueprintEditorTabs.h"
#include "SSCSEditorViewport.h"

#include "SUMGEditorTree.h"
#include "SUMGDesigner.h"

#define LOCTEXT_NAMESPACE "UMG_EXTENSION"

/////////////////////////////////////////////////////
// FSlatePreviewSummoner

static const FName SlatePreviewTabID(TEXT("SlatePreview"));

struct FSlatePreviewSummoner : public FWorkflowTabFactory
{
protected:
	TWeakPtr<class FBlueprintEditor> BlueprintEditor;
public:
	FSlatePreviewSummoner(TSharedPtr<class FBlueprintEditor> InBlueprintEditor)
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
		TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();

		TSharedPtr<SWidget> Result;
		if ( BlueprintEditorPtr->CanAccessComponentsMode() )
		{
			Result = BlueprintEditorPtr->GetSCSViewport();
		}

		if ( Result.IsValid() )
		{
			return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0)
			[
				// HACK: Super hack to make sure we have a preview actor.
				Result.ToSharedRef()
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SNew(SUMGDesigner, BlueprintEditorPtr)
			];
		}
		else
		{
			return SNew(SErrorText)
				.BackgroundColor(FLinearColor::Transparent)
				.ErrorText(LOCTEXT("SCSViewportView_Unavailable", "Viewport is not available for this Blueprint.").ToString());
		}
	}
};

/////////////////////////////////////////////////////
// FSlateTreeSummoner

static const FName SlateHierarchyTabID(TEXT("SlateHierarchy"));

struct FSlateTreeSummoner : public FWorkflowTabFactory
{
protected:
	TWeakPtr<class FBlueprintEditor> BlueprintEditor;
public:
	FSlateTreeSummoner(TSharedPtr<class FBlueprintEditor> InBlueprintEditor)
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
	TWeakPtr<class FBlueprintEditor> BlueprintEditor;
public:
	FWidgetTemplatesSummoner(TSharedPtr<class FBlueprintEditor> InBlueprintEditor)
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


//////////////////////////////////////////////////////////////////////////
// FComponentsEditorModeOverride

class FComponentsEditorModeOverride : public FBlueprintComponentsApplicationMode
{
public:
	FComponentsEditorModeOverride(TSharedPtr<class FBlueprintEditor> InBlueprintEditor)
		: FBlueprintComponentsApplicationMode(InBlueprintEditor)
	{
		//BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FConstructionScriptEditorSummoner(InBlueprintEditor)));
		//BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FSCSViewportSummoner(InBlueprintEditor)));
		//BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FSelectionDetailsSummoner(InBlueprintEditor)));
		//BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FDefaultsEditorSummoner(InBlueprintEditor)));
		//BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FFindResultsSummoner(InBlueprintEditor)));

		//BlueprintComponentsTabFactories.UnregisterFactory(FBlueprintEditorTabs::ConstructionScriptEditorID);
		BlueprintComponentsTabFactories.UnregisterFactory(FBlueprintEditorTabs::SCSViewportID);
		BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FSlatePreviewSummoner(InBlueprintEditor)));
		BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FSlateTreeSummoner(InBlueprintEditor)));
		BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FWidgetTemplatesSummoner(InBlueprintEditor)));

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
		);
	}

	virtual void PreDeactivateMode() OVERRIDE
	{
		FBlueprintComponentsApplicationMode::PreDeactivateMode();

		GetBlueprintEditor()->GetInspector()->EnableComponentDetailsCustomization(false);
	}

	virtual void PostActivateMode() OVERRIDE
	{
		FBlueprintComponentsApplicationMode::PostActivateMode();

		GetBlueprintEditor()->GetInspector()->EnableComponentDetailsCustomization(false);
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
// FBlueprintEditorExtenderForSlatePreview

class FBlueprintEditorExtenderForSlatePreview
{
public:
	static TSharedRef<FApplicationMode> OnModeCreated(const FName ModeName, TSharedRef<FApplicationMode> InMode)
	{
		if (ModeName == FBlueprintEditorApplicationModes::BlueprintComponentsMode)
		{
			//@TODO: Bit of a lie - push GetBlueprint up, or pass in editor!
			auto LieMode = StaticCastSharedRef<FComponentsEditorModeOverride>(InMode);

			if (UBlueprint* BP = LieMode->GetBlueprint())
			{
				if (BP->ParentClass->IsChildOf(AUserWidget::StaticClass()))
				{
					return MakeShareable(new FComponentsEditorModeOverride(LieMode->GetBlueprintEditor()));
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