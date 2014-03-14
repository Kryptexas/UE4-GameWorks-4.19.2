// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PackagesDialog.h"
#include "SPackagesDialog.h"
#include "Editor/UnrealEd/Public/PackageTools.h"
#include "AssetToolsModule.h"
#include "ISourceControlModule.h"

#define LOCTEXT_NAMESPACE "SPackagesDialog"

/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SPackagesDialog::Construct(const FArguments& InArgs)
{
	bReadOnly = InArgs._ReadOnly.Get();
	bAllowSourceControlConnection = InArgs._AllowSourceControlConnection.Get();
	Message = InArgs._Message;

	ButtonsBox = SNew( SHorizontalBox );

	if(bAllowSourceControlConnection)
	{
		ButtonsBox->AddSlot()
		.AutoWidth()
		.Padding( 2 )
		[
			SNew(SButton) 
				.Text(LOCTEXT("ConnectToSourceControl", "Connect To Source Control"))
				.ToolTipText(LOCTEXT("ConnectToSourceControl_Tooltip", "Connect to source control to allow source control operations to be performed on content and levels."))
				.ContentPadding(FMargin(10, 3))
				.HAlign(HAlign_Right)
				.Visibility(this, &SPackagesDialog::GetConnectToSourceControlVisibility)
				.OnClicked(this, &SPackagesDialog::OnConnectToSourceControlClicked)
		];
	}

	this->ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot() .Padding(10) .AutoHeight()
			[
				SNew( SHorizontalBox )
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew( STextBlock )
					.Text( this, &SPackagesDialog::GetMessage )
				]
			]
			+SVerticalBox::Slot()  .FillHeight(0.8)
			[
				SNew(SBorder)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot() .AutoHeight()
					[
						SNew(SBorder)
						.BorderImage( FEditorStyle::GetBrush( TEXT("PackageDialog.ListHeader") ) )
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot().AutoWidth() .VAlign(VAlign_Center) .HAlign(HAlign_Center)
							[
								SAssignNew( ToggleSelectedCheckBox, SCheckBox )
									.IsChecked(this, &SPackagesDialog::GetToggleSelectedState)
									.OnCheckStateChanged(this, &SPackagesDialog::OnToggleSelectedCheckBox)
								.Visibility( bReadOnly ? EVisibility::Collapsed : EVisibility::Visible )
							]
							+SHorizontalBox::Slot() .Padding(FMargin(10, 0, 0, 0)) .VAlign(VAlign_Center) .HAlign(HAlign_Left) .FillWidth(0.65)
							[
								SNew(STextBlock) 
									.Text(NSLOCTEXT("FileDialogModule", "File", "File"))
							]
							+SHorizontalBox::Slot() .VAlign(VAlign_Center) .HAlign(HAlign_Right) .FillWidth(0.35)
							[
								SNew(STextBlock) 
								.Text(NSLOCTEXT("TypeDialogModule", "Type", "Type"))
							]
						]
					]
					+SVerticalBox::Slot()
					[
						SAssignNew(ItemListView, SListView< TSharedPtr<FPackageItem> >)
							.ListItemsSource(&Items)
							.OnGenerateRow(this, &SPackagesDialog::MakePackageListItemWidget)
							.OnContextMenuOpening(this, &SPackagesDialog::MakePackageListContextMenu)
							.ItemHeight(20)
					]
				]
			]
			+SVerticalBox::Slot() .AutoHeight() .Padding(2) .HAlign(HAlign_Right) .VAlign(VAlign_Bottom)
			[
				ButtonsBox.ToSharedRef()
			]
		]
	];
}

/**
 * Adds a new checkbox item to the dialog
 *
 * @param	Item	The item to be added
 */
void SPackagesDialog::Add(TSharedPtr<FPackageItem> Item)
{
	FSimpleDelegate RefreshCallback = FSimpleDelegate::CreateSP(this, &SPackagesDialog::RefreshButtons);
	Item->SetRefreshCallback(RefreshCallback);
	Items.Add(Item);
	ItemListView->RequestListRefresh();
}

/**
 * Adds a new button to the dialog
 *
 * @param	Button	The button to be added
 */
void SPackagesDialog::AddButton(TSharedPtr<FPackageButton> Button)
{
	Buttons.Add(Button);

	ButtonsBox->AddSlot()
	.AutoWidth()
	.Padding( 2 )
	[
		SNew(SButton) 
			.Text(Button->GetName())
			.ContentPadding(FMargin(10, 3))
			.ToolTipText(Button->GetToolTip())
			.IsEnabled(Button.Get(), &FPackageButton::IsEnabled )
			.HAlign(HAlign_Right)
			.OnClicked(Button.Get(), &FPackageButton::OnButtonClicked)
	];
}

/**
 * Sets the message of the widget
 *
 * @param	InMessage	The string that the message should be set to
 */
void SPackagesDialog::SetMessage(const FText& InMessage)
{
	Message = InMessage;
}

/**
 * Gets the return type of the dialog and it populates the package array results
 *
 * @param	OutCheckedPackages		Will be populated with the checked packages
 * @param	OutUncheckedPackages	Will be populated with the unchecked packages
 * @param	OutUndeterminedPackages	Will be populated with the undetermined packages
 *
 * @return	returns the button that was pressed to remove the dialog
 */
EDialogReturnType SPackagesDialog::GetReturnType(OUT TArray<UPackage*>& OutCheckedPackages, OUT TArray<UPackage*>& OutUncheckedPackages, OUT TArray<UPackage*>& OutUndeterminedPackages)
{
	/** Set the return type to which button was pressed */
	EDialogReturnType ReturnType = DRT_None;
	for( int32 ButtonIndex = 0; ButtonIndex < Buttons.Num(); ++ButtonIndex)
	{
		FPackageButton& Button = *Buttons[ButtonIndex];
		if(Button.IsClicked())
		{
			ReturnType = Button.GetType();
			break;
		}
	}

	/** Populate the results */
	if(ReturnType != DRT_Cancel && ReturnType != DRT_None)
	{
		for( int32 ItemIndex = 0; ItemIndex < Items.Num(); ++ItemIndex)
		{
			FPackageItem& item = *Items[ItemIndex];
			if(item.GetState() == ESlateCheckBoxState::Checked)
			{
				OutCheckedPackages.Add(item.GetPackage());
			}
			else if(item.GetState() == ESlateCheckBoxState::Unchecked)
			{
				OutUncheckedPackages.Add(item.GetPackage());
			}
			else
			{
				OutUndeterminedPackages.Add(item.GetPackage());
			}
		}
	}

	return ReturnType;
}

/**
 * Gets the widget which is to have keyboard focus on activating the dialog
 *
 * @return	returns the widget
 */
TSharedPtr< SWidget > SPackagesDialog::GetWidgetToFocusOnActivate() const
{
	return ButtonsBox->GetChildren()->Num() > 0 ? ButtonsBox->GetChildren()->GetChildAt( 0 ) : TSharedPtr< SWidget >();
}

/**
 * Called when the checkbox items have changed state
 */
void SPackagesDialog::RefreshButtons()
{
	int32 CheckedItems = 0;
	int32 UncheckedItems = 0;
	int32 UndeterminedItems = 0;

	/** Count the number of checkboxes that we have for each state */
	for( int32 ItemIndex = 0; ItemIndex < Items.Num(); ++ItemIndex)
	{
		FPackageItem& item = *Items[ItemIndex];
		if(item.GetState() == ESlateCheckBoxState::Checked)
		{
			CheckedItems++;
		}
		else if(item.GetState() == ESlateCheckBoxState::Unchecked)
		{
			UncheckedItems++;
		}
		else
		{
			UndeterminedItems++;
		}
	}

	/** Change the button state based on our selection */
	for( int32 ButtonIndex = 0; ButtonIndex < Buttons.Num(); ++ButtonIndex)
	{
		FPackageButton& Button = *Buttons[ButtonIndex];
		if(Button.GetType() == DRT_MakeWritable)
		{
			if(UndeterminedItems > 0 || CheckedItems > 0)
				Button.SetDisabled(false);
			else
				Button.SetDisabled(true);
		}
		else if(Button.GetType() == DRT_CheckOut)
		{
			if(CheckedItems > 0)
				Button.SetDisabled(false);
			else
				Button.SetDisabled(true);
		}
	}
}

