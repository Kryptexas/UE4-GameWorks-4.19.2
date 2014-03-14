// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "KismetWidgetsPrivatePCH.h"
#include "SCreateBlueprintFromActorDialog.h"
#include "Editor/UnrealEd/Public/Kismet2/KismetEditorUtilities.h"

#include "ContentBrowserModule.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "Editor/UnrealEd/Public/PackageTools.h"

#define LOCTEXT_NAMESPACE "SCreateBlueprintFromActor"

void SCreateBlueprintFromActorDialog::OpenDialog(bool bInHarvest)
{
	TSharedPtr<SWindow> PickBlueprintPathWidget;
	SAssignNew(PickBlueprintPathWidget, SWindow)
		.Title(LOCTEXT("SelectPath", "Select Path"))
		.ToolTipText(LOCTEXT("SelectPathTooltip", "Select the path where the Blueprint will be created at"))
		.ClientSize(FVector2D(400, 400));

	TSharedPtr<SCreateBlueprintFromActorDialog> CreateBlueprintFromActorDialog;
	PickBlueprintPathWidget->SetContent
		(
		SAssignNew(CreateBlueprintFromActorDialog, SCreateBlueprintFromActorDialog, PickBlueprintPathWidget, bInHarvest)
		);

	TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
	FSlateApplication::Get().AddWindowAsNativeChild(PickBlueprintPathWidget.ToSharedRef(), RootWindow.ToSharedRef() );

	if (CreateBlueprintFromActorDialog.IsValid() && CreateBlueprintFromActorDialog->FileNameWidget.IsValid())
	{
		CreateBlueprintFromActorDialog->OnFilenameChanged(CreateBlueprintFromActorDialog->FileNameWidget.Pin()->GetText());
	}
}

void SCreateBlueprintFromActorDialog::Construct( const FArguments& InArgs, TSharedPtr< SWindow > InParentWindow, bool bInHarvest )
{
	USelection::SelectionChangedEvent.AddRaw(this, &SCreateBlueprintFromActorDialog::OnLevelSelectionChanged);

	bIsReportingError = false;
	PathForActorBlueprint = FString("/Game");
	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.DefaultPath = PathForActorBlueprint;
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateRaw(this, &SCreateBlueprintFromActorDialog::OnSelectBlueprintPath);

	// Set up PathPickerConfig.
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	ParentWindow = InParentWindow;

	FString PackageName;
	ActorInstanceLabel.Empty();

	USelection* SelectedActors = GEditor->GetSelectedActors();
	for(FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if(Actor)
		{
			ActorInstanceLabel += Actor->GetActorLabel();
			ActorInstanceLabel += TEXT( "_" );
		}
	}
	if( ActorInstanceLabel.IsEmpty() )
	{
		ActorInstanceLabel = LOCTEXT("BlueprintName_Default", "NewBlueprint").ToString();
	}
	else
	{
		ActorInstanceLabel += LOCTEXT("BlueprintName_Suffix", "Blueprint").ToString();
	}

	ActorInstanceLabel = PackageTools::SanitizePackageName(ActorInstanceLabel);

	FString AssetName = ActorInstanceLabel;
	FString BasePath = PathForActorBlueprint + TEXT("/") + AssetName;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(BasePath, TEXT(""), PackageName, AssetName);

	ChildSlot
		[
			SNew( SVerticalBox )
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig)
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew( SHorizontalBox )
				+SHorizontalBox::Slot()
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				.AutoWidth()
				[
					SNew(STextBlock)
						.Text(LOCTEXT("BlueprintNameLabel", "Blueprint Name:"))
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(FileNameWidget, SEditableTextBox)
						.Text(FText::FromString(AssetName))
						.OnTextChanged(this, &SCreateBlueprintFromActorDialog::OnFilenameChanged)
				]
			]
			+SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			.AutoHeight()
			[
				SNew( SHorizontalBox )
				+SHorizontalBox::Slot()
				.Padding( 0, 2, 6, 0 )
				.AutoWidth()
				[
					SNew(SButton)
					.VAlign( VAlign_Center )
					.HAlign( HAlign_Center )
					.OnClicked( this, &SCreateBlueprintFromActorDialog::OnCreateBlueprintFromActorClicked, bInHarvest)
					.IsEnabled( this, &SCreateBlueprintFromActorDialog::IsCreateBlueprintEnabled )
					[
						SNew( STextBlock )
							.Text( LOCTEXT("CreateBlueprintFromActorButton", "Create Blueprint") )
					]
				]
				+SHorizontalBox::Slot()
				.Padding( 0, 2, 0, 0 )
				.AutoWidth()
				[
					SNew(SButton)
					.VAlign( VAlign_Center )
					.HAlign( HAlign_Center )
					.ContentPadding( FMargin( 8, 2, 8, 2 ) )
					.OnClicked( this, &SCreateBlueprintFromActorDialog::OnCancelCreateBlueprintFromActor)
					[
						SNew( STextBlock )
							.Text( LOCTEXT("CancelBlueprintFromActorButton", "Cancel") )
					]
				]
			]
		];
}

FReply SCreateBlueprintFromActorDialog::OnCreateBlueprintFromActorClicked( bool bInHarvest)
{
	ParentWindow->RequestDestroyWindow();

	UBlueprint* Blueprint = NULL;

	if(bInHarvest)
	{
		TArray<AActor*> Actors;

		USelection* SelectedActors = GEditor->GetSelectedActors();
		for(FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
		{
			// We only care about actors that are referenced in the world for literals, and also in the same level as this blueprint
			AActor* Actor = Cast<AActor>(*Iter);
			if(Actor)
			{
				Actors.Add(Actor);
			}
		}
		Blueprint = FKismetEditorUtilities::HarvestBlueprintFromActors(PathForActorBlueprint + TEXT("/") + FileNameWidget.Pin()->GetText().ToString(), Actors, true);
	}
	else
	{
		TArray< UObject* > SelectedActors;
		GEditor->GetSelectedActors()->GetSelectedObjects(AActor::StaticClass(), SelectedActors);
		check(SelectedActors.Num());
		Blueprint = FKismetEditorUtilities::CreateBlueprintFromActor(PathForActorBlueprint + TEXT("/") + FileNameWidget.Pin()->GetText().ToString(), (AActor*)SelectedActors[0], true);
	}

	if(Blueprint)
	{
		// Rename new instance based on the original actor label rather than the asset name
		USelection* SelectedActors = GEditor->GetSelectedActors();
		if( SelectedActors && SelectedActors->Num() == 1 )
		{
			AActor* Actor = Cast<AActor>(SelectedActors->GetSelectedObject(0));
			if(Actor)
			{
				GEditor->SetActorLabelUnique(Actor, ActorInstanceLabel);
			}
		}

		TArray<UObject*> Objects;
		Objects.Add(Blueprint);
		GEditor->SyncBrowserToObjects( Objects );
	}
	else
	{
		FNotificationInfo Info( LOCTEXT("CreateBlueprintFromActorFailed", "Unable to create a blueprint from actor.") );
		Info.ExpireDuration = 3.0f;
		Info.bUseLargeFont = false;
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if ( Notification.IsValid() )
		{
			Notification->SetCompletionState( SNotificationItem::CS_Fail );
		}
	}

	return FReply::Handled();
}

FReply SCreateBlueprintFromActorDialog::OnCancelCreateBlueprintFromActor()
{
	ParentWindow->RequestDestroyWindow();
	return FReply::Handled();
}

void SCreateBlueprintFromActorDialog::OnSelectBlueprintPath( const FString& Path )
{
	PathForActorBlueprint = Path;
	OnFilenameChanged(FileNameWidget.Pin()->GetText());
}

void SCreateBlueprintFromActorDialog::OnLevelSelectionChanged(UObject* InObjectSelected)
{
	// When actor selection changes, this window is no longer valid.
	if(ParentWindow.IsValid())
	{
		ParentWindow->RequestDestroyWindow();
	}
}

void SCreateBlueprintFromActorDialog::OnFilenameChanged(const FText& InNewName)
{
	TArray<FAssetData> AssetData;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().GetAssetsByPath(FName(*PathForActorBlueprint), AssetData);

	FText ErrorText;
	if(!FEditorFileUtils::IsFilenameValidForSaving(InNewName.ToString(), ErrorText) || !FName(*InNewName.ToString()).IsValidObjectName( ErrorText ))
	{
		FileNameWidget.Pin()->SetError(ErrorText);
		bIsReportingError = true;
		return;
	}
	else
	{
		// Check to see if the name conflicts
		for( auto Iter = AssetData.CreateConstIterator(); Iter; ++Iter)
		{
			if(Iter->AssetName.ToString() == InNewName.ToString())
			{
				FileNameWidget.Pin()->SetError(LOCTEXT("AssetInUseError", "Asset name already in use!"));
				bIsReportingError = true;
				return;
			}
		}
	}

	FileNameWidget.Pin()->SetError(FText::FromString(TEXT("")));
	bIsReportingError = false;
}

bool SCreateBlueprintFromActorDialog::IsCreateBlueprintEnabled() const
{
	return !bIsReportingError;
}

#undef LOCTEXT_NAMESPACE
