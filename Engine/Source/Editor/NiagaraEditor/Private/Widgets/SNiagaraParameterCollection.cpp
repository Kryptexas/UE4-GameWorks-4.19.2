// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SNiagaraParameterCollection.h"
#include "NiagaraEditorModule.h"
#include "NiagaraParameterCollectionViewModel.h"
#include "NiagaraParameterViewModel.h"
#include "NiagaraTypes.h"
#include "INiagaraEditorTypeUtilities.h"
#include "SNiagaraParameterEditor.h"
#include "NiagaraEditorStyle.h"

#include "SButton.h"
#include "SExpandableArea.h"
#include "SInlineEditableTextBlock.h"
#include "SComboBox.h"
#include "SImage.h"
#include "SSplitter.h"
#include "MultiBoxBuilder.h"

#include "Modules/ModuleManager.h"
#include "IStructureDetailsView.h"
#include "GenericCommands.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "IDetailsView.h"


#define LOCTEXT_NAMESPACE "NiagaraParameterCollectionEditor"

class SSimpleExpander : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSimpleExpander)
		:_IsExpanded(false)
	{}
		SLATE_ARGUMENT(bool, IsExpanded)
		SLATE_NAMED_SLOT(FArguments, Header)
		SLATE_NAMED_SLOT(FArguments, Body)

	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs)
	{
		bIsExpanded = InArgs._IsExpanded;
		ExpandedImage = FCoreStyle::Get().GetBrush("TreeArrow_Expanded");
		CollapsedImage = FCoreStyle::Get().GetBrush("TreeArrow_Collapsed");

		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0)
				[
					SNew(SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.OnClicked(this, &SSimpleExpander::ExpandButtonClicked)
					.ForegroundColor(FSlateColor::UseForeground())
					[
						SNew(SImage)
						.Image(this, &SSimpleExpander::GetExpandButtonImage)
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
				]
				+ SHorizontalBox::Slot()
				[
					InArgs._Header.Widget
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.Visibility(this, &SSimpleExpander::GetBodyVisibility)
				[
					InArgs._Body.Widget
				]
			]
		];
	}
private:
	FReply ExpandButtonClicked()
	{
		bIsExpanded = !bIsExpanded;
		return FReply::Handled();
	}

	const FSlateBrush* GetExpandButtonImage() const
	{
		return bIsExpanded ? ExpandedImage : CollapsedImage;
	}

	EVisibility GetBodyVisibility() const
	{
		return bIsExpanded ? EVisibility::Visible : EVisibility::Collapsed;
	}

private:
	bool bIsExpanded;
	const FSlateBrush* ExpandedImage;
	const FSlateBrush* CollapsedImage;
};

void SNiagaraParameterCollection::Construct(const FArguments& InArgs, TSharedRef<INiagaraParameterCollectionViewModel> InCollection)
{
	Collection = InCollection;
	Collection->OnCollectionChanged().AddSP(this, &SNiagaraParameterCollection::ViewModelCollectionChanged);
	Collection->GetSelection().OnSelectedObjectsChanged().AddSP(this, &SNiagaraParameterCollection::ViewModelSelectionChanged);
	Collection->OnExpandedChanged().AddSP(this, &SNiagaraParameterCollection::ViewModelIsExpandedChanged);

	NameColumnWidth = InArgs._NameColumnWidth;
	ContentColumnWidth = InArgs._ContentColumnWidth;
	OnNameColumnWidthChanged = InArgs._OnNameColumnWidthChanged;
	OnContentColumnWidthChanged = InArgs._OnContentColumnWidthChanged;

	BindCommands();

	bUpdatingListSelectionFromViewModel = false;

	SAssignNew(ParameterListView, SListView<TSharedRef<INiagaraParameterViewModel>>)
		.ListItemsSource(&Collection->GetParameters())
		.OnGenerateRow(this, &SNiagaraParameterCollection::OnGenerateRowForParameter)
		.OnSelectionChanged(this, &SNiagaraParameterCollection::OnParameterListSelectionChanged);

	if (Collection->GetParameters().Num() > 0)
	{
		ParameterListView->SetSelection(Collection->GetParameters()[0], ESelectInfo::Direct);
	}

	ChildSlot
	[
		SAssignNew(ExpandableArea, SExpandableArea)
		.InitiallyCollapsed(Collection->GetIsExpanded() == false)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.OnAreaExpansionChanged(this, &SNiagaraParameterCollection::AreaExpandedChanged)
		.Padding(0)
		.HeaderContent()
		[
			SAssignNew(HeaderBox, SBox)
			[
				//~ Title
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(Collection.ToSharedRef(), &INiagaraParameterCollectionViewModel::GetDisplayName)
				]
				//~ Add button
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SAssignNew(AddButton, SComboButton)
					.HasDownArrow(false)
					.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
					.ForegroundColor(FSlateColor::UseForeground())
					.OnGetMenuContent(this, &SNiagaraParameterCollection::GetAddMenuContent)
					.Visibility(Collection.ToSharedRef(), &INiagaraParameterCollectionViewModel::GetAddButtonVisibility)
					.ButtonContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0, 1, 2, 1)
						[
							SNew(SImage)
							.ColorAndOpacity(FSlateColor::UseForeground())
							.Image(FEditorStyle::GetBrush("Plus"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "SmallText")
							.Text(Collection.ToSharedRef(), &INiagaraParameterCollectionViewModel::GetAddButtonText)
							.Visibility(this, &SNiagaraParameterCollection::GetAddButtonTextVisibility)
						]
					]
				]
			]
		]
		.BodyContent()
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				ParameterListView.ToSharedRef()
			]
		]
	];

}

void SNiagaraParameterCollection::BindCommands()
{
	Commands = MakeShareable(new FUICommandList());
	Commands->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(Collection.ToSharedRef(), &INiagaraParameterCollectionViewModel::DeleteSelectedParameters),
		FCanExecuteAction::CreateSP(Collection.ToSharedRef(), &INiagaraParameterCollectionViewModel::CanDeleteParameters));
}

FReply SNiagaraParameterCollection::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (Commands->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SNiagaraParameterCollection::ViewModelCollectionChanged()
{
	ParameterListView->RequestListRefresh();
}

void SNiagaraParameterCollection::ViewModelSelectionChanged()
{
	if (FNiagaraEditorUtilities::ArrayMatchesSet(ParameterListView->GetSelectedItems(), Collection->GetSelection().GetSelectedObjects()) == false)
	{
		bUpdatingListSelectionFromViewModel = true;
		{
			ParameterListView->ClearSelection();
			for (TSharedRef<INiagaraParameterViewModel> Parameter : Collection->GetSelection().GetSelectedObjects())
			{
				ParameterListView->SetItemSelection(Parameter, true);
			}
		}
		bUpdatingListSelectionFromViewModel = false;
	}
}

void SNiagaraParameterCollection::ViewModelIsExpandedChanged()
{
	ExpandableArea->SetExpanded(Collection->GetIsExpanded());
}

void SNiagaraParameterCollection::AreaExpandedChanged(bool bIsExpanded)
{
	Collection->SetIsExpanded(bIsExpanded);
}

EVisibility SNiagaraParameterCollection::GetAddButtonTextVisibility() const
{
	return HeaderBox->IsHovered() || AddButton->IsOpen() ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<SWidget> SNiagaraParameterCollection::GetAddMenuContent()
{
	FMenuBuilder AddMenuBuilder(true, nullptr);
	for (TSharedPtr<FNiagaraTypeDefinition> AvailableType : Collection->GetAvailableTypes())
	{
		AddMenuBuilder.AddMenuEntry
		(
			AvailableType->GetStruct()->GetDisplayNameText(),
			FText(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(Collection.ToSharedRef(), &INiagaraParameterCollectionViewModel::AddParameter, AvailableType))
		);
	}
	return AddMenuBuilder.MakeWidget();
}

TSharedRef<ITableRow> SNiagaraParameterCollection::OnGenerateRowForParameter(TSharedRef<INiagaraParameterViewModel> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	// Name widget
	TSharedPtr<SWidget> NameWidget;
	if (Item->CanRenameParameter())
	{
		SAssignNew(NameWidget, SInlineEditableTextBlock)
			.Style(FNiagaraEditorStyle::Get(), "NiagaraEditor.ParameterInlineEditableText")
			.Text(Item, &INiagaraParameterViewModel::GetNameText)
			.ToolTipText(Item, &INiagaraParameterViewModel::GetNameText)
			.OnTextCommitted(Item, &INiagaraParameterViewModel::NameTextComitted)
			.IsSelected(this, &SNiagaraParameterCollection::IsItemSelected, Item);
	}
	else
	{
		SAssignNew(NameWidget, SBox)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(FNiagaraEditorStyle::Get(), "NiagaraEditor.ParameterText")
				.Text(Item, &INiagaraParameterViewModel::GetNameText)
				.ToolTipText(Item, &INiagaraParameterViewModel::GetNameText)
			];
	}

	// Type Widget
	TSharedPtr<SWidget> TypeWidget;
	if (Item->CanChangeParameterType())
	{
		// Because we use templating behind the scenes to get the combo box to work, we will ultimately end up comparing the SharedPtr's by pointer value.
		// Since the Available Types array comes from one location and the current type comes from another, their smart pointers are not guaranteed to 
		// point to the same FNiagaraTypeDefinition in memory. We need to enforce that the current type is from the values in the AvailableTypes array, which is why 
		// we look up the current type before creating the combo box.
		const TArray<TSharedPtr<FNiagaraTypeDefinition>>& AvailableTypes = Collection->GetAvailableTypes();
		TSharedPtr<FNiagaraTypeDefinition> CurrentType = Item->GetType();
		if (CurrentType.IsValid())
		{
			const TSharedPtr<FNiagaraTypeDefinition>* CurrentTypeFromAvailableList = AvailableTypes.FindByPredicate([&](const TSharedPtr<FNiagaraTypeDefinition>& AvailableType) { return (*AvailableType.Get()) == (*CurrentType.Get()); });
			if (CurrentTypeFromAvailableList != nullptr)
				CurrentType = *CurrentTypeFromAvailableList;
		}

		SAssignNew(TypeWidget, SComboBox<TSharedPtr<FNiagaraTypeDefinition>>)
			.OptionsSource(&AvailableTypes)
			.OnGenerateWidget(this, &SNiagaraParameterCollection::OnGenerateWidgetForTypeComboBox)
			.OnSelectionChanged(Item, &INiagaraParameterViewModel::SelectedTypeChanged)
			.InitiallySelectedItem(CurrentType)
			.Content()
			[
				SNew(STextBlock)
				.TextStyle(FEditorStyle::Get(), "SmallText")
				.Text(Item, &INiagaraParameterViewModel::GetTypeDisplayName)
			];
	}
	else
	{
		SAssignNew(TypeWidget, STextBlock)
			.Text(Item, &INiagaraParameterViewModel::GetTypeDisplayName);
	}

	// Details and parameter editor widgets.
	TSharedPtr<SNiagaraParameterEditor> ParameterEditor;
	TSharedPtr<SWidget> DetailsWidget;
	if (Item->GetDefaultValueType() == INiagaraParameterViewModel::EDefaultValueType::Struct)
	{
		FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::GetModuleChecked<FNiagaraEditorModule>("NiagaraEditor");
		TSharedPtr<INiagaraEditorTypeUtilities> TypeEditorUtilities = NiagaraEditorModule.GetTypeUtilities(*Item->GetType().Get());
		if (TypeEditorUtilities.IsValid() && TypeEditorUtilities->CanCreateParameterEditor())
		{
			ParameterEditor = TypeEditorUtilities->CreateParameterEditor();
			ParameterEditor->UpdateInternalValueFromStruct(Item->GetDefaultValueStruct());
			ParameterEditor->SetOnBeginValueChange(SNiagaraParameterEditor::FOnValueChange::CreateSP(
				this, &SNiagaraParameterCollection::ParameterEditorBeginValueChange, Item));
			ParameterEditor->SetOnEndValueChange(SNiagaraParameterEditor::FOnValueChange::CreateSP(
				this, &SNiagaraParameterCollection::ParameterEditorEndValueChange, Item));
			ParameterEditor->SetOnValueChanged(SNiagaraParameterEditor::FOnValueChange::CreateSP(
				this, &SNiagaraParameterCollection::ParameterEditorValueChanged, ParameterEditor.ToSharedRef(), Item));
		}

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		TSharedRef<IStructureDetailsView> StructureDetailsView = PropertyEditorModule.CreateStructureDetailView(
			FDetailsViewArgs(false, false, false, FDetailsViewArgs::HideNameArea, true),
			FStructureDetailsViewArgs(),
			nullptr);

		StructureDetailsView->SetStructureData(Item->GetDefaultValueStruct());
		StructureDetailsView->GetOnFinishedChangingPropertiesDelegate().AddSP(Item, &INiagaraParameterViewModel::NotifyDefaultValuePropertyChanged);

		Item->OnDefaultValueChanged().AddSP(this, &SNiagaraParameterCollection::ParameterViewModelDefaultValueChanged, Item, ParameterEditor, StructureDetailsView);
		Item->OnTypeChanged().AddSP(this, &SNiagaraParameterCollection::ParameterViewModelTypeChanged);

		DetailsWidget = StructureDetailsView->GetWidget();
	}
	else if (Item->GetDefaultValueType() == INiagaraParameterViewModel::EDefaultValueType::Object)
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(
			FDetailsViewArgs(false, false, false, FDetailsViewArgs::HideNameArea, true, this));
		DetailsView->SetObject(Item->GetDefaultValueObject());

		DetailsWidget = DetailsView;
	}
	else
	{
		DetailsWidget = SNullWidget::NullWidget;
	}

	if (ParameterEditor.IsValid())
	{
		return SNew(STableRow<TSharedRef<INiagaraParameterViewModel>>, OwnerTable)
		.Padding(FMargin(2, 3, 2, 3))
		.Content()
		[
			SNew(SSimpleExpander)
			.IsExpanded(false)
			.Header()
			[
				SNew(SSplitter)
				+ SSplitter::Slot()
				.Value(NameColumnWidth)
				.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SNiagaraParameterCollection::ParameterNameColumnWidthChanged))
				[
					NameWidget.ToSharedRef()
				]
				+ SSplitter::Slot()
				.Value(ContentColumnWidth)
				.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SNiagaraParameterCollection::ParameterContentColumnWidthChanged))
				[
					SNew(SBox)
					.Padding(FMargin(3, 0, 0, 0))
					[
						ParameterEditor.ToSharedRef()
					]
				]
			]
			.Body()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(25, 2, 0, 0)
				[
					TypeWidget.ToSharedRef()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(25, 2, 0, 0)
				[
					DetailsWidget.ToSharedRef()
				]
			]
		];
	}
	else
	{
		return SNew(STableRow<TSharedRef<INiagaraParameterViewModel>>, OwnerTable)
		.Padding(FMargin(2))
		.Content()
		[
			SNew(SSimpleExpander)
			.IsExpanded(true)
			.Header()
			[
				SNew(SSplitter)
				+ SSplitter::Slot()
				.Value(NameColumnWidth)
				.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SNiagaraParameterCollection::ParameterNameColumnWidthChanged))
				[
					NameWidget.ToSharedRef()
				]
				+ SSplitter::Slot()
				.Value(ContentColumnWidth)
				.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SNiagaraParameterCollection::ParameterContentColumnWidthChanged))
				[
					TypeWidget.ToSharedRef()
				]
			]
			.Body()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(25, 2, 0, 0)
				[
					DetailsWidget.ToSharedRef()
				]
			]
		];
	}
}

TSharedRef<SWidget> SNiagaraParameterCollection::OnGenerateWidgetForTypeComboBox(TSharedPtr<FNiagaraTypeDefinition> Item)
{
	return SNew(STextBlock)
		.Text(Collection->GetTypeDisplayName(Item));
}

void SNiagaraParameterCollection::ParameterViewModelDefaultValueChanged(const FNiagaraVariable* ChangedVariable, TSharedRef<INiagaraParameterViewModel> Item, TSharedPtr<SNiagaraParameterEditor> ParameterEditor, TSharedRef<IStructureDetailsView> StructureDetailsView)
{
	if (ParameterEditor.IsValid())
	{
		ParameterEditor->UpdateInternalValueFromStruct(Item->GetDefaultValueStruct());
	}
	// Only update the details view if the parameter editor isn't currently the exclusive editor.  This hack is necessary because the details 
	// view closes all color pickers when it's changed! */
	if (ParameterEditor->GetIsEditingExclusively() == false)
	{
		StructureDetailsView->SetStructureData(Item->GetDefaultValueStruct());
	}
}

void SNiagaraParameterCollection::ParameterViewModelTypeChanged()
{
	if (ParameterListView.IsValid())
	{
		ParameterListView->RequestListRefresh();
	}
}

void SNiagaraParameterCollection::OnParameterListSelectionChanged(TSharedPtr<INiagaraParameterViewModel> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (bUpdatingListSelectionFromViewModel == false)
	{
		Collection->GetSelection().SetSelectedObjects(ParameterListView->GetSelectedItems());
	}
}

bool SNiagaraParameterCollection::IsItemSelected(TSharedRef<INiagaraParameterViewModel> Item)
{
	return Collection->GetSelection().GetSelectedObjects().Contains(Item);
}

void SNiagaraParameterCollection::ParameterEditorBeginValueChange(TSharedRef<INiagaraParameterViewModel> Item)
{
	Item->NotifyBeginDefaultValueChange();
}

void SNiagaraParameterCollection::ParameterEditorEndValueChange(TSharedRef<INiagaraParameterViewModel> Item)
{
	Item->NotifyEndDefaultValueChange();
}

void SNiagaraParameterCollection::ParameterEditorValueChanged(TSharedRef<SNiagaraParameterEditor> ParameterEditor, TSharedRef<INiagaraParameterViewModel> Item)
{
	ParameterEditor->UpdateStructFromInternalValue(Item->GetDefaultValueStruct());
	Item->NotifyDefaultValueChanged();
}

void SNiagaraParameterCollection::ParameterNameColumnWidthChanged(float Width)
{
	if (NameColumnWidth.IsBound() == false)
	{
		NameColumnWidth.Set(Width);
	}
	OnNameColumnWidthChanged.ExecuteIfBound(Width);
}

void SNiagaraParameterCollection::ParameterContentColumnWidthChanged(float Width)
{
	if (ContentColumnWidth.IsBound() == false)
	{
		ContentColumnWidth.Set(Width);
	}
	OnContentColumnWidthChanged.ExecuteIfBound(Width);
}

void SNiagaraParameterCollection::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged)
{
	for (TSharedRef<INiagaraParameterViewModel> Parameter : Collection->GetParameters())
	{
		if (Parameter->GetDefaultValueType() == INiagaraParameterViewModel::EDefaultValueType::Object)
		{
			UObject* ParameterObject = Parameter->GetDefaultValueObject();
			const UObject* ChangedObject = PropertyChangedEvent.GetObjectBeingEdited(0);
			const UObject* CurrentObject = ChangedObject;
			bool ParameterIsInObjectChain = false;
			while (ParameterIsInObjectChain == false && CurrentObject != nullptr)
			{
				if (ParameterObject == CurrentObject)
				{
					ParameterIsInObjectChain = true;
				}
				else
				{
					CurrentObject = CurrentObject->GetOuter();
				}
			}
			if (ParameterIsInObjectChain)
			{
				Parameter->NotifyDefaultValuePropertyChanged(PropertyChangedEvent);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE // NiagaraParameterCollectionEditor
