// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealEd.h"
#include "UObjectGlobals.h"
#include "FbxImporter.h"
#include "SFbxSceneOptionWindow.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "IDocumentation.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "SDockTab.h"
#include "TabManager.h"
#include "STextComboBox.h"
#include "SEditableTextBox.h"
#include "../FbxImporter.h"

#define LOCTEXT_NAMESPACE "FBXOption"

SFbxSceneOptionWindow::SFbxSceneOptionWindow()
{
	SceneInfo = nullptr;
	SceneInfoOriginal = nullptr;
	MeshStatusMap = nullptr;
	GlobalImportSettings = nullptr;
	SceneImportOptionsDisplay = nullptr;
	SceneImportOptionsStaticMeshDisplay = nullptr;
	OverrideNameOptionsMap = nullptr;
	SceneImportOptionsSkeletalMeshDisplay = nullptr;
	SceneImportOptionsAnimationDisplay = nullptr;
	SceneImportOptionsMaterialDisplay = nullptr;
	OwnerWindow = nullptr;
	FbxSceneImportTabManager = nullptr;
	Layout = nullptr;
	bShouldImport = false;
	SceneTabTreeview = nullptr;
	SceneTabDetailsView = nullptr;
	StaticMeshTabListView = nullptr;
	StaticMeshTabDetailsView = nullptr;
	SceneReimportTreeview = nullptr;
	StaticMeshReimportListView = nullptr;
	StaticMeshReimportDetailsView = nullptr;
	MaterialsTabListView = nullptr;
	TexturesArray.Reset();
	MaterialPrefixName.Empty();
}

SFbxSceneOptionWindow::~SFbxSceneOptionWindow()
{
	if (FbxSceneImportTabManager.IsValid())
	{
		FbxSceneImportTabManager->UnregisterAllTabSpawners();
	}
	SceneInfo = nullptr;
	SceneInfoOriginal = nullptr;
	MeshStatusMap = nullptr;
	GlobalImportSettings = nullptr;
	SceneImportOptionsDisplay = nullptr;
	SceneImportOptionsStaticMeshDisplay = nullptr;
	OverrideNameOptionsMap = nullptr;
	SceneImportOptionsSkeletalMeshDisplay = nullptr;
	SceneImportOptionsAnimationDisplay = nullptr;
	SceneImportOptionsMaterialDisplay = nullptr;
	OwnerWindow = nullptr;
	FbxSceneImportTabManager = nullptr;
	Layout = nullptr;
	bShouldImport = false;
	SceneTabTreeview = nullptr;
	SceneTabDetailsView = nullptr;
	StaticMeshTabListView = nullptr;
	StaticMeshTabDetailsView = nullptr;
	SceneReimportTreeview = nullptr;
	StaticMeshReimportListView = nullptr;
	StaticMeshReimportDetailsView = nullptr;
	MaterialsTabListView = nullptr;
	TexturesArray.Reset();
	MaterialPrefixName.Empty();
}

TSharedRef<SDockTab> SFbxSceneOptionWindow::SpawnSceneTab(const FSpawnTabArgs& Args)
{
	//Create the treeview
	SceneTabTreeview = SNew(SFbxSceneTreeView)
		.SceneInfo(SceneInfo);

	TSharedPtr<SBox> InspectorBox;
	TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("WidgetFbxSceneActorTab", "Scene"))
		.ToolTipText(LOCTEXT("WidgetFbxSceneActorTabTextToolTip", "Switch to the scene tab."))
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			+ SSplitter::Slot()
			.Value(0.4f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoHeight()
				[
					SNew(SUniformGridPanel)
					.SlotPadding(2)
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SCheckBox)
							.HAlign(HAlign_Center)
							.OnCheckStateChanged(SceneTabTreeview.Get(), &SFbxSceneTreeView::OnToggleSelectAll)
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(0.0f, 3.0f, 6.0f, 3.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("FbxOptionWindow_Scene_All", "All"))
						]
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("FbxOptionWindow_Scene_ExpandAll", "Expand All"))
						.OnClicked(SceneTabTreeview.Get(), &SFbxSceneTreeView::OnExpandAll)
					]
					+ SUniformGridPanel::Slot(2, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("FbxOptionWindow_Scene_CollapseAll", "Collapse All"))
						.OnClicked(SceneTabTreeview.Get(), &SFbxSceneTreeView::OnCollapseAll)
					]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SBox)
					[
						SceneTabTreeview.ToSharedRef()
					]
				]
			]
			+ SSplitter::Slot()
			.Value(0.6f)
			[
				SAssignNew(InspectorBox, SBox)
			]
		];
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	SceneTabDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	InspectorBox->SetContent(SceneTabDetailsView->AsShared());
	SceneTabDetailsView->SetObject(SceneImportOptionsDisplay);
	SceneTabDetailsView->OnFinishedChangingProperties().AddSP(this, &SFbxSceneOptionWindow::OnFinishedChangingPropertiesSceneTabDetailView);
	return DockTab;
}

void SFbxSceneOptionWindow::OnFinishedChangingPropertiesSceneTabDetailView(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!SceneInfoOriginal.IsValid())
	{
		MaterialsTabListView->SetCreateContentFolderHierarchy(SceneImportOptionsDisplay->bCreateContentFolderHierarchy);
		//Update the MaterialList
		MaterialsTabListView->UpdateMaterialPrefixName();
	}
}

TSharedRef<SDockTab> SFbxSceneOptionWindow::SpawnStaticMeshTab(const FSpawnTabArgs& Args)
{
	//Create the static mesh listview
	StaticMeshTabListView = SNew(SFbxSceneStaticMeshListView)
		.SceneInfo(SceneInfo)
		.GlobalImportSettings(GlobalImportSettings)
		.OverrideNameOptionsMap(OverrideNameOptionsMap)
		.SceneImportOptionsStaticMeshDisplay(SceneImportOptionsStaticMeshDisplay);

	TSharedPtr<SBox> InspectorBox;
	TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("WidgetFbxSceneStaticMeshTab", "Static Meshes"))
		.ToolTipText(LOCTEXT("WidgetFbxSceneStaticMeshTabTextToolTip", "Switch to the static meshes tab."))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SSplitter)
				.Orientation(Orient_Horizontal)
				+ SSplitter::Slot()
				.Value(0.4f)
				[
					SNew(SBox)
					[
						StaticMeshTabListView.ToSharedRef()
					]
				]
				+ SSplitter::Slot()
				.Value(0.6f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							StaticMeshTabListView->CreateOverrideOptionComboBox().ToSharedRef()
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("FbxOptionWindow_SM_CreateOverride", "Create Override"))
							.OnClicked(StaticMeshTabListView.Get(), &SFbxSceneStaticMeshListView::OnCreateOverrideOptions)
						]
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SAssignNew(InspectorBox, SBox)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(2)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(2)
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("FbxOptionWindow_SM_Select_asset_using", "Select Asset Using"))
					.OnClicked(StaticMeshTabListView.Get(), &SFbxSceneStaticMeshListView::OnSelectAssetUsing)
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("FbxOptionWindow_SM_Delete", "Delete"))
					.IsEnabled(StaticMeshTabListView.Get(), &SFbxSceneStaticMeshListView::CanDeleteOverride)
					.OnClicked(StaticMeshTabListView.Get(), &SFbxSceneStaticMeshListView::OnDeleteOverride)
				]
			]
		];
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	StaticMeshTabDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	InspectorBox->SetContent(StaticMeshTabDetailsView->AsShared());
	StaticMeshTabDetailsView->SetObject(SceneImportOptionsStaticMeshDisplay);
	StaticMeshTabDetailsView->OnFinishedChangingProperties().AddSP(StaticMeshTabListView.Get(), &SFbxSceneStaticMeshListView::OnFinishedChangingProperties);
	return DockTab;
}

TSharedRef<SDockTab> SFbxSceneOptionWindow::SpawnSkeletalMeshTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("WidgetFbxSceneSkeletalMeshTab", "Skeletal Meshes"))
		.ToolTipText(LOCTEXT("WidgetFbxSceneSkeletalMeshTabTextToolTip", "Switch to the skeletal meshes tab."))
		[
			SNew(STextBlock).Text(LOCTEXT("SkeletalMeshTextBoxPlaceHolder", "Skeletal Mesh Import options placeholder"))
		];
}

FText SFbxSceneOptionWindow::GetMaterialPrefixName() const
{
	return FText::FromString(MaterialPrefixName);
}

void SFbxSceneOptionWindow::OnMaterialPrefixCommited(const FText& InText, ETextCommit::Type InCommitType)
{
	MaterialPrefixName = InText.ToString();
	//Commit only if all the rules are respected
	if (MaterialPrefixName.StartsWith(TEXT("/")) && MaterialPrefixName.EndsWith(TEXT("/")))
	{
		GlobalImportSettings->MaterialPrefixName = FName(*MaterialPrefixName);
		MaterialsTabListView->UpdateMaterialPrefixName();
	}
}

FSlateColor SFbxSceneOptionWindow::GetMaterialPrefixTextColor() const
{
	if (MaterialPrefixName.StartsWith(TEXT("/")) && MaterialPrefixName.EndsWith(TEXT("/")))
	{
		return FSlateColor::UseForeground();
	}
	else
	{
		return FLinearColor(0.75f, 0.75f, 0.0f, 1.0f);
	}
}

TSharedRef<SDockTab> SFbxSceneOptionWindow::SpawnMaterialTab(const FSpawnTabArgs& Args)
{
	//Create the static mesh listview
	MaterialsTabListView = SNew(SFbxSceneMaterialsListView)
		.SceneInfo(SceneInfo)
		.SceneInfoOriginal(SceneInfoOriginal)
		.GlobalImportSettings(GlobalImportSettings)
		.TexturesArray(&TexturesArray)
		.FullPath(FullPath)
		.IsReimport(SceneInfoOriginal.IsValid())
		.CreateContentFolderHierarchy(SceneImportOptionsDisplay->bCreateContentFolderHierarchy != 0);

	TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("WidgetFbxSceneMaterialsTab", "Materials"))
		.ToolTipText(LOCTEXT("WidgetFbxSceneMaterialsTabTextToolTip", "Switch to the materials tab."))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FbxOptionWindow_Scene_Material_Prefix", "Material name prefix path: "))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(5.0f, 3.0f, 6.0f, 3.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SEditableText)
						.SelectAllTextWhenFocused(true)
						.Text(this, &SFbxSceneOptionWindow::GetMaterialPrefixName)
						.ToolTipText(LOCTEXT("FbxOptionWindow_Scene_MaterialPrefixName_tooltip", "The prefix must be start and end by '/' use this prefix to add a folder to all material (i.e. /Materials/)"))
						.OnTextCommitted(this, &SFbxSceneOptionWindow::OnMaterialPrefixCommited)
						.OnTextChanged(this, &SFbxSceneOptionWindow::OnMaterialPrefixCommited, ETextCommit::Default)
						.ColorAndOpacity(this, &SFbxSceneOptionWindow::GetMaterialPrefixTextColor)
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SSplitter)
						.Orientation(Orient_Vertical)
						+ SSplitter::Slot()
						.Value(0.4f)
						[
							SNew(SBox)
							[
								MaterialsTabListView.ToSharedRef()
							]
						]
						+ SSplitter::Slot()
						.Value(0.6f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.FillHeight(1.0f)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("FbxOptionWindow_Scene_Texture_Slot", "Put the texture array here"))
							]
						]
					]
				]
			]
		];
	return DockTab;
}

TSharedRef<SDockTab> SFbxSceneOptionWindow::SpawnSceneReimportTab(const FSpawnTabArgs& Args)
{
	//Create the treeview
	SceneReimportTreeview = SNew(SFbxReimportSceneTreeView)
		.SceneInfo(SceneInfo)
		.SceneInfoOriginal(SceneInfoOriginal)
		.NodeStatusMap(NodeStatusMap);

	TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("WidgetFbxSceneActorTab", "Scene"))
		.ToolTipText(LOCTEXT("WidgetFbxSceneActorTabTextToolTip", "Switch to the scene tab."))
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			+ SSplitter::Slot()
			.Value(0.6f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoHeight()
				[
					SNew(SUniformGridPanel)
					.SlotPadding(2)
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SCheckBox)
							.HAlign(HAlign_Center)
							.OnCheckStateChanged(SceneReimportTreeview.Get(), &SFbxReimportSceneTreeView::OnToggleSelectAll)
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(0.0f, 3.0f, 6.0f, 3.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("FbxOptionWindow_Scene_All", "All"))
						]
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("FbxOptionWindow_Scene_ExpandAll", "Expand All"))
						.OnClicked(SceneReimportTreeview.Get(), &SFbxReimportSceneTreeView::OnExpandAll)
					]
					+ SUniformGridPanel::Slot(2, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("FbxOptionWindow_Scene_CollapseAll", "Collapse All"))
						.OnClicked(SceneReimportTreeview.Get(), &SFbxReimportSceneTreeView::OnCollapseAll)
					]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SBox)
					[
						SceneReimportTreeview.ToSharedRef()
					]
				]
			]
			+ SSplitter::Slot()
			.Value(0.4f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Left)
				[
					SNew(SCheckBox)
					.HAlign(HAlign_Center)
					.OnCheckStateChanged(this, &SFbxSceneOptionWindow::OnToggleReimportHierarchy)
					.IsChecked(this, &SFbxSceneOptionWindow::IsReimportHierarchyChecked)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 3.0f, 6.0f, 3.0f)
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Left)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FbxOptionWindow_Scene_Reimport_ImportHierarchy", "Reimport Hierarchy"))
					.ToolTipText(LOCTEXT("FbxOptionWindow_Scene_Reimport_ImportHierarchy_Tooltip", "If Check and the original import was done in a blueprint, the blueprint hierarchy will be revisited to include the fbx changes"))
				]
			]
		];
	return DockTab;
}