/** 
 * Makes the widget for the checkbox items in the list view 
 */
TSharedRef<ITableRow> SPackagesDialog::MakePackageListItemWidget(TSharedPtr<FPackageItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(Item.IsValid());

	// Choose the icon based on the severity
	const FSlateBrush* IconBrush = FEditorStyle::GetBrush( *( Item->GetIconName() ) );

	// Set up checkbox paddings
	const FMargin PackageMargin(10, 0, 0, 0);
	const FMargin TypeMargin(5, 0, 0, 0);

	// Extract the type and color for the package
	FColor PackageColor;
	FString PackageType;
	const FString PackageName = Item->GetName();
	const UObject* Object = GetPackageObject( Item );
	if ( Object )
	{
		// Load the asset tools module to get access to the class color
		const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TWeakPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass( Object->GetClass() );
		if ( AssetTypeActions.IsValid() )
		{
			const FColor EngineBorderColor = AssetTypeActions.Pin()->GetTypeColor();
			PackageColor = FColor(					// Copied from ContentBrowserCLR.cpp
				127 + EngineBorderColor.R / 2,		// Desaturate the colors a bit (GB colors were too.. much)
				127 + EngineBorderColor.G / 2,
				127 + EngineBorderColor.B / 2, 
				200);		// Opacity
			PackageType += FString ( TEXT( "(" ) ) + AssetTypeActions.Pin()->GetName().ToString() + FString ( TEXT( ")" ) );	// Only set the text if we have a valid color to display it
		}
	}

	TSharedPtr<SWidget> ItemContentWidget;

	if ( bReadOnly )
	{
		// If read only, just display a text block
		ItemContentWidget = SNew(SHorizontalBox)
			.ToolTipText(Item->GetToolTip())

			+SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SImage) .Image( IconBrush ) .IsEnabled(!Item->IsDisabled())
			]
			+SHorizontalBox::Slot() .Padding(PackageMargin) .VAlign(VAlign_Center) .HAlign(HAlign_Left) .FillWidth(1)
			[
				SNew(STextBlock) .Text(PackageName) .IsEnabled(!Item->IsDisabled())
			]
			+SHorizontalBox::Slot().AutoWidth() .Padding(TypeMargin) .VAlign(VAlign_Center) .HAlign(HAlign_Right) .AutoWidth()
			[
				SNew(STextBlock) .Text(PackageType) .IsEnabled(!Item->IsDisabled()) .ColorAndOpacity(PackageColor)
			];
	}
	else
	{
		// Otherwise, display a checkbox
		ItemContentWidget = SNew(SHorizontalBox)
			.ToolTipText(Item->GetToolTip())
			
			+SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SCheckBox)
				.IsChecked(Item.Get(), &FPackageItem::OnGetDisplayCheckState)
				.OnCheckStateChanged(Item.Get(), &FPackageItem::OnDisplayCheckStateChanged)
			]
			+SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SImage) .Image( IconBrush ) .IsEnabled(!Item->IsDisabled())
			]
			+SHorizontalBox::Slot() .Padding(PackageMargin) .VAlign(VAlign_Center) .HAlign(HAlign_Left) .FillWidth(1)
			[
				SNew(STextBlock) .Text(PackageName) .IsEnabled(!Item->IsDisabled())
			]
			+SHorizontalBox::Slot().AutoWidth() .Padding(TypeMargin) .VAlign(VAlign_Center) .HAlign(HAlign_Right) .AutoWidth()
			[
				SNew(STextBlock) .Text(PackageType) .IsEnabled(!Item->IsDisabled()) .ColorAndOpacity(PackageColor)
			];
	}

	return
	SNew(STableRow< TSharedPtr<FPackageItem> >, OwnerTable)
	.Padding(FMargin(2, 0))
	//.IsEnabled(!Item->IsDisabled()) // if we disable the item we will not be able to select it when it in undetermined state
	[
		ItemContentWidget.ToSharedRef()
	];
}

TSharedPtr<SWidget> SPackagesDialog::MakePackageListContextMenu() const
{
	FMenuBuilder MenuBuilder( true, NULL );

	const TArray< TSharedPtr<FPackageItem> > SelectedItems = GetSelectedItems( false );
	if( SelectedItems.Num() > 0 )
	{	
		MenuBuilder.BeginSection("FilePackage", LOCTEXT("PackageHeading", "Asset"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("SCCDiffAgainstDepot", "Diff Against Depot"),
				LOCTEXT("SCCDiffAgainstDepotTooltip", "Look at differences between your version of the asset and that in source control."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP( this, &SPackagesDialog::ExecuteSCCDiffAgainstDepot ),
					FCanExecuteAction::CreateSP( this, &SPackagesDialog::CanExecuteSCCDiffAgainstDepot )
				)
			);	
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

bool SPackagesDialog::CanExecuteSCCDiffAgainstDepot() const
{
	return ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable();
}

void SPackagesDialog::ExecuteSCCDiffAgainstDepot() const
{
	const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	const TArray< TSharedPtr<FPackageItem> > SelectedItems = GetSelectedItems( false );
	for(int32 ItemIdx=0; ItemIdx<SelectedItems.Num(); ItemIdx++)
	{
		const TSharedPtr<FPackageItem> SelectedItem = SelectedItems[ItemIdx];
		check( SelectedItem.IsValid() );

		UObject* Object = GetPackageObject( SelectedItem );
		if( Object )
		{
			const FString PackagePath = SelectedItem->GetName();
			const FString PackageName = FPaths::GetBaseFilename( PackagePath );
			AssetToolsModule.Get().DiffAgainstDepot( Object, PackagePath, PackageName );
		}
	}
}

UObject* SPackagesDialog::GetPackageObject( const TSharedPtr<FPackageItem> Item ) const
{
	check( Item.IsValid() );
	const FString PackageName = Item->GetName();
	const bool bIsLegacyOrMapPackage = !PackageTools::IsSingleAssetPackage(PackageName);
	if ( !bIsLegacyOrMapPackage && PackageName.StartsWith(TEXT("/Temp/Untitled")) == false )
	{
		// Get the object which belongs in this package (there has to be a quicker function than GetObjectsInPackages!)
		TArray<UPackage*> Packages;
		Packages.Add( Item->GetPackage() );
		TArray<UObject*> ObjectsInPackages;
		PackageTools::GetObjectsInPackages(&Packages, ObjectsInPackages);
		return ( ObjectsInPackages.Num() > 0 ? ObjectsInPackages.Last() : NULL );
	}
	return NULL;
}

TArray< TSharedPtr<FPackageItem> > SPackagesDialog::GetSelectedItems( bool bAllIfNone ) const
{
	//get the list of highlighted packages
	TArray< TSharedPtr<FPackageItem> > SelectedItems = ItemListView->GetSelectedItems();
	if ( SelectedItems.Num() == 0 && bAllIfNone )
	{
		//If no packages are explicitly highlighted, return all packages in the list.
		SelectedItems = Items;
	}

	return SelectedItems;
}

ESlateCheckBoxState::Type SPackagesDialog::GetToggleSelectedState() const
{
	//default to a Checked state
	ESlateCheckBoxState::Type PendingState = ESlateCheckBoxState::Checked;

	TArray< TSharedPtr<FPackageItem> > SelectedItems = GetSelectedItems( true );

	//Iterate through the list of selected packages
	for ( auto SelectedItem = SelectedItems.CreateConstIterator(); SelectedItem; ++SelectedItem )
	{
		if ( SelectedItem->Get()->GetState() == ESlateCheckBoxState::Unchecked )
		{
			//if any package in the selection is Unchecked, then represent the entire set of highlighted packages as Unchecked,
			//so that the first (user) toggle of ToggleSelectedCheckBox consistently Checks all highlighted packages
			PendingState = ESlateCheckBoxState::Unchecked;
		}
	}

	return PendingState;
}

void SPackagesDialog::OnToggleSelectedCheckBox(ESlateCheckBoxState::Type InNewState)
{
	TArray< TSharedPtr<FPackageItem> > SelectedItems = GetSelectedItems( true );

	for ( auto SelectedItem = SelectedItems.CreateConstIterator(); SelectedItem; ++SelectedItem )
	{
		FPackageItem *item = (*SelectedItem).Get();
		if (InNewState == ESlateCheckBoxState::Checked)
		{
			if(item->IsDisabled())
			{
				item->SetState(ESlateCheckBoxState::Undetermined);
			}
			else
			{
				item->SetState(ESlateCheckBoxState::Checked);
			}
		}
		else
		{
			item->SetState(ESlateCheckBoxState::Unchecked);
		}
	}

	ItemListView->RequestListRefresh();
}

FReply SPackagesDialog::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	if( InKeyboardEvent.GetKey() == EKeys::Escape )
	{
		for( int32 ButtonIndex = 0; ButtonIndex < Buttons.Num(); ++ButtonIndex )
		{
			FPackageButton& Button = *Buttons[ ButtonIndex ];
			if( Button.GetType() == DRT_Cancel )
			{
				return Button.OnButtonClicked();
			}
		}
	}

	return SCompoundWidget::OnKeyDown( MyGeometry, InKeyboardEvent );
}

EVisibility SPackagesDialog::GetConnectToSourceControlVisibility() const
{
	if(bAllowSourceControlConnection)
	{
		if(!ISourceControlModule::Get().IsEnabled() || !ISourceControlModule::Get().GetProvider().IsAvailable())
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

FReply SPackagesDialog::OnConnectToSourceControlClicked() const
{
	ISourceControlModule::Get().ShowLoginDialog(FSourceControlLoginClosed(), ELoginWindowMode::Modal);
	return FReply::Handled();
}

void SPackagesDialog::PopulateIgnoreForSaveItems( const TSet<FString>& InIgnorePackages )
{
	for( auto ItItem=Items.CreateIterator(); ItItem; ++ItItem )
	{
		const FString& ItemName = (*ItItem)->GetName();

		const ESlateCheckBoxState::Type CheckedStatus = (InIgnorePackages.Find(ItemName) != NULL) ? ESlateCheckBoxState::Unchecked : ESlateCheckBoxState::Checked;

		(*ItItem)->SetState( CheckedStatus );
	}
}

void SPackagesDialog::PopulateIgnoreForSaveArray( OUT TSet<FString>& InOutIgnorePackages ) const
{
	for( auto ItItem=Items.CreateConstIterator(); ItItem; ++ItItem )
	{
		if((*ItItem)->GetState()==ESlateCheckBoxState::Unchecked)
		{
			InOutIgnorePackages.Add( (*ItItem)->GetName() );
		}
		else
		{
			InOutIgnorePackages.Remove( (*ItItem)->GetName() );
		}
	}
}

void SPackagesDialog::Reset()
{
	for( int32 ButtonIndex = 0; ButtonIndex < Buttons.Num(); ++ButtonIndex)
	{
		Buttons[ButtonIndex]->Reset();
	}
}

FText SPackagesDialog::GetMessage() const
{
	return Message;
}

#undef LOCTEXT_NAMESPACE
