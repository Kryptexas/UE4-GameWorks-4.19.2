// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LiveEditorPrivatePCH.h"
#include "LiveEditorDevice.h"
#include "LiveEditorConfigWindow.h"
#include "LiveEditorConnectionWindow.h"
#include "LiveEditorDeviceSetupWindow.h"
#include "LiveEditorWizardBase.h"

DEFINE_LOG_CATEGORY_STATIC(LiveEditorConfigWindow, Log, All);

class SLiveEditorBlueprint : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SLiveEditorBlueprint ) {}
	SLATE_END_ARGS()

	TSharedPtr<FString> Name;
	UBlueprint *Blueprint;	//todo TShared?
	SLiveEditorConfigWindow *Owner;

	/**
	 * Constructs this widget
	 *
	 * @param InArgs    Declaration from which to construct the widget
	 */
	void Construct(const FArguments& InArgs, const TSharedPtr<FString> &_Name, SLiveEditorConfigWindow *_Owner)
	{
		Name = _Name;
		Owner = _Owner;
		bIsActive = (Name.Get())? FLiveEditorManager::Get().CheckActive( *Name.Get() ) : false;

		Blueprint = LoadObject<UBlueprint>( NULL, *(*Name.Get()), NULL, 0, NULL );
		check(	Blueprint != NULL
				&& Blueprint->GeneratedClass != NULL
				&& Blueprint->GeneratedClass->IsChildOf( ULiveEditorBlueprint::StaticClass() ) );

		ChildSlot
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew( SButton )
				.Text( this, &SLiveEditorBlueprint::GetBindButtonText )
				.ToolTipText( FString(TEXT("Bind your MIDI hardware to this Blueprint")) )
				.HAlign( HAlign_Center )
				.VAlign( VAlign_Center )
				.OnClicked( this, &SLiveEditorBlueprint::BindBlueprint )
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew( SButton )
				.Text( FString(TEXT("Activate")) )
				.ToolTipText( FString(TEXT("Make this Blueprint active in the LiveEditor context")) )
				.HAlign( HAlign_Center )
				.VAlign( VAlign_Center )
				.Visibility( this, &SLiveEditorBlueprint::CanActivate )
				.OnClicked( this, &SLiveEditorBlueprint::ActivateBlueprint )
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew( SButton )
				.Text( FString(TEXT("DeActivate")) )
				.ToolTipText( FString(TEXT("Stop this Blueprint running in the LiveEditor context")) )
				.HAlign( HAlign_Center )
				.VAlign( VAlign_Center )
				.Visibility( this, &SLiveEditorBlueprint::CanDeActivate )
				.OnClicked( this, &SLiveEditorBlueprint::DeActivateBlueprint )
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew( SButton )
				.Text( FString(TEXT("Remove")) )
				.ToolTipText( FString(TEXT("Remove this Blueprint from the LiveEditor context")) )
				.HAlign( HAlign_Center )
				.VAlign( VAlign_Center )
				.OnClicked( this, &SLiveEditorBlueprint::RemoveBlueprint )
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign( VAlign_Center )
			.Padding(2.0f)
			[
				SAssignNew( NameView, STextBlock )
				.Text( *Name.Get() )
				.ColorAndOpacity( GetFontColor() )
			]
		];
	}

	FString GetBindButtonText() const
	{
		check( Blueprint != NULL );
		return (FLiveEditorManager::Get().BlueprintHasBinding(*Blueprint))? FString(TEXT("Re-Bind")) : FString(TEXT("Bind"));
	}
	FReply BindBlueprint()
	{
		check( Blueprint != NULL );
		FLiveEditorManager::Get().StartBlueprintBinding( *Blueprint );
		Owner->WizardActivated();

		return FReply::Handled();
	}

	FReply ActivateBlueprint()
	{
		FString *n = Name.Get();
		if ( n )
		{
			FLiveEditorManager::Get().Activate( *n );
			UE_LOG( LiveEditorConfigWindow, Log, TEXT("%s Activated"), *(*n) );

			bIsActive = (Name.Get())? FLiveEditorManager::Get().CheckActive( *Name.Get() ) : false;
			NameView->SetForegroundColor( GetFontColor() );
		}
		return FReply::Handled();
	}
	FReply DeActivateBlueprint()
	{
		FString *n = Name.Get();
		if ( n )
		{
			FLiveEditorManager::Get().DeActivate( *n );
			UE_LOG( LiveEditorConfigWindow, Log, TEXT("%s DeActivated"), *(*n) );

			bIsActive = (Name.Get())? FLiveEditorManager::Get().CheckActive( *Name.Get() ) : false;
			NameView->SetForegroundColor( GetFontColor() );
		}
		return FReply::Handled();
	}
	FReply RemoveBlueprint()
	{
		FString *n = Name.Get();
		if ( n )
		{
			FLiveEditorManager::Get().Remove( *n );
			UE_LOG( LiveEditorConfigWindow, Log, TEXT("%s Removed"), *(*n) );

			Owner->RefreshBlueprintTemplates();
		}
		return FReply::Handled();
	}

	FLinearColor GetFontColor() const
	{
		return (bIsActive)? FLinearColor::Green : FLinearColor::White;
	}
	EVisibility CanActivate() const
	{
		check( Blueprint != NULL );
		if ( !FLiveEditorManager::Get().BlueprintHasBinding(*Blueprint) )
		{
			return EVisibility::Hidden;
		}
		return (bIsActive)? EVisibility::Hidden : EVisibility::Visible;
	}
	EVisibility CanDeActivate() const
	{
		check( Blueprint != NULL );
		if ( !FLiveEditorManager::Get().BlueprintHasBinding(*Blueprint) )
		{
			return EVisibility::Hidden;
		}
		return (bIsActive)? EVisibility::Visible : EVisibility::Hidden;
	}

private:
	bool bIsActive;
	TSharedPtr< STextBlock > NameView;
};

//////////////////////////////////////////////////////////////////////////
// SLiveEditorDeviceData

static const float sInputVisibleTime = 0.1f;
class SLiveEditorDeviceData : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SLiveEditorDeviceData ) {}
	SLATE_END_ARGS()

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE
	{
		SCompoundWidget::Tick( AllottedGeometry, InCurrentTime, InDeltaTime );

		bool success = FLiveEditorManager::Get().GetDeviceData( *DeviceID.Get(), DeviceData );
		check(success);
	}

	void Construct(const FArguments& InArgs, const TSharedPtr<PmDeviceID> &_DeviceID, SLiveEditorConfigWindow *_Owner)
	{
		Owner = _Owner;

		static bool bDoOnce = false;
		if ( !bDoOnce )
		{
			HasInputBackgroundBrush = new FSlateBoxBrush( "LiveEditor.HasInputBrush", 4.0f/16.0f, FLinearColor(0.0f, 0.0f, 0.5f, 1.0f) );
			NeedsConfigureBackgroundBrush = new FSlateBoxBrush( "LiveEditor.NeedsConfigureBrush", 4.0f/16.0f, FLinearColor(0.5f, 0.0f, 0.0f, 1.0f) );
			bDoOnce = true;
		}

		DeviceID = _DeviceID;
		bool success = FLiveEditorManager::Get().GetDeviceData( *DeviceID.Get(), DeviceData );
		check( success );

		ChildSlot
		[
			SNew( SBorder )
			.BorderImage( this, &SLiveEditorDeviceData::GetBorderBrush )
			.Padding( 2.0f )
			[
				SNew( SVerticalBox )
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2.0f)
				[
					SNew( STextBlock )
					.Text( DeviceData.DeviceName )
					.ColorAndOpacity( FLinearColor::White )
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2.0f)
				[
					SNew( SButton )
					.Text( this, &SLiveEditorDeviceData::GetConfigureButtonTitle )
					.ToolTipText( FString(TEXT("Configure this MIDI hardware for use in the LiveEditor")) )
					.HAlign( HAlign_Center )
					.VAlign( VAlign_Center )
					.OnClicked( this, &SLiveEditorDeviceData::ConfigureDevice )
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 20.0f, 2.0f, 20.0f, 2.0f )
				[
					SNew( SVerticalBox )
					.Visibility( this, &SLiveEditorDeviceData::IsConfigured )
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f)
					[
						SNew( STextBlock )
						.Text( this, &SLiveEditorDeviceData::GetTextButtonDown )
						.ColorAndOpacity( FLinearColor::White )
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f)
					[
						SNew( STextBlock )
						.Text( this, &SLiveEditorDeviceData::GetTextButtonUp )
						.ColorAndOpacity( FLinearColor::White )
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f)
					[
						SNew( STextBlock )
						.Text( this, &SLiveEditorDeviceData::GetTextCCIncrement )
						.ColorAndOpacity( FLinearColor::White )
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f)
					[
						SNew( STextBlock )
						.Text( this, &SLiveEditorDeviceData::GetTextCCDecrement )
						.ColorAndOpacity( FLinearColor::White )
					]
				]
			]
		];
	}

	bool HasInput() const
	{
		float CurTime = GWorld->GetTimeSeconds();
		return ((CurTime-DeviceData.LastInputTime) <= sInputVisibleTime );
	}
	const FSlateBrush *GetBorderBrush() const
	{
		if ( DeviceData.ConfigState == FLiveEditorDeviceData::UNCONFIGURED )
		{
			return (HasInput())? NeedsConfigureBackgroundBrush : FEditorStyle::GetBrush( "NoBorder" );
		}
		else
		{
			return (HasInput())? HasInputBackgroundBrush : FEditorStyle::GetBrush( "NoBorder" );
		}
	}

	FReply ConfigureDevice()
	{
		Owner->ConfigureDevice( *DeviceID.Get() );
		return FReply::Handled();
	}

	EVisibility IsConfigured() const
	{
		return ( DeviceData.ConfigState == FLiveEditorDeviceData::CONFIGURED )? EVisibility::Visible : EVisibility::Hidden;
	}

	FString GetConfigureButtonTitle() const
	{
		return ( DeviceData.ConfigState == FLiveEditorDeviceData::CONFIGURED )? FString(TEXT("Re-Configure")) : FString(TEXT("Configure"));
	}
	FString GetTextButtonDown() const
	{
		return FString::Printf( TEXT("ButtonDown Signal: %d"), DeviceData.ButtonSignalDown );
	}
	FString GetTextButtonUp() const
	{
		return FString::Printf( TEXT("ButtonUp Signal: %d"), DeviceData.ButtonSignalUp );
	}
	FString GetTextCCIncrement() const
	{
		return FString::Printf( TEXT("ContinuousIncrement Signal: %d"), DeviceData.ContinuousIncrement );
	}
	FString GetTextCCDecrement() const
	{
		return FString::Printf( TEXT("ContinuousDecrement Signal: %d"), DeviceData.ContinuousDecrement );
	}

private:
	static FSlateBrush *HasInputBackgroundBrush;
	static FSlateBrush *NeedsConfigureBackgroundBrush;
	TSharedPtr<PmDeviceID> DeviceID;

	FLiveEditorDeviceData DeviceData;
	SLiveEditorConfigWindow *Owner;
};
FSlateBrush *SLiveEditorDeviceData::HasInputBackgroundBrush = NULL;
FSlateBrush *SLiveEditorDeviceData::NeedsConfigureBackgroundBrush = NULL;


//////////////////////////////////////////////////////////////////////////
// SLiveEditorConfigWindow

FReply SLiveEditorConfigWindow::AddBlueprintToList()
{
	FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();

	UBlueprint* SelectedBlueprint = GEditor->GetSelectedObjects()->GetTop<UBlueprint>();
	if(	SelectedBlueprint != NULL
		&& SelectedBlueprint->GeneratedClass != NULL
		&& SelectedBlueprint->GeneratedClass->IsChildOf( ULiveEditorBlueprint::StaticClass() ) )
	{
		FLiveEditorManager::Get().SelectLiveEditorBlueprint( SelectedBlueprint );
		RefreshBlueprintTemplates();
	}
	return FReply::Handled();
}

FReply SLiveEditorConfigWindow::RefreshDevices()
{
	FLiveEditorManager::Get().RefreshDeviceConnections();

	SharedDeviceIDs.Empty();
	FLiveEditorManager::Get().GetDeviceIDs( SharedDeviceIDs );
	if ( DeviceListView.Get() )
	{
		DeviceListView->RequestListRefresh();
	}

	return FReply::Handled();
}

void SLiveEditorConfigWindow::RefreshBlueprintTemplates()
{
	SharedBlueprintTemplates.Empty();
	TArray<FString> &BPs = FLiveEditorManager::Get().GetActiveBlueprintTemplates();
	for ( TArray<FString>::TIterator It(BPs); It; It++ )
	{
		SharedBlueprintTemplates.Add( MakeShareable( new FString(*It) ) );
	}
	if ( BlueprintListView.Get() )
	{
		BlueprintListView->RequestListRefresh();
	}
}

void SLiveEditorConfigWindow::WizardActivated()
{
	check( LiveEditorWizardWindow.IsValid() );
	LiveEditorWizardWindow.Get()->Activated();
}

void SLiveEditorConfigWindow::Construct(const FArguments& InArgs)
{
	RefreshBlueprintTemplates();
	RefreshDevices();

	ChildSlot
	[
		SNew( SOverlay )
		+SOverlay::Slot()
		[
			SNew( SVerticalBox )
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2.0f)
			[
				SNew(SExpandableArea)
				.InitiallyCollapsed(false)
				.BorderImage( FEditorStyle::GetBrush( "ToolBar.Background" ) )
				.Padding(8.0f)
				.HeaderContent()
				[
					SNew( STextBlock )
					.Text( FString(TEXT("Blueprints")) )
					.Font( FEditorStyle::GetFontStyle("ContentBrowser.SourceTitleFont") )
				]
				.BodyContent()
				[
					SNew( SBorder )
					.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
					[
						SNew( SVerticalBox )
						+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(2.0f)
						[
							SNew( SButton )
							.Text( FString(TEXT("Select Blueprint")) )
							.ToolTipText( FString(TEXT("Use Selected Asset from Content Browser")) )
							.OnClicked( this, &SLiveEditorConfigWindow::AddBlueprintToList )
						]
						+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(2.0f)
						[
							SAssignNew( BlueprintListView, SListView< TSharedPtr<FString> > )
							.ListItemsSource(&SharedBlueprintTemplates)
							.OnGenerateRow(this, &SLiveEditorConfigWindow::OnGenerateWidgetForBlueprintList)
						]
					]
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2.0f)
			[
				SNew(SExpandableArea)
				.InitiallyCollapsed(true)
				.BorderImage( FEditorStyle::GetBrush( "ToolBar.Background" ) )
				.Padding(8.0f)
				.HeaderContent()
				[
					SNew( STextBlock )
					.Text( FString(TEXT("Remote Connection Manager")) )
					.Font( FEditorStyle::GetFontStyle("ContentBrowser.SourceTitleFont") )
				]
				.BodyContent()
				[
					SNew( SBorder )
					.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
					[
						SNew(SLiveEditorConnectionWindow)
					]
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2.0f)
			[
				SNew(SExpandableArea)
				.InitiallyCollapsed(false)
				.BorderImage( FEditorStyle::GetBrush( "ToolBar.Background" ) )
				.Padding(8.0f)
				.HeaderContent()
				[
					SNew( STextBlock )
					.Text( FString(TEXT("Devices")) )
					.Font( FEditorStyle::GetFontStyle("ContentBrowser.SourceTitleFont") )
				]
				.BodyContent()
				[
					SNew( SBorder )
					.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
					[
						SNew( SVerticalBox )
						+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(2.0f)
						[
							SNew( SButton )
							.Text( FString(TEXT("Refresh Devices")) )
							.ToolTipText( FString(TEXT("Refresh MIDI Device Connections")) )
							.OnClicked( this, &SLiveEditorConfigWindow::RefreshDevices )
						]
						+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(2.0f)
						[
							SAssignNew( DeviceListView, SListView< TSharedPtr<PmDeviceID> > )
							.ListItemsSource(&SharedDeviceIDs)
							.OnGenerateRow(this, &SLiveEditorConfigWindow::OnGenerateWidgetForDeviceList)
						]
					]
				]
			]
		]
		+SOverlay::Slot()
		[
			SNew( SBorder )
			.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
			.Visibility( this, &SLiveEditorConfigWindow::ShowDeviceConfigWindow )
			.Padding( 2.0f )
			.HAlign( HAlign_Center )
			.VAlign( VAlign_Center )
			[
				SAssignNew( LiveEditorWizardWindow, SLiveEditorWizardWindow )
			]
		]
	];
}

EVisibility SLiveEditorConfigWindow::ShowDeviceConfigWindow() const
{
	FLiveEditorWizardBase *Wizard = FLiveEditorManager::Get().GetActiveWizard();
	if ( Wizard == NULL || !Wizard->IsActive() )
	{
		return EVisibility::Collapsed;
	}
	else
	{
		return EVisibility::Visible;
	}
}

void SLiveEditorConfigWindow::ConfigureDevice( PmDeviceID DeviceID )
{
	FLiveEditorManager::Get().ConfigureDevice( DeviceID );
	WizardActivated();
}

TSharedRef<ITableRow> SLiveEditorConfigWindow::OnGenerateWidgetForBlueprintList(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew( STableRow< TSharedPtr<FString> >, OwnerTable )
		.Padding(2.0f)
		[
			SNew(SLiveEditorBlueprint, Item, this)
		];
}

TSharedRef<ITableRow> SLiveEditorConfigWindow::OnGenerateWidgetForDeviceList(TSharedPtr<PmDeviceID> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew( STableRow< TSharedPtr<FString> >, OwnerTable )
		.Padding(5.0f)
		[
			SNew(SLiveEditorDeviceData, Item, this)
		];
}