void SFbxSceneOptionWindow::OnToggleReimportHierarchy(ECheckBoxState CheckType)
{
	if (GlobalImportSettings != nullptr)
	{
		GlobalImportSettings->bImportScene = CheckType == ECheckBoxState::Checked;
	}
}

ECheckBoxState SFbxSceneOptionWindow::IsReimportHierarchyChecked() const
{
	return  GlobalImportSettings->bImportScene ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

TSharedRef<SDockTab> SFbxSceneOptionWindow::SpawnStaticMeshReimportTab(const FSpawnTabArgs& Args)
{
	//Create the static mesh listview
	StaticMeshReimportListView = SNew(SFbxSceneStaticMeshReimportListView)
		.SceneInfo(SceneInfo)
		.SceneInfoOriginal(SceneInfoOriginal)
		.GlobalImportSettings(GlobalImportSettings)
		.OverrideNameOptionsMap(OverrideNameOptionsMap)
		.SceneImportOptionsStaticMeshDisplay(SceneImportOptionsStaticMeshDisplay)
		.MeshStatusMap(MeshStatusMap);

	TSharedPtr<SBox> InspectorBox;
	TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("WidgetFbxSceneReimportStaticMeshTab", "Reimport Static Meshes"))
		.ToolTipText(LOCTEXT("WidgetFbxSceneReimportStaticMeshTabTextToolTip", "Switch to the reimport static meshes tab."))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Left)
					.AutoHeight()
					[
						SNew(SUniformGridPanel)
						.SlotPadding(2)
						+ SUniformGridPanel::Slot(0, 0)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("FbxOptionWindow_Scene_Filters_Label", "Filters:"))
						]
						+ SUniformGridPanel::Slot(1, 0)
						[
							SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SCheckBox)
									.HAlign(HAlign_Center)
									.OnCheckStateChanged(StaticMeshReimportListView.Get(), &SFbxSceneStaticMeshReimportListView::OnToggleFilterAddContent)
									.IsChecked(StaticMeshReimportListView.Get(), &SFbxSceneStaticMeshReimportListView::IsFilterAddContentChecked)
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(0.0f, 3.0f, 6.0f, 3.0f)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("FbxOptionWindow_Scene_Reimport_Filter_Add_Content", "Add"))
								]
							]
						]
						+ SUniformGridPanel::Slot(2, 0)
						[
							SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SCheckBox)
									.HAlign(HAlign_Center)
									.OnCheckStateChanged(StaticMeshReimportListView.Get(), &SFbxSceneStaticMeshReimportListView::OnToggleFilterDeleteContent)
									.IsChecked(StaticMeshReimportListView.Get(), &SFbxSceneStaticMeshReimportListView::IsFilterDeleteContentChecked)
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(0.0f, 3.0f, 6.0f, 3.0f)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("FbxOptionWindow_Scene_Reimport_Filter_Delete_Content", "Delete"))
								]
							]
						]
						+ SUniformGridPanel::Slot(3, 0)
						[
							SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SCheckBox)
									.HAlign(HAlign_Center)
									.OnCheckStateChanged(StaticMeshReimportListView.Get(), &SFbxSceneStaticMeshReimportListView::OnToggleFilterOverwriteContent)
									.IsChecked(StaticMeshReimportListView.Get(), &SFbxSceneStaticMeshReimportListView::IsFilterOverwriteContentChecked)
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(0.0f, 3.0f, 6.0f, 3.0f)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("FbxOptionWindow_Scene_Reimport_Filter_Overwrite_Content", "Overwrite"))
								]
							]
						]
						+ SUniformGridPanel::Slot(4, 0)
						[
							SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SCheckBox)
									.HAlign(HAlign_Center)
									.OnCheckStateChanged(StaticMeshReimportListView.Get(), &SFbxSceneStaticMeshReimportListView::OnToggleFilterDiff)
									.IsChecked(StaticMeshReimportListView.Get(), &SFbxSceneStaticMeshReimportListView::IsFilterDiffChecked)
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(0.0f, 3.0f, 6.0f, 3.0f)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("FbxOptionWindow_Scene_Reimport_Filter_Diff", "Diff"))
									.ToolTipText(LOCTEXT("FbxOptionWindow_Scene_Reimport_Filter_Diff_Tooltip", "Show every reimport item that dont match between the original fbx and the new one."))
								]
							]
						]
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SSplitter)
						.Orientation(Orient_Vertical)
						+ SSplitter::Slot()
						.Value(0.4f)
						[
							SNew(SBox)
							[
								StaticMeshReimportListView.ToSharedRef()
							]
						]
						+ SSplitter::Slot()
						.Value(0.6f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									StaticMeshReimportListView->CreateOverrideOptionComboBox().ToSharedRef()
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SButton)
									.Text(LOCTEXT("FbxOptionWindow_SM_CreateOverride", "Create Override"))
									.OnClicked(StaticMeshReimportListView.Get(), &SFbxSceneStaticMeshReimportListView::OnCreateOverrideOptions)
								]
							]
							+ SVerticalBox::Slot()
							.FillHeight(1.0f)
							[
								SAssignNew(InspectorBox, SBox)
							]
						]
					]
				]
			]
		];
	
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	StaticMeshReimportDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	InspectorBox->SetContent(StaticMeshReimportDetailsView->AsShared());
	StaticMeshReimportDetailsView->SetObject(SceneImportOptionsStaticMeshDisplay);
	StaticMeshReimportDetailsView->OnFinishedChangingProperties().AddSP(StaticMeshReimportListView.Get(), &SFbxSceneStaticMeshReimportListView::OnFinishedChangingProperties);
	return DockTab;
}


TSharedPtr<SWidget> SFbxSceneOptionWindow::SpawnDockTab()
{
	return FbxSceneImportTabManager->RestoreFrom(Layout.ToSharedRef(), OwnerWindow).ToSharedRef();
}

void SFbxSceneOptionWindow::InitAllTabs()
{
	const TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.TabRole(ETabRole::MajorTab);

	FbxSceneImportTabManager = FGlobalTabmanager::Get()->NewTabManager(DockTab);

	if (!SceneInfoOriginal.IsValid())
	{
		Layout = FTabManager::NewLayout("FbxSceneImportUI_Layout")
			->AddArea
			(
				FTabManager::NewPrimaryArea()
				->Split
				(
					FTabManager::NewStack()
					->AddTab("Scene", ETabState::OpenedTab)
					->AddTab("StaticMeshes", ETabState::OpenedTab)
					->AddTab("Materials", ETabState::OpenedTab)
				)
			);
		//					->AddTab("SkeletalMeshes", ETabState::OpenedTab)
							
		FbxSceneImportTabManager->RegisterTabSpawner("Scene", FOnSpawnTab::CreateSP(this, &SFbxSceneOptionWindow::SpawnSceneTab));
		FbxSceneImportTabManager->RegisterTabSpawner("StaticMeshes", FOnSpawnTab::CreateSP(this, &SFbxSceneOptionWindow::SpawnStaticMeshTab));
		//FbxSceneImportTabManager->RegisterTabSpawner("SkeletalMeshes", FOnSpawnTab::CreateSP(this, &SFbxSceneOptionWindow::SpawnSkeletalMeshTab));
		FbxSceneImportTabManager->RegisterTabSpawner("Materials", FOnSpawnTab::CreateSP(this, &SFbxSceneOptionWindow::SpawnMaterialTab));
	}
	else
	{
		if (bCanReimportHierarchy)
		{
			Layout = FTabManager::NewLayout("FbxSceneImportUI_Layout")
				->AddArea
				(
					FTabManager::NewPrimaryArea()
					->Split
					(
						FTabManager::NewStack()
						->AddTab("SceneReImport", ETabState::OpenedTab)
						->AddTab("StaticMeshesReImport", ETabState::OpenedTab)
						->AddTab("Materials", ETabState::OpenedTab)
					)
				);
			FbxSceneImportTabManager->RegisterTabSpawner("SceneReImport", FOnSpawnTab::CreateSP(this, &SFbxSceneOptionWindow::SpawnSceneReimportTab));
		}
		else
		{
			//Re import only the assets, the hierarchy cannot be re import.
			Layout = FTabManager::NewLayout("FbxSceneImportUI_Layout")
				->AddArea
				(
					FTabManager::NewPrimaryArea()
					->Split
					(
						FTabManager::NewStack()
						->AddTab("StaticMeshesReImport", ETabState::OpenedTab)
						->AddTab("Materials", ETabState::OpenedTab)
					)
				);
		}
		FbxSceneImportTabManager->RegisterTabSpawner("StaticMeshesReimport", FOnSpawnTab::CreateSP(this, &SFbxSceneOptionWindow::SpawnStaticMeshReimportTab));
		FbxSceneImportTabManager->RegisterTabSpawner("Materials", FOnSpawnTab::CreateSP(this, &SFbxSceneOptionWindow::SpawnMaterialTab));
	}
}

void SFbxSceneOptionWindow::Construct(const FArguments& InArgs)
{
	SceneInfo = InArgs._SceneInfo;
	SceneInfoOriginal = InArgs._SceneInfoOriginal;
	MeshStatusMap = InArgs._MeshStatusMap;
	bCanReimportHierarchy = InArgs._CanReimportHierarchy;
	NodeStatusMap = InArgs._NodeStatusMap;
	GlobalImportSettings = InArgs._GlobalImportSettings;
	SceneImportOptionsDisplay = InArgs._SceneImportOptionsDisplay;
	SceneImportOptionsStaticMeshDisplay = InArgs._SceneImportOptionsStaticMeshDisplay;
	OverrideNameOptionsMap = InArgs._OverrideNameOptionsMap;
	SceneImportOptionsSkeletalMeshDisplay = InArgs._SceneImportOptionsSkeletalMeshDisplay;
	SceneImportOptionsAnimationDisplay = InArgs._SceneImportOptionsAnimationDisplay;
	SceneImportOptionsMaterialDisplay = InArgs._SceneImportOptionsMaterialDisplay;
	OwnerWindow = InArgs._OwnerWindow;
	FullPath = InArgs._FullPath;

	check(SceneInfo.IsValid());
	check(GlobalImportSettings != nullptr);
	check(SceneImportOptionsDisplay != nullptr);
	check(SceneImportOptionsStaticMeshDisplay != nullptr);
	check(OverrideNameOptionsMap != nullptr);

	if (!SceneInfoOriginal.IsValid())
	{
		check(SceneImportOptionsSkeletalMeshDisplay != nullptr);
		check(SceneImportOptionsAnimationDisplay != nullptr);
		check(SceneImportOptionsMaterialDisplay != nullptr);
	}
	else
	{
		check(MeshStatusMap != nullptr);
		check(NodeStatusMap);
	}

	check(OwnerWindow.IsValid());

	MaterialPrefixName = GlobalImportSettings->MaterialPrefixName.ToString();

	InitAllTabs();

	FText SubmitText = SceneInfoOriginal.IsValid() ? LOCTEXT("FbxOptionWindow_Import", "Reimport") : LOCTEXT("FbxOptionWindow_Import", "Import");

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Font(FEditorStyle::GetFontStyle("CurveEd.LabelFont"))
					.Text(LOCTEXT("FbxSceneImport_CurrentPath", "Import Asset Path: "))
				]
				+SHorizontalBox::Slot()
				.Padding(5, 0, 0, 0)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
					.Text(FText::FromString(FullPath))
				]
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(2)
		[
			SpawnDockTab().ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(2)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(2)
			+ SUniformGridPanel::Slot(0, 0)
			[
				IDocumentation::Get()->CreateAnchor(FString("Engine/Content/FBX/ImportOptions"))
			]
			+ SUniformGridPanel::Slot(1, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(SubmitText)
				.IsEnabled(this, &SFbxSceneOptionWindow::CanImport)
				.OnClicked(this, &SFbxSceneOptionWindow::OnImport)
			]
			+ SUniformGridPanel::Slot(2, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("FbxOptionWindow_Cancel", "Cancel"))
				.ToolTipText(LOCTEXT("FbxOptionWindow_Cancel_ToolTip", "Cancels importing this FBX file"))
				.OnClicked(this, &SFbxSceneOptionWindow::OnCancel)
			]
		]
	];

	//By default we want to see the Scene tab
	FbxSceneImportTabManager->InvokeTab(FTabId("Scene"));
}

void SFbxSceneOptionWindow::CloseFbxSceneOption()
{
	//Free all resource before closing the window
	//Unregister spawn tab
	if (FbxSceneImportTabManager.IsValid())
	{
		FbxSceneImportTabManager->UnregisterAllTabSpawners();
		FbxSceneImportTabManager->CloseAllAreas();
	}
	FbxSceneImportTabManager = nullptr;
	Layout = nullptr;

	//Clear scene tab resource
	SceneTabTreeview = nullptr;
	SceneTabDetailsView = nullptr;

	StaticMeshTabListView = nullptr;
	StaticMeshTabDetailsView = nullptr;

	SceneReimportTreeview = nullptr;

	StaticMeshReimportListView = nullptr;
	StaticMeshReimportDetailsView = nullptr;

	MaterialsTabListView = nullptr;
	TexturesArray.Reset();
	MaterialPrefixName.Empty();

	SceneInfo = nullptr;
	SceneInfoOriginal = nullptr;
	SceneImportOptionsDisplay = nullptr;
	GlobalImportSettings = nullptr;
	SceneImportOptionsDisplay = nullptr;
	SceneImportOptionsStaticMeshDisplay = nullptr;
	OverrideNameOptionsMap = nullptr;
	SceneImportOptionsSkeletalMeshDisplay = nullptr;
	SceneImportOptionsAnimationDisplay = nullptr;
	SceneImportOptionsMaterialDisplay = nullptr;

	MeshStatusMap = nullptr;
	NodeStatusMap = nullptr;

	if (OwnerWindow.IsValid())
	{
		//Close the window
		OwnerWindow->RequestDestroyWindow();
	}
	OwnerWindow = nullptr;
}

bool SFbxSceneOptionWindow::CanImport()  const
{
	return true;
}

void SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(UnFbx::FBXImportOptions *SourceOptions, UnFbx::FBXImportOptions *DestinationOptions)
{
	FMemory::BigBlockMemcpy(DestinationOptions, SourceOptions, sizeof(UnFbx::FBXImportOptions));
}

void SFbxSceneOptionWindow::CopyStaticMeshOptionsToFbxOptions(UnFbx::FBXImportOptions *ImportSettings, UFbxSceneImportOptionsStaticMesh* StaticMeshOptions)
{
	ImportSettings->bAutoGenerateCollision = StaticMeshOptions->bAutoGenerateCollision;
	ImportSettings->bBuildAdjacencyBuffer = StaticMeshOptions->bBuildAdjacencyBuffer;
	ImportSettings->bBuildReversedIndexBuffer = StaticMeshOptions->bBuildReversedIndexBuffer;
	ImportSettings->bGenerateLightmapUVs = StaticMeshOptions->bGenerateLightmapUVs;
	ImportSettings->bOneConvexHullPerUCX = StaticMeshOptions->bOneConvexHullPerUCX;
	ImportSettings->bRemoveDegenerates = StaticMeshOptions->bRemoveDegenerates;
	ImportSettings->bTransformVertexToAbsolute = StaticMeshOptions->bTransformVertexToAbsolute;
	ImportSettings->StaticMeshLODGroup = StaticMeshOptions->StaticMeshLODGroup;
	switch (StaticMeshOptions->VertexColorImportOption)
	{
	case EFbxSceneVertexColorImportOption::Type::Replace:
		ImportSettings->VertexColorImportOption = EVertexColorImportOption::Type::Replace;
		break;
	case EFbxSceneVertexColorImportOption::Type::Override:
		ImportSettings->VertexColorImportOption = EVertexColorImportOption::Type::Override;
		break;
	case EFbxSceneVertexColorImportOption::Type::Ignore:
		ImportSettings->VertexColorImportOption = EVertexColorImportOption::Type::Ignore;
		break;
	default:
		ImportSettings->VertexColorImportOption = EVertexColorImportOption::Type::Replace;
	}
	ImportSettings->VertexOverrideColor = StaticMeshOptions->VertexOverrideColor;
	switch (StaticMeshOptions->NormalImportMethod)
	{
		case EFBXSceneNormalImportMethod::FBXSceneNIM_ComputeNormals:
			ImportSettings->NormalImportMethod = FBXNIM_ComputeNormals;
			break;
		case EFBXSceneNormalImportMethod::FBXSceneNIM_ImportNormals:
			ImportSettings->NormalImportMethod = FBXNIM_ImportNormals;
			break;
		case EFBXSceneNormalImportMethod::FBXSceneNIM_ImportNormalsAndTangents:
			ImportSettings->NormalImportMethod = FBXNIM_ImportNormalsAndTangents;
			break;
	}
	switch (StaticMeshOptions->NormalGenerationMethod)
	{
	case EFBXSceneNormalGenerationMethod::BuiltIn:
		ImportSettings->NormalGenerationMethod = EFBXNormalGenerationMethod::BuiltIn;
		break;
	case EFBXSceneNormalGenerationMethod::MikkTSpace:
		ImportSettings->NormalGenerationMethod = EFBXNormalGenerationMethod::MikkTSpace;
		break;
	}
}

void SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(UnFbx::FBXImportOptions *ImportSettings, UFbxSceneImportOptionsStaticMesh* StaticMeshOptions)
{
	StaticMeshOptions->bAutoGenerateCollision = ImportSettings->bAutoGenerateCollision;
	StaticMeshOptions->bBuildAdjacencyBuffer = ImportSettings->bBuildAdjacencyBuffer;
	StaticMeshOptions->bBuildReversedIndexBuffer = ImportSettings->bBuildReversedIndexBuffer;
	StaticMeshOptions->bGenerateLightmapUVs = ImportSettings->bGenerateLightmapUVs;
	StaticMeshOptions->bOneConvexHullPerUCX = ImportSettings->bOneConvexHullPerUCX;
	StaticMeshOptions->bRemoveDegenerates = ImportSettings->bRemoveDegenerates;
	StaticMeshOptions->bTransformVertexToAbsolute = ImportSettings->bTransformVertexToAbsolute;
	StaticMeshOptions->StaticMeshLODGroup = ImportSettings->StaticMeshLODGroup;
	switch (ImportSettings->VertexColorImportOption)
	{
	case EVertexColorImportOption::Type::Replace:
		StaticMeshOptions->VertexColorImportOption = EFbxSceneVertexColorImportOption::Type::Replace;
		break;
	case EVertexColorImportOption::Type::Override:
		StaticMeshOptions->VertexColorImportOption = EFbxSceneVertexColorImportOption::Type::Override;
		break;
	case EVertexColorImportOption::Type::Ignore:
		StaticMeshOptions->VertexColorImportOption = EFbxSceneVertexColorImportOption::Type::Ignore;
		break;
	default:
		StaticMeshOptions->VertexColorImportOption = EFbxSceneVertexColorImportOption::Type::Replace;
	}
	StaticMeshOptions->VertexOverrideColor = ImportSettings->VertexOverrideColor;
	switch (ImportSettings->NormalImportMethod)
	{
	case FBXNIM_ComputeNormals:
		StaticMeshOptions->NormalImportMethod = EFBXSceneNormalImportMethod::FBXSceneNIM_ComputeNormals;
		break;
	case FBXNIM_ImportNormals:
		StaticMeshOptions->NormalImportMethod = EFBXSceneNormalImportMethod::FBXSceneNIM_ImportNormals;
		break;
	case FBXNIM_ImportNormalsAndTangents:
		StaticMeshOptions->NormalImportMethod = EFBXSceneNormalImportMethod::FBXSceneNIM_ImportNormalsAndTangents;
		break;
	}
	switch (ImportSettings->NormalGenerationMethod)
	{
	case EFBXNormalGenerationMethod::BuiltIn:
		StaticMeshOptions->NormalGenerationMethod = EFBXSceneNormalGenerationMethod::BuiltIn;
		break;
	case EFBXNormalGenerationMethod::MikkTSpace:
		StaticMeshOptions->NormalGenerationMethod = EFBXSceneNormalGenerationMethod::MikkTSpace;
		break;
	}
}

void SFbxSceneOptionWindow::CopySkeletalMeshOptionsToFbxOptions(UnFbx::FBXImportOptions *ImportSettings, UFbxSceneImportOptionsSkeletalMesh* SkeletalMeshOptions)
{
}

void SFbxSceneOptionWindow::CopyFbxOptionsToSkeletalMeshOptions(UnFbx::FBXImportOptions *ImportSettings, class UFbxSceneImportOptionsSkeletalMesh* SkeletalMeshOptions)
{
}

void SFbxSceneOptionWindow::CopyAnimationOptionsToFbxOptions(UnFbx::FBXImportOptions *ImportSettings, UFbxSceneImportOptionsAnimation* AnimationOptions)
{
}

void SFbxSceneOptionWindow::CopyFbxOptionsToAnimationOptions(UnFbx::FBXImportOptions *ImportSettings, class UFbxSceneImportOptionsAnimation* AnimationOptions)
{
}

void SFbxSceneOptionWindow::CopyMaterialOptionsToFbxOptions(UnFbx::FBXImportOptions *ImportSettings, UFbxSceneImportOptionsMaterial* MaterialOptions)
{
}

void SFbxSceneOptionWindow::CopyFbxOptionsToMaterialOptions(UnFbx::FBXImportOptions *ImportSettings, class UFbxSceneImportOptionsMaterial* MaterialOptions)
{
}

#undef LOCTEXT_NAMESPACE
