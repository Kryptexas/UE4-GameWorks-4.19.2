// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "KismetWidgetsPrivatePCH.h"
#include "UnrealEd.h"
#include "ClassIconFinder.h"
#include "SPinTypeSelector.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "PinTypeSelector"

static const FString BigTooltipDocLink = TEXT("Shared/Editor/Blueprint/VariableTypes");

void SPinTypeSelector::Construct(const FArguments& InArgs, FGetPinTypeTree GetPinTypeTreeFunc)
{
	SearchText = FText::GetEmpty();

	OnTypeChanged = InArgs._OnPinTypeChanged;
	OnTypePreChanged = InArgs._OnPinTypePreChanged;

	check(GetPinTypeTreeFunc.IsBound());
	GetPinTypeTree = GetPinTypeTreeFunc;

	Schema = (UEdGraphSchema_K2*)(InArgs._Schema);
	bAllowExec = InArgs._bAllowExec;
	bAllowWildcard = InArgs._bAllowWildcard;
	TreeViewWidth = InArgs._TreeViewWidth;
	TreeViewHeight = InArgs._TreeViewHeight;

	TargetPinType = InArgs._TargetPinType;

	GetPinTypeTree.Execute(TypeTreeRoot, bAllowExec, bAllowWildcard);
	FilteredTypeTreeRoot = TypeTreeRoot;

	this->ChildSlot
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		[
			SAssignNew( TypeComboButton, SComboButton )
			.OnGetMenuContent(this, &SPinTypeSelector::GetMenuContent)
			.ContentPadding(0)
			.ToolTipText(this, &SPinTypeSelector::GetToolTipForComboBoxType)
			.ButtonContent()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image( this, &SPinTypeSelector::GetTypeIconImage )
					.ColorAndOpacity( this, &SPinTypeSelector::GetTypeIconColor )
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(STextBlock)
					.Text( this, &SPinTypeSelector::GetTypeDescription )
					.Font(InArgs._Font)
				]
			]
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew( SCheckBox )
			.ToolTip(IDocumentation::Get()->CreateToolTip( TAttribute<FText>(this, &SPinTypeSelector::GetToolTipForArrayWidget), NULL, *BigTooltipDocLink, TEXT("Array")))
			.Padding( 1 )
			.OnCheckStateChanged( this, &SPinTypeSelector::OnArrayCheckStateChanged )
			.IsChecked( this, &SPinTypeSelector::IsArrayChecked )
			.IsEnabled( TargetPinType.Get().PinCategory != Schema->PC_Exec )
			.Style( FEditorStyle::Get(), "ToggleButtonCheckbox" )
			.Visibility(InArgs._bAllowArrays ? EVisibility::Visible : EVisibility::Collapsed)
			[
				SNew(SImage)
				.Image( FEditorStyle::GetBrush(TEXT("Kismet.VariableList.ArrayTypeIcon")) )
				.ColorAndOpacity( this, &SPinTypeSelector::GetTypeIconColor )
			]
		]
	];
}

//=======================================================================
// Attribute Helpers

FString SPinTypeSelector::GetTypeDescription() const
{
 	if( TargetPinType.Get().PinSubCategoryObject != NULL )
 	{
 		return TargetPinType.Get().PinSubCategoryObject->GetName();
 	}
 	else
 	{
 		return TargetPinType.Get().PinCategory;
 	}
}

const FSlateBrush* SPinTypeSelector::GetTypeIconImage() const
{
	return GetIconFromPin( TargetPinType.Get() );
}

FSlateColor SPinTypeSelector::GetTypeIconColor() const
{
	return Schema->GetPinTypeColor(TargetPinType.Get());
}

ESlateCheckBoxState::Type SPinTypeSelector::IsArrayChecked() const
{
	return TargetPinType.Get().bIsArray;
}

void SPinTypeSelector::OnArrayCheckStateChanged(ESlateCheckBoxState::Type NewState)
{
	FEdGraphPinType NewTargetPinType = TargetPinType.Get();
	NewTargetPinType.bIsArray = (NewState == ESlateCheckBoxState::Checked)? true : false;

	OnTypeChanged.ExecuteIfBound(NewTargetPinType);
}

//=======================================================================
// Type TreeView Support
TSharedRef<ITableRow> SPinTypeSelector::GenerateTypeTreeRow(FPinTypeTreeItem InItem, const TSharedRef<STableViewBase>& OwnerTree)
{
	const bool bHasChildren = (InItem->Children.Num() > 0);
	const FString Description = InItem->GetDescription();

	// Determine the best icon the to represents this item
	const FSlateBrush* IconBrush = GetIconFromPin( InItem->PinType );

	// Use tooltip if supplied, otherwise just repeat description
	const FString OrgTooltip = InItem->GetToolTip();
	const FString Tooltip = !OrgTooltip.IsEmpty() ? OrgTooltip : Description;

	return SNew( STableRow<FPinTypeTreeItem>, OwnerTree )
		.ToolTip( IDocumentation::Get()->CreateToolTip( FText::FromString( Tooltip ), NULL, *BigTooltipDocLink, *Description) )
		.ShowSelection(!InItem->bReadOnly)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.f)
			[
				SNew(SImage)
				.Image(IconBrush)
				.ColorAndOpacity( Schema->GetPinTypeColor(InItem->PinType) )
				.Visibility( InItem->bReadOnly ? EVisibility::Collapsed : EVisibility::Visible )
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.f)
			[
				SNew(STextBlock)
				.Text(Description)
				.HighlightText(SearchText)
				.Font( bHasChildren ? FEditorStyle::GetFontStyle(TEXT("Kismet.TypePicker.CategoryFont")) : FEditorStyle::GetFontStyle(TEXT("Kismet.TypePicker.NormalFont")) )
			]
		];
}

void SPinTypeSelector::OnTypeSelectionChanged(FPinTypeTreeItem Selection, ESelectInfo::Type /*SelectInfo*/)
{
	// Only handle selection for non-read only items, since STreeViewItem doesn't actually support read-only
 	if( Selection.IsValid() )
 	{
		if( !Selection->bReadOnly )
		{
			const FScopedTransaction Transaction( LOCTEXT("ChangeParam", "Change Paramater Type") );
			
			FEdGraphPinType NewTargetPinType = TargetPinType.Get();
			//Call delegate in order to notify pin type change is about to happen
			OnTypePreChanged.ExecuteIfBound(NewTargetPinType);

			// Change the pin's type
			NewTargetPinType.PinCategory = Selection->PinType.PinCategory;
			NewTargetPinType.PinSubCategory = Selection->PinType.PinSubCategory;
			NewTargetPinType.PinSubCategoryObject = Selection->PinType.PinSubCategoryObject;

			TypeComboButton->SetIsOpen(false);

			if( NewTargetPinType.PinCategory == Schema->PC_Exec )
			{
				NewTargetPinType.bIsArray = false;
			}

			OnTypeChanged.ExecuteIfBound(NewTargetPinType);
		}
		else
		{
			// Expand / contract the category, if applicable
			if( Selection->Children.Num() > 0 )
			{
				const bool bIsExpanded = TypeTreeView->IsItemExpanded(Selection);
				TypeTreeView->SetItemExpansion(Selection, !bIsExpanded);
				TypeTreeView->ClearSelection();
			}
		}
 	}
}

void SPinTypeSelector::GetTypeChildren(FPinTypeTreeItem InItem, TArray<FPinTypeTreeItem>& OutChildren)
{
	OutChildren = InItem->Children;
}

TSharedRef<SWidget>	SPinTypeSelector::GetMenuContent()
{
	// Refresh the type tree, in case we've gotten any new valid types from other blueprints
	GetPinTypeTree.Execute(TypeTreeRoot, bAllowExec, bAllowWildcard);

	if( !MenuContent.IsValid() )
	{
		MenuContent = SNew( SVerticalBox )
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.f, 4.f, 4.f, 4.f)
			[
				SAssignNew(FilterTextBox, SSearchBox)
				.OnTextChanged( this, &SPinTypeSelector::OnFilterTextChanged )
				.OnTextCommitted( this, &SPinTypeSelector::OnFilterTextCommitted )
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.f, 4.f, 4.f, 4.f)
			[
				SNew(SBox)
				.HeightOverride(TreeViewHeight)
				.WidthOverride(TreeViewWidth)
				[
					SAssignNew(TypeTreeView, SPinTypeTreeView)
					.TreeItemsSource(&FilteredTypeTreeRoot)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SPinTypeSelector::GenerateTypeTreeRow)
					.OnSelectionChanged(this, &SPinTypeSelector::OnTypeSelectionChanged)
					.OnGetChildren(this, &SPinTypeSelector::GetTypeChildren)
				]
			];
			

			TypeComboButton->SetMenuContentWidgetToFocus(FilterTextBox);
	}

	// Clear the filter text box with each opening
	if( FilterTextBox.IsValid() )
	{
		FilterTextBox->SetText( FText::GetEmpty() );
	}

	return MenuContent.ToSharedRef();
}

//=======================================================================
// Search Support
void SPinTypeSelector::OnFilterTextChanged(const FText& NewText)
{
	SearchText = NewText;
	FilteredTypeTreeRoot.Empty();

	GetChildrenMatchingSearch(NewText.ToString(), TypeTreeRoot, FilteredTypeTreeRoot);
	TypeTreeView->RequestTreeRefresh();
}

void SPinTypeSelector::OnFilterTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo)
{
	if( FilteredTypeTreeRoot.Num() == 1 && CommitInfo == ETextCommit::OnEnter)
	{
		if(FilteredTypeTreeRoot[0]->Children.Num() == 0)
		{
			OnTypeSelectionChanged(FilteredTypeTreeRoot[0], ESelectInfo::Direct);
		}
		else if(FilteredTypeTreeRoot[0]->Children.Num() == 1)
		{
			OnTypeSelectionChanged(FilteredTypeTreeRoot[0]->Children[0], ESelectInfo::Direct);
		}
	}
}

bool SPinTypeSelector::GetChildrenMatchingSearch(const FString& InSearchText, const TArray<FPinTypeTreeItem>& UnfilteredList, TArray<FPinTypeTreeItem>& OutFilteredList)
{
	bool bReturnVal = false;

	for( auto it = UnfilteredList.CreateConstIterator(); it; ++it )
	{
		FPinTypeTreeItem Item = *it;
		FPinTypeTreeItem NewInfo = MakeShareable( new UEdGraphSchema_K2::FPinTypeTreeInfo(Item) );
		TArray<FPinTypeTreeItem> ValidChildren;

		// Have to run GetChildrenMatchingSearch first, so that we can make sure we get valid children for the list!
		if( GetChildrenMatchingSearch(InSearchText, Item->Children, ValidChildren)
			|| InSearchText.IsEmpty()
			|| Item->GetDescription().Contains(InSearchText) )
		{
			NewInfo->Children = ValidChildren;
			OutFilteredList.Add(NewInfo);

			TypeTreeView->SetItemExpansion(NewInfo, !InSearchText.IsEmpty());

			bReturnVal = true;
		}
	}

	return bReturnVal;
}

const FSlateBrush* SPinTypeSelector::GetIconFromPin( const FEdGraphPinType& PinType ) const
{
	const FSlateBrush* IconBrush = FEditorStyle::GetBrush(TEXT("Kismet.VariableList.TypeIcon"));
	const UObject* PinSubObject = PinType.PinSubCategoryObject.Get();
	if( (PinType.bIsArray || (IsArrayChecked() == ESlateCheckBoxState::Checked)) && PinType.PinCategory != Schema->PC_Exec )
	{
		IconBrush = FEditorStyle::GetBrush(TEXT("Kismet.VariableList.ArrayTypeIcon"));
	}
	else if( PinSubObject )
	{
		UClass* VarClass = FindObject<UClass>(ANY_PACKAGE, *PinSubObject->GetName());
		if( VarClass )
		{
			IconBrush = FClassIconFinder::FindIconForClass( VarClass );
		}
	}
	return IconBrush;
}

FText SPinTypeSelector::GetToolTipForComboBoxType() const
{
	if(IsEnabled())
	{
		return LOCTEXT("PinTypeSelector", "Select the variable's pin type.");
	}

	return LOCTEXT("PinTypeSelector_Disabled", "Cannot edit variable type while the variable is placed in a graph or inherited from parent.");
}

FText SPinTypeSelector::GetToolTipForArrayWidget() const
{
	if(IsEnabled())
	{
		// The entire widget may be enabled, but the array button disabled because it is an "exec" pin.
		if(TargetPinType.Get().PinCategory == Schema->PC_Exec)
		{
			return LOCTEXT("ArrayCheckBox_ExecDisabled", "Exec pins cannot be arrays.");
		}
		return LOCTEXT("ArrayCheckBox", "Make this variable an array of selected type.");
	}

	return LOCTEXT("ArrayCheckBox_Disabled", "Cannot edit variable type while the variable is placed in a graph or inherited from parent.");
}
#undef LOCTEXT_NAMESPACE
