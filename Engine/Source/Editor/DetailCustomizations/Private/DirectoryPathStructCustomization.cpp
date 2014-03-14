// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "DirectoryPathStructCustomization.h"
#include "DesktopPlatformModule.h"
#include "ContentBrowserDelegates.h"

#define LOCTEXT_NAMESPACE "DirectoryPathStructCustomization"

TSharedRef<IStructCustomization> FDirectoryPathStructCustomization::MakeInstance()
{
	return MakeShareable(new FDirectoryPathStructCustomization());
}

void FDirectoryPathStructCustomization::CustomizeStructHeader( TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	TSharedPtr<IPropertyHandle> PathProperty = StructPropertyHandle->GetChildHandle("Path");
	if(PathProperty.IsValid())
	{
		HeaderRow.ValueContent()
		.MinDesiredWidth(125.0f)
		.MaxDesiredWidth(600.0f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				PathProperty->CreatePropertyValueWidget()
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
				.ToolTipText( LOCTEXT( "FolderButtonToolTipText", "Choose a directory from this computer").ToString() )
				.OnClicked( FOnClicked::CreateSP(this, &FDirectoryPathStructCustomization::OnPickDirectory, PathProperty.ToSharedRef()) )
				.ContentPadding( 2.0f )
				.ForegroundColor( FSlateColor::UseForeground() )
				.IsFocusable( false )
				[
					SNew( SImage )
					.Image( FEditorStyle::GetBrush("PropertyWindow.Button_Ellipsis") )
					.ColorAndOpacity( FSlateColor::UseForeground() )
				]
			]
		]
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		];
	}
}

void FDirectoryPathStructCustomization::CustomizeStructChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
}

FReply FDirectoryPathStructCustomization::OnPickDirectory(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	FString Directory;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if ( DesktopPlatform )
	{
		if(DesktopPlatform->OpenDirectoryDialog(NULL, LOCTEXT( "FolderDialogTitle", "Choose a directory").ToString(), FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT), Directory))
		{
			PropertyHandle->SetValue(Directory);
			FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_IMPORT, Directory);
		}
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE