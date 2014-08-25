// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "EditorClassUtils.h"

#include "IDocumentation.h"
#include "SourceCodeNavigation.h"

FString FEditorClassUtils::GetDocumentationPage(const UClass* Class)
{
	return (Class ? FString::Printf( TEXT("Shared/Types/%s%s"), Class->GetPrefixCPP(), *Class->GetName() ) : FString());
}

FString FEditorClassUtils::GetDocumentationExcerpt(const UClass* Class)
{
	return (Class ? FString::Printf( TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName() ) : FString());
}

TSharedRef<SToolTip> FEditorClassUtils::GetTooltip(const UClass* Class)
{
	return (Class ? GetTooltip(Class, Class->GetToolTipText()) : SNew(SToolTip));
}

TSharedRef<SToolTip> FEditorClassUtils::GetTooltip(const UClass* Class, const TAttribute<FText>& OverrideText)
{
	return (Class ? IDocumentation::Get()->CreateToolTip(OverrideText, nullptr, GetDocumentationPage(Class), GetDocumentationExcerpt(Class)) : SNew(SToolTip));
}

FString FEditorClassUtils::GetDocumentationLink(const UClass* Class, const FString& OverrideExcerpt)
{
	FString DocumentationLink;
	const FString ClassDocsPage = GetDocumentationPage(Class);

	TSharedRef<IDocumentation> Documentation = IDocumentation::Get();
	if (Documentation->PageExists(ClassDocsPage))
	{
		TSharedRef<IDocumentationPage> ClassDocs = Documentation->GetPage(ClassDocsPage, NULL);

		const FString ExcerptSection = (OverrideExcerpt.IsEmpty() ? GetDocumentationExcerpt(Class) : OverrideExcerpt);

		FExcerpt Excerpt;
		if (ClassDocs->GetExcerpt(ExcerptSection, Excerpt))
		{
			FString* FullDocumentationLink = Excerpt.Variables.Find( TEXT("ToolTipFullLink") );
			if (FullDocumentationLink)
			{
				DocumentationLink = *FullDocumentationLink;
			}
		}
	}

	return DocumentationLink;
}

TSharedRef<SWidget> FEditorClassUtils::GetDocumentationLinkWidget(const UClass* Class)
{
	TSharedRef<SWidget> DocLinkWidget = SNullWidget::NullWidget;
	const FString DocumentationLink = GetDocumentationLink(Class);

	if (!DocumentationLink.IsEmpty())
	{
		DocLinkWidget = IDocumentation::Get()->CreateAnchor(DocumentationLink);
	}

	return DocLinkWidget;
}

TSharedRef<SWidget> FEditorClassUtils::GetSourceLink(const UClass* Class, const TWeakObjectPtr<UObject> ObjectWeakPtr)
{
	TSharedRef<SWidget> SourceHyperlink = SNew( SSpacer );
	UBlueprint* Blueprint = (Class ? Cast<UBlueprint>(Class->ClassGeneratedBy) : nullptr);

	if (Blueprint)
	{
		struct Local
		{
			static void OnEditBlueprintClicked( TWeakObjectPtr<UBlueprint> InBlueprint, TWeakObjectPtr<UObject> InAsset )
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
		};

		TWeakObjectPtr<UBlueprint> BlueprintPtr = Blueprint;

		SAssignNew(SourceHyperlink, SHyperlink)
			.Style(FEditorStyle::Get(), "EditBPHyperlink")
			.TextStyle(FEditorStyle::Get(), "DetailsView.EditBlueprintHyperlinkStyle")
			.OnNavigate_Static(&Local::OnEditBlueprintClicked, BlueprintPtr, ObjectWeakPtr)
			.Text(FText::Format(NSLOCTEXT("SourceHyperlink", "EditBlueprint", "Edit {0}"), FText::FromString( Blueprint->GetName() ) ))
			.ToolTipText(NSLOCTEXT("SourceHyperlink", "EditBlueprint_ToolTip", "Click to edit the blueprint"));
	}
	else if( FSourceCodeNavigation::IsCompilerAvailable() )
	{
		FString ClassHeaderPath;
		if( FSourceCodeNavigation::FindClassHeaderPath( Class, ClassHeaderPath ) && IFileManager::Get().FileSize( *ClassHeaderPath ) != INDEX_NONE )
		{
			struct Local
			{
				static void OnEditCodeClicked( FString InClassHeaderPath )
				{
					FString AbsoluteHeaderPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*InClassHeaderPath);
					FSourceCodeNavigation::OpenSourceFile( AbsoluteHeaderPath );
				}
			};

			SAssignNew(SourceHyperlink, SHyperlink)
				.Style(FCoreStyle::Get(), "Hyperlink")
				.TextStyle(FEditorStyle::Get(), "DetailsView.GoToCodeHyperlinkStyle")
				.OnNavigate_Static(&Local::OnEditCodeClicked, ClassHeaderPath)
				.Text(FText::Format(NSLOCTEXT("SourceHyperlink", "GoToCode", "{0}" ), FText::FromString(FPaths::GetCleanFilename( *ClassHeaderPath ) ) ) )
				.ToolTipText(NSLOCTEXT("SourceHyperlink", "GoToCode_ToolTip", "Click to open this source file in a text editor"));
		}
	}

	return SourceHyperlink;
}

UClass* FEditorClassUtils::GetClassFromString(const FString& ClassName)
{
	if(ClassName.IsEmpty() || ClassName == "None")
	{
		return nullptr;
	}

	UClass* Class = FindObject<UClass>(ANY_PACKAGE, *ClassName);
	if(!Class)
	{
		Class = LoadObject<UClass>(nullptr, *ClassName);
	}
	return Class;
}
