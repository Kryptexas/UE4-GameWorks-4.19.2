// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "FilePathStructCustomization.h"
#include "DesktopPlatformModule.h"
#include "ContentBrowserDelegates.h"

#define LOCTEXT_NAMESPACE "FilePathStructCustomization"

class SFilePathPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SFilePathPicker ){}

	SLATE_ARGUMENT(FOnPathPicked, OnPathPicked)

	SLATE_ARGUMENT(FString, FileFilterExtension)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<IPropertyHandle> InStringProperty)
	{
		StringProperty = InStringProperty;
		FileFilterExtension = InArgs._FileFilterExtension;
		OnPathPicked = InArgs._OnPathPicked;

		ChildSlot
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				StringProperty->CreatePropertyValueWidget()
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
				.ToolTipText( LOCTEXT( "FileButtonToolTipText", "Choose a file from this computer").ToString() )
				.OnClicked( FOnClicked::CreateSP(this, &SFilePathPicker::OnPickFile) )
				.ContentPadding( 2.0f )
				.ForegroundColor( FSlateColor::UseForeground() )
				.IsFocusable( false )
				[
					SNew( SImage )
					.Image( FEditorStyle::GetBrush("PropertyWindow.Button_Ellipsis") )
					.ColorAndOpacity( FSlateColor::UseForeground() )
				]
			]
		];
	}

private:
	/** Delegate handling the picker button being clicked */
	FReply OnPickFile()
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if ( DesktopPlatform )
		{
			const FString DefaultPath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_OPEN);

			TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
			void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid()) ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;

			TArray<FString> OutFiles;
			const FString filter = FString::Printf(TEXT("%s files (*.%s)|*.%s"), *FileFilterExtension, *FileFilterExtension, *FileFilterExtension);
			if (DesktopPlatform->OpenFileDialog(ParentWindowHandle, LOCTEXT("PropertyEditorTitle", "File picker...").ToString(), DefaultPath, TEXT(""), filter, EFileDialogFlags::None, OutFiles))
			{
				FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_OPEN, FPaths::GetPath(OutFiles[0]));

				bool bSetProperty = true;
				if(OnPathPicked.IsBound())
				{
					bSetProperty = OnPathPicked.Execute(OutFiles[0]);
				}

				if(bSetProperty)
				{
					StringProperty->SetValueFromFormattedString( OutFiles[0] );
				}
			}
		}

		return FReply::Handled();
	}

private:

	/** Delegate fired when a path is picked */
	FOnPathPicked OnPathPicked;

	/** The property we are representing */
	TSharedPtr<IPropertyHandle> StringProperty;

	/** Extension to filter files with */
	FString FileFilterExtension;
};

TSharedRef<IStructCustomization> FFilePathStructCustomization::MakeInstance()
{
	return MakeShareable(new FFilePathStructCustomization());
}

void FFilePathStructCustomization::CustomizeStructHeader( TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	const UProperty* Property = StructPropertyHandle->GetProperty();
	checkSlow(Property);

	TSharedPtr<SWidget> PickerWidget = CreatePickerWidget(StructPropertyHandle, Property->GetMetaData( TEXT("FilePathFilter") ));

	if(PickerWidget.IsValid())
	{
		HeaderRow.ValueContent()
		.MinDesiredWidth(125.0f)
		.MaxDesiredWidth(600.0f)
		[
			PickerWidget.ToSharedRef()
		]
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		];
	}
}

TSharedPtr<SWidget> FFilePathStructCustomization::CreatePickerWidget(TSharedRef<class IPropertyHandle> StructPropertyHandle, const FString& FileFilterExtension, const FOnPathPicked& OnPathPicked)
{
	TSharedPtr<IPropertyHandle> PathProperty = StructPropertyHandle->GetChildHandle("FilePath");
	if(PathProperty.IsValid())
	{
		return SNew(SFilePathPicker, PathProperty.ToSharedRef())
			.OnPathPicked(OnPathPicked)
			.FileFilterExtension(FileFilterExtension);
	}

	return TSharedPtr<SWidget>();
}

void FFilePathStructCustomization::CustomizeStructChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
}


#undef LOCTEXT_NAMESPACE