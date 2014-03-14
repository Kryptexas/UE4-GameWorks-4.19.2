// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PropertyEditorPrivatePCH.h"
#include "SDetailNameArea.h"
#include "ClassIconFinder.h"
#include "Editor/EditorWidgets/Public/EditorWidgets.h"
#include "SourceCodeNavigation.h"

#define LOCTEXT_NAMESPACE "SDetailsView"

void SDetailNameArea::Construct( const FArguments& InArgs, const TArray< TWeakObjectPtr<UObject> >* SelectedObjects )
{
	OnLockButtonClicked = InArgs._OnLockButtonClicked;
	IsLocked = InArgs._IsLocked;
	SelectionTip = InArgs._SelectionTip;
	bShowLockButton = InArgs._ShowLockButton;
	bShowActorLabel = InArgs._ShowActorLabel;
	Refresh( *SelectedObjects );
}

void SDetailNameArea::Refresh( const TArray< TWeakObjectPtr<UObject> >& SelectedObjects )
{
	ChildSlot
	[
		BuildObjectNameArea( SelectedObjects )
	];
}

void SDetailNameArea::Refresh( const TArray< TWeakObjectPtr<AActor> >& SelectedActors )
{
	// Convert the actor array to base object type
	TArray< TWeakObjectPtr<UObject> > SelectedObjects;
	for( int32 ActorIndex = 0 ; ActorIndex < SelectedActors.Num() ; ++ActorIndex )
	{
		const TWeakObjectPtr<UObject> ObjectWeakPtr = SelectedActors[ActorIndex].Get();
		SelectedObjects.Add( ObjectWeakPtr );
	}

	Refresh( SelectedObjects );
}

const FSlateBrush* SDetailNameArea::OnGetLockButtonImageResource() const
{
	if( IsLocked.Get() )
	{
		return FEditorStyle::GetBrush(TEXT("PropertyWindow.Locked"));
	}
	else
	{
		return FEditorStyle::GetBrush(TEXT("PropertyWindow.Unlocked"));
	}
}

TSharedRef< SWidget > SDetailNameArea::BuildObjectNameArea( const TArray< TWeakObjectPtr<UObject> >& SelectedObjects )
{
	// Get the common base class of the selected objects
	UClass* BaseClass = NULL;
	for( int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex )
	{
		TWeakObjectPtr<UObject> ObjectWeakPtr = SelectedObjects[ObjectIndex];
		if( ObjectWeakPtr.IsValid() )
		{
			UClass* ObjClass = ObjectWeakPtr->GetClass();

			if (!BaseClass)
			{
				BaseClass = ObjClass;
			}

			while (!ObjClass->IsChildOf(BaseClass))
			{
				BaseClass = BaseClass->GetSuperClass();
			}
		}
	}

	TSharedRef< SHorizontalBox > ObjectNameArea = SNew( SHorizontalBox );

	if (BaseClass)
	{
		// Get selection icon based on actor(s) classes and add before the selection label
		const FSlateBrush* ActorIcon = FClassIconFinder::FindIconForClass(BaseClass);

		ObjectNameArea->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(0,0,0,0)
		.AspectRatio()
		[
			SNew(SImage)
			.Image(ActorIcon)
			.ToolTipText( FText::FromString( BaseClass->GetName() ) )
		];
	}


	// Add the selected object(s) type name, along with buttons for either opening C++ code or editing blueprints
	const int32 NumSelectedSurfaces = AssetSelectionUtils::GetNumSelectedSurfaces( GWorld );
	if( SelectedObjects.Num() > 0 )
	{
		if ( bShowActorLabel )
		{
			FEditorWidgetsModule& EdWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>(TEXT("EditorWidgets"));
			TSharedRef<IObjectNameEditableTextBox> ObjectNameBox = EdWidgetsModule.CreateObjectNameEditableTextBox(SelectedObjects);

			ObjectNameArea->AddSlot()
			.AutoWidth()
			.Padding(0,0,6,0)
			[
				SNew(SBox)
				.WidthOverride( 200.0f )
				.VAlign(VAlign_Center)
				[
					ObjectNameBox
				]
			];
		}

		const TWeakObjectPtr< UObject > ObjectWeakPtr = SelectedObjects.Num() == 1 ? SelectedObjects[0] : NULL;
		BuildObjectNameAreaSelectionLabel( ObjectNameArea, ObjectWeakPtr, SelectedObjects.Num() );

		if( bShowLockButton )
		{
			ObjectNameArea->AddSlot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew( SButton )
					.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
					.OnClicked(	OnLockButtonClicked )
					.ToolTipText( LOCTEXT("LockSelectionButton_ToolTip", "Locks the current selection into the Details panel") )
					[
						SNew( SImage )
						.Image( this, &SDetailNameArea::OnGetLockButtonImageResource )
					]
				];
		}
	}
	else
	{
		if ( SelectionTip.Get() && NumSelectedSurfaces == 0 )
		{
			ObjectNameArea->AddSlot()
			.FillWidth( 1.0f )
			.HAlign( HAlign_Center )
			.Padding( 2.0f, 24.0f, 2.0f, 2.0f )
			[
				SNew( STextBlock )
				.Text( LOCTEXT("NoObjectsSelected", "Select an object to view details.") )
			];
		}
		else
		{
			// Fill the empty space
			ObjectNameArea->AddSlot();
		}
	}
	
	return ObjectNameArea;
}

void SDetailNameArea::OnEditBlueprintClicked( TWeakObjectPtr<UBlueprint> InBlueprint, TWeakObjectPtr<UObject> InAsset )
{
	if (UBlueprint* Blueprint = InBlueprint.Get())
	{
		// Set the object being debugged if given an actor reference (if we don't do this before we edit the object the editor wont know we are debugging something)
		if (UObject* Asset = InAsset.Get())
		{
			check(Asset->GetClass()->ClassGeneratedBy == Blueprint);
			Blueprint->SetObjectBeingDebugged(Asset);
		}
		// Open the blueprint
		GEditor->EditObject( Blueprint );
	}
}

void SDetailNameArea::BuildObjectNameAreaSelectionLabel( TSharedRef< SHorizontalBox > SelectionLabelBox, const TWeakObjectPtr<UObject> ObjectWeakPtr, const int32 NumSelectedObjects ) 
{
	check( NumSelectedObjects > 1 || ObjectWeakPtr.IsValid() );

	if( NumSelectedObjects == 1 )
	{
		UClass* ObjectClass = ObjectWeakPtr.Get()->GetClass();
		if( ObjectClass != nullptr )
		{
			if (UBlueprint* Blueprint = Cast<UBlueprint>(ObjectClass->ClassGeneratedBy))
			{
				TWeakObjectPtr<UBlueprint> BlueprintPtr = Blueprint;

				SelectionLabelBox->AddSlot()
					.VAlign( VAlign_Center )
					.HAlign( HAlign_Left )
					.Padding( 6.0f, 1.0f, 0.0f, 0.0f )
					.FillWidth( 1.0f )
					[
						SNew(SHyperlink)
							.Style(FEditorStyle::Get(), "HoverOnlyHyperlink")
							.TextStyle(FEditorStyle::Get(), "DetailsView.EditBlueprintHyperlinkStyle")
							.OnNavigate(this, &SDetailNameArea::OnEditBlueprintClicked, BlueprintPtr, ObjectWeakPtr)
							.Text(FText::Format(LOCTEXT("EditBlueprint", "(Edit {0})"), FText::FromString( Blueprint->GetName() ) ))
							.ToolTipText(LOCTEXT("EditBlueprint_ToolTip", "Click to edit the blueprint"))
					];
			}		
			else if( FSourceCodeNavigation::IsCompilerAvailable() )
			{
				FString ClassHeaderPath;
				if( FSourceCodeNavigation::FindClassHeaderPath( ObjectClass, ClassHeaderPath ) && IFileManager::Get().FileSize( *ClassHeaderPath ) != INDEX_NONE )
				{
					struct Local
					{
						static void OnEditCodeClicked( FString ClassHeaderPath )
						{
							FString AbsoluteHeaderPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead( *ClassHeaderPath );
							FSourceCodeNavigation::OpenSourceFile( AbsoluteHeaderPath );
						}
					};

					SelectionLabelBox->AddSlot()
						.VAlign( VAlign_Center )
						.HAlign( HAlign_Left )
						.Padding( 6.0f, 1.0f, 0.0f, 0.0f )
						.FillWidth( 1.0f )
						[
							SNew(SHyperlink)
								.Style(FEditorStyle::Get(), "HoverOnlyHyperlink")
								.TextStyle(FEditorStyle::Get(), "DetailsView.GoToCodeHyperlinkStyle")
								.OnNavigate_Static(&Local::OnEditCodeClicked, ClassHeaderPath)
//								.Text(LOCTEXT("GoToCode", "(C++)" ))
								.Text(FText::Format(LOCTEXT("GoToCode", "({0})" ), FText::FromString(FPaths::GetCleanFilename( *ClassHeaderPath ) ) ) )
								.ToolTipText(LOCTEXT("GoToCode_ToolTip", "Click to open this source file in a text editor"))
						];
				}
			}
			else
			{
				// Fill the empty space
				SelectionLabelBox->AddSlot();
			}
		}
		else
		{
			// Fill the empty space
			SelectionLabelBox->AddSlot();
		}
	}
	else
	{
		const FString SelectionText = FString::Printf( *LOCTEXT("MultipleObjectsSelected", "%d objects").ToString(), NumSelectedObjects );
		SelectionLabelBox->AddSlot()
		.VAlign(VAlign_Center)
		.HAlign( HAlign_Left )
		.FillWidth( 1.0f )
		[
			SNew(STextBlock)
			.Text( SelectionText )
		];

	}
}


#undef LOCTEXT_NAMESPACE

