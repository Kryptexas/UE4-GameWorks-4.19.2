// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SProfilerWindow.cpp: Implements the SProfilerWindow class.
=============================================================================*/

#include "ProfilerPrivatePCH.h"
#include "SProfilerSettings.h"
#include "DesktopPlatformModule.h"

#define LOCTEXT_NAMESPACE "SProfilerWindow"

/** Contains names of all widgets used in the profiler window. */
namespace FNames
{
	static const FName GraphView = FName("GraphViewTab");
	static const FName SentinelView = FName("SentinelViewTab");
	static const FName FiltersAndPresets = FName("FilterAndPreset");
	static const FName EventGraphView = FName("EventGraphView");
}

static FText GetTextForNotification( const EProfilerNotificationTypes::Type NotificatonType, const ELoadingProgressStates::Type ProgressState, const FString& Filename, const float ProgressPercent = 0.0f )
{
	FText Result;

	if( NotificatonType == EProfilerNotificationTypes::LoadingOfflineCapture )
	{
		if( ProgressState == ELoadingProgressStates::Started )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("Filename"), FText::FromString( Filename ) );
			Result = FText::Format( LOCTEXT("DescF_OfflineCapture_Started", "Started loading a file ../../{Filename}"), Args );
		}
		else if( ProgressState == ELoadingProgressStates::InProgress )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("Filename"), FText::FromString( Filename ) );
			Args.Add( TEXT("DataLoadingProgressPercent"), FText::AsPercent( ProgressPercent ) );
			Result = FText::Format( LOCTEXT("DescF_OfflineCapture_InProgress", "Loading a file ../../{Filename} {DataLoadingProgressPercent}"), Args );
		}
		else if( ProgressState == ELoadingProgressStates::Loaded )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("Filename"), FText::FromString( Filename ) );
			Result = FText::Format( LOCTEXT("DescF_OfflineCapture_Loaded", "Capture file ../../{Filename} has been successfully loaded"), Args );
		}
		else if( ProgressState == ELoadingProgressStates::Failed )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("Filename"), FText::FromString( Filename ) );
			Result = FText::Format( LOCTEXT("DescF_OfflineCapture_Failed", "Failed to load capture file ../../{Filename}"), Args );
		}
	}
	else if( NotificatonType == EProfilerNotificationTypes::SendingServiceSideCapture )
	{
		if( ProgressState == ELoadingProgressStates::Started )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("Filename"), FText::FromString( Filename ) );
			Result = FText::Format( LOCTEXT("DescF_ServiceSideCapture_Started", "Started receiving a file ../../{Filename}"), Args );
		}
		else if( ProgressState == ELoadingProgressStates::InProgress )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("Filename"), FText::FromString( Filename ) );
			Args.Add( TEXT("DataLoadingProgressPercent"), FText::AsPercent( ProgressPercent ) );
			Result = FText::Format( LOCTEXT("DescF_ServiceSideCapture_InProgress", "Receiving a file ../../{Filename} {DataLoadingProgressPercent}"), Args );
		}
		else if( ProgressState == ELoadingProgressStates::Loaded )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("Filename"), FText::FromString( Filename ) );
			Result = FText::Format( LOCTEXT("DescF_ServiceSideCapture_Loaded", "Capture file ../../{Filename} has been successfully received"), Args );
		}
		else if( ProgressState == ELoadingProgressStates::Failed )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("Filename"), FText::FromString( Filename ) );
			Result = FText::Format( LOCTEXT("DescF_ServiceSideCapture_Failed", "Failed to receive capture file ../../{Filename}"), Args );
		}
	}

	return Result;
}

SProfilerWindow::SProfilerWindow()
{}

SProfilerWindow::~SProfilerWindow()
{}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SProfilerWindow::Construct( const FArguments& InArgs, const ISessionManagerRef& InSessionManager )
{
	SessionManager = InSessionManager;

	// TODO: Cleanup overlay code.
	ChildSlot
		[
			SNew(SOverlay)

			// Overlay slot for the main profiler window area, the first
			+SOverlay::Slot()
			[
				SAssignNew(MainContentPanel,SVerticalBox)

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( SHorizontalBox )

					+SHorizontalBox::Slot()
					.FillWidth( 1.0f )
					.Padding( 0.0f )
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SProfilerToolbar)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( SSpacer )
					.Size( FVector2D( 2.0f, 2.0f ) )
				]

				+SVerticalBox::Slot()
				.FillHeight( 1.0f )
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					.IsEnabled( this, &SProfilerWindow::IsProfilerEnabled )

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride( 256.0f )
						[
							SNew(SVerticalBox)

							// Header
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)

								+SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SImage)
									.Image( FEditorStyle::GetBrush( TEXT("Profiler.Tab.FiltersAndPresets") ) )
								]

								+SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(STextBlock)
									.Text( LOCTEXT("FiltersAndPresetsLabel", "Filters And Presets") )
								]
							]

							// Spacer
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew( SSpacer )
								.Size( FVector2D( 2.0f, 2.0f ) )
							]

							// Filters And Presets
							+SVerticalBox::Slot()
							.FillHeight( 1.0f )
							[
								SNew(SFiltersAndPresets)
							]
						]
					]

					+SHorizontalBox::Slot()
					.FillWidth( 1.0f )
					.Padding(6.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew( SSplitter )
						.Orientation( Orient_Vertical )
						//.PhysicalSplitterHandleSize( 2.0f )
						//.HitDetectionSplitterHandleSize( 4.0f )

						+SSplitter::Slot()
						.Value( 1.0f )
						[
							SNew(SVerticalBox)

							// Header
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)

								+SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SImage)
									.Image( FEditorStyle::GetBrush( TEXT("Profiler.Tab.GraphView") ) )
								]

								+SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(STextBlock)
									.Text( LOCTEXT("GraphViewLabel", "Graph View") )
								]
							]
							
							// Spacer
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew( SSpacer )
								.Size( FVector2D( 2.0f, 2.0f ) )
							]

							// Graph View
							+SVerticalBox::Slot()
							.FillHeight( 1.0f )
							[
								SAssignNew(GraphPanel,SProfilerGraphPanel)
							]
						]

						+SSplitter::Slot()
						.Value( 2.0f )
						[
							SAssignNew(EventGraphPanel,SVerticalBox)
						]
					]
				]
			]

			// Overlay slot for the notification area.
			+SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderImage( FEditorStyle::GetBrush("NotificationList.ItemBackground") )
				.Padding( 8.0f )
				.Visibility( this, &SProfilerWindow::IsSessionOverlayVissible )
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SelectSessionOverlayText", "Please select a session from the Session Browser or load a saved capture.").ToString())
				]
			]

			// Overlay slot for the notification area, the second
			+SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding( 16.0f )
			[
				SAssignNew(NotificationList, SNotificationList)
			]

			// Overlay slot for the profiler settings, the third
			+SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Expose( OverlaySettingsSlot )
		];

	GraphPanel->GetMainDataGraph()->OnSelectionChangedForIndex().AddSP( FProfilerManager::Get().Get(), &FProfilerManager::DataGraph_OnSelectionChangedForIndex );
}
void SProfilerWindow::ManageEventGraphTab( const FGuid ProfilerInstanceID, const bool bCreateFakeTab, const FString TabName )
{
	// TODO: Add support for multiple instances.
	if( bCreateFakeTab )
	{
		TSharedPtr<SEventGraph> EventGraphWidget;

		EventGraphPanel->ClearChildren();

		// Header
		EventGraphPanel->AddSlot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SImage)
				.Image( FEditorStyle::GetBrush( TEXT("Profiler.Tab.EventGraph") ) )
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text( TabName )
			]
		];

		EventGraphPanel->AddSlot()
		.AutoHeight()
		[
			SNew( SSpacer )
			.Size( FVector2D( 2.0f, 2.0f ) )
		];

		// Event graph
		EventGraphPanel->AddSlot()
		.FillHeight( 1.0f )
		[
			SAssignNew(EventGraphWidget,SEventGraph)
		];

		ActiveEventGraphs.Add( ProfilerInstanceID, EventGraphWidget.ToSharedRef() );

		// Register data graph with the new event graph tab.
		EventGraphWidget->OnEventGraphRestoredFromHistory().AddSP( GraphPanel->GetMainDataGraph().Get(), &SDataGraph::EventGraph_OnRestoredFromHistory );
	}
	else
	{
		ActiveEventGraphs.Remove( ProfilerInstanceID );
	}
}

void SProfilerWindow::UpdateEventGraph( const FGuid ProfilerInstanceID, const FEventGraphDataRef AverageEventGraph, const FEventGraphDataRef MaximumEventGraph, bool bInitial )
{
	const auto* EventGraph = ActiveEventGraphs.Find( ProfilerInstanceID );
	if( EventGraph )
	{
		(*EventGraph)->SetNewEventGraphState( AverageEventGraph, MaximumEventGraph, bInitial );
	}
}

EVisibility SProfilerWindow::IsSessionOverlayVissible() const
{
	if( FProfilerManager::Get()->GetProfilerInstancesNum() > 0 )
	{
		return EVisibility::Hidden;
	}

	return EVisibility::Visible;
}

bool SProfilerWindow::IsProfilerEnabled() const
{
	const bool bIsActive = FProfilerManager::Get()->IsConnected() || FProfilerManager::Get()->IsCaptureFileFullyProcessed();
	return bIsActive;
}

void SProfilerWindow::ManageLoadingProgressNotificationState( const FString& Filename, const EProfilerNotificationTypes::Type NotificatonType, const ELoadingProgressStates::Type ProgressState, const float DataLoadingProgress )
{
	const FString BaseFilename = FPaths::GetBaseFilename( Filename );

	if( ProgressState == ELoadingProgressStates::Started )
	{
		const bool bContains = ActiveNotifications.Contains( Filename );
		if( !bContains )
		{
			FNotificationInfo NotificationInfo( GetTextForNotification(NotificatonType,ProgressState,BaseFilename) );
			NotificationInfo.bFireAndForget = false;
			NotificationInfo.bUseLargeFont = false;

			// Add two buttons, one for cancel, one for loading the received file.
			if( NotificatonType == EProfilerNotificationTypes::SendingServiceSideCapture )
			{
				const FText CancelButtonText = LOCTEXT("CancelButton_Text", "Cancel");
				const FText CancelButtonTT = LOCTEXT("CancelButton_TTText", "Hides this notification");
				const FText LoadButtonText = LOCTEXT("LoadButton_Text", "Load file");
				const FText LoadButtonTT = LOCTEXT("LoadButton_TTText", "Loads the received file and hides this notification");

				NotificationInfo.ButtonDetails.Add( FNotificationButtonInfo( CancelButtonText, CancelButtonTT, FSimpleDelegate::CreateSP( this, &SProfilerWindow::SendingServiceSideCapture_Cancel, Filename ), SNotificationItem::CS_Success ) );
				NotificationInfo.ButtonDetails.Add( FNotificationButtonInfo( LoadButtonText, LoadButtonTT, FSimpleDelegate::CreateSP( this, &SProfilerWindow::SendingServiceSideCapture_Load, Filename ), SNotificationItem::CS_Success ) );
			}

			SNotificationItemWeak LoadingProgress = NotificationList->AddNotification(NotificationInfo);

			LoadingProgress.Pin()->SetCompletionState( SNotificationItem::CS_Pending );

			ActiveNotifications.Add( Filename, LoadingProgress );
		}
	}
	else if( ProgressState == ELoadingProgressStates::InProgress )
	{
		const SNotificationItemWeak* LoadingProgressPtr = ActiveNotifications.Find( Filename );
		if( LoadingProgressPtr )
		{
			( *LoadingProgressPtr ).Pin()->SetText( GetTextForNotification( NotificatonType, ProgressState, BaseFilename, DataLoadingProgress ) );
			( *LoadingProgressPtr ).Pin()->SetCompletionState( SNotificationItem::CS_Pending );
		}
	}
	else if( ProgressState == ELoadingProgressStates::Loaded )
	{
		const SNotificationItemWeak* LoadingProgressPtr = ActiveNotifications.Find( Filename );
		if( LoadingProgressPtr )
		{
			( *LoadingProgressPtr ).Pin()->SetText( GetTextForNotification( NotificatonType, ProgressState, BaseFilename ) );
			( *LoadingProgressPtr ).Pin()->SetCompletionState( SNotificationItem::CS_Success );
			
			// Notifications for received files are handled by ty user.
			if( NotificatonType == EProfilerNotificationTypes::LoadingOfflineCapture )
			{
				(*LoadingProgressPtr).Pin()->ExpireAndFadeout();
				ActiveNotifications.Remove( Filename );
			}
		}
	}
	else if( ProgressState == ELoadingProgressStates::Failed )
	{
		const SNotificationItemWeak* LoadingProgressPtr = ActiveNotifications.Find( Filename );
		if( LoadingProgressPtr )
		{
			(*LoadingProgressPtr).Pin()->SetText( GetTextForNotification(NotificatonType,ProgressState,BaseFilename) );
			(*LoadingProgressPtr).Pin()->SetCompletionState(SNotificationItem::CS_Fail);

			(*LoadingProgressPtr).Pin()->ExpireAndFadeout();
			ActiveNotifications.Remove( Filename );
		}
	}
}

void SProfilerWindow::SendingServiceSideCapture_Cancel( const FString Filename )
{
	const SNotificationItemWeak* LoadingProgressPtr = ActiveNotifications.Find( Filename );
	if( LoadingProgressPtr )
	{
		(*LoadingProgressPtr).Pin()->ExpireAndFadeout();
		ActiveNotifications.Remove( Filename );
	}
}

void SProfilerWindow::SendingServiceSideCapture_Load( const FString Filename )
{
	const SNotificationItemWeak* LoadingProgressPtr = ActiveNotifications.Find( Filename );
	if( LoadingProgressPtr )
	{
		(*LoadingProgressPtr).Pin()->ExpireAndFadeout();
		ActiveNotifications.Remove( Filename );

		// TODO: Move to a better place.
		const FString PathName = FPaths::ProfilingDir() + TEXT("UnrealStats/Received/");
		const FString StatFilepath = PathName + Filename;
		FProfilerManager::Get()->LoadProfilerCapture( StatFilepath );
	}
}

FReply SProfilerWindow::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	return FProfilerManager::Get()->GetCommandList()->ProcessCommandBindings( InKeyboardEvent ) ? FReply::Handled() : FReply::Unhandled();
}

FReply SProfilerWindow::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if( DragDrop::IsTypeMatch<FExternalDragOperation>( DragDropEvent.GetOperation() ) )
	{
		TSharedPtr<FExternalDragOperation> DragDropOp = StaticCastSharedPtr<FExternalDragOperation>(DragDropEvent.GetOperation());

		if( DragDropOp->HasFiles() )
		{
			const TArray<FString>& Files = DragDropOp->GetFiles();
			if( Files.Num() == 1 )
			{
				const bool IsValidFile = FPaths::GetExtension( Files[0] ) == TEXT("ue4stats");// TODO: Move to core/stats2
				if( IsValidFile )
				{
					return FReply::Handled();
				}
			}
		}
	}

	return SCompoundWidget::OnDragOver(MyGeometry,DragDropEvent);
}

FReply SProfilerWindow::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if( DragDrop::IsTypeMatch<FExternalDragOperation>( DragDropEvent.GetOperation() ) )
	{
		TSharedPtr<FExternalDragOperation> DragDropOp = StaticCastSharedPtr<FExternalDragOperation>(DragDropEvent.GetOperation());

		if( DragDropOp->HasFiles() )
		{
			// For now, only allow a single file.
			const TArray<FString>& Files = DragDropOp->GetFiles();
			if( Files.Num() == 1 )
			{
				const bool IsValidFile = FPaths::GetExtension( Files[0] ) == TEXT("ue4stats");
				if( IsValidFile )
				{
					// Enqueue load operation.
					FProfilerManager::Get()->LoadProfilerCapture( Files[0] );
					return FReply::Handled();
				}
			}
		}
	}

	return SCompoundWidget::OnDrop(MyGeometry,DragDropEvent);
}

void SProfilerWindow::OpenProfilerSettings()
{
	MainContentPanel->SetEnabled( false );
	(*OverlaySettingsSlot)
	[
		SNew(SBorder)
		.BorderImage( FEditorStyle::GetBrush("NotificationList.ItemBackground") )
		.Padding( 8.0f )
		[
		 	SNew(SProfilerSettings)
		 	.OnClose( this, &SProfilerWindow::CloseProfilerSettings )
			.SettingPtr( &FProfilerManager::GetSettings() )
		]
	];
}

void SProfilerWindow::CloseProfilerSettings()
{
	// Close the profiler settings by simply replacing widget with a null one.
	(*OverlaySettingsSlot)
	[
		SNullWidget::NullWidget
	];
	MainContentPanel->SetEnabled( true );
}


#undef LOCTEXT_NAMESPACE
