// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEditorPrivatePCH.h"
#include "SNiagaraEffectEditorWidget.h"
#include "SNotificationList.h"

void SNiagaraEffectEditorWidget::Construct(const FArguments& InArgs)
{
	FString NiagaraEffectText(TEXT("Niagara Effect"));
	FString ReadOnlyText(TEXT("Read Only"));
	/*
		Commands = MakeShareable(new FUICommandList());
		IsEditable = InArgs._IsEditable;
		Appearance = InArgs._Appearance;
		TitleBarEnabledOnly = InArgs._TitleBarEnabledOnly;
		TitleBar = InArgs._TitleBar;
		bAutoExpandActionMenu = InArgs._AutoExpandActionMenu;
		ShowGraphStateOverlay = InArgs._ShowGraphStateOverlay;
		OnNavigateHistoryBack = InArgs._OnNavigateHistoryBack;
		OnNavigateHistoryForward = InArgs._OnNavigateHistoryForward;
		OnNodeSpawnedByKeymap = InArgs._GraphEvents.OnNodeSpawnedByKeymap;
		*/

	/*
	// Make sure that the editor knows about what kinds
	// of commands GraphEditor can do.
	FGraphEditorCommands::Register();

	// Tell GraphEditor how to handle all the known commands
	{
	Commands->MapAction(FGraphEditorCommands::Get().ReconstructNodes,
	FExecuteAction::CreateSP(this, &SGraphEditorImpl::ReconstructNodes),
	FCanExecuteAction::CreateSP(this, &SGraphEditorImpl::CanReconstructNodes)
	);

	Commands->MapAction(FGraphEditorCommands::Get().BreakNodeLinks,
	FExecuteAction::CreateSP(this, &SGraphEditorImpl::BreakNodeLinks),
	FCanExecuteAction::CreateSP(this, &SGraphEditorImpl::CanBreakNodeLinks)
	);

	Commands->MapAction(FGraphEditorCommands::Get().BreakPinLinks,
	FExecuteAction::CreateSP(this, &SGraphEditorImpl::BreakPinLinks, true),
	FCanExecuteAction::CreateSP(this, &SGraphEditorImpl::CanBreakPinLinks)
	);

	// Append any additional commands that a consumer of GraphEditor wants us to be aware of.
	const TSharedPtr<FUICommandList>& AdditionalCommands = InArgs._AdditionalCommands;
	if (AdditionalCommands.IsValid())
	{
	Commands->Append(AdditionalCommands.ToSharedRef());
	}

	}
	*/

	EffectObj = InArgs._EffectObj;
	//bNeedsRefresh = false;


	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	const FDetailsViewArgs DetailsViewArgs(false, false, true, false, true, this);
	EmitterDetails = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	//FOnGetDetailCustomizationInstance LayoutMICDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FMaterialInstanceParameterDetails::MakeInstance, MaterialEditorInstance, FGetShowHiddenParameters::CreateSP(this, &FMaterialInstanceEditor::GetShowHiddenParameters));
	//EmitterDetails->RegisterInstancedCustomPropertyLayout(UMaterialEditorInstanceConstant::StaticClass(), LayoutMICDetails);

	//EmitterDetails->SetObjects(EmitterObj, true);


	TSharedPtr<SOverlay> OverlayWidget;
	this->ChildSlot
		[
			SAssignNew(OverlayWidget, SOverlay)

			// details view on the left
			+ SOverlay::Slot()
			.Expose(DetailsViewSlot)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				SNew(SBorder)
				.Padding(4)
				[
					EmitterDetails.ToSharedRef()
				]
			]

			/*
			// The graph panel
			+ SOverlay::Slot()
			.Expose(GraphPanelSlot)
			[
			SAssignNew(GraphPanel, SGraphPanel)
			.EffectObj(EffectObj)
			.GraphObjToDiff(InArgs._GraphToDiff)
			//					.OnGetContextMenuFor(this, &SGraphEditorImpl::GraphEd_OnGetContextMenuFor)
			.OnSelectionChanged(InArgs._GraphEvents.OnSelectionChanged)
			.OnNodeDoubleClicked(InArgs._GraphEvents.OnNodeDoubleClicked)
			//					.IsEditable(this, &SGraphEditorImpl::IsGraphEditable)
			.OnDropActor(InArgs._GraphEvents.OnDropActor)
			.OnDropStreamingLevel(InArgs._GraphEvents.OnDropStreamingLevel)
			//					.IsEnabled(this, &SGraphEditorImpl::GraphEd_OnGetGraphEnabled)
			.OnVerifyTextCommit(InArgs._GraphEvents.OnVerifyTextCommit)
			.OnTextCommitted(InArgs._GraphEvents.OnTextCommitted)
			.OnSpawnNodeByShortcut(InArgs._GraphEvents.OnSpawnNodeByShortcut)
			//					.OnUpdateGraphPanel(this, &SGraphEditorImpl::GraphEd_OnPanelUpdated)
			.OnDisallowedPinConnection(InArgs._GraphEvents.OnDisallowedPinConnection)
			.ShowGraphStateOverlay(InArgs._ShowGraphStateOverlay)
			]*/

			// Title bar - optional
			+SOverlay::Slot()
				.VAlign(VAlign_Top)
				[
					InArgs._TitleBar.IsValid() ? InArgs._TitleBar.ToSharedRef() : (TSharedRef<SWidget>)SNullWidget::NullWidget
				]

			// Bottom-right corner text indicating the type of tool
			+ SOverlay::Slot()
				.Padding(10)
				.VAlign(VAlign_Bottom)
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Visibility(EVisibility::HitTestInvisible)
					.TextStyle(FEditorStyle::Get(), "Graph.CornerText")
					.Text(NiagaraEffectText)
				]

			// Top-right corner text indicating read only when not simulating
			+ SOverlay::Slot()
				.Padding(20)
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Visibility(this, &SNiagaraEffectEditorWidget::ReadOnlyVisibility)
					.TextStyle(FEditorStyle::Get(), "Graph.CornerText")
					.Text(ReadOnlyText)
				]

			// Bottom-right corner text for notification list position
			+ SOverlay::Slot()
				.Padding(15.f)
				.VAlign(VAlign_Bottom)
				.HAlign(HAlign_Right)
				[
					SAssignNew(NotificationListPtr, SNotificationList)
					.Visibility(EVisibility::HitTestInvisible)
				]
		];

	//GraphPanel->RestoreViewSettings(FVector2D::ZeroVector, -1);

	//NotifyGraphChanged();
}
