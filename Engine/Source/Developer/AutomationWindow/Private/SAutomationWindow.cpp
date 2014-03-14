// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AutomationWindowPrivatePCH.h"

#include "AutomationController.h"
#include "SAutomationWindow.h"

#define LOCTEXT_NAMESPACE "AutomationTest"

//////////////////////////////////////////////////////////////////////////
// FAutomationWindowCommands

class FAutomationWindowCommands : public TCommands<FAutomationWindowCommands>
{
public:
	FAutomationWindowCommands()
		: TCommands<FAutomationWindowCommands>(
		TEXT("AutomationWindow"),
		NSLOCTEXT("Contexts", "AutomationWindow", "Automation Window"),
		NAME_None, FEditorStyle::GetStyleSetName()
		)
	{
	}

	virtual void RegisterCommands() OVERRIDE
	{
		UI_COMMAND( RefreshTests, "Refresh Tests", "Refresh Tests", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( FindWorkers, "Find Workers", "Find Workers", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( ErrorFilter, "Errors", "Toggle Error Filter", EUserInterfaceActionType::ToggleButton, FInputGesture() );
		UI_COMMAND( WarningFilter, "Warnings", "Toggle Warning Filter", EUserInterfaceActionType::ToggleButton, FInputGesture() );
		UI_COMMAND( SmokeTestFilter, "Smoke Tests", "Toggle Smoke Test Filter", EUserInterfaceActionType::ToggleButton, FInputGesture() );
		UI_COMMAND( DeveloperDirectoryContent, "Dev Content", "Developer Directory Content Filter (when enabled, developer directories are also included)", EUserInterfaceActionType::ToggleButton, FInputGesture() );
		UI_COMMAND( VisualCommandlet, "Vis Cmdlet", "Visual Commandlet Filter", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	}
public:
	TSharedPtr<FUICommandInfo> RefreshTests;
	TSharedPtr<FUICommandInfo> FindWorkers;
	TSharedPtr<FUICommandInfo> ErrorFilter;
	TSharedPtr<FUICommandInfo> WarningFilter;
	TSharedPtr<FUICommandInfo> SmokeTestFilter;
	TSharedPtr<FUICommandInfo> DeveloperDirectoryContent;
	TSharedPtr<FUICommandInfo> VisualCommandlet;
};

//////////////////////////////////////////////////////////////////////////
// SAutomationWindow

SAutomationWindow::SAutomationWindow() 
	: ColumnWidth(50.0f)
{

}

SAutomationWindow::~SAutomationWindow()
{
	// @todo PeterMcW: is there an actual delegate missing here?
	//give the controller a way to indicate it requires a UI update
	//AutomationController->SetRefreshTestCallback(FOnAutomationControllerTestsRefreshed());

	// Remove ourselves from the session manager
	if( SessionManager.IsValid( ) )
	{
		SessionManager->OnCanSelectSession().RemoveAll(this);
		SessionManager->OnSelectedSessionChanged().RemoveAll(this);
	}

	if (AutomationController.IsValid())
	{
		AutomationController->RemoveCallbacks();
	}
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SAutomationWindow::Construct( const FArguments& InArgs, const IAutomationControllerManagerRef& InAutomationController, const ISessionManagerRef& InSessionManager )
{
	FAutomationWindowCommands::Register();
	CreateCommands();

	SessionManager = InSessionManager;
	AutomationController = InAutomationController;

	AutomationController->OnTestsRefreshed().BindRaw(this, &SAutomationWindow::OnRefreshTestCallback);
	AutomationController->OnTestsAvailable().BindRaw(this, &SAutomationWindow::OnTestAvailableCallback);

	AutomationControllerState = AutomationController->GetTestState();
	
	//cache off reference to filtered reports
	TArray <TSharedPtr <IAutomationReport> >& TestReports = AutomationController->GetReports();

	// Create the search filter and set criteria
	AutomationTextFilter = MakeShareable( new AutomationReportTextFilter( AutomationReportTextFilter::FItemToStringArray::CreateSP( this, &SAutomationWindow::PopulateReportSearchStrings ) ) );
	AutomationGeneralFilter = MakeShareable( new FAutomationFilter() ); 
	AutomationFilters = MakeShareable( new AutomationFilterCollection() );
	AutomationFilters->Add( AutomationTextFilter );
	AutomationFilters->Add( AutomationGeneralFilter );

	bIsRequestingTests = false;

	//make the widget for platforms
	PlatformsHBox = SNew (SHorizontalBox);

	TestTable = SNew(STreeView< TSharedPtr< IAutomationReport > >)
		.SelectionMode(ESelectionMode::Multi)
		.TreeItemsSource( &TestReports )
		// Generates the actual widget for a tree item
		.OnGenerateRow( this, &SAutomationWindow::OnGenerateWidgetForTest )
		// Gets children
		.OnGetChildren(this, &SAutomationWindow::OnGetChildren)
		//on selection
		.OnSelectionChanged(this, &SAutomationWindow::OnTestSelectionChanged)
		// Allow for some spacing between items with a larger item height.
		.ItemHeight(20.0f)
#if WITH_EDITOR
		// If in editor - add a context menu for opening assets when in editor
		.OnContextMenuOpening(this, &SAutomationWindow::HandleAutomationListContextMenuOpening)
#endif
		.HeaderRow
		(
		SNew(SHeaderRow)
		+ SHeaderRow::Column( AutomationTestWindowConstants::Title )
		.FillWidth(300.0f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			[
				//global enable/disable check box
				SAssignNew(HeaderCheckbox, SCheckBox)
				.OnCheckStateChanged( this, &SAutomationWindow::HeaderCheckboxStateChange)
				.ToolTipText( LOCTEXT( "Enable Disable Test", "Enable / Disable  Test" ) )
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew( STextBlock )
				.Text( LOCTEXT("TestName_Header", "Test Name") )
			]
		]
		+ SHeaderRow::Column( AutomationTestWindowConstants::SmokeTest )
		.HAlignHeader(HAlign_Center)
		.VAlignHeader(VAlign_Center)
		.HAlignCell(HAlign_Center)
		.VAlignCell(VAlign_Center)
		.FixedWidth( 50.0f )
		[
			//icon for the smoke test column
			SNew(SImage)
			.ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.4f))
			.ToolTipText( LOCTEXT( "Smoke Test", "Smoke Test" ) )
			.Image(FEditorStyle::GetBrush("Automation.SmokeTest"))
		]
		+ SHeaderRow::Column( AutomationTestWindowConstants::RequiredDeviceCount )
			.HAlignHeader(HAlign_Center)
			.VAlignHeader(VAlign_Center)
			.HAlignCell(HAlign_Center)
			.VAlignCell(VAlign_Center)
			.FixedWidth( 50.0f )
			[
				SNew( SImage )
				.Image( FEditorStyle::GetBrush("Automation.ParticipantsWarning") )
				.ToolTipText( LOCTEXT( "RequiredDeviceCountWarningToolTip", "Number of devices required." ) )
			]
		+ SHeaderRow::Column( AutomationTestWindowConstants::Status )
		.FillWidth(650.0f)
		[
			//platform header placeholder
			PlatformsHBox.ToSharedRef()
		]
		+ SHeaderRow::Column( AutomationTestWindowConstants::Timing )
		.FillWidth(150.0f)
		.DefaultLabel( LOCTEXT("TestDurationRange", "Duration") )

		);

	TSharedRef<SNotificationList> NotificationList = SNew(SNotificationList) .Visibility( EVisibility::HitTestInvisible );

	//build the actual guts of the window
	this->ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew( SSplitter )
			.Orientation(Orient_Vertical)
			+SSplitter::Slot()
			.Value(0.66f)
			[
				//automation test panel
				SAssignNew( MenuBar, SVerticalBox )
				//ACTIONS
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( SHorizontalBox )
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SAutomationWindow::MakeAutomationWindowToolBar( AutomationWindowActions.ToSharedRef(), SharedThis(this) )
					]
				]
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SOverlay)
					+SOverlay::Slot()
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.Padding(0.0f)
						[
							//the actual table full of tests
							TestTable.ToSharedRef()
						]
					]
					+SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SThrobber)	
							.Visibility(this, &SAutomationWindow::GetTestsUpdatingThrobberVisibility)
					]
				]
			]
			+SSplitter::Slot()
			.Value(0.33f)
			[
				//results panel
				SNew( SVerticalBox )
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( STextBlock )
						.Text( LOCTEXT("AutomationTest_Results", "Automation Test Results:") )
				]
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					//list of results for the selected test
					SNew(SBorder)
					[
						SAssignNew(LogListView, SListView<TSharedPtr<FAutomationOutputMessage> >)
						.ItemHeight(18)
						.ListItemsSource(&LogMessages)
						.SelectionMode(ESelectionMode::Multi)
						.OnGenerateRow(this, &SAutomationWindow::OnGenerateWidgetForLog)
						.OnSelectionChanged(this, &SAutomationWindow::HandleLogListSelectionChanged)
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.Padding(FMargin(8.0f, 6.0f))
						[
							// Add the command bar
							SAssignNew(CommandBar, SAutomationWindowCommandBar, NotificationList)
							.OnCopyLogClicked(this, &SAutomationWindow::HandleCommandBarCopyLogClicked)
						]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign( HAlign_Center )
		.VAlign( VAlign_Center )
		.Padding( 15.0f )
		[
			NotificationList
		]
		+ SOverlay::Slot()
		.HAlign( HAlign_Center )
		.VAlign( VAlign_Center )
		.Padding( 15.0f )
		[
			SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("NotificationList.ItemBackground"))
				.Padding(8.0f)
				.Visibility(this, &SAutomationWindow::HandleSelectSessionOverlayVisibility)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("SelectSessionOverlayText", "Please select a session from the Session Browser"))
				]
		]
	];

	SessionManager->OnCanSelectSession().AddSP( this, &SAutomationWindow::HandleSessionManagerCanSelectSession );
	SessionManager->OnSelectedSessionChanged().AddSP( this, &SAutomationWindow::HandleSessionManagerSelectionChanged );

	FindWorkers();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAutomationWindow::CreateCommands()
{
	check(!AutomationWindowActions.IsValid());
	AutomationWindowActions = MakeShareable(new FUICommandList);

	const FAutomationWindowCommands& Commands = FAutomationWindowCommands::Get();
	FUICommandList& ActionList = *AutomationWindowActions;

	ActionList.MapAction( Commands.RefreshTests,
		FExecuteAction::CreateRaw( this, &SAutomationWindow::ListTests ),
		FCanExecuteAction::CreateRaw( this, &SAutomationWindow::IsNotRunningTests )
		);

	ActionList.MapAction( Commands.FindWorkers,
		FExecuteAction::CreateRaw( this, &SAutomationWindow::FindWorkers ),
		FCanExecuteAction::CreateRaw( this, &SAutomationWindow::IsNotRunningTests )
		);

	ActionList.MapAction( Commands.ErrorFilter,
		FExecuteAction::CreateRaw( this, &SAutomationWindow::OnToggleErrorFilter ),
		FCanExecuteAction::CreateRaw( this, &SAutomationWindow::IsNotRunningTests ),
		FIsActionChecked::CreateRaw( this, &SAutomationWindow::IsErrorFilterOn )
		);

	ActionList.MapAction( Commands.WarningFilter,
		FExecuteAction::CreateRaw( this, &SAutomationWindow::OnToggleWarningFilter ),
		FCanExecuteAction::CreateRaw( this, &SAutomationWindow::IsNotRunningTests ),
		FIsActionChecked::CreateRaw( this, &SAutomationWindow::IsWarningFilterOn )
		);

	ActionList.MapAction( Commands.SmokeTestFilter,
		FExecuteAction::CreateRaw( this, &SAutomationWindow::OnToggleSmokeTestFilter ),
		FCanExecuteAction::CreateRaw( this, &SAutomationWindow::IsNotRunningTests ),
		FIsActionChecked::CreateRaw( this, &SAutomationWindow::IsSmokeTestFilterOn )
		);

	ActionList.MapAction( Commands.DeveloperDirectoryContent,
		FExecuteAction::CreateRaw( this, &SAutomationWindow::OnToggleDeveloperDirectoryIncluded ),
		FCanExecuteAction::CreateRaw( this, &SAutomationWindow::IsNotRunningTests ),
		FIsActionChecked::CreateRaw( this, &SAutomationWindow::IsDeveloperDirectoryIncluded )
		);

	ActionList.MapAction( Commands.VisualCommandlet,
		FExecuteAction::CreateRaw( this, &SAutomationWindow::OnToggleVisualCommandletFilter ),
		FCanExecuteAction::CreateRaw( this, &SAutomationWindow::IsNotRunningTests ),
		FIsActionChecked::CreateRaw( this, &SAutomationWindow::IsVisualCommandletFilterOn )
		);
}

TSharedRef< SWidget > SAutomationWindow::MakeAutomationWindowToolBar( const TSharedRef<FUICommandList>& InCommandList, TSharedPtr<class SAutomationWindow> InAutomationWindow )
{
	return InAutomationWindow->MakeAutomationWindowToolBar(InCommandList);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedRef< SWidget > SAutomationWindow::MakeAutomationWindowToolBar( const TSharedRef<FUICommandList>& InCommandList )
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, TSharedRef<SWidget> RunTests, TSharedRef<SWidget> Searchbox)
		{
			ToolbarBuilder.BeginSection("Automation");
			{
				ToolbarBuilder.AddWidget( RunTests );
				ToolbarBuilder.AddToolBarButton( FAutomationWindowCommands::Get().RefreshTests );
				ToolbarBuilder.AddToolBarButton( FAutomationWindowCommands::Get().FindWorkers );
			}
			ToolbarBuilder.EndSection();
			ToolbarBuilder.BeginSection("Filters");
			{
				ToolbarBuilder.AddToolBarButton( FAutomationWindowCommands::Get().ErrorFilter );
				ToolbarBuilder.AddToolBarButton( FAutomationWindowCommands::Get().WarningFilter );
				ToolbarBuilder.AddToolBarButton( FAutomationWindowCommands::Get().SmokeTestFilter );
				ToolbarBuilder.AddToolBarButton( FAutomationWindowCommands::Get().DeveloperDirectoryContent );
				ToolbarBuilder.AddToolBarButton( FAutomationWindowCommands::Get().VisualCommandlet );
			}
			ToolbarBuilder.EndSection();
			ToolbarBuilder.BeginSection("Search");
			{
				ToolbarBuilder.AddWidget( Searchbox );
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedRef<SWidget> RunTests = 
		SNew( SButton )
		.ButtonStyle( FEditorStyle::Get(), "ToggleButton" )
		.ToolTipText( LOCTEXT( "StartStop Tests", "Start / Stop tests" ) )
		.OnClicked( this, &SAutomationWindow::RunTests )
		.IsEnabled( this, &SAutomationWindow::IsAutomationRunButtonEnabled )
		.ContentPadding(0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.VAlign(VAlign_Center)
			[
				SNew( SVerticalBox )
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign( HAlign_Center )
				[
					SNew( SOverlay )
					+SOverlay::Slot()
					[
						SNew( SImage ) 
						.Image( this, &SAutomationWindow::GetRunAutomationIcon )
					]
					+SOverlay::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Bottom)
					[
						SNew( STextBlock )
						.Text( this, &SAutomationWindow::OnGetNumEnabledTestsString )
						.ColorAndOpacity( FLinearColor::White )
						.ShadowOffset( FVector2D::UnitVector )
						.Font( FEditorStyle::GetFontStyle( FName( "ToggleButton.LabelFont" ) ) )
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign( HAlign_Center )
				[
					SNew( STextBlock )
					.Visibility( this, &SAutomationWindow::GetLargeToolBarVisibility )
					.Text( this, &SAutomationWindow::GetRunAutomationLabel )
					.Font( FEditorStyle::GetFontStyle( FName( "ToggleButton.LabelFont" ) ) )
					.ColorAndOpacity(FLinearColor::White)
					.ShadowOffset( FVector2D::UnitVector )
				]
			]
		];

	const float SearchWidth = 200.f;
	TSharedRef<SWidget> Searchbox = 
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.MaxWidth(SearchWidth)
		.FillWidth(1.f)
		.VAlign(VAlign_Bottom)
		[
			SAssignNew( AutomationSearchBox, SSearchBox )
			.ToolTipText( LOCTEXT( "Search Tests", "Search Tests" ) )
			.OnTextChanged( this, &SAutomationWindow::OnFilterTextChanged )
			.IsEnabled( this, &SAutomationWindow::IsNotRunningTests )
			.MinDesiredWidth(SearchWidth)
		];

	FToolBarBuilder ToolbarBuilder( InCommandList, FMultiBoxCustomization::None );
	Local::FillToolbar( ToolbarBuilder, RunTests, Searchbox );

	// Create the tool bar!
	return
		SNew( SHorizontalBox )
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew( SBorder )
			.Padding(0)
			.BorderImage( FEditorStyle::GetBrush("NoBorder") )
			.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
			[
				ToolbarBuilder.MakeWidget()
			]
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

// Only valid in the editor
#if WITH_EDITOR
TSharedPtr<SWidget> SAutomationWindow::HandleAutomationListContextMenuOpening()
{
 	TArray< TSharedPtr<IAutomationReport> >SelectedReport = TestTable->GetSelectedItems();

	if (SelectedReport.Num() > 0 && SelectedReport[0].IsValid())
	{
		if (SelectedReport[0]->GetAssetName().Len() > 0)
		{
			return SNew(SAutomationTestItemContextMenu, SelectedReport[0]->GetAssetName());
		}
	}		

	return NULL;
}
#endif


void SAutomationWindow::PopulateReportSearchStrings( const TSharedPtr< IAutomationReport >& Report, OUT TArray< FString >& OutSearchStrings ) const
{
	if( !Report.IsValid() )
	{
		return;
	}

	OutSearchStrings.Add( Report->GetDisplayName() );
}


void SAutomationWindow::OnGetChildren(TSharedPtr<IAutomationReport> InItem, TArray<TSharedPtr<IAutomationReport> >& OutItems)
{
	OutItems = InItem->GetFilteredChildren();
}


void SAutomationWindow::OnTestSelectionChanged(TSharedPtr<IAutomationReport> Selection, ESelectInfo::Type /*SelectInfo*/)
{
	//empty the previous log
	LogMessages.Empty();

	if (Selection.IsValid())
	{
		//accumulate results for each device cluster that supports the test
		int32 NumClusters = AutomationController->GetNumDeviceClusters();
		for (int32 ClusterIndex = 0; ClusterIndex < NumClusters; ++ClusterIndex)
		{
			//get strings out of the report and populate the Log Messages

			FAutomationTestResults TestResults = Selection->GetResults(ClusterIndex);

			//no sense displaying device name if only one is available
			if (NumClusters > 1)
			{
				FString DeviceTypeName = AutomationController->GetDeviceTypeName(ClusterIndex);
				LogMessages.Add(MakeShareable(new FAutomationOutputMessage(DeviceTypeName, TEXT("Automation.Header"))));
			}

			for (int32 ErrorIndex = 0; ErrorIndex < TestResults.Errors.Num(); ++ErrorIndex)
			{
				LogMessages.Add(MakeShareable(new FAutomationOutputMessage(TestResults.Errors[ErrorIndex], TEXT("Automation.Error"))));
			}
			for (int32 WarningIndex = 0; WarningIndex < TestResults.Warnings.Num(); ++WarningIndex)
			{
				LogMessages.Add(MakeShareable(new FAutomationOutputMessage(TestResults.Warnings[WarningIndex], TEXT("Automation.Warning"))));
			}
			for (int32 LogIndex = 0; LogIndex < TestResults.Logs.Num(); ++LogIndex)
			{
				LogMessages.Add(MakeShareable(new FAutomationOutputMessage(TestResults.Logs[LogIndex], TEXT("Automation.Normal"))));
			}
			if ((TestResults.Warnings.Num() == 0) &&(TestResults.Errors.Num() == 0) && (Selection->GetState(ClusterIndex)==EAutomationState::Success))
			{
				LogMessages.Add(MakeShareable(new FAutomationOutputMessage(LOCTEXT( "AutomationTest_SuccessMessage", "Success" ).ToString(), TEXT("Automation.Normal"))));
			}

			LogMessages.Add(MakeShareable(new FAutomationOutputMessage(TEXT(""), TEXT("Log.Normal"))));
		}
		//rebuild UI
		LogListView->RequestListRefresh();
	}
}


void SAutomationWindow::HeaderCheckboxStateChange(ESlateCheckBoxState::Type InCheckboxState)
{
	const bool bState = (InCheckboxState == ESlateCheckBoxState::Checked)? true : false;

	AutomationController->SetVisibleTestsEnabled(bState);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SAutomationWindow::RebuildPlatformIcons()
{
	//empty header UI
	PlatformsHBox->ClearChildren();

	//for each device type
	int32 NumClusters = AutomationController->GetNumDeviceClusters();
	for (int32 ClusterIndex = 0; ClusterIndex < NumClusters; ++ClusterIndex)
	{
		//find the right platform icon
		FString DeviceImageName = TEXT("Launcher.Platform_");
		FString DeviceTypeName = AutomationController->GetDeviceTypeName(ClusterIndex);
		DeviceImageName += DeviceTypeName;
		const FSlateBrush* ImageToUse = FEditorStyle::GetBrush(*DeviceImageName);

		PlatformsHBox->AddSlot()
		.AutoWidth()
		.MaxWidth(ColumnWidth)
		[
			SNew( SOverlay )
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage( FEditorStyle::GetBrush("ErrorReporting.Box") )
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding( FMargin(3,0) )
				.BorderBackgroundColor( FSlateColor( FLinearColor( 1.0f, 0.0f, 1.0f, 0.0f ) ) )
				.ToolTipText( CreateDeviceTooltip( ClusterIndex ) )
				[
					SNew(SImage)
					.Image(ImageToUse)
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			[
				//Overlay how many devices are in the cluster
				SNew( STextBlock )
				.Text( this, &SAutomationWindow::OnGetNumDevicesInClusterString, ClusterIndex )
			]
		];
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FText SAutomationWindow::CreateDeviceTooltip(int32 ClusterIndex)
{
	//the tool tip
	FString ToolTip = LOCTEXT("ToolTipGameInstances", "Game Instances:").ToString();

	int32 NumDevices = AutomationController->GetNumDevicesInCluster( ClusterIndex );
	for ( int32 DeviceIndex = 0; DeviceIndex < NumDevices; ++DeviceIndex )
	{
		ToolTip += TEXT("\n   ");
		ToolTip += AutomationController->GetGameInstanceName( ClusterIndex, DeviceIndex );
	}

	return FText::FromString( ToolTip );
}


void SAutomationWindow::ClearAutomationUI ()
{
	// Clear results from the automation controller
	AutomationController->ClearAutomationReports();
	TestTable->RequestTreeRefresh();

	// Clear the platform icons
	if (PlatformsHBox.IsValid())
	{
		PlatformsHBox->ClearChildren();
	}
	
	// Clear the log
	LogMessages.Empty();
	LogListView->RequestListRefresh();
}


TSharedRef<ITableRow> SAutomationWindow::OnGenerateWidgetForTest( TSharedPtr<IAutomationReport> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	bIsRequestingTests = false;
	return SNew( SAutomationTestItem, OwnerTable )
		.TestStatus( InItem )
		.ColumnWidth( ColumnWidth )
		.HighlightText(this, &SAutomationWindow::HandleAutomationHighlightText)
		.OnCheckedStateChanged(this, &SAutomationWindow::HandleItemCheckBoxCheckedStateChanged);
}


TSharedRef<ITableRow> SAutomationWindow::OnGenerateWidgetForLog(TSharedPtr<FAutomationOutputMessage> Message, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(Message.IsValid());

	return SNew(STableRow<TSharedPtr<FAutomationOutputMessage> >, OwnerTable)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0)
			[
				SNew(STextBlock)
				.TextStyle( FEditorStyle::Get(), Message->Style )
				.Text(FString::Printf(TEXT("%s"), *Message->Text))
			]
		];
}


FString SAutomationWindow::OnGetNumEnabledTestsString() const
{
	return FString::Printf(TEXT("%d"), AutomationController->GetEnabledTestsNum());
}


FString SAutomationWindow::OnGetNumDevicesInClusterString(const int32 ClusterIndex) const
{
	return FString::Printf(TEXT("%d"), AutomationController->GetNumDevicesInCluster(ClusterIndex));
}


void SAutomationWindow::OnRefreshTestCallback()
{
	//rebuild the platform header
	RebuildPlatformIcons();

	//filter the tests that are shown
	AutomationController->SetFilter( AutomationFilters );

	// Only expand the child nodes if we have a text filter
	bool ExpandChildren = !AutomationTextFilter->GetRawFilterText().IsEmpty();

	TArray< TSharedPtr< IAutomationReport > >& TestReports = AutomationController->GetReports();

	for( int32 Index = 0; Index < TestReports.Num(); Index++ )
	{
		ExpandTreeView( TestReports[ Index ], ExpandChildren );
	}

	//rebuild the UI
	TestTable->RequestTreeRefresh();
}


void SAutomationWindow::OnTestAvailableCallback( EAutomationControllerModuleState::Type InAutomationControllerState )
{
	AutomationControllerState = InAutomationControllerState;
}


void SAutomationWindow::ExpandTreeView( TSharedPtr< IAutomationReport > InReport, const bool ShouldExpand )
{
	// Expand node if the report is flagged
	TestTable->SetItemExpansion( InReport, ShouldExpand && InReport->ExpandInUI() );

	// Iterate through the child nodes to see if they should be expanded
	TArray<TSharedPtr< IAutomationReport > > Reports = InReport->GetFilteredChildren();

	for ( int32 ChildItem = 0; ChildItem < Reports.Num(); ChildItem++ )
	{
		ExpandTreeView( Reports[ ChildItem ], ShouldExpand );
	}
}

//TODO AUTOMATION - remove
/** Updates list of all the tests */
void SAutomationWindow::ListTests( )
{
	AutomationController->RequestTests();
}


//TODO AUTOMATION - remove
/** Finds available workers */
void SAutomationWindow::FindWorkers()
{
	ActiveSession = SessionManager->GetSelectedSession();

	bool SessionIsValid = ActiveSession.IsValid() && (ActiveSession->GetSessionOwner() == FPlatformProcess::UserName(false));

	if (SessionIsValid)
	{
		bIsRequestingTests = true;

		AutomationController->RequestAvailableWorkers(ActiveSession->GetSessionId());

		RebuildPlatformIcons();
	}
	else
	{
		bIsRequestingTests = false;
		// Clear UI if the session is invalid
		ClearAutomationUI();
	}

	MenuBar->SetEnabled( SessionIsValid );
}


FReply SAutomationWindow::RunTests()
{
	if( AutomationControllerState == EAutomationControllerModuleState::Running )
	{
		AutomationController->StopTests();
	}
	else
	{
		AutomationController->RunTests( ActiveSession->IsStandalone() );
	}

	LogMessages.Empty();
	LogListView->RequestListRefresh();

	return FReply::Handled();
}


/** Filtering */
void SAutomationWindow::OnFilterTextChanged( const FText& InFilterText )
{
	AutomationTextFilter->SetRawFilterText( InFilterText );

	//update the widget
	OnRefreshTestCallback();
}


bool SAutomationWindow::IsVisualCommandletFilterOn() const
{
	return AutomationController->IsVisualCommandletFilterOn();
}


void SAutomationWindow::OnToggleVisualCommandletFilter()
{
	//Change controller filter
	AutomationController->SetVisualCommandletFilter(!IsVisualCommandletFilterOn());
	// need to call this to request update
	ListTests( );
}


bool SAutomationWindow::IsDeveloperDirectoryIncluded() const
{
	return AutomationController->IsDeveloperDirectoryIncluded();
}


void SAutomationWindow::OnToggleDeveloperDirectoryIncluded()
{
	//Change controller filter
	AutomationController->SetDeveloperDirectoryIncluded(!IsDeveloperDirectoryIncluded());
	// need to call this to request update
	ListTests();
}


bool SAutomationWindow::IsSmokeTestFilterOn() const
{
	return AutomationGeneralFilter->OnlyShowSmokeTests();
}


void SAutomationWindow::OnToggleSmokeTestFilter()
{
	AutomationGeneralFilter->SetOnlyShowSmokeTests( !IsSmokeTestFilterOn() );
	OnRefreshTestCallback();
}


bool SAutomationWindow::IsWarningFilterOn() const
{
	return AutomationGeneralFilter->ShouldShowWarnings();
}


void SAutomationWindow::OnToggleWarningFilter()
{
	AutomationGeneralFilter->SetShowWarnings( !IsWarningFilterOn() );
	OnRefreshTestCallback();
}


bool SAutomationWindow::IsErrorFilterOn() const
{
	return AutomationGeneralFilter->ShouldShowErrors();
}


void SAutomationWindow::OnToggleErrorFilter()
{
	AutomationGeneralFilter->SetShowErrors( !IsErrorFilterOn() );
	OnRefreshTestCallback();
}


FString SAutomationWindow::GetSmallIconExtension() const
{
	FString Brush;
	if (FMultiBoxSettings::UseSmallToolBarIcons.Get())
	{
		Brush += TEXT( ".Small" );
	}
	return Brush;
}


EVisibility SAutomationWindow::GetLargeToolBarVisibility() const
{
	return FMultiBoxSettings::UseSmallToolBarIcons.Get() ? EVisibility::Collapsed : EVisibility::Visible;
}


const FSlateBrush* SAutomationWindow::GetRunAutomationIcon() const
{
	FString Brush = TEXT( "AutomationWindow" );
	if( AutomationControllerState == EAutomationControllerModuleState::Running )
	{
		Brush += TEXT( ".StopTests" );	// Temporary brush type for stop tests
	}
	else
	{
		Brush += TEXT( ".RunTests" );
	}
	Brush += GetSmallIconExtension();
	return FEditorStyle::GetBrush( *Brush );
}


FString SAutomationWindow::GetRunAutomationLabel() const
{
	if( AutomationControllerState == EAutomationControllerModuleState::Running )
	{
		return LOCTEXT( "RunStopTestsLabel", "Stop Tests" ).ToString();
	}
	else
	{
		return LOCTEXT( "RunStartTestsLabel", "Start Tests" ).ToString();
	}
}


FText SAutomationWindow::HandleAutomationHighlightText( ) const
{
	if ( AutomationSearchBox.IsValid() )
	{
		return AutomationSearchBox->GetText();
	}
	return FText();
}


EVisibility SAutomationWindow::HandleSelectSessionOverlayVisibility( ) const
{
	if (SessionManager->GetSelectedSession().IsValid())
	{
		return EVisibility::Hidden;
	}

	return EVisibility::Visible;
}


void SAutomationWindow::HandleSessionManagerCanSelectSession( const ISessionInfoPtr& Session, bool& CanSelect )
{
	if (ActiveSession.IsValid() && AutomationController->CheckTestResultsAvailable())
	{
		EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("ChangeSessionDialog", "Are you sure you want to change sessions?\nAll automation results data will be lost"));
		CanSelect = Result == EAppReturnType::Yes ? true : false;
	}
}


void SAutomationWindow::HandleSessionManagerSelectionChanged( const ISessionInfoPtr& SelectedSession )
{
	FindWorkers();
}


bool SAutomationWindow::IsNotRunningTests() const
{
	return AutomationControllerState != EAutomationControllerModuleState::Running;
}


bool SAutomationWindow::IsAutomationRunButtonEnabled() const
{
	return AutomationControllerState != EAutomationControllerModuleState::Disabled;
}


void SAutomationWindow::CopyLog( )
{
	TArray<TSharedPtr<FAutomationOutputMessage> > SelectedItems = LogListView->GetSelectedItems();

	if (SelectedItems.Num() > 0)
	{
		FString SelectedText;

		for( int32 Index = 0; Index < SelectedItems.Num(); ++Index )
		{
			SelectedText += SelectedItems[Index]->Text;
			SelectedText += LINE_TERMINATOR;
		}

		FPlatformMisc::ClipboardCopy( *SelectedText );
	}
}


FReply SAutomationWindow::HandleCommandBarCopyLogClicked( )
{
	CopyLog();

	return FReply::Handled();
}


void SAutomationWindow::HandleLogListSelectionChanged( TSharedPtr<FAutomationOutputMessage> InItem, ESelectInfo::Type SelectInfo )
{
	CommandBar->SetNumLogMessages(LogListView->GetNumItemsSelected());
}


void SAutomationWindow::ChangeTheSelectionToThisRow(TSharedPtr< IAutomationReport >  ThisRow) 
{
	TestTable->SetSelection(ThisRow, ESelectInfo::Direct);
}


bool SAutomationWindow::IsRowSelected(TSharedPtr< IAutomationReport >  ThisRow)
{
	TArray< TSharedPtr<IAutomationReport> >SelectedReport = TestTable->GetSelectedItems();

	bool ThisRowIsInTheSelectedSet = false;

	for (int i = 0; i<SelectedReport.Num();++i)
	{
		if (SelectedReport[i] == ThisRow)
		{
			ThisRowIsInTheSelectedSet = true;
		}
	}
	return ThisRowIsInTheSelectedSet;
}


void SAutomationWindow::SetAllSelectedTestsChecked( bool InChecked )
{
	TArray< TSharedPtr<IAutomationReport> >SelectedReport = TestTable->GetSelectedItems();

	for (int i = 0; i<SelectedReport.Num();++i)
	{
		if (SelectedReport[i].IsValid())
		{
			SelectedReport[i]->SetEnabled(InChecked);
		}
	}
}


bool SAutomationWindow::IsAnySelectedRowEnabled()
{
	TArray< TSharedPtr<IAutomationReport> >SelectedReport = TestTable->GetSelectedItems();

	//Do check or uncheck selected rows based on current settings
	bool bFoundCheckedRow = false;
	bool bFoundNotCheckedRow = false;
	bool bRowCheckedValue = true;

	//Check all the rows if there is a mixture of checked and unchecked then we set all checked, otherwise set to opposite of current values

	for (int i = 0; i<SelectedReport.Num();++i)
	{
		if (SelectedReport[i].IsValid())
		{
			if (SelectedReport[i]->IsEnabled())
			{
				bFoundCheckedRow = true;
			}
			else
			{
				bFoundNotCheckedRow = true;
			}
		}
		//break when all rows checked or different values found
		if (bFoundCheckedRow && bFoundNotCheckedRow)
		{
			break;
		}
	}

	//if rows were all checked set to unchecked otherwise we can set to checked
	if (bFoundCheckedRow && !bFoundNotCheckedRow)
	{
		bRowCheckedValue = false;
	}

	return bRowCheckedValue;
}


/* SWidget implementation
 *****************************************************************************/

FReply SAutomationWindow::OnKeyUp( const FGeometry& InGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	if (InKeyboardEvent.GetKey() == EKeys::SpaceBar)
	{
		SetAllSelectedTestsChecked(IsAnySelectedRowEnabled());
		return FReply::Handled();
	}
	return FReply::Unhandled();
}


FReply SAutomationWindow::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	if (InKeyboardEvent.IsControlDown())
	{
		if (InKeyboardEvent.GetKey() == EKeys::C)
		{
			CopyLog();

			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}


/* SAutomationWindow callbacks
 *****************************************************************************/

void SAutomationWindow::HandleItemCheckBoxCheckedStateChanged( TSharedPtr< IAutomationReport > TestStatus )
{
	//If multiple rows selected then handle all the rows
	if (AreMultipleRowsSelected())
	{
		//if current row is not in the selected list select that row
		if(IsRowSelected(TestStatus))
		{
			//Just set them all to the opposite of the row just clicked.
			SetAllSelectedTestsChecked(!TestStatus->IsEnabled());
		}
		else
		{
			//Change the selection to this row rather than keep other rows selected unrelated to the ticked/unticked item
			ChangeTheSelectionToThisRow(TestStatus);
			TestStatus->SetEnabled( !TestStatus->IsEnabled() );
		}
	}
	else
	{
		TestStatus->SetEnabled( !TestStatus->IsEnabled() );
	}
}


bool SAutomationWindow::HandleItemCheckBoxIsEnabled( ) const
{
	return IsNotRunningTests();
}


#undef LOCTEXT_NAMESPACE
