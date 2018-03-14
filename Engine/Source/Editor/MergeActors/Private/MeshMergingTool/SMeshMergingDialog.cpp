// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MeshMergingTool/SMeshMergingDialog.h"
#include "MeshMergingTool/MeshMergingTool.h"
#include "EditorStyleSet.h"
#include "PropertyEditorModule.h"
#include "IDetailChildrenBuilder.h"
#include "Modules/ModuleManager.h"
#include "SlateOptMacros.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SCheckBox.h"
#include "Engine/Selection.h"
#include "Editor.h"
#include "Components/ChildActorComponent.h"
#include "Components/ShapeComponent.h"

#include "IDetailsView.h"

#define LOCTEXT_NAMESPACE "SMeshMergingDialog"

//////////////////////////////////////////////////////////////////////////
// SMeshMergingDialog

SMeshMergingDialog::SMeshMergingDialog()
{
	bRefreshListView = false;
}

SMeshMergingDialog::~SMeshMergingDialog()
{
	// Remove all delegates
	USelection::SelectionChangedEvent.RemoveAll(this);
	USelection::SelectObjectEvent.RemoveAll(this);	
	FEditorDelegates::MapChange.RemoveAll(this);
	FEditorDelegates::NewCurrentLevel.RemoveAll(this);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SMeshMergingDialog::Construct(const FArguments& InArgs, FMeshMergingTool* InTool)
{
	checkf(InTool != nullptr, TEXT("Invalid owner tool supplied"));
	Tool = InTool;

	UpdateSelectedStaticMeshComponents();
	CreateSettingsView();
	
	// Create widget layout
	this->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 10, 0, 0)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				// Static mesh component selection
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MergeStaticMeshComponentsLabel", "Mesh Components to be incorporated in the merge:"))						
					]
				]
				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
					[
						SAssignNew(ComponentSelectionControl.ComponentsListView, SListView<TSharedPtr<FMergeComponentData>>)
						.ListItemsSource(&ComponentSelectionControl.SelectedComponents)
					    .OnGenerateRow(this, &SMeshMergingDialog::MakeComponentListItemWidget)
						.ToolTipText(LOCTEXT("SelectedComponentsListBoxToolTip", "The selected mesh components will be incorporated into the merged mesh"))
					]
			]
		]

		+ SVerticalBox::Slot()
		.Padding(0, 10, 0, 0)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				// Static mesh component selection
				+ SVerticalBox::Slot()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SettingsView->AsShared()
					]
				]
			]
		]

		// Replace source actors
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
		[
			SNew(SCheckBox)
			.Type(ESlateCheckBoxType::CheckBox)
			.IsChecked(this, &SMeshMergingDialog::GetReplaceSourceActors)
			.OnCheckStateChanged(this, &SMeshMergingDialog::SetReplaceSourceActors)
			.ToolTipText(LOCTEXT("ReplaceSourceActorsToolTip", "When enabled the Source Actors will be replaced with the newly generated merged mesh"))
			.Content()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ReplaceSourceActorsLabel", "Replace Source Actors"))
				.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor::Yellow)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Visibility_Lambda([this]()->EVisibility { return this->GetContentEnabledState() ? EVisibility::Collapsed : EVisibility::Visible; })
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DeleteUndo", "Insufficient mesh components found for merging."))
			]
		]
	];	


	// Selection change
	USelection::SelectionChangedEvent.AddRaw(this, &SMeshMergingDialog::OnLevelSelectionChanged);
	USelection::SelectObjectEvent.AddRaw(this, &SMeshMergingDialog::OnLevelSelectionChanged);
	FEditorDelegates::MapChange.AddSP(this, &SMeshMergingDialog::OnMapChange);
	FEditorDelegates::NewCurrentLevel.AddSP(this, &SMeshMergingDialog::OnNewCurrentLevel);

	MergeSettings = UMeshMergingSettingsObject::Get();
	SettingsView->SetObject(MergeSettings);
}

void SMeshMergingDialog::OnMapChange(uint32 MapFlags)
{
	Reset();
}

void SMeshMergingDialog::OnNewCurrentLevel()
{
	Reset();
}

void SMeshMergingDialog::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Check if we need to update selected components and the listbox
	if (bRefreshListView == true)
	{
		ComponentSelectionControl.UpdateSelectedCompnentsAndListBox();
		bRefreshListView = false;		
	}
}

void SMeshMergingDialog::Reset()
{	
	bRefreshListView = true;
}

ECheckBoxState SMeshMergingDialog::GetReplaceSourceActors() const
{
	return (Tool->bReplaceSourceActors ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetReplaceSourceActors(ECheckBoxState NewValue)
{
	Tool->bReplaceSourceActors = (ECheckBoxState::Checked == NewValue);
}

bool SMeshMergingDialog::GetContentEnabledState() const
{
	return (GetNumSelectedMeshComponents() >= 1); // Only enabled if a mesh is selected
}

void SMeshMergingDialog::UpdateSelectedStaticMeshComponents()
{	
	ComponentSelectionControl.UpdateSelectedStaticMeshComponents();
}


TSharedRef<ITableRow> SMeshMergingDialog::MakeComponentListItemWidget(TSharedPtr<FMergeComponentData> ComponentData, const TSharedRef<STableViewBase>& OwnerTable)
{
	return ComponentSelectionControl.MakeComponentListItemWidget(ComponentData, OwnerTable);
}


void SMeshMergingDialog::CreateSettingsView()
{
	// Create a property view
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = true;
	DetailsViewArgs.bLockable = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ComponentsAndActorsUseNameArea;
	DetailsViewArgs.bCustomNameAreaLocation = false;
	DetailsViewArgs.bCustomFilterAreaLocation = true;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
	
		
	// Tiny hack to hide this setting, since we have no way / value to go off to 
	struct Local
	{
		/** Delegate to show all properties */
		static bool IsPropertyVisible(const FPropertyAndParent& PropertyAndParent, bool bInShouldShowNonEditable)
		{
			return (PropertyAndParent.Property.GetFName() != GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, GutterSpace));
		}
	};

	SettingsView = EditModule.CreateDetailView(DetailsViewArgs);
	SettingsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateStatic(&Local::IsPropertyVisible, true));
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SMeshMergingDialog::OnLevelSelectionChanged(UObject* Obj)
{
	Reset();
}

#undef LOCTEXT_NAMESPACE
