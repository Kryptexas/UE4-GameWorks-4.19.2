// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "MoviePlayerSettingsDetails.h"
#include "FilePathStructCustomization.h"
#include "SourceControlHelpers.h"

#define LOCTEXT_NAMESPACE "MoviePlayerSettingsDetails"

TSharedRef<IDetailCustomization> FMoviePlayerSettingsDetails::MakeInstance()
{
	return MakeShareable(new FMoviePlayerSettingsDetails);
}

bool FMoviePlayerSettingsDetails::HandlePathPicked(FString& InOutPath)
{
	bool bSucceeded = false;

	// sanitize the location of the chosen movies to the content/movies directory
	const FString MoviesBaseDir = FPaths::ConvertRelativePathToFull(FPaths::GameContentDir() + TEXT("Movies/"));
	const FString FullPath = FPaths::ConvertRelativePathToFull(InOutPath);
	if(FullPath.StartsWith(MoviesBaseDir))
	{
		// mark for add/checkout
		FText FailReason;
		bSucceeded = SourceControlHelpers::CheckoutOrMarkForAdd(InOutPath, LOCTEXT("MovieFileDescription", "movie"), FOnPostCheckOut(), FailReason);
		if(!bSucceeded)
		{
			FNotificationInfo Info(FailReason);
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
		// already in the movies dir, so just trim the path so we just have a partial path with no extension (the movie player expects this)
		InOutPath = FPaths::GetBaseFilename(FullPath.RightChop(MoviesBaseDir.Len()));
	}
	else
	{
		// ask the user if they want to import this movie
		FSuppressableWarningDialog::FSetupInfo Info( 
			LOCTEXT("ExternalMovieImportWarning", "This movie needs to be imported into your project, would you like to import the file now?"), 
			LOCTEXT("ExternalMovieImportTitle", "Import Movie"), 
			TEXT("ImportMovieIntoProject") );
		Info.ConfirmText = LOCTEXT("ExternalMovieImport_Confirm", "Import");
		Info.CancelText = LOCTEXT("ExternalMovieImport_Cancel", "Don't Import");

		FSuppressableWarningDialog ImportWarningDialog( Info );

		if(ImportWarningDialog.ShowModal() != FSuppressableWarningDialog::EResult::Cancel)
		{
			const FString FileName = FPaths::GetCleanFilename(InOutPath);
			const FString DestPath = MoviesBaseDir / FileName;

			FText FailReason;
			bSucceeded = SourceControlHelpers::CopyFileUnderSourceControl(DestPath, InOutPath, LOCTEXT("MovieFileDescription", "movie"), FailReason);
			if(!bSucceeded)
			{
				FNotificationInfo FailureInfo(FailReason);
				FailureInfo.ExpireDuration = 3.0f;
				FSlateNotificationManager::Get().AddNotification(FailureInfo);
			}
			// trim the path so we just have a partial path with no extension (the movie player expects this)
			InOutPath = FPaths::GetBaseFilename(DestPath.RightChop(MoviesBaseDir.Len()));
		}		
	}

	return bSucceeded;
}

void FMoviePlayerSettingsDetails::GenerateArrayElementWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	const UProperty* Property = StartupMoviesPropertyHandle->GetProperty();
	checkSlow(Property);
	FString FileFilterExtension = Property->GetMetaData( TEXT("FilePathFilter") );
	if( FileFilterExtension.IsEmpty() )
	{
		FileFilterExtension = TEXT("*");
	}

	TSharedPtr<SWidget> NewValueWidget = FFilePathStructCustomization::CreatePickerWidget(PropertyHandle, FileFilterExtension, FOnPathPicked::CreateSP(this, &FMoviePlayerSettingsDetails::HandlePathPicked));
	if(NewValueWidget.IsValid())
	{
		IDetailPropertyRow& FilePathRow = ChildrenBuilder.AddChildProperty( PropertyHandle );

		TSharedPtr<SWidget> NameWidget;
		TSharedPtr<SWidget> ValueWidget;
		FilePathRow.GetDefaultWidgets(NameWidget, ValueWidget);

		const bool bShowChildren = false;
		FilePathRow.CustomWidget(bShowChildren)
		.NameContent()
		[
			NameWidget.ToSharedRef()
		]
		.ValueContent()
		.MaxDesiredWidth(325.0f)
		[
			NewValueWidget.ToSharedRef()
		];
	}
};

void FMoviePlayerSettingsDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	IDetailCategoryBuilder& MoviesCategory = DetailLayout.EditCategory("Movies");

	StartupMoviesPropertyHandle = DetailLayout.GetProperty("StartupMovies");

	TSharedRef<FDetailArrayBuilder> StartupMoviesBuilder = MakeShareable( new FDetailArrayBuilder( StartupMoviesPropertyHandle.ToSharedRef() ) );
	StartupMoviesBuilder->OnGenerateArrayElementWidget( FOnGenerateArrayElementWidget::CreateSP(this, &FMoviePlayerSettingsDetails::GenerateArrayElementWidget) );

	const bool bForAdvanced = false;
	MoviesCategory.AddCustomBuilder( StartupMoviesBuilder, bForAdvanced );
}

#undef LOCTEXT_NAMESPACE