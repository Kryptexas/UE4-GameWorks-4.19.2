// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "UMGBlueprintEditorExtensionHook.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

#include "BlueprintEditor.h"
#include "BlueprintEditorModes.h"
#include "BlueprintEditorTabs.h"
#include "SSCSEditorViewport.h"

#include "SUMGEditorTree.h"

#define LOCTEXT_NAMESPACE "UMG_EXTENSION"

//class SDesignerWidget : public SBorder
//{
//	SLATE_BEGIN_ARGS(SDesignerWidget) {}
//		/** Slot for the wrapped content (optional) */
//		SLATE_DEFAULT_SLOT(FArguments, Content)
//	SLATE_END_ARGS()
//
//	void Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor)
//	{
//		LastPreviewActor = NULL;
//		BlueprintEditor = InBlueprintEditor;
//
//		SBorder::Construct(SBorder::FArguments()
//			.BorderImage(FStyleDefaults::GetNoBrush())
//			.Padding(0.0f)
//			[
//
//				InArgs._Content.Widget
//			]);
//		}
//	}
//
//	//virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
//	//virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
///*virtual bool OnHitTest(const FGeometry& MyGeometry, FVector2D InAbsoluteCursorPosition) OVERRIDE;
//{
//return true;
//}*/
//
//protected:
//	TWeakPtr<FBlueprintEditor> BlueprintEditor;
//	TWeakObjectPtr<AActor> LastPreviewActor;
//};


//////////////////////////////////////////////////////////////////////////

FWorkflowApplicationModeExtender BlueprintEditorExtenderDelegate;

static bool LocateWidgetsUnderCursor_Helper(FArrangedWidget& Candidate, FVector2D InAbsoluteCursorLocation, FArrangedChildren& OutWidgetsUnderCursor, bool bIgnoreEnabledStatus)
{
	const bool bCandidateUnderCursor =
		// Candidate is physically under the cursor
		Candidate.Geometry.IsUnderLocation(InAbsoluteCursorLocation) &&
		// Candidate actually considers itself hit by this test
		( Candidate.Widget->OnHitTest(Candidate.Geometry, InAbsoluteCursorLocation) );

	bool bHitAnyWidget = false;
	if ( bCandidateUnderCursor )
	{
		// The candidate widget is under the mouse
		OutWidgetsUnderCursor.AddWidget(Candidate);

		// Check to see if we were asked to still allow children to be hit test visible
		bool bHitChildWidget = false;
		if ( ( Candidate.Widget->GetVisibility().AreChildrenHitTestVisible() ) != 0 )
		{
			FArrangedChildren ArrangedChildren(OutWidgetsUnderCursor.GetFilter());
			Candidate.Widget->ArrangeChildren(Candidate.Geometry, ArrangedChildren);

			// A widget's children are implicitly Z-ordered from first to last
			for ( int32 ChildIndex = ArrangedChildren.Num() - 1; !bHitChildWidget && ChildIndex >= 0; --ChildIndex )
			{
				FArrangedWidget& SomeChild = ArrangedChildren(ChildIndex);
				bHitChildWidget = ( SomeChild.Widget->IsEnabled() || bIgnoreEnabledStatus ) && LocateWidgetsUnderCursor_Helper(SomeChild, InAbsoluteCursorLocation, OutWidgetsUnderCursor, bIgnoreEnabledStatus);
			}
		}

		// If we hit a child widget or we hit our candidate widget then we'll append our widgets
		const bool bHitCandidateWidget = Candidate.Widget->GetVisibility().IsHitTestVisible();
		bHitAnyWidget = bHitChildWidget || bHitCandidateWidget;
		if ( !bHitAnyWidget )
		{
			// No child widgets were hit, and even though the cursor was over our candidate widget, the candidate
			// widget was not hit-testable, so we won't report it
			check(OutWidgetsUnderCursor.Last() == Candidate);
			OutWidgetsUnderCursor.Remove(OutWidgetsUnderCursor.Num() - 1);
		}
	}

	return bHitAnyWidget;
}

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

		ChildSlot
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(PreviewSurface, SBorder)
				.Visibility(EVisibility::HitTestInvisible)
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(STextBlock)
			]
		];
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE
	{
		FVector2D LocalMouseCoordinates = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

		FArrangedChildren Children(EVisibility::All);

		PreviewSurface->SetVisibility(EVisibility::Visible);
		FArrangedWidget WindowWidgetGeometry(PreviewSurface.ToSharedRef(), MyGeometry);
		LocateWidgetsUnderCursor_Helper(WindowWidgetGeometry, MouseEvent.GetScreenSpacePosition(), Children, true);

		PreviewSurface->SetVisibility(EVisibility::HitTestInvisible);

		return FReply::Handled();
	}

	// SWidget interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) OVERRIDE;
	// End of Swidget interface
protected:
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
	TWeakObjectPtr<AActor> LastPreviewActor;
	TWeakPtr<SWidget> LastPreviewWidget;

	TSharedPtr<SBorder> PreviewSurface;
};

void SSlateBlueprintPreview::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	AActor* PreviewActor = BlueprintEditor.Pin()->GetPreviewActor();
	if (PreviewActor != LastPreviewActor.Get())
	{
		LastPreviewActor = PreviewActor;
	}

	if (AUserWidget* WidgetActor = Cast<AUserWidget>(PreviewActor))
	{
		TSharedRef<SWidget> CurrentWidget = WidgetActor->GetWidget();

		if (CurrentWidget != LastPreviewWidget.Pin())
		{
			LastPreviewWidget = CurrentWidget;
			PreviewSurface->SetContent(CurrentWidget);
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
				.Text(LOCTEXT("NoWrappedWidget", "No actor; Open the viewport and tab back"))
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
				Result.ToSharedRef()
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SNew(SSlateBlueprintPreview, BlueprintEditorPtr)
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

		BlueprintComponentsTabFactories.UnregisterFactory(FBlueprintEditorTabs::SCSViewportID);
		BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FSlatePreviewSummoner(InBlueprintEditor)));
		BlueprintComponentsTabFactories.RegisterFactory(MakeShareable(new FSlateTreeSummoner(InBlueprintEditor)));

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
						->AddTab( FBlueprintEditorTabs::ConstructionScriptEditorID, ETabState::OpenedTab )
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