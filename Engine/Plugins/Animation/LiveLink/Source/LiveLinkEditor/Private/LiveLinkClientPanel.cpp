// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkClientPanel.h"
#include "Editor.h"
#include "SBoxPanel.h"
#include "SSplitter.h"
#include "SOverlay.h"
#include "SharedPointer.h"
#include "SListView.h"
#include "SButton.h"
#include "SlateApplication.h"
#include "MultiBoxBuilder.h"
#include "SlateDelegates.h"
#include "LiveLinkClientCommands.h"
#include "ILiveLinkSource.h"
#include "LiveLinkClient.h"
#include "UObjectHash.h"
#include "EditorStyleSet.h"
#include "ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IStructureDetailsView.h"
#include "LiveLinkVirtualSubjectDetails.h"
#include "Editor/EditorPerformanceSettings.h"

#include "LiveLinkSourceFactory.h"

#include "Widgets/Input/STextEntryPopup.h"

#define LOCTEXT_NAMESPACE "LiveLinkClientPanel"

// Static Source UI FNames
namespace SourceListUI
{
	static const FName TypeColumnName(TEXT("Type"));
	static const FName MachineColumnName(TEXT("Machine"));
	static const FName StatusColumnName(TEXT("Status"));
};

// Static Subject UI FNames
namespace SubjectTreeUI
{
	static const FName NameColumnName(TEXT("Name"));
};

// Structure that defines a single entry in the source UI
struct FLiveLinkSourceUIEntry
{
public:
	FLiveLinkSourceUIEntry(FGuid InEntryGuid, FLiveLinkClient* InClient)
		: EntryGuid(InEntryGuid)
		, Client(InClient)
	{}

	FText GetSourceType() { return Client->GetSourceTypeForEntry(EntryGuid); }
	FText GetMachineName() { return Client->GetMachineNameForEntry(EntryGuid); }
	FText GetEntryStatus() { return Client->GetEntryStatusForEntry(EntryGuid); }
	ULiveLinkSourceSettings* GetSourceSettings() { return Client->GetSourceSettingsForEntry(EntryGuid); }
	void RemoveFromClient() { Client->RemoveSource(EntryGuid); }
	void OnPropertyChanged(const FPropertyChangedEvent& InEvent) { Client->OnPropertyChanged(EntryGuid, InEvent); }

private:
	FGuid EntryGuid;
	FLiveLinkClient* Client;
};

// Structure that defines a single entry in the subject UI
struct FLiveLinkSubjectUIEntry
{
	// Id of the subject
	FLiveLinkSubjectKey SubjectKey;

	// If SubjectKey.SubjectName is None then we are a source, this should be set instead
	FText SourceName;

	// Children (if this entry represents a source instead of a specific subject
	TArray<FLiveLinkSubjectUIEntryPtr> Children;
};

class SLiveLinkClientPanelSourcesRow : public SMultiColumnTableRow<FLiveLinkSourceUIEntryPtr>
{
public:
	SLATE_BEGIN_ARGS(SLiveLinkClientPanelSourcesRow) {}

		/** The list item for this row */
		SLATE_ARGUMENT(FLiveLinkSourceUIEntryPtr, Entry)

	SLATE_END_ARGS()

	void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		EntryPtr = Args._Entry;

		SourceType = EntryPtr->GetSourceType();
		MachineName = EntryPtr->GetMachineName();

		SMultiColumnTableRow<FLiveLinkSourceUIEntryPtr>::Construct(
			FSuperRowType::FArguments()
			.Padding(1.0f),
			OwnerTableView
		);
	}

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the list view. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (ColumnName == SourceListUI::TypeColumnName)
		{
			return	SNew(STextBlock)
					.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SLiveLinkClientPanelSourcesRow::GetSourceTypeName)));
		}
		else if (ColumnName == SourceListUI::MachineColumnName)
		{
			return	SNew(STextBlock)
					.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SLiveLinkClientPanelSourcesRow::GetSourceMachineName)));
		}
		else if (ColumnName == SourceListUI::StatusColumnName)
		{
			return	SNew(STextBlock)
					.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SLiveLinkClientPanelSourcesRow::GetSourceStatus)));
		}

		return SNullWidget::NullWidget;
	}

private:

	FText GetSourceTypeName() const
	{
		return SourceType;
	}

	FText GetSourceMachineName() const
	{
		return MachineName;
	}

	FText GetSourceStatus() const
	{
		return EntryPtr->GetEntryStatus();
	}

	FLiveLinkSourceUIEntryPtr EntryPtr;

	FText SourceType;

	FText MachineName;
};

SLiveLinkClientPanel::~SLiveLinkClientPanel()
{
	if (Client)
	{
		Client->UnregisterSourcesChangedHandle(OnSourcesChangedHandle);
		OnSourcesChangedHandle.Reset();

		Client->UnregisterSubjectsChangedHandle(OnSubjectsChangedHandle);
		OnSubjectsChangedHandle.Reset();
	}
	GEditor->UnregisterForUndo(this);
}

void SLiveLinkClientPanel::Construct(const FArguments& Args, FLiveLinkClient* InClient)
{
	GEditor->RegisterForUndo(this);

	check(InClient);
	Client = InClient;

	OnSourcesChangedHandle = Client->RegisterSourcesChangedHandle(FSimpleMulticastDelegate::FDelegate::CreateSP(this, &SLiveLinkClientPanel::OnSourcesChangedHandler));
	OnSubjectsChangedHandle = Client->RegisterSubjectsChangedHandle(FSimpleMulticastDelegate::FDelegate::CreateSP(this, &SLiveLinkClientPanel::OnSubjectsChangedHandler));

	RefreshSourceData(false);

	CommandList = MakeShareable(new FUICommandList);

	BindCommands();

	FToolBarBuilder ToolBarBuilder(CommandList, FMultiBoxCustomization::None);

	ToolBarBuilder.BeginSection(TEXT("Add"));
	{
		ToolBarBuilder.AddComboButton(	FUIAction(),
										FOnGetContent::CreateSP(this, &SLiveLinkClientPanel::GenerateSourceMenu),
										LOCTEXT("AddSource", "Add"),
										LOCTEXT("AddSource_ToolTip", "Add a new live link source"),
										FSlateIcon(FName("LiveLinkStyle"), "LiveLinkClient.Common.AddSource")
										);
	}
	ToolBarBuilder.EndSection();

	ToolBarBuilder.BeginSection(TEXT("Remove"));
	{
		ToolBarBuilder.AddToolBarButton(FLiveLinkClientCommands::Get().RemoveSource);
		ToolBarBuilder.AddToolBarButton(FLiveLinkClientCommands::Get().RemoveAllSources);
	}
	ToolBarBuilder.EndSection();

	// Connection Settings
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	//DetailsViewArgs.
	FStructureDetailsViewArgs StructureViewArgs;
	StructureViewArgs.bShowAssets = true;
	StructureViewArgs.bShowClasses = true;
	StructureViewArgs.bShowInterfaces = true;
	StructureViewArgs.bShowObjects = true;

	SettingsDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	SettingsDetailsView->OnFinishedChangingProperties().AddSP(this, &SLiveLinkClientPanel::OnPropertyChanged);

	SettingsDetailsView->RegisterInstancedCustomPropertyLayout(ULiveLinkVirtualSubjectDetails::StaticClass(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FLiveLinkVirtualSubjectDetailCustomization::MakeInstance, Client));

	SAssignNew(SubjectsTreeView, STreeView<FLiveLinkSubjectUIEntryPtr>)
		.TreeItemsSource(&SubjectData)
		.OnGenerateRow(this, &SLiveLinkClientPanel::MakeTreeRowWidget)
		.OnGetChildren(this, &SLiveLinkClientPanel::GetChildrenForInfo)
		.OnSelectionChanged(this, &SLiveLinkClientPanel::OnSelectionChanged)
		.SelectionMode(ESelectionMode::Single)
		.HeaderRow
		(
			SNew(SHeaderRow)
			+ SHeaderRow::Column(SubjectTreeUI::NameColumnName)
			.DefaultLabel(LOCTEXT("SubjectItemName", "Subject Name"))
			.FillWidth(1.f)
		);

	const int WarningPadding = 8;

	UProperty* PerformanceThrottlingProperty = FindFieldChecked<UProperty>(UEditorPerformanceSettings::StaticClass(), GET_MEMBER_NAME_CHECKED(UEditorPerformanceSettings, bThrottleCPUWhenNotForeground));
	PerformanceThrottlingProperty->GetDisplayNameText();
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("PropertyName"), PerformanceThrottlingProperty->GetDisplayNameText());
	FText PerformanceWarningText = FText::Format(LOCTEXT("LiveLinkPerformanceWarningMessage", "Warning: The editor setting '{PropertyName}' is currently enabled\nThis will stop editor windows from updating in realtime while the editor is not in focus"), Arguments);
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
		[
			ToolBarBuilder.MakeWidget()
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
		[
			SNew(SSplitter)
			.Orientation(EOrientation::Orient_Horizontal)
			+SSplitter::Slot()
			.Value(0.5f)
			[
				SNew(SSplitter)
				.Orientation(EOrientation::Orient_Vertical)
				+SSplitter::Slot()
				.Value(0.5f)
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(FMargin(4.0f, 4.0f))
					[
						SAssignNew(ListView, SListView<FLiveLinkSourceUIEntryPtr>)
						.ListItemsSource(&SourceData)
						.SelectionMode(ESelectionMode::SingleToggle)
						.OnGenerateRow(this, &SLiveLinkClientPanel::MakeSourceListViewWidget)
						.OnSelectionChanged(this, &SLiveLinkClientPanel::OnSourceListSelectionChanged)
						.HeaderRow
						(
							SNew(SHeaderRow)
							+ SHeaderRow::Column(SourceListUI::TypeColumnName)
							.FillWidth(43.0f)
							.DefaultLabel(LOCTEXT("TypeColumnHeaderName", "Source Type"))
							+ SHeaderRow::Column(SourceListUI::MachineColumnName)
							.FillWidth(43.0f)
							.DefaultLabel(LOCTEXT("MachineColumnHeaderName", "Source Machine"))
							+ SHeaderRow::Column(SourceListUI::StatusColumnName)
							.FillWidth(14.0f)
							.DefaultLabel(LOCTEXT("StatusColumnHeaderName", "Status"))
						)
					]
				]
				+SSplitter::Slot()
				.Value(0.5f)
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(FMargin(4.0f, 4.0f))
					[
						SubjectsTreeView->AsShared()
					]
				]
			]
			+SSplitter::Slot()
			.Value(0.5f)
			[
				SettingsDetailsView.ToSharedRef()
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("SettingsEditor.CheckoutWarningBorder"))
			.BorderBackgroundColor(FColor(166, 137, 0))
			.Visibility(this, &SLiveLinkClientPanel::ShowEditorPerformanceThrottlingWarning)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(FMargin(WarningPadding, WarningPadding, WarningPadding, WarningPadding))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(PerformanceWarningText)
					.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.ShadowColorAndOpacity(FLinearColor::Black.CopyWithNewOpacity(0.3f))
					.ShadowOffset(FVector2D::UnitVector)
				]

				+ SHorizontalBox::Slot()
				.Padding(FMargin(0.f, 0.f, WarningPadding, 0.f))
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.OnClicked(this, &SLiveLinkClientPanel::DisableEditorPerformanceThrottling)
					.Text(LOCTEXT("LiveLinkPerformanceWarningDisable", "Disable"))
				]
			]
		]
	];


	RebuildSubjectList();
}

void SLiveLinkClientPanel::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(DetailsPanelEditorObjects);
}

void SLiveLinkClientPanel::BindCommands()
{
	CommandList->MapAction(FLiveLinkClientCommands::Get().RemoveSource,
		FExecuteAction::CreateSP(this, &SLiveLinkClientPanel::HandleRemoveSource),
		FCanExecuteAction::CreateSP(this, &SLiveLinkClientPanel::CanRemoveSource)
	);

	CommandList->MapAction(FLiveLinkClientCommands::Get().RemoveAllSources,
		FExecuteAction::CreateSP(this, &SLiveLinkClientPanel::HandleRemoveAllSources),
		FCanExecuteAction::CreateSP(this, &SLiveLinkClientPanel::CanRemoveSource)
	);
}

void SLiveLinkClientPanel::RefreshSourceData(bool bRefreshUI)
{
	SourceData.Reset();

	for (FGuid Source : Client->GetSourceEntries())
	{
		if (Client->ShowSourceInUI(Source))
		{
			SourceData.Add(MakeShareable(new FLiveLinkSourceUIEntry(Source, Client)));
		}
	}

	if (bRefreshUI)
	{
		ListView->RequestListRefresh();
	}
}

const TArray<FLiveLinkSourceUIEntryPtr>& SLiveLinkClientPanel::GetCurrentSources() const
{
	return SourceData;
}

TSharedRef<ITableRow> SLiveLinkClientPanel::MakeSourceListViewWidget(FLiveLinkSourceUIEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SLiveLinkClientPanelSourcesRow, OwnerTable)
		   .Entry(Entry);
}

void SLiveLinkClientPanel::OnSourceListSelectionChanged(FLiveLinkSourceUIEntryPtr Entry, ESelectInfo::Type SelectionType) const
{
	if(Entry.IsValid())
	{
		SettingsDetailsView->SetObject(Entry->GetSourceSettings());
	}
	else
	{
		SettingsDetailsView->SetObject(nullptr);
	}
}

TSharedRef<ITableRow> SLiveLinkClientPanel::MakeTreeRowWidget(FLiveLinkSubjectUIEntryPtr InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	FText ItemText = InInfo->SubjectKey.SubjectName.IsNone() ? InInfo->SourceName : FText::FromName(InInfo->SubjectKey.SubjectName);
	return SNew(STableRow<FLiveLinkSubjectUIEntryPtr>, OwnerTable)
		.Content()
		[
			SNew(STextBlock)
			.Text(ItemText)
		];
}

void SLiveLinkClientPanel::GetChildrenForInfo(FLiveLinkSubjectUIEntryPtr InInfo, TArray< FLiveLinkSubjectUIEntryPtr >& OutChildren)
{
	OutChildren = InInfo->Children;
}

void SLiveLinkClientPanel::OnSelectionChanged(FLiveLinkSubjectUIEntryPtr SubjectEntry, ESelectInfo::Type SelectInfo)
{
	SettingsDetailsView->SetObject(nullptr);
	if (SubjectEntry.IsValid() && SubjectEntry->SubjectKey.SubjectName != NAME_None)
	{
		if (Client->IsVirtualSubject(SubjectEntry->SubjectKey))
		{
			UObject* Obj = DetailsPanelEditorObjects.Contains(ULiveLinkVirtualSubjectDetails::StaticClass()) ? *DetailsPanelEditorObjects.Find(ULiveLinkVirtualSubjectDetails::StaticClass()) : nullptr;

			if (Obj == NULL)
			{
				FString ObjName = MakeUniqueObjectName(GetTransientPackage(), ULiveLinkVirtualSubjectDetails::StaticClass()).ToString();
				ObjName += "_EdObj";
				Obj = NewObject<ULiveLinkVirtualSubjectDetails>(GetTransientPackage(), ULiveLinkVirtualSubjectDetails::StaticClass(), FName(*ObjName), RF_Public | RF_Standalone | RF_Transient);
				Obj->SetFlags(RF_Transactional);
				DetailsPanelEditorObjects.Add(ULiveLinkVirtualSubjectDetails::StaticClass(), Obj);
			}

			ULiveLinkVirtualSubjectDetails* Details = CastChecked<ULiveLinkVirtualSubjectDetails>(Obj);

			Details->Client = Client;
			Details->SubjectKey = SubjectEntry->SubjectKey;
			Details->VirtualSubjectProxy = Client->GetVirtualSubjectProperties(SubjectEntry->SubjectKey);
			SettingsDetailsView->SetObject(Details);
		}
	}
}

void SLiveLinkClientPanel::RebuildSubjectList()
{
	TArray<FLiveLinkSubjectKey> SavedSelection;
	TArray<FLiveLinkSubjectUIEntryPtr> SelectedItems = SubjectsTreeView->GetSelectedItems();
	for (const FLiveLinkSubjectUIEntryPtr& SelectedItem : SelectedItems)
	{
		SavedSelection.Add(SelectedItem->SubjectKey);
	}

	TArray<FLiveLinkSubjectKey> SubjectKeys = Client->GetSubjects();
	SubjectData.Reset();

	TMap<FGuid, FLiveLinkSubjectUIEntryPtr> SourceHeaderItems;
	TArray<FLiveLinkSubjectUIEntryPtr> AllItems;
	AllItems.Reserve(SubjectKeys.Num());

	for (const FLiveLinkSubjectKey& SubjectKey : SubjectKeys)
	{
		FLiveLinkSubjectUIEntryPtr Source;
		if(FLiveLinkSubjectUIEntryPtr* SourcePtr = SourceHeaderItems.Find(SubjectKey.Source))
		{
			Source = *SourcePtr;
		}
		else
		{
			Source = MakeShared<FLiveLinkSubjectUIEntry>();
			SubjectData.Add(Source);
			SourceHeaderItems.Add(SubjectKey.Source) = Source;

			Source->SourceName = Client->GetSourceTypeForEntry(SubjectKey.Source);

			SubjectsTreeView->SetItemExpansion(Source, true);
			AllItems.Add(Source);
		}

		FLiveLinkSubjectUIEntryPtr SubjectEntry = MakeShared<FLiveLinkSubjectUIEntry>();
		SubjectEntry->SubjectKey = SubjectKey;

		Source->Children.Add(SubjectEntry);
		AllItems.Add(SubjectEntry);
	}

	for (const FLiveLinkSubjectUIEntryPtr& Item : AllItems)
	{
		for (FLiveLinkSubjectKey& Selection : SavedSelection)
		{
			if (Item->SubjectKey.Source == Selection.Source && Item->SubjectKey.SubjectName == Selection.SubjectName)
			{
				SubjectsTreeView->SetItemSelection(Item, true);
				break;
			}
		}
	}

	SubjectsTreeView->RequestTreeRefresh();
}

void SLiveLinkClientPanel::OnPropertyChanged(const FPropertyChangedEvent& InEvent)
{
	TArray<FLiveLinkSourceUIEntryPtr> Selected;
	ListView->GetSelectedItems(Selected);
	for (FLiveLinkSourceUIEntryPtr Item : Selected)
	{
		Item->OnPropertyChanged(InEvent);
	}
}

TSharedRef< SWidget > SLiveLinkClientPanel::GenerateSourceMenu()
{
	TArray<UClass*> Results;
	GetDerivedClasses(ULiveLinkSourceFactory::StaticClass(), Results, true);

	const bool CloseAfterSelection = true;
	FMenuBuilder MenuBuilder(CloseAfterSelection, NULL);

	MenuBuilder.BeginSection("SourceSection", LOCTEXT("Sources", "Live Link Sources"));

	for (UClass* SourceFactory : Results)
	{
		ULiveLinkSourceFactory* FactoryCDO = SourceFactory->GetDefaultObject<ULiveLinkSourceFactory>();

		//Prep UI
		TSharedPtr<SWidget> SourcePanel = FactoryCDO->CreateSourceCreationPanel();
		SourcePanels.Add(FactoryCDO) = SourcePanel;

		MenuBuilder.AddSubMenu(
			FactoryCDO->GetSourceDisplayName(),
			FactoryCDO->GetSourceTooltip(),
			FNewMenuDelegate::CreateRaw(this, &SLiveLinkClientPanel::RetrieveFactorySourcePanel, FactoryCDO),
			false
		);
	}

	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("SourceSection", LOCTEXT("Sources", "Live Link Sources"));
	FUIAction Action_AddVirtualSubject(
		FExecuteAction::CreateSP(this, &SLiveLinkClientPanel::AddVirtualSubject));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddVirtualSubject", "Add Virtual Subject"),
		LOCTEXT("AddVirtualSubject_Tooltip", "Adds a new virtual subject to live link. Instead of coming from a source a virtual subject is a combination of 2 or more real subjects"),
		FSlateIcon(),
		Action_AddVirtualSubject,
		NAME_None,
		EUserInterfaceActionType::Button);
	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

void SLiveLinkClientPanel::RetrieveFactorySourcePanel(FMenuBuilder& MenuBuilder, ULiveLinkSourceFactory* FactoryCDO)
{
	TSharedPtr<SWidget> SourcePanel = SourcePanels[FactoryCDO];
	MenuBuilder.AddWidget(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			SourcePanel.ToSharedRef()
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.f)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(EHorizontalAlignment::HAlign_Right)
			[
				SNew(SButton)
				.Text(LOCTEXT("OkButton", "Ok"))
				.OnClicked(this, &SLiveLinkClientPanel::OnCloseSourceSelectionPanel, FactoryCDO, true)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(EHorizontalAlignment::HAlign_Right)
			[
				SNew(SButton)
				.Text(LOCTEXT("CancelButton", "Cancel"))
				.OnClicked(this, &SLiveLinkClientPanel::OnCloseSourceSelectionPanel, FactoryCDO, false)
			]
		]
		, FText(), true);
}

FReply SLiveLinkClientPanel::OnCloseSourceSelectionPanel(ULiveLinkSourceFactory* FactoryCDO, bool bMakeSource)
{
	TSharedPtr<ILiveLinkSource> Source = FactoryCDO->OnSourceCreationPanelClosed(bMakeSource);
	
	if (Source.IsValid())
	{
		Client->AddSource(Source);

		RefreshSourceData(true);
	}
	
	FSlateApplication::Get().DismissAllMenus();
	return FReply::Handled();
}

void SLiveLinkClientPanel::AddVirtualSubject()
{
	// Show dialog to enter weight
	TSharedRef<STextEntryPopup> TextEntry =
		SNew(STextEntryPopup)
		.Label(LOCTEXT("AddVirtualSubject", "New Virtual Subject Name"))
		.OnTextCommitted(this, &SLiveLinkClientPanel::HandleAddVirtualSubject);

	FSlateApplication::Get().PushMenu(
		AsShared(), // Menu being summoned from a menu that is closing: Parent widget should be k2 not the menu thats open or it will be closed when the menu is dismissed
		FWidgetPath(),
		TextEntry,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup)
	);
}

void SLiveLinkClientPanel::HandleAddVirtualSubject(const FText& NewSubjectName, ETextCommit::Type CommitInfo)
{
	if (CommitInfo == ETextCommit::OnEnter || CommitInfo == ETextCommit::OnUserMovedFocus)
	{
		Client->AddVirtualSubject(*NewSubjectName.ToString());
		RebuildSubjectList();
	}

	FSlateApplication::Get().DismissAllMenus();
}

void SLiveLinkClientPanel::HandleRemoveSource()
{
	TArray<FLiveLinkSourceUIEntryPtr> Selected;
	ListView->GetSelectedItems(Selected);
	if (Selected.Num() > 0)
	{
		Selected[0]->RemoveFromClient();
	}
}

bool SLiveLinkClientPanel::CanRemoveSource()
{
	return ListView->GetNumItemsSelected() > 0;
}

void SLiveLinkClientPanel::HandleRemoveAllSources()
{
	Client->RemoveAllSources();
}

void SLiveLinkClientPanel::OnSourcesChangedHandler()
{
	RefreshSourceData(true);
}

void SLiveLinkClientPanel::OnSubjectsChangedHandler()
{
	RebuildSubjectList();
}

void SLiveLinkClientPanel::PostUndo(bool bSuccess)
{
	SettingsDetailsView->ForceRefresh();
}

void SLiveLinkClientPanel::PostRedo(bool bSuccess)
{
	SettingsDetailsView->ForceRefresh();
}

EVisibility SLiveLinkClientPanel::ShowEditorPerformanceThrottlingWarning() const
{
	const UEditorPerformanceSettings* Settings = GetDefault<UEditorPerformanceSettings>();
	return Settings->bThrottleCPUWhenNotForeground ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply SLiveLinkClientPanel::DisableEditorPerformanceThrottling()
{
	UEditorPerformanceSettings* Settings = GetMutableDefault<UEditorPerformanceSettings>();
	Settings->bThrottleCPUWhenNotForeground = false;
	Settings->PostEditChange();
	Settings->SaveConfig();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE