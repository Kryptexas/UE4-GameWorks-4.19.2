// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "FilePathStructCustomization.h"
#include "DesktopPlatformModule.h"
#include "ContentBrowserDelegates.h"

#define LOCTEXT_NAMESPACE "FilePathStructCustomization"

TSharedRef<IStructCustomization> FFilePathStructCustomization::MakeInstance()
{
	return MakeShareable(new FFilePathStructCustomization());
}

void FFilePathStructCustomization::CustomizeStructHeader( TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	TSharedPtr<IPropertyHandle> PathProperty = StructPropertyHandle->GetChildHandle("FilePath");

	const UProperty* Property = StructPropertyHandle->GetProperty();
	checkSlow(Property);
	FileFilterExtension = Property->GetMetaData( TEXT("FilePathFilter") );
	if( FileFilterExtension.IsEmpty() )
	{
		FileFilterExtension = TEXT("*");
	}

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
				.ToolTipText( LOCTEXT( "FileButtonToolTipText", "Choose a file from this computer").ToString() )
				.OnClicked( FOnClicked::CreateSP(this, &FFilePathStructCustomization::OnPickFile, PathProperty.ToSharedRef()) )
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

void FFilePathStructCustomization::CustomizeStructChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
}

FReply FFilePathStructCustomization::OnPickFile(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if ( DesktopPlatform )
	{
		TArray<FString> OutFiles;
		const FString filter = FString::Printf(TEXT("%s files (*.%s)|*.%s"), *FileFilterExtension, *FileFilterExtension, *FileFilterExtension);
		if (DesktopPlatform->OpenFileDialog(NULL, LOCTEXT("PropertyEditorTitle", "File picker...").ToString(), TEXT(""), TEXT(""), filter, EFileDialogFlags::None, OutFiles))
		{
			PropertyHandle->SetValueFromFormattedString( OutFiles[0] );
		}
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE