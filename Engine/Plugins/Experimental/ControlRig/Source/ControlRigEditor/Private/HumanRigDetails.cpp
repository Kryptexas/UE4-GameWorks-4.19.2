	// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "HumanRigDetails.h"
#include "UObject/Class.h"
#include "HumanRig.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Framework/Commands/GenericCommands.h"
#include "ScopedTransaction.h"
#include "SButton.h"
#include "STreeView.h"
#include "SSearchBox.h"
#include "Framework/Commands/UICommandList.h"
#include "MultiBoxBuilder.h"
#include "ControlRigEditMode.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "HumanRigDetails"
#define MAXSPINE 5
#define MAXNODEINPUT	MAXSPINE
//////////////////////////////
// FControlRigNodeCommand
//////////////////////////////
void FHumanRigNodeCommand::RegisterCommands() 
{
	UI_COMMAND(AddNode, "Add Node", "Add new node", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AddManipulator, "Add Widget", "Add widget to the selected node.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AddLeg, "Add Leg", "Add Leg", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DefineSpine, "Set Up Spine", "Set Up Spline", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ClearRotation, "Clear Rotation", "Clear Rotation for all nodes", EUserInterfaceActionType::Button, FInputChord());
}
//////////////////////////////
TSharedRef<IDetailCustomization> FHumanRigDetails::MakeInstance()
{
	return MakeShareable( new FHumanRigDetails );
}

void FHumanRigDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	TArray< TWeakObjectPtr<UObject> > Objects;
	
	// initialize input boxes
	NodeInputs.Reset(MAXNODEINPUT);
	NodeInputs.AddDefaulted(MAXNODEINPUT);

	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() == 0 || Objects.Num() > 1)
	{
		// for now no support on multi selection
		return;
	}

	CurrentlySelectedObject = Cast<class UHumanRig>(Objects[0].Get());

	CurrentEditMode = CurrentlySelectedObject->IsTemplate()? EControlRigEditoMode::EditMode : EControlRigEditoMode::InputMode;

	// property visibility check happens in the ControlRigEditMode

	TSharedPtr<SHorizontalBox> ImportMeshBox;
	SAssignNew(ImportMeshBox, SHorizontalBox);

	if (CurrentEditMode == EControlRigEditoMode::EditMode)
	{
		ImportMeshBox->AddSlot()
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(USkeletalMesh::StaticClass())
				.OnObjectChanged(this, &FHumanRigDetails::SetCurrentMesh)
				.ObjectPath(this, &FHumanRigDetails::GetCurrentlySelectedMesh)
			];

		ImportMeshBox->AddSlot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("HumanRig_ImportMeshButton", "Import Selected Mesh.."))
				.ToolTipText(LOCTEXT("HumanRig_ImportMeshTooltip", "Import Mesh from the SkeletalMesh. This will clear all the existing nodes and restart."))
				.OnClicked(this, &FHumanRigDetails::ImportMesh)
			];

		// build tree view
		SAssignNew(TreeView, STreeView<TSharedPtr<FNodeNameInfo>>)
			.TreeItemsSource(&SkeletonTreeInfo)
			.OnGenerateRow(this, &FHumanRigDetails::MakeTreeRowWidget)
			.OnGetChildren(this, &FHumanRigDetails::GetChildrenForInfo)
			.OnSelectionChanged(this, &FHumanRigDetails::OnSelectionChanged)
			.OnContextMenuOpening(this, &FHumanRigDetails::OnContextMenuOpening)
			.SelectionMode(ESelectionMode::Multi);
	}
	else
	{
		// build tree view for input mode
		SAssignNew(TreeView, STreeView<TSharedPtr<FNodeNameInfo>>)
			.TreeItemsSource(&SkeletonTreeInfo)
			.OnGenerateRow(this, &FHumanRigDetails::MakeTreeRowWidget)
			.OnGetChildren(this, &FHumanRigDetails::GetChildrenForInfo)
			.OnSelectionChanged(this, &FHumanRigDetails::OnSelectionChanged);
	}

	// technically this should only allow to customize in 
	// add to set up category
	IDetailCategoryBuilder& SetUpCategory = DetailBuilder.EditCategory("Nodes");
	SetUpCategory.AddCustomRow(LOCTEXT("HumanRig_ImportMesh", "Mesh"))
	.WholeRowWidget
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSearchBox)
			.SelectAllTextWhenFocused(true)
			.OnTextChanged(this, &FHumanRigDetails::OnFilterTextChanged)
			.HintText(LOCTEXT("HumanRig_SearchNode", "Search..."))
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.HeightOverride(300)
			.MinDesiredHeight(100)
			.MaxDesiredHeight(300)
			.Content()
			[
				TreeView->AsShared()
			]
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
			.Orientation(Orient_Horizontal)
		]

		+ SVerticalBox::Slot()
		.Padding(2)
		.AutoHeight()
		[
			ImportMeshBox.ToSharedRef()
		]
	];

	RebuildTree();

	CreateCommandList();
}

FReply FHumanRigDetails::ImportMesh()
{
	if (CurrentlySelectedAssetData.IsValid())
	{
		// warn users for overriding data
		USkeletalMesh* SelectedMesh = CastChecked<USkeletalMesh>(CurrentlySelectedAssetData.GetAsset());
		
		if (CurrentlySelectedObject.IsValid())
		{
			const FScopedTransaction Transaction(LOCTEXT("HumanRigDetail_ImportMesh", "ImportMesh"));
			UHumanRig* ControlRig = CurrentlySelectedObject.Get();
			ControlRig->Modify();
			ControlRig->BuildHierarchyFromSkeletalMesh(SelectedMesh);
		}

		RebuildTree();
	}

	return FReply::Handled();
}

void FHumanRigDetails::SetCurrentMesh(const FAssetData& InAssetData)
{
	CurrentlySelectedAssetData = InAssetData;
}

FString FHumanRigDetails::GetCurrentlySelectedMesh() const
{ 
	return CurrentlySelectedAssetData.ObjectPath.ToString();  
}

TSharedRef<ITableRow> FHumanRigDetails::MakeTreeRowWidget(TSharedPtr<FNodeNameInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FNodeNameInfo>>, OwnerTable)
		.Content()
		[
			SNew(STextBlock)
			.HighlightText(FilterText)
			.Text(FText::FromName(InInfo->NodeName))
		];
}

void FHumanRigDetails::GetChildrenForInfo(TSharedPtr<FNodeNameInfo> InInfo, TArray< TSharedPtr<FNodeNameInfo> >& OutChildren)
{
	OutChildren = InInfo->Children;
}

void FHumanRigDetails::OnFilterTextChanged(const FText& InFilterText)
{
	FilterText = InFilterText;

	RebuildTree();
}

void FHumanRigDetails::OnSelectionChanged(TSharedPtr<FNodeNameInfo> NodeInfo, ESelectInfo::Type SelectInfo)
{
	//Because we recreate all our items on tree refresh we will get a spurious null selection event initially.
	if (NodeInfo.IsValid() && CurrentlySelectedObject.IsValid())
	{
		 // only when inputmode
		if ( EControlRigEditoMode::InputMode == CurrentEditMode )
		{
			if (FControlRigEditMode* ControlRigEditMode = static_cast<FControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FControlRigEditMode::ModeName)))
			{
				ControlRigEditMode->SetNodeSelection(NodeInfo->NodeName);
			}
		}
	}
}

void FHumanRigDetails::RebuildTree()
{
	SkeletonTreeInfo.Empty();
	SkeletonTreeInfoFlat.Empty();

	if (CurrentlySelectedObject.IsValid())
	{
		UHumanRig* HumanRig = CurrentlySelectedObject.Get();

		const FAnimationHierarchy& Hierarchy = HumanRig->GetHierarchy();
		const int32 MaxNode = Hierarchy.GetNum();

		for (int32 NodeIdx = 0; NodeIdx < MaxNode; ++NodeIdx)
		{
			const FName NodeName = Hierarchy.GetNodeName(NodeIdx);
			const bool bHasManipulator = HumanRig->FindManipulator(NodeName) != nullptr;
			TSharedRef<FNodeNameInfo> NodeInfo = MakeShareable(new FNodeNameInfo(NodeName));

			// Filter if Necessary
			if (!FilterText.IsEmpty() && !NodeInfo->NodeName.ToString().Contains(FilterText.ToString()))
			{
				continue;
			}

			if (CurrentEditMode == EControlRigEditoMode::InputMode && bHasManipulator == false)
			{
				// in the input mode, only display the ones that have manipulator, it's too confusing to see everything
				continue;
			}

			int32 ParentIdx = Hierarchy.GetParentIndex(NodeIdx);
			bool bAddToParent = false;

			if (ParentIdx != INDEX_NONE && FilterText.IsEmpty())
			{
				// We have a parent, search for it in the flat list
				FName ParentName = Hierarchy.GetNodeName(ParentIdx);

				for (int32 FlatListIdx = 0; FlatListIdx < SkeletonTreeInfoFlat.Num(); ++FlatListIdx)
				{
					TSharedPtr<FNodeNameInfo> InfoEntry = SkeletonTreeInfoFlat[FlatListIdx];
					if (InfoEntry->NodeName == ParentName)
					{
						bAddToParent = true;
						ParentIdx = FlatListIdx;
						break;
					}
				}

				if (bAddToParent)
				{
					SkeletonTreeInfoFlat[ParentIdx]->Children.Add(NodeInfo);
				}
				else
				{
					SkeletonTreeInfo.Add(NodeInfo);
				}
			}
			else
			{
				SkeletonTreeInfo.Add(NodeInfo);
			}

			SkeletonTreeInfoFlat.Add(NodeInfo);
			TreeView->SetItemExpansion(NodeInfo, true);
		}
	}

	TreeView->RequestTreeRefresh();
}

void FHumanRigDetails::CreateCommandList()
{
	const FHumanRigNodeCommand& Commands = FHumanRigNodeCommand::Get();
	CommandList = MakeShareable(new FUICommandList);

	CommandList->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FHumanRigDetails::OnDeleteNodeSelected)
	);

	CommandList->MapAction(
		Commands.AddManipulator,
		FExecuteAction::CreateSP(this, &FHumanRigDetails::OnAddManipulator)
		);

	CommandList->MapAction(
		Commands.AddLeg,
		FExecuteAction::CreateSP(this, &FHumanRigDetails::OnAddLeg)
	);

	CommandList->MapAction(
		Commands.DefineSpine,
		FExecuteAction::CreateSP(this, &FHumanRigDetails::OnAddSpline)
	);

	CommandList->MapAction(
		Commands.AddNode,
		FExecuteAction::CreateSP(this, &FHumanRigDetails::OnAddNode)
		);

	CommandList->MapAction(
		Commands.ClearRotation,
		FExecuteAction::CreateSP(this, &FHumanRigDetails::OnClearRotation)
	);
}

void FHumanRigDetails::OnClearRotation()
{
	if (CurrentlySelectedObject.IsValid())
	{
		const FScopedTransaction Transaction(LOCTEXT("HumanRigDetail_ClearRotation", "Clear Rotation"));
		UHumanRig* ControlRig = CurrentlySelectedObject.Get();
		ControlRig->Modify();

		FAnimationHierarchy& Hierarchy = ControlRig->GetHierarchy();
		TArray<TSharedPtr<FNodeNameInfo>> SelectedItems = TreeView->GetSelectedItems();
		for (int32 Index = 0; Index < SelectedItems.Num(); ++Index)
		{
			FName NodeName = SelectedItems[Index]->NodeName;
			int32 NodeIndex = Hierarchy.GetNodeIndex(NodeName);
			FTransform Transform = Hierarchy.GetGlobalTransform(NodeIndex);
			Transform.SetRotation(FQuat::Identity);
			Hierarchy.SetGlobalTransform(NodeIndex, Transform);
		}

		RebuildTree();
	}
}

void FHumanRigDetails::OnAddManipulator()
{
	if (CurrentlySelectedObject.IsValid())
	{
		FName CurrentlySelectedNodeName = GetFirstSelectedNodeName();
		if (CurrentlySelectedNodeName != NAME_None)
		{
			const FScopedTransaction Transaction(LOCTEXT("HumanRigDetail_AddManipulator", "Add Manipulator"));
			UHumanRig* ControlRig = CurrentlySelectedObject.Get();
			ControlRig->Modify();

			FAnimationHierarchy& Hierarchy = ControlRig->GetHierarchy();
			ControlRig->AddManipulator(CurrentlySelectedNodeName, NAME_None);

			RebuildTree();
		}
	}
}

void FHumanRigDetails::OnDeleteNodeSelected()
{
	if (CurrentlySelectedObject.IsValid())
	{
		const FScopedTransaction Transaction(LOCTEXT("HumanRigDetail_DeleteNode", "Delete Node(s)"));
		UHumanRig* ControlRig = CurrentlySelectedObject.Get();
		ControlRig->Modify();

		FAnimationHierarchy& Hierarchy = ControlRig->GetHierarchy();
		TArray<TSharedPtr<FNodeNameInfo>> SelectedItems = TreeView->GetSelectedItems();
		for (int32 Index = 0; Index < SelectedItems.Num(); ++Index)
		{
			FName NodeName = SelectedItems[Index]->NodeName;
			// @todo ideally you should removed linked node also or optional
			ControlRig->DeleteNode(NodeName);
		}

		RebuildTree();
	}
}

TSharedRef<SWidget> FHumanRigDetails::CreateAddLegMenu() const
{
	FMenuBuilder MenuBuilder(true, NULL);

	// get the curve data
	if (CurrentlySelectedObject.IsValid())
	{
		MenuBuilder.BeginSection("HumanRigDetails_AddLegLabel", LOCTEXT("AddLeg_Heading", "Add Leg"));
		{
			MenuBuilder.AddWidget(
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AddLeg_Upper", "Upper Part"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(SEditableTextBox)
						.Text(this, &FHumanRigDetails::OnGetNodeInputText, 0)
						.OnTextCommitted(this, &FHumanRigDetails::OnNodeInputTextCommitted, 0)
						.SelectAllTextWhenFocused(true)
						.RevertTextOnEscape(true)
						.MinDesiredWidth(30.f)
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AddLeg_Middle", "Middle Part"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(SEditableTextBox)
						.Text(this, &FHumanRigDetails::OnGetNodeInputText, 1)
						.OnTextCommitted(this, &FHumanRigDetails::OnNodeInputTextCommitted, 1)
						.SelectAllTextWhenFocused(true)
						.RevertTextOnEscape(true)
						.MinDesiredWidth(30.f)
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AddLeg_Lower", "Lower Part"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(SEditableTextBox)
						.Text(this, &FHumanRigDetails::OnGetNodeInputText, 2)
						.OnTextCommitted(this, &FHumanRigDetails::OnNodeInputTextCommitted, 2)
						.SelectAllTextWhenFocused(true)
						.RevertTextOnEscape(true)
						.MinDesiredWidth(30.f)
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3)
				[
					SNew(SButton)
					.Text(LOCTEXT("HumanRig_AddLegButton", "Add"))
					.OnClicked(this, &FHumanRigDetails::AddLegButtonClicked)
				]
				,
				FText()
				);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FHumanRigDetails::CreateAddNodeMenu() const
{
	FMenuBuilder MenuBuilder(true, NULL);

	// get the curve data
	if (CurrentlySelectedObject.IsValid())
	{
		MenuBuilder.BeginSection("HumanRigDetails_AddNodeLabel", LOCTEXT("AddNode_Heading", "Add New Node"));
		{
			MenuBuilder.AddWidget(
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3)
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AddNode_Parent", "Parent"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(SEditableTextBox)
						.Text(this, &FHumanRigDetails::OnGetNodeInputText, 0)
						.OnTextCommitted(this, &FHumanRigDetails::OnNodeInputTextCommitted, 0)
						.SelectAllTextWhenFocused(true)
						.RevertTextOnEscape(true)
						.MinDesiredWidth(30.f)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3)
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AddNode_New", "New"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(SEditableTextBox)
						.Text(this, &FHumanRigDetails::OnGetNodeInputText, 1)
						.OnTextCommitted(this, &FHumanRigDetails::OnNodeInputTextCommitted, 1)
						.SelectAllTextWhenFocused(true)
						.RevertTextOnEscape(true)
						.MinDesiredWidth(30.f)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3)
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AddNode_Translation", "Translation"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.Padding(2, 0)
						[
							SNew(SNumericEntryBox<float>)
							.AllowSpin(true)
							.Value(this, &FHumanRigDetails::OnGet_TransX_EntryBoxValue)
							.OnValueChanged(this, &FHumanRigDetails::On_TransX_EntryBoxChanged)
						]

						+ SHorizontalBox::Slot()
						.Padding(2, 0)
						[
							SNew(SNumericEntryBox<float>)
							.AllowSpin(true)
							.Value(this, &FHumanRigDetails::OnGet_TransY_EntryBoxValue)
							.OnValueChanged(this, &FHumanRigDetails::On_TransY_EntryBoxChanged)
						]

						+ SHorizontalBox::Slot()
						.Padding(2, 0)
						[
							SNew(SNumericEntryBox<float>)
							.AllowSpin(true)
							.Value(this, &FHumanRigDetails::OnGet_TransZ_EntryBoxValue)
							.OnValueChanged(this, &FHumanRigDetails::On_TransZ_EntryBoxChanged)
						]
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3)
				[
					SNew(SButton)
					.Text(LOCTEXT("HumanRig_AddNodeButton", "Add"))
					.OnClicked(this, &FHumanRigDetails::AddNodeButtonClicked)
				]
				,
				FText()
			);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FHumanRigDetails::CreateDefineSpineMenu() const
{
	FMenuBuilder MenuBuilder(true, NULL);

	// get the curve data
	if (CurrentlySelectedObject.IsValid())
	{
		MenuBuilder.BeginSection("HumanRigDetails_DefineSpineLabel", LOCTEXT("DefineSpine_Heading", "Add Spine"));
		{
			MenuBuilder.AddWidget(
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DefineSpine_0", "Top Node"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(SEditableTextBox)
						.Text(this, &FHumanRigDetails::OnGetNodeInputText, 0)
						.OnTextCommitted(this, &FHumanRigDetails::OnNodeInputTextCommitted, 0)
						.SelectAllTextWhenFocused(true)
						.RevertTextOnEscape(true)
						.MinDesiredWidth(30.f)
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DefineSpine_1", "End Node"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(SEditableTextBox)
						.Text(this, &FHumanRigDetails::OnGetNodeInputText, 1)
						.OnTextCommitted(this, &FHumanRigDetails::OnNodeInputTextCommitted, 1)
						.SelectAllTextWhenFocused(true)
						.RevertTextOnEscape(true)
						.MinDesiredWidth(30.f)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3)
				[
					SNew(SButton)
					.Text(LOCTEXT("HumanRig_DefineSpineButton", "Add"))
					.OnClicked(this, &FHumanRigDetails::DefineSpineButtonClicked)
				]
				,
				FText()
				);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

FReply FHumanRigDetails::AddNodeButtonClicked()
{
	AddNode(FName(*NodeInputs[0].ToString()), FName(*NodeInputs[1].ToString()), InputTranslation);
	FSlateApplication::Get().DismissAllMenus();
	return FReply::Handled();
}

FReply FHumanRigDetails::AddLegButtonClicked()
{
	AddLeg(FName(*NodeInputs[0].ToString()), FName(*NodeInputs[1].ToString()), FName(*NodeInputs[2].ToString()));
	FSlateApplication::Get().DismissAllMenus();
	return FReply::Handled();
}

FReply FHumanRigDetails::DefineSpineButtonClicked()
{
	DefineSpine(FName(*NodeInputs[0].ToString()), FName(*NodeInputs[1].ToString()));
	FSlateApplication::Get().DismissAllMenus();
	return FReply::Handled();
}

void FHumanRigDetails::OnAddLeg()
{
	// Create context menu
	TSharedPtr< SWindow > Parent = FSlateApplication::Get().GetActiveTopLevelWindow();
	if (Parent.IsValid())
	{
		NodeInputs[0] = FText::FromName(GetFirstSelectedNodeName());
		NodeInputs[1] = FText::GetEmpty();
		NodeInputs[2] = FText::GetEmpty();
		FSlateApplication::Get().PushMenu(
			Parent.ToSharedRef(),
			FWidgetPath(),
			CreateAddLegMenu(),
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup));
	}
}

void FHumanRigDetails::OnAddSpline()
{
	TSharedPtr< SWindow > Parent = FSlateApplication::Get().GetActiveTopLevelWindow();
	if (Parent.IsValid())
	{
		NodeInputs[0] = FText::FromName(GetFirstSelectedNodeName());
		NodeInputs[1] = FText::GetEmpty();

		FSlateApplication::Get().PushMenu(
			Parent.ToSharedRef(),
			FWidgetPath(),
			CreateDefineSpineMenu(),
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup));
	}
}

void FHumanRigDetails::OnAddNode()
{
	TSharedPtr< SWindow > Parent = FSlateApplication::Get().GetActiveTopLevelWindow();
	if (Parent.IsValid())
	{
		NodeInputs[0] = FText::FromName(GetFirstSelectedNodeName());
		NodeInputs[1] = FText::GetEmpty();
		InputTranslation = FVector::ZeroVector;

		FSlateApplication::Get().PushMenu(
			Parent.ToSharedRef(),
			FWidgetPath(),
			CreateAddNodeMenu(),
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup));
	}
}

void FHumanRigDetails::AddNode(FName ParentName, FName NewNodeName, const FVector& Translation)
{
	if (CurrentlySelectedObject.IsValid())
	{
		UHumanRig* ControlRig = CurrentlySelectedObject.Get();
		FAnimationHierarchy& Hierarchy = ControlRig->GetHierarchy();
		if (!Hierarchy.Contains(NewNodeName) && (ParentName == NAME_None || Hierarchy.Contains(ParentName)))
		{
			const FScopedTransaction Transaction(LOCTEXT("HumanRigDetail_AddNode", "Add Node"));
			ControlRig->Modify();
			// @todo for now we only add to identity
			ControlRig->AddNode(NewNodeName, ParentName, FTransform(Translation));
			RebuildTree();
		}
		else
		{
			// failed
		}
	}
}

void FHumanRigDetails::DefineSpine(FName TopNode, FName EndNode)
{
	if (CurrentlySelectedObject.IsValid())
	{
		UHumanRig* ControlRig = CurrentlySelectedObject.Get();
// 		FAnimationHierarchy& Hierarchy = ControlRig->GetHierarchy();
// 		if (!Hierarchy.Contains(NewNodeName) && (ParentName == NAME_None || Hierarchy.Contains(ParentName)))
// 		{
// 			const FScopedTransaction Transaction(LOCTEXT("HumanRigDetail_AddNode", "Add Node"));
// 			ControlRig->Modify();
// 			// @todo for now we only add to identity
// 			ControlRig->AddNode(NewNodeName, ParentName, FTransform(Translation));
// 			RebuildTree();
// 		}
// 		else
// 		{
// 			// failed
// 		}
	}
}


void FHumanRigDetails::AddLeg(FName Upper, FName Middle, FName Lower)
{
	if (CurrentlySelectedObject.IsValid())
	{
		UHumanRig* ControlRig = CurrentlySelectedObject.Get();
		FAnimationHierarchy& Hierarchy = ControlRig->GetHierarchy();
		if (Hierarchy.Contains(Upper) && Hierarchy.Contains(Middle) && Hierarchy.Contains(Lower) )
		{
			// if only contains, give users warning
			const FScopedTransaction Transaction(LOCTEXT("HumanRigDetail_AddLeg", "Add Leg"));
			ControlRig->Modify();
			ControlRig->AddLeg(Upper, Middle, Lower);
			RebuildTree();
		}
		else
		{
			// failed
		}
	}
}

FName FHumanRigDetails::GetFirstSelectedNodeName() const
{
	TArray<TSharedPtr<FNodeNameInfo>> SelectedItems = TreeView->GetSelectedItems();
	if (SelectedItems.Num() > 0)
	{
		return SelectedItems[0]->NodeName;
	}

	return NAME_None;
}

TSharedPtr<SWidget> FHumanRigDetails::OnContextMenuOpening() const
{
	FMenuBuilder MenuBuilder(true, CommandList.ToSharedRef());

	if (CurrentEditMode == EControlRigEditoMode::EditMode)
	{
		bool bAnySelected = TreeView->GetSelectedItems().Num() > 0;
		MenuBuilder.BeginSection("Edit", LOCTEXT("Edit", "Edit"));
		{
			MenuBuilder.AddMenuEntry(FHumanRigNodeCommand::Get().AddNode);
			MenuBuilder.AddMenuEntry(FHumanRigNodeCommand::Get().AddManipulator);
			MenuBuilder.AddMenuEntry(FHumanRigNodeCommand::Get().AddLeg);
//			MenuBuilder.AddMenuEntry(FHumanRigNodeCommand::Get().DefineSpine);

			if (bAnySelected)
			{
				MenuBuilder.AddMenuSeparator();
				MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
			}

			MenuBuilder.AddMenuSeparator();
			MenuBuilder.AddMenuEntry(FHumanRigNodeCommand::Get().ClearRotation);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
