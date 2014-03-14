// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ContentBrowserPCH.h"

FOtherDevelopersAssetFilter::FOtherDevelopersAssetFilter()
	: BaseDeveloperPath( FPackageName::FilenameToLongPackageName(FPaths::GameDevelopersDir()).LeftChop(1) )
	, UserDeveloperPath( FPackageName::FilenameToLongPackageName(FPaths::GameUserDeveloperDir()).LeftChop(1) )
	, ShowOtherDeveloperAssets( false )
{
}

FOtherDevelopersAssetFilter::~FOtherDevelopersAssetFilter()
{
}

bool FOtherDevelopersAssetFilter::PassesFilter( AssetFilterType InItem ) const
{
	if ( !ShowOtherDeveloperAssets )
	{
		const FString PackageName = InItem.PackageName.ToString();
		const bool bPackageInDevelopersFolder = PackageName.StartsWith(BaseDeveloperPath);

		if ( bPackageInDevelopersFolder )
		{
			const bool bPackageInUserDeveloperFolder = PackageName.StartsWith(UserDeveloperPath);
			if ( !bPackageInUserDeveloperFolder )
			{
				return false;
			}
		}
	}

	return true;
}

void FOtherDevelopersAssetFilter::SetShowOtherDeveloperAssets(bool value)
{
	ShowOtherDeveloperAssets = value;
	ChangedEvent.Broadcast();
}

bool FOtherDevelopersAssetFilter::GetShowOtherDeveloperAssets() const
{
	return ShowOtherDeveloperAssets;
}