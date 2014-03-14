// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GameProjectGenerationPrivatePCH.h"

UTemplateProjectDefs::UTemplateProjectDefs(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UTemplateProjectDefs::FixupStrings(const FString& TemplateName, const FString& ProjectName)
{
	for ( auto IgnoreIt = FoldersToIgnore.CreateIterator(); IgnoreIt; ++IgnoreIt )
	{
		FixString(*IgnoreIt, TemplateName, ProjectName);
	}

	for ( auto IgnoreIt = FilesToIgnore.CreateIterator(); IgnoreIt; ++IgnoreIt )
	{
		FixString(*IgnoreIt, TemplateName, ProjectName);
	}

	for ( auto RenameIt = FolderRenames.CreateIterator(); RenameIt; ++RenameIt )
	{
		FTemplateFolderRename& FolderRename = *RenameIt;
		FixString(FolderRename.From, TemplateName, ProjectName);
		FixString(FolderRename.To, TemplateName, ProjectName);
	}

	for ( auto ReplacementsIt = FilenameReplacements.CreateIterator(); ReplacementsIt; ++ReplacementsIt )
	{
		FTemplateReplacement& Replacement = *ReplacementsIt;
		FixString(Replacement.From, TemplateName, ProjectName);
		FixString(Replacement.To, TemplateName, ProjectName);
	}

	for ( auto ReplacementsIt = ReplacementsInFiles.CreateIterator(); ReplacementsIt; ++ReplacementsIt )
	{
		FTemplateReplacement& Replacement = *ReplacementsIt;
		FixString(Replacement.From, TemplateName, ProjectName);
		FixString(Replacement.To, TemplateName, ProjectName);
	}
}

FText UTemplateProjectDefs::GetDisplayNameText()
{
	const TSharedRef< FCulture > CurrentCulture = FInternationalization::GetCurrentCulture();
	for ( auto NameIt = LocalizedDisplayNames.CreateConstIterator(); NameIt; ++NameIt )
	{
		const FLocalizedTemplateString& Name = *NameIt;
		if ( Name.Language == CurrentCulture->GetTwoLetterISOLanguageName() )
		{
			return FText::FromString(Name.Text);
		}
	}

	return FText();
}

FText UTemplateProjectDefs::GetLocalizedDescription()
{
	const TSharedRef< FCulture >  CurrentCulture = FInternationalization::GetCurrentCulture();
	for ( auto DescriptionIt = LocalizedDescriptions.CreateConstIterator(); DescriptionIt; ++DescriptionIt )
	{
		const FLocalizedTemplateString& Description = *DescriptionIt;
		if ( Description.Language == CurrentCulture->GetTwoLetterISOLanguageName() )
		{
			return FText::FromString(Description.Text);
		}
	}

	return FText();
}

void UTemplateProjectDefs::FixString(FString& InOutStringToFix, const FString& TemplateName, const FString& ProjectName)
{
	InOutStringToFix.ReplaceInline(TEXT("%TEMPLATENAME%"), *TemplateName, ESearchCase::CaseSensitive);
	InOutStringToFix.ReplaceInline(TEXT("%TEMPLATENAME_UPPERCASE%"), *TemplateName.ToUpper(), ESearchCase::CaseSensitive);
	InOutStringToFix.ReplaceInline(TEXT("%TEMPLATENAME_LOWERCASE%"), *TemplateName.ToLower(), ESearchCase::CaseSensitive);

	InOutStringToFix.ReplaceInline(TEXT("%PROJECTNAME%"), *ProjectName, ESearchCase::CaseSensitive);
	InOutStringToFix.ReplaceInline(TEXT("%PROJECTNAME_UPPERCASE%"), *ProjectName.ToUpper(), ESearchCase::CaseSensitive);
	InOutStringToFix.ReplaceInline(TEXT("%PROJECTNAME_LOWERCASE%"), *ProjectName.ToLower(), ESearchCase::CaseSensitive);
}