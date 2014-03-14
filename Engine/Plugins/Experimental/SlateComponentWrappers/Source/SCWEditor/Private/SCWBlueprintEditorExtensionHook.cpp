// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersEditorPrivatePCH.h"
#include "SCWBlueprintEditorExtensionHook.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

#include "BlueprintEditor.h"
#include "BlueprintEditorModes.h"
#include "BlueprintEditorTabs.h"

#define LOCTEXT_NAMESPACE "SlateComponentWrappers"

//////////////////////////////////////////////////////////////////////////

FWorkflowApplicationModeExtender BlueprintEditorExtenderDelegate;

/////////////////////////////////////////////////////
// SSlateBlueprintPreview

class SSlateBlueprintPreview : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SSlateBlueprintPreview ) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor)
	{
		LastPreviewActor = NULL;
		BlueprintEditor = InBlueprintEditor;
	}

	// SWidget interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) OVERRIDE;
	// End of Swidget interface
protected:
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
	TWeakObjectPtr<AActor> LastPreviewActor;
	TWeakPtr<SWidget> LastPreviewWidget;
};

void SSlateBlueprintPreview::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	AActor* PreviewActor = BlueprintEditor.Pin()->GetPreviewActor();
	if (PreviewActor != LastPreviewActor.Get())
	{
		LastPreviewActor = PreviewActor;
	}

	if (ACompoundWidgetActor* WidgetActor = Cast<ACompoundWidgetActor>(PreviewActor))
	{
		TSharedRef<SWidget> CurrentWidget = WidgetActor->GetWidget();

		if (CurrentWidget != LastPreviewWidget.Pin())
		{
			LastPreviewWidget = CurrentWidget;
			ChildSlot
			[
				CurrentWidget
			];
		}
	}
	else
	{
		ChildSlot
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NoWrappedWidget", "No actor; Open the viewport and tab back").ToString())
			]
		];
	}

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

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
		return SNew(SSlateBlueprintPreview, BlueprintEditor.Pin());
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
		BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FSlatePreviewSummoner(InBlueprintEditor)));

	TabLayout = FTabManager::NewLayout( "Standalone_SlateWrappedComponentsEditor_Layout_v1" )
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
						->AddTab( FBlueprintEditorTabs::ConstructionScriptEditorID, ETabState::OpenedTab )
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient( 0.5f )
						->AddTab( FBlueprintEditorTabs::DetailsID, ETabState::OpenedTab )
					)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient( 0.45f )
					->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetHideTabWell(true)
						->AddTab( FBlueprintEditorTabs::SCSViewportID, ETabState::OpenedTab )
					)
					->Split
					(
						//@TODO: Widget reflector should go here!
						FTabManager::NewStack()
						->SetHideTabWell(true)
					)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient( 0.55f )
					->SetHideTabWell(true)
					->AddTab( SlatePreviewTabID, ETabState::OpenedTab )
				)
			)
		);

	}

	UBlueprint* GetBlueprint() const
	{
		if (FBlueprintEditor* Editor = MyBlueprintEditor.Pin().Get())
		{
			return Editor->GetBlueprintObj();
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

	void Foo()
	{
		if (AActor* PreviewActor = GetPreviewActor())
		{
		}
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
				if (BP->ParentClass->IsChildOf(ACompoundWidgetActor::StaticClass()))
				{
					return MakeShareable(new FComponentsEditorModeOverride(LieMode->GetBlueprintEditor()));
				}
			}
		}

		return InMode;
	}
};

//////////////////////////////////////////////////////////////////////////
// FSCWBlueprintEditorExtensionHook

void FSCWBlueprintEditorExtensionHook::InstallHooks()
{
	BlueprintEditorExtenderDelegate = FWorkflowApplicationModeExtender::CreateStatic(&FBlueprintEditorExtenderForSlatePreview::OnModeCreated);
	FWorkflowCentricApplication::GetModeExtenderList().Add(BlueprintEditorExtenderDelegate);
}

void FSCWBlueprintEditorExtensionHook::RemoveHooks()
{
	FWorkflowCentricApplication::GetModeExtenderList().Remove(BlueprintEditorExtenderDelegate);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE