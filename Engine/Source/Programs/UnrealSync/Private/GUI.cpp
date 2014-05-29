// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealSync.h"

#include "Slate.h"
#include "StandaloneRenderer.h"
#include "LaunchEngineLoop.h"

/**
 * Simple text combo box widget.
 */
class SSimpleTextComboBox : public SCompoundWidget
{
	/* Delegate for selection changed event. */
	DECLARE_DELEGATE_TwoParams(FOnSelectionChanged, int32, ESelectInfo::Type);

	SLATE_BEGIN_ARGS(SSimpleTextComboBox){}
		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)
	SLATE_END_ARGS()

public:
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct(const FArguments& InArgs)
	{
		OnSelectionChanged = InArgs._OnSelectionChanged;

		this->ChildSlot
			[
				SNew(SComboBox<TSharedPtr<FString> >)
				.OptionsSource(&OptionsSource)
				.OnSelectionChanged(this, &SSimpleTextComboBox::ComboBoxSelectionChanged)
				.OnGenerateWidget(this, &SSimpleTextComboBox::MakeWidgetFromOption)
				[
					SNew(STextBlock).Text(this, &SSimpleTextComboBox::GetCurrentFTextOption)
				]
			];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	/**
	 * Adds value to the options source.
	 *
	 * @param Value Value to add.
	 */
	void Add(const FString& Value)
	{
		OptionsSource.Add(MakeShareable(new FString(Value)));
	}

	/**
	 * Gets current option value.
	 *
	 * @returns Reference to current option value.
	 */
	const FString& GetCurrentOption() const
	{
		return *OptionsSource[CurrentOption];
	}

	/**
	 * Checks if options source is empty.
	 *
	 * @returns True if options source is empty. False otherwise.
	 */
	bool IsEmpty()
	{
		return OptionsSource.Num() == 0;
	}

	/**
	 * Clears the options source.
	 */
	void Clear()
	{
		OptionsSource.Empty();
	}

private:
	/**
	 * Combo box selection changed event handling.
	 *
	 * @param Value Value of the picked selection.
	 * @param SelectInfo Selection info enum.
	 */
	void ComboBoxSelectionChanged(TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo)
	{
		CurrentOption = OptionsSource.Find(Value);

		OnSelectionChanged.ExecuteIfBound(CurrentOption, SelectInfo);
	}

	/**
	 * Coverts given value into widget.
	 *
	 * @param Value Value of the option.
	 *
	 * @return Widget.
	 */
	TSharedRef<SWidget> MakeWidgetFromOption(TSharedPtr<FString> Value)
	{
		return SNew(STextBlock).Text(FText::FromString(*Value));
	}

	/**
	 * Gets current option converted to FText class.
	 *
	 * @returns Current option converted to FText class.
	 */
	FText GetCurrentFTextOption() const
	{
		return FText::FromString(OptionsSource.Num() == 0 ? "" : GetCurrentOption());
	}

	/* Currently selected option. */
	int32 CurrentOption = 0;
	/* Array of options. */
	TArray<TSharedPtr<FString> > OptionsSource;
	/* Delegate that will be called when selection had changed. */
	FOnSelectionChanged OnSelectionChanged;
};

/**
 * Widget to store and select enabled widgets by radio buttons.
 */
class SRadioContentSelection : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, int32);

	/**
	 * Radio content selection item.
	 */
	class FItem : public TSharedFromThis<FItem>
	{
	public:
		/**
		 * Constructor
		 *
		 * @param Parent Parent SRadioContentSelection widget reference.
		 * @param Id Id of the item.
		 * @param Name Name of the item.
		 * @param Content Content widget to store by this item.
		 */
		FItem(SRadioContentSelection& Parent, int32 Id, const FString& Name, TSharedRef<SWidget> Content)
			: Parent(Parent), Id(Id), Name(Name), Content(Content)
		{

		}

		/**
		 * Item initialization method.
		 */
		void Init()
		{
			Disable();

			WholeItemWidget = SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SCheckBox)
					.OnCheckStateChanged(this, &FItem::OnCheckStateChange)
					.IsChecked(this, &FItem::IsCheckboxChecked)
					[
						SNew(STextBlock)
						.Text(FText::FromString(GetName()))
					]
				]
			+ SVerticalBox::Slot().VAlign(VAlign_Fill)
				[
					GetContent()
				];
		}

		/**
		 * Gets this item name.
		 *
		 * @returns Item name.
		 */
		const FString& GetName() const
		{
			return Name;
		}
		
		/**
		 * Gets this item content widget.
		 *
		 * @returns Content widget.
		 */
		TSharedRef<SWidget> GetContent()
		{
			return Content;
		}
		
		/**
		 * Gets widget that represents this item.
		 *
		 * @returns Widget that represents this item.
		 */
		TSharedRef<SWidget> GetWidget()
		{
			return WholeItemWidget.ToSharedRef();
		}

		/**
		 * Enables this item.
		 */
		void Enable()
		{
			GetContent()->SetEnabled(true);
		}

		/**
		 * Disables this item.
		 */
		void Disable()
		{
			GetContent()->SetEnabled(false);
		}

		/**
		 * Tells if check box needs to be in checked state.
		 *
		 * @returns State of the check box enum.
		 */
		ESlateCheckBoxState::Type IsCheckboxChecked() const
		{
			return Parent.GetChosen() == Id
				? ESlateCheckBoxState::Checked
				: ESlateCheckBoxState::Unchecked;
		}

	private:
		/**
		 * Function that is called when check box state is changed.
		 *
		 * @parent InNewState New state enum.
		 */
		void OnCheckStateChange(ESlateCheckBoxState::Type InNewState)
		{
			Parent.ChooseEnabledItem(Id);
		}

		/* Item id. */
		int32 Id;
		/* Item name. */
		FString Name;

		/* Outer item widget. */
		TSharedPtr<SWidget> WholeItemWidget;
		/* Stored widget. */
		TSharedRef<SWidget> Content;

		/* Parent reference. */
		SRadioContentSelection& Parent;
	};

	SLATE_BEGIN_ARGS(SRadioContentSelection){}
		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)
	SLATE_END_ARGS()

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct(const FArguments& InArgs)
	{
			OnSelectionChanged = InArgs._OnSelectionChanged;

			MainBox = SNew(SVerticalBox);

			this->ChildSlot
				[
					MainBox.ToSharedRef()
				];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	/**
	 * Adds widget to this selection with given name.
	 *
	 * @param Name Name of the widget in this selection.
	 * @param Content The widget itself.
	 */
	void Add(const FString& Name, TSharedRef<SWidget> Content)
	{
		TSharedRef<FItem> Item = MakeShareable(new FItem(*this, Items.Num(), Name, Content));
		Item->Init();
		Items.Add(Item);
		RebuildMainBox();
	}

	/**
	 * Enables chosen item and disable others.
	 *
	 * @param ItemId Chosen item id.
	 */
	void ChooseEnabledItem(int32 ItemId)
	{
		Chosen = ItemId;

		for (int32 CurrentItemId = 0; CurrentItemId < Items.Num(); ++CurrentItemId)
		{
			if (CurrentItemId == ItemId)
			{
				Items[CurrentItemId]->Enable();
			}
			else
			{
				Items[CurrentItemId]->Disable();
			}
		}

		OnSelectionChanged.ExecuteIfBound(Chosen);
	}

	/**
	 * Gets currently chosen item from this widget.
	 *
	 * @returns Chosen item id.
	 */
	int32 GetChosen() const
	{
		return Chosen;
	}

	/**
	 * Gets widget based on item id.
	 *
	 * @param ItemId Widget item id.
	 *
	 * @returns Reference to requested widget.
	 */
	TSharedRef<SWidget> GetContentWidget(int32 ItemId)
	{
		return Items[ItemId]->GetContent();
	}

private:
	/**
	 * Function to rebuild main and fill it with items provided.
	 */
	void RebuildMainBox()
	{
		MainBox->ClearChildren();

		for (auto Item : Items)
		{
			MainBox->AddSlot().AutoHeight().Padding(FMargin(0, 20, 0, 0))
				[
					Item->GetWidget()
				];
		}

		if (Items.Num() > 0)
		{
			ChooseEnabledItem(0);
		}
	}

	/* Delegate that will be called when the selection changed. */
	FOnSelectionChanged OnSelectionChanged;

	/* Main box ptr. */
	TSharedPtr<SVerticalBox> MainBox;
	/* Array of items/widgets. */
	TArray<TSharedRef<FItem> > Items;
	/* Chosen widget. */
	int32 Chosen;
};

/**
 * Interface of sync command line provider widget.
 */
class ISyncCommandLineProvider
{
public:
	/**
	 * Gets command line from current options picked by user.
	 *
	 * @returns UAT UnrealSync command line.
	 */
	virtual FString GetCommandLine() const = 0;

	/**
	 * Refresh widget method.
	 *
	 * @param GameName Game name to refresh for.
	 */
	virtual void RefreshWidget(const FString& GameName) = 0;

	/**
	 * Tells if this particular widget has data ready for sync.
	 *
	 * @returns True if ready. False otherwise.
	 */
	virtual bool IsReadyForSync() const = 0;
};

/**
 * Sync latest promoted widget.
 */
class SLatestPromoted : public SCompoundWidget, public ISyncCommandLineProvider
{
public:
	SLATE_BEGIN_ARGS(SLatestPromoted) {}
	SLATE_END_ARGS()

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct(const FArguments& InArgs)
	{
		this->ChildSlot
			[
				SNew(STextBlock).Text(FText::FromString("This option will sync to the latest promoted label for given game."))
			];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	/**
	 * Gets command line from current options picked by user.
	 *
	 * @returns UAT UnrealSync command line.
	 */
	FString GetCommandLine() const OVERRIDE
	{
		return !CurrentGameName.IsEmpty()
			? ("game=" + CurrentGameName)
			: "";
	}

	/**
	 * Refresh widget method.
	 *
	 * @param GameName Game name to refresh for.
	 */
	void RefreshWidget(const FString& GameName) OVERRIDE
	{
		CurrentGameName = GameName;
	}

	/**
	 * Tells if this particular widget has data ready for sync.
	 *
	 * @returns True if ready. False otherwise.
	 */
	bool IsReadyForSync() const OVERRIDE
	{
		return true;
	}

private:
	/* Currently picked game name. */
	FString CurrentGameName;
};

/**
 * Base class for pick label widgets.
 */
class SPickLabel : public SCompoundWidget, public ISyncCommandLineProvider
{
public:
	SLATE_BEGIN_ARGS(SPickLabel) {}
		SLATE_ATTRIBUTE(FString, Title)
	SLATE_END_ARGS()

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct(const FArguments& InArgs)
	{
		LabelsCombo = SNew(SSimpleTextComboBox);
		LabelsCombo->Add("Needs refreshing");

		this->ChildSlot
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(InArgs._Title)
				]
				+ SHorizontalBox::Slot().HAlign(HAlign_Fill)
					[
						LabelsCombo.ToSharedRef()
					]
			];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	/**
	 * Gets command line from current options picked by user.
	 *
	 * @returns UAT UnrealSync command line.
	 */
	FString GetCommandLine() const OVERRIDE
	{
		return "-label=" + LabelsCombo->GetCurrentOption();
	}

	/**
	 * Tells if this particular widget has data ready for sync.
	 *
	 * @returns True if ready. False otherwise.
	 */
	bool IsReadyForSync() const OVERRIDE
	{
		return FUnrealSync::HasValidData() && !LabelsCombo->IsEmpty();
	}

protected:
	/**
	 * Reset current labels combo options.
	 *
	 * @param LabelOptions Option to fill labels combo.
	 */
	void SetLabelOptions(const TArray<FString> &LabelOptions)
	{
		LabelsCombo->Clear();

		if (FUnrealSync::HasValidData())
		{
			for (auto LabelName : LabelOptions)
			{
				LabelsCombo->Add(LabelName);
			}

			LabelsCombo->SetEnabled(true);
		}
		else
		{
			LabelsCombo->Add(!FUnrealSync::LoadingFinished() ? "Loading P4 labels data..." : "P4 data loading failed.");
			LabelsCombo->SetEnabled(false);
		}
	}

private:
	/* Labels combo widget. */
	TSharedPtr<SSimpleTextComboBox> LabelsCombo;
};

/**
 * Pick promoted label widget.
 */
class SPickPromoted : public SPickLabel
{
public:
	/**
	 * Refresh widget method.
	 *
	 * @param GameName Game name to refresh for.
	 */
	void RefreshWidget(const FString& GameName) OVERRIDE
	{
		SetLabelOptions(*FUnrealSync::GetPromotedLabelsForGame(GameName));
	}
};

/**
 * Pick promotable label widget.
 */
class SPickPromotable : public SPickLabel
{
public:
	/**
	 * Refresh widget method.
	 *
	 * @param GameName Game name to refresh for.
	 */
	void RefreshWidget(const FString& GameName) OVERRIDE
	{
		SetLabelOptions(*FUnrealSync::GetPromotableLabelsForGame(GameName));
	}
};

/**
 * Pick any label widget.
 */
class SPickAny : public SPickLabel
{
public:
	/**
	 * Refresh widget method.
	 *
	 * @param GameName Game name to refresh for.
	 */
	void RefreshWidget(const FString& GameName) OVERRIDE
	{
		SetLabelOptions(*FUnrealSync::GetAllLabels());
	}
};

/**
 * Pick game widget.
 */
class SPickGameWidget : public SCompoundWidget
{
public:
	/* Delegate for game picked event. */
	DECLARE_DELEGATE_OneParam(FOnGamePicked, const FString&);

	SLATE_BEGIN_ARGS(SPickGameWidget) {}
		SLATE_EVENT(FOnGamePicked, OnGamePicked)
	SLATE_END_ARGS()

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct(const FArguments& InArgs)
	{
		OnGamePicked = InArgs._OnGamePicked;

		GamesOptions = SNew(SSimpleTextComboBox)
			.OnSelectionChanged(this, &SPickGameWidget::OnComboBoxSelectionChanged);

		auto PossibleGameNames = FUnrealSync::GetPossibleGameNames();

		GamesOptions->Add(FUnrealSync::GetSharedPromotableDisplayName());

		for (auto PossibleGameName : *PossibleGameNames)
		{
			GamesOptions->Add(PossibleGameName);
		}

		this->ChildSlot
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString("Pick game to sync: "))
				]
				+ SHorizontalBox::Slot().HAlign(HAlign_Fill)
					[
						GamesOptions.ToSharedRef()
					]
			];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	/**
	 * Gets current game selected.
	 *
	 * @returns Currently selected game.
	 */
	const FString& GetCurrentGame()
	{
		return GamesOptions->GetCurrentOption();
	}

private:
	/**
	 * Function is called when combo box selection is changed.
	 * It calles OnGamePicked event.
	 */
	void OnComboBoxSelectionChanged(int32 SelectionId, ESelectInfo::Type SelectionInfo)
	{
		OnGamePicked.ExecuteIfBound(GamesOptions->GetCurrentOption());
	}

	/* Game options widget. */
	TSharedPtr<SSimpleTextComboBox> GamesOptions;
	/* Delegate that is going to be called when game was picked by the user. */
	FOnGamePicked OnGamePicked;
};

/**
 * Main tab widget.
 */
class SMainTabWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMainTabWidget){}
	SLATE_END_ARGS()

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct(const FArguments& InArgs)
	{
		RadioSelection = SNew(SRadioContentSelection);

		AddToRadioSelection("Sync to the latest promoted", SNew(SLatestPromoted));
		AddToRadioSelection("Sync to chosen promoted label", SNew(SPickPromoted).Title("Pick promoted label: "));
		AddToRadioSelection("Sync to chosen promotable label since last promoted", SNew(SPickPromotable).Title("Pick promotable label: "));
		AddToRadioSelection("Sync to any chosen label", SNew(SPickAny).Title("Pick label: "));

		PickGameWidget = SNew(SPickGameWidget)
			.OnGamePicked(this, &SMainTabWidget::OnCurrentGameChanged);

		ArtistSyncCheckBox = SNew(SCheckBox)
			[
				SNew(STextBlock).Text(FText::FromString("Artist sync?"))
			];

		// Checked by default.
		ArtistSyncCheckBox->ToggleCheckedState();

		PreviewSyncCheckBox = SNew(SCheckBox)
			[
				SNew(STextBlock).Text(FText::FromString("Preview sync?"))
			];

		GoBackButton = SNew(SButton)
			.IsEnabled(false)
			.OnClicked(this, &SMainTabWidget::OnGoBackButtonClick)
			[
				SNew(STextBlock).Text(FText::FromString("Go back"))
			];

		LogListView = SNew(SListView<TSharedPtr<FString> >)
			.ListItemsSource(&LogLines)
			.OnGenerateRow(this, &SMainTabWidget::GenerateLogItem);
		
		Switcher = SNew(SWidgetSwitcher)
			+ SWidgetSwitcher::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					PickGameWidget.ToSharedRef()
				]
				+ SVerticalBox::Slot().VAlign(VAlign_Fill)
				[
					RadioSelection.ToSharedRef()
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton).HAlign(HAlign_Right)
						.OnClicked(this, &SMainTabWidget::OnReloadLabels)
						.IsEnabled(this, &SMainTabWidget::IsReloadLabelsReady)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString("Reload Labels"))
						]
					]
					+ SHorizontalBox::Slot().HAlign(HAlign_Fill)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						[
							ArtistSyncCheckBox.ToSharedRef()
						]
						+ SVerticalBox::Slot()
							[
								PreviewSyncCheckBox.ToSharedRef()
							]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(30, 0, 0, 0))
						[
							SNew(SButton).HAlign(HAlign_Right)
							.OnClicked(this, &SMainTabWidget::OnSync)
							.IsEnabled(this, &SMainTabWidget::IsSyncReady)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text(FText::FromString("Sync!"))
							]
						]
				]
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					LogListView->AsWidget()
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					+ SHorizontalBox::Slot().AutoWidth()
					[
						GoBackButton.ToSharedRef()
					]
				]
			];
		
		this->ChildSlot
			[
				Switcher.ToSharedRef()
			];

		FUnrealSync::RegisterOnDataRefresh(FUnrealSync::FOnDataRefresh::CreateRaw(this, &SMainTabWidget::DataRefresh));

		OnReloadLabels();
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

private:
	/**
	 * Generates list view item from FString.
	 *
	 * @param Value Value to convert.
	 * @param OwnerTable List view that will contain this item.
	 *
	 * @returns Converted STableRow object.
	 */
	TSharedRef<ITableRow> GenerateLogItem(TSharedPtr<FString> Value, const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(STableRow<TSharedPtr<FString> >, OwnerTable)
			[
				SNew(STextBlock).Text(FText::FromString(*Value))
			];
	}

	/**
	 * Gets current sync command line provider.
	 *
	 * @returns Current sync cmd line provider.
	 */
	ISyncCommandLineProvider& GetCurrentSyncCmdLineProvider()
	{
		return *SyncCommandLineProviders[RadioSelection->GetChosen()];
	}

	/**
	 * Gets current sync command line provider.
	 *
	 * @returns Current sync cmd line provider.
	 */
	const ISyncCommandLineProvider& GetCurrentSyncCmdLineProvider() const
	{
		return *SyncCommandLineProviders[RadioSelection->GetChosen()];
	}

	/**
	 * Function that is called on when go back button is clicked.
	 * It tells the app to go back to the beginning.
	 *
	 * @returns Tells that this event was handled.
	 */
	FReply OnGoBackButtonClick()
	{
		Switcher->SetActiveWidgetIndex(0);
		LogLines.Empty();
		LogListView->RequestListRefresh();
		OnReloadLabels();

		return FReply::Handled();
	}

	/**
	 * Tells Sync button if it should be enabled. It depends on the combination
	 * of user choices and if P4 data is currently ready.
	 *
	 * @returns True if Sync button should be enabled. False otherwise.
	 */
	bool IsSyncReady() const
	{
		return GetCurrentSyncCmdLineProvider().IsReadyForSync();
	}

	/**
	 * Tells reload labels button if it should be enabled.
	 *
	 * @returns True if reload labels button should be enabled. False otherwise.
	 */
	bool IsReloadLabelsReady() const
	{
		return !FUnrealSync::IsLoadingInProgress();
	}

	/**
	 * Launches reload labels background process.
	 *
	 * @returns Tells that this event was handled.
	 */
	FReply OnReloadLabels()
	{
		FUnrealSync::StartLoadingData();

		return FReply::Handled();
	}

	/**
	 * Sync button click function. Gets all widget data, constructs command
	 * line and launches syncing.
	 *
	 * @returns Tells that this event was handled.
	 */
	FReply OnSync()
	{
		auto bArtist = ArtistSyncCheckBox->IsChecked();
		auto bPreview = PreviewSyncCheckBox->IsChecked();
		auto CommandLine = GetCurrentSyncCmdLineProvider().GetCommandLine();

		if (!FUnrealSync::LoadingFinished())
		{
			FUnrealSync::TerminateLoadingProcess();
		}

		Switcher->SetActiveWidgetIndex(1);
		FUnrealSync::LaunchSync(bArtist, bPreview, CommandLine,
			FUnrealSync::FOnSyncFinished::CreateRaw(this, &SMainTabWidget::SyncingFinished),
			FUnrealSync::FOnSyncProgress::CreateRaw(this, &SMainTabWidget::SyncingProgress));

		return FReply::Handled();
	}

	/**
	 * Function that is called whenever monitoring thread will update its log.
	 *
	 * @param Log Log of the operation up to this moment.
	 *
	 * @returns True if process should continue, false otherwise.
	 */
	bool SyncingProgress(const FString& Log)
	{
		if (Log.IsEmpty())
		{
			return true;
		}

		static FString Buffer;

		Buffer += Log.Replace(TEXT("\r"), TEXT(""));

		FString Line;
		FString Rest;

		while (Buffer.Split("\n", &Line, &Rest))
		{
			LogLines.Add(MakeShareable(new FString(Line)));

			Buffer = Rest;
		}

		LogListView->RequestListRefresh();
		LogListView->RequestScrollIntoView(LogLines.Last());

		return true;
	}

	/**
	 * Function that is called when P4 syncing is finished.
	 *
	 * @param bSuccess True if syncing finished. Otherwise false.
	 */
	void SyncingFinished(bool bSuccess)
	{
		GoBackButton->SetEnabled(true);
	}

	/**
	 * Function to call when current game changed. Is refreshing
	 * label lists in the widget.
	 *
	 * @param CurrentGameName Game name choosen.
	 */
	void OnCurrentGameChanged(const FString& CurrentGameName)
	{
		for (auto SyncCommandLineProvider : SyncCommandLineProviders)
		{
			SyncCommandLineProvider->RefreshWidget(CurrentGameName);
		}
	}

	/**
	 * Function to call when P4 cached data is refreshed.
	 * This function is refreshing whole widget data.
	 */
	void DataRefresh()
	{
		const FString& GameName = PickGameWidget->GetCurrentGame();
		OnCurrentGameChanged(
			GameName.Equals(FUnrealSync::GetSharedPromotableDisplayName())
			? ""
			: GameName);
	}

	/**
	 * Method to add command line provider/widget to correct arrays.
	 *
	 * @param Name Name of the radio selection widget item.
	 * @param Widget Widget to add.
	 */
	template <typename TWidgetType>
	void AddToRadioSelection(const FString& Name, TSharedRef<TWidgetType> Widget)
	{
		RadioSelection->Add(Name, Widget);
		SyncCommandLineProviders.Add(Widget);
	}

	/* Main widget switcher. */
	TSharedPtr<SWidgetSwitcher> Switcher;

	/* Radio selection used to chose method to sync. */
	TSharedPtr<SRadioContentSelection> RadioSelection;

	/* Widget to pick game used to sync. */
	TSharedPtr<SPickGameWidget> PickGameWidget;
	/* Check box to tell if this should be an artist sync. */
	TSharedPtr<SCheckBox> ArtistSyncCheckBox;
	/* Check box to tell if this should be a preview sync. */
	TSharedPtr<SCheckBox> PreviewSyncCheckBox;

	/* Report log. */
	TArray<TSharedPtr<FString> > LogLines;
	/* Log list view. */
	TSharedPtr<SListView<TSharedPtr<FString> > > LogListView;
	/* Go back button reference. */
	TSharedPtr<SButton> GoBackButton;

	/* Array of sync command line providers. */
	TArray<TSharedRef<ISyncCommandLineProvider> > SyncCommandLineProviders;
};

/**
 * Creates and returns main tab.
 *
 * @param Args Args used to spawn tab.
 *
 * @returns Main UnrealSync tab.
 */
TSharedRef<SDockTab> GetMainTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> MainTab =
		SNew(SDockTab)
		.TabRole(ETabRole::MajorTab)
		.Label(FText::FromString("UnrealSync"))
		.ToolTipText(FText::FromString("Sync Unreal Engine tool."));

	MainTab->SetContent(
		SNew(SMainTabWidget)
		);

	return MainTab;
}

void InitGUI()
{
	// start up the main loop
	GEngineLoop.PreInit(GetCommandLineW());

	// crank up a normal Slate application using the platform's standalone renderer
	FSlateApplication::InitializeAsStandaloneApplication(GetStandardStandaloneRenderer());

	// set the application name
	FGlobalTabmanager::Get()->SetApplicationTitle(NSLOCTEXT("UnrealSync", "AppTitle", "Unreal Sync"));

	FGlobalTabmanager::Get()->RegisterTabSpawner("MainTab", FOnSpawnTab::CreateStatic(&GetMainTab));

	TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("UnrealSyncLayout")
		->AddArea(
			FTabManager::NewArea(720, 370)
			->SetWindow(FVector2D(420, 10), false)
			->Split(
				FTabManager::NewStack()
				->AddTab("MainTab", ETabState::OpenedTab)
			)
		);

	FGlobalTabmanager::Get()->RestoreFrom(Layout, TSharedPtr<SWindow>());

	// loop while the server does the rest
	while (!GIsRequestingExit)
	{
		FSlateApplication::Get().PumpMessages();
		FSlateApplication::Get().Tick();
		FPlatformProcess::Sleep(0);
	}

	FSlateApplication::Shutdown();
}