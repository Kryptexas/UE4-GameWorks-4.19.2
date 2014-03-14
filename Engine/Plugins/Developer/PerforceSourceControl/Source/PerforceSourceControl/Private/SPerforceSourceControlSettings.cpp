// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PerforceSourceControlPrivatePCH.h"
#include "SPerforceSourceControlSettings.h"
#include "PerforceSourceControlModule.h"

#define LOCTEXT_NAMESPACE "SPerforceSourceControlSettings"

void SPerforceSourceControlSettings::Construct(const FArguments& InArgs)
{
	FPerforceSourceControlModule& PerforceSourceControl = FModuleManager::LoadModuleChecked<FPerforceSourceControlModule>( "PerforceSourceControl" );

	// check our settings & query if we don't already have any
	FString HostName = PerforceSourceControl.AccessSettings().GetHost();
	FString UserName = PerforceSourceControl.AccessSettings().GetUserName();

	if ( HostName.IsEmpty() && UserName.IsEmpty() )
	{
		ClientApi TestP4;
		Error P4Error;
		TestP4.Init(&P4Error);
		HostName = ANSI_TO_TCHAR(TestP4.GetPort().Text());
		UserName = ANSI_TO_TCHAR(TestP4.GetUser().Text());
		TestP4.Final(&P4Error);

		PerforceSourceControl.AccessSettings().SetHost(HostName);
		PerforceSourceControl.AccessSettings().SetUserName(UserName);
		PerforceSourceControl.SaveSettings();
	}

	FSlateFontInfo Font = FEditorStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font"));

	ChildSlot
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("HostLabel", "Host").ToString())
				.ToolTipText( LOCTEXT("HostLabel_Tooltip", "The host and port for your Perforce server. Usage hostname:1234.").ToString() )
				.Font(Font)
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("UserNameLabel", "User Name").ToString())
				.ToolTipText( LOCTEXT("UserNameLabel_Tooltip", "Perforce username.").ToString() )
				.Font(Font)
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("WorkspaceLabel", "Workspace").ToString())
				.ToolTipText( LOCTEXT("WorkspaceLabel_Tooltip", "Perforce workspace.").ToString() )
				.Font(Font)
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AutoWorkspaces", "Available Workspaces").ToString())
				.ToolTipText( LOCTEXT("AutoWorkspaces_Tooltip", "Choose from a list of available workspaces. Requires a host and username before use.").ToString() )
				.Font(Font)
			]
		]
		+SHorizontalBox::Slot()
		.FillWidth(2.0f)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SPerforceSourceControlSettings::GetHostText)
				.ToolTipText( LOCTEXT("HostLabel_Tooltip", "The host and port for your Perforce server. Usage hostname:1234.").ToString() )
				.OnTextCommitted(this, &SPerforceSourceControlSettings::OnHostTextCommited)
				.OnTextChanged(this, &SPerforceSourceControlSettings::OnHostTextCommited, ETextCommit::Default)
				.Font(Font)
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SPerforceSourceControlSettings::GetUserNameText)
				.ToolTipText( LOCTEXT("UserNameLabel_Tooltip", "Perforce username.").ToString() )
				.OnTextCommitted(this, &SPerforceSourceControlSettings::OnUserNameTextCommited)
				.OnTextChanged(this, &SPerforceSourceControlSettings::OnUserNameTextCommited, ETextCommit::Default)
				.Font(Font)
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SPerforceSourceControlSettings::GetWorkspaceText)
				.ToolTipText( LOCTEXT("WorkspaceLabel_Tooltip", "Perforce workspace.").ToString() )
				.OnTextCommitted(this, &SPerforceSourceControlSettings::OnWorkspaceTextCommited)
				.OnTextChanged(this, &SPerforceSourceControlSettings::OnWorkspaceTextCommited, ETextCommit::Default)
				.Font(Font)
			]			
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				SAssignNew(WorkspaceCombo, SComboButton)
				.OnGetMenuContent(this, &SPerforceSourceControlSettings::OnGetMenuContent)
				.ContentPadding(1)
				.ToolTipText( LOCTEXT("AutoWorkspaces_Tooltip", "Choose from a list of available workspaces. Requires a host and username before use.").ToString() )
				.ButtonContent()
				[
					SNew( STextBlock )
					.Text( this, &SPerforceSourceControlSettings::OnGetButtonText )
					.Font(Font)
				]
			]			
		]	
	];

	// fire off the workspace query
	State = ESourceControlOperationState::NotQueried;
	QueryWorkspaces();
}

FText SPerforceSourceControlSettings::GetHostText() const
{
	FPerforceSourceControlModule& PerforceSourceControl = FModuleManager::LoadModuleChecked<FPerforceSourceControlModule>( "PerforceSourceControl" );
	return FText::FromString(PerforceSourceControl.AccessSettings().GetHost());
}

void SPerforceSourceControlSettings::OnHostTextCommited(const FText& InText, ETextCommit::Type InCommitType) const
{
	FPerforceSourceControlModule& PerforceSourceControl = FModuleManager::LoadModuleChecked<FPerforceSourceControlModule>( "PerforceSourceControl" );
	PerforceSourceControl.AccessSettings().SetHost(InText.ToString());
	PerforceSourceControl.SaveSettings();
}

FText SPerforceSourceControlSettings::GetUserNameText() const
{
	FPerforceSourceControlModule& PerforceSourceControl = FModuleManager::LoadModuleChecked<FPerforceSourceControlModule>( "PerforceSourceControl" );
	return FText::FromString(PerforceSourceControl.AccessSettings().GetUserName());
}

void SPerforceSourceControlSettings::OnUserNameTextCommited(const FText& InText, ETextCommit::Type InCommitType) const
{
	FPerforceSourceControlModule& PerforceSourceControl = FModuleManager::LoadModuleChecked<FPerforceSourceControlModule>( "PerforceSourceControl" );
	PerforceSourceControl.AccessSettings().SetUserName(InText.ToString());
	PerforceSourceControl.SaveSettings();
}

FText SPerforceSourceControlSettings::GetWorkspaceText() const
{
	FPerforceSourceControlModule& PerforceSourceControl = FModuleManager::LoadModuleChecked<FPerforceSourceControlModule>( "PerforceSourceControl" );
	return FText::FromString(PerforceSourceControl.AccessSettings().GetWorkspace());
}

void SPerforceSourceControlSettings::OnWorkspaceTextCommited(const FText& InText, ETextCommit::Type InCommitType) const
{
	FPerforceSourceControlModule& PerforceSourceControl = FModuleManager::LoadModuleChecked<FPerforceSourceControlModule>( "PerforceSourceControl" );
	PerforceSourceControl.AccessSettings().SetWorkspace(InText.ToString());
	PerforceSourceControl.SaveSettings();
}

void SPerforceSourceControlSettings::QueryWorkspaces()
{
	if(State != ESourceControlOperationState::Querying)
	{
		Workspaces.Empty();
		CurrentWorkspace = FString();

		// fire off the workspace query
		ISourceControlModule& SourceControl = FModuleManager::LoadModuleChecked<ISourceControlModule>( "SourceControl" );
		ISourceControlProvider& Provider = SourceControl.GetProvider();
		GetWorkspacesOperation = ISourceControlOperation::Create<FGetWorkspaces>();
		Provider.Execute(GetWorkspacesOperation.ToSharedRef(), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SPerforceSourceControlSettings::OnSourceControlOperationComplete) );

		State = ESourceControlOperationState::Querying;
	}
}

void SPerforceSourceControlSettings::OnSourceControlOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	if(InResult == ECommandResult::Succeeded)
	{
		check(InOperation->GetName() == "GetWorkspaces");
		check(GetWorkspacesOperation == StaticCastSharedRef<FGetWorkspaces>(InOperation));

		// refresh workspaces list from operation results
		Workspaces.Empty();
		for(auto Iter(GetWorkspacesOperation->Results.CreateConstIterator()); Iter; Iter++)
		{
			Workspaces.Add(MakeShareable(new FString(*Iter)));
		}
	}

	GetWorkspacesOperation.Reset();
	State = ESourceControlOperationState::Queried;
}

TSharedRef<SWidget> SPerforceSourceControlSettings::OnGetMenuContent()
{
	// fire off the workspace query - we may have just edited the settings
	QueryWorkspaces();
	
	return
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(1)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SPerforceSourceControlSettings::GetThrobberVisibility)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SThrobber)
			]
			+SHorizontalBox::Slot()
			.FillWidth(1)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("WorkspacesOperationInProgress", "Looking for Perforce workspaces..."))
				.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))	
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.OnClicked(this, &SPerforceSourceControlSettings::OnCancelWorkspacesRequest)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CancelButtonLabel", "Cancel"))
				]
			]
		]
		+SHorizontalBox::Slot()
		.FillWidth(1)
		.Padding(2.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoWorkspaces", "No Workspaces found!"))
			.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))	
			.Visibility(this, &SPerforceSourceControlSettings::GetNoWorkspacesVisibility)
		]
		+SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SNew(SListView< TSharedRef<FString> >)
			.ListItemsSource(&Workspaces)
			.OnGenerateRow(this, &SPerforceSourceControlSettings::OnGenerateWorkspaceRow)
			.Visibility(this, &SPerforceSourceControlSettings::GetWorkspaceListVisibility)
			.OnSelectionChanged(this, &SPerforceSourceControlSettings::OnWorkspaceSelected)
		];
}

EVisibility SPerforceSourceControlSettings::GetThrobberVisibility() const
{
	return State == ESourceControlOperationState::Querying ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SPerforceSourceControlSettings::GetNoWorkspacesVisibility() const
{
	return State == ESourceControlOperationState::Queried && Workspaces.Num() == 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SPerforceSourceControlSettings::GetWorkspaceListVisibility() const
{
	return State == ESourceControlOperationState::Queried && Workspaces.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<ITableRow> SPerforceSourceControlSettings::OnGenerateWorkspaceRow(TSharedRef<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew(SComboRow< TSharedRef<FString> >, OwnerTable)
		.RowContent( 
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding(2.0f)
			[
				SNew(STextBlock)
				.Text(*InItem)
				.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
		);
}

void SPerforceSourceControlSettings::OnWorkspaceSelected(TSharedPtr<FString> InItem, ESelectInfo::Type InSelectInfo)
{
	CurrentWorkspace = *InItem;
	FPerforceSourceControlModule& PerforceSourceControl = FModuleManager::LoadModuleChecked<FPerforceSourceControlModule>( "PerforceSourceControl" );
	PerforceSourceControl.AccessSettings().SetWorkspace(CurrentWorkspace);
	PerforceSourceControl.SaveSettings();
	WorkspaceCombo->SetIsOpen(false);
}

FString SPerforceSourceControlSettings::OnGetButtonText() const
{
	return CurrentWorkspace;
}

FReply SPerforceSourceControlSettings::OnCancelWorkspacesRequest()
{
	if(GetWorkspacesOperation.IsValid())
	{
		ISourceControlModule& SourceControl = FModuleManager::LoadModuleChecked<ISourceControlModule>( "SourceControl" );
		SourceControl.GetProvider().CancelOperation(GetWorkspacesOperation.ToSharedRef());
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE