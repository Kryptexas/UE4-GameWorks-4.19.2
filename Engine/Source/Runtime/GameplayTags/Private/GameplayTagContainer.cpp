// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsModulePrivatePCH.h"

FGameplayTagContainer::FGameplayTagContainer()
{}

FGameplayTagContainer::FGameplayTagContainer(FGameplayTagContainer const& Other)
{
	*this = Other;
}

FGameplayTagContainer& FGameplayTagContainer::operator=(FGameplayTagContainer const& Other)
{
	// Guard against self-assignment
	if (this == &Other)
	{
		return *this;
	}

	Tags = Other.Tags;

	return *this;
}

bool FGameplayTagContainer::operator==(FGameplayTagContainer const& Other) const
{
	if( Tags.Num() == Other.Tags.Num() )
	{
		for( int TagIndex = 0; TagIndex < Tags.Num(); ++TagIndex )
		{
			if( Other.Tags.Contains( Tags[TagIndex] ) == false )
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool FGameplayTagContainer::operator!=(FGameplayTagContainer const& Other) const
{
	if( Tags.Num() == Other.Tags.Num() )
	{
		for( int TagIndex = 0; TagIndex < Tags.Num(); ++TagIndex )
		{
			if( Other.Tags.Contains( Tags[TagIndex] ) == false )
			{
				return true;
			}
		}
	}
	else
	{
		return true;
	}
	return false;
}

void FGameplayTagContainer::GetTags(OUT TSet<FName>& OutTags) const
{
	OutTags.Append(Tags);
}

bool FGameplayTagContainer::HasTag(FName TagToCheck) const
{
	return Tags.Contains(TagToCheck);
}

bool FGameplayTagContainer::HasAllTags(FGameplayTagContainer const& Other) const
{
	TSet<FName> OutTags;
	Other.GetTags(OutTags);
	for(TSet<FName>::TIterator It(OutTags); It; ++It)
	{
		if(HasTag(*It) == false)
		{
			return false;
		}
	}
	return true;
}

bool FGameplayTagContainer::HasAnyTag(FGameplayTagContainer const& Other) const
{
	TSet<FName> OutTags;
	Other.GetTags(OutTags);
	for(TSet<FName>::TIterator It(OutTags); It; ++It)
	{
		if(HasTag(*It) == true)
		{
			return true;
		}
	}
	return false;
}

void FGameplayTagContainer::AddTag(FName TagToAdd)
{
	Tags.AddUnique(TagToAdd);
}

void FGameplayTagContainer::AppendTags(FGameplayTagContainer const& Other)
{
	TSet<FName> OutTags;
	Other.GetTags(OutTags);

	//add all the tags
	for(TSet<FName>::TConstIterator It(OutTags); It; ++It)
	{
		AddTag(*It);
	}
}

void FGameplayTagContainer::RemoveTag(FName TagToRemove)
{
	Tags.Remove(TagToRemove);
}

void FGameplayTagContainer::RemoveAllTags()
{
	Tags.Empty();
}

bool FGameplayTagContainer::Serialize(FArchive& Ar)
{
	Ar << Tags;

	if( Ar.IsLoading() && Tags.Num() > 0 )
	{
		RedirectTags();
	}
	return true;
}

void FGameplayTagContainer::RedirectTags()
{
	FConfigSection* PackageRedirects = GConfig->GetSectionPrivate( TEXT("/Script/Engine.Engine"), false, true, GEngineIni );
	for (FConfigSection::TIterator It(*PackageRedirects); It; ++It)
	{
		if (It.Key() == TEXT("GameplayTagRedirects"))
		{
			FName OldTagName = NAME_None;
			FName NewTagName;

			FParse::Value( *It.Value(), TEXT("OldTagName="), OldTagName );
			FParse::Value( *It.Value(), TEXT("NewTagName="), NewTagName );

			int32 TagIndex = 0;
			if( Tags.Find( OldTagName, TagIndex ) )
			{
				Tags.RemoveAt( TagIndex );
				Tags.AddUnique( NewTagName );
			}
		}
	}
}

int32 FGameplayTagContainer::Num() const
{
	return Tags.Num();
}

FString FGameplayTagContainer::ToString() const
{
	FString RetString = TEXT("[");

	for (int i = 0; i < Tags.Num(); ++i)
	{
		RetString += Tags[i].ToString();
		if (i < Tags.Num() - 1)
		{
			RetString += TEXT(", ");
		}
	}

	RetString += TEXT("]");

	return RetString;
}
