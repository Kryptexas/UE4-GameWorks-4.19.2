// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ContentBrowserPCH.h"
#include "FrontendFilters.h"
#include "ISourceControlModule.h"
#include "ObjectTools.h"


/////////////////////////////////////////
// FFrontendFilter_CheckedOut
/////////////////////////////////////////

FFrontendFilter_CheckedOut::FFrontendFilter_CheckedOut()
{
	RequestStatus();
}

void FFrontendFilter_CheckedOut::ActiveStateChanged(bool bActive)
{
	if(bActive)
	{
		RequestStatus();
	}
}

bool FFrontendFilter_CheckedOut::PassesFilter( AssetFilterType InItem ) const
{
	FSourceControlStatePtr SourceControlState = ISourceControlModule::Get().GetProvider().GetState(SourceControlHelpers::PackageFilename(InItem.PackageName.ToString()), EStateCacheUsage::Use);
	return SourceControlState.IsValid() && (SourceControlState->IsCheckedOut() || SourceControlState->IsAdded());
}

void FFrontendFilter_CheckedOut::RequestStatus()
{
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( ISourceControlModule::Get().IsEnabled() )
	{
		// Request the opened files at filter construction time to make sure checked out files have the correct state for the filter
		TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> UpdateStatusOperation = ISourceControlOperation::Create<FUpdateStatus>();
		UpdateStatusOperation->SetGetOpenedOnly(true);
		SourceControlProvider.Execute(UpdateStatusOperation, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateRaw(this, &FFrontendFilter_CheckedOut::SourceControlOperationComplete) );
	}
}

void FFrontendFilter_CheckedOut::SourceControlOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	BroadcastChangedEvent();
}

/////////////////////////////////////////
// FFrontendFilter_Modified
/////////////////////////////////////////

bool FFrontendFilter_Modified::PassesFilter( AssetFilterType InItem ) const
{
	UPackage* Package = FindPackage(NULL, *InItem.PackageName.ToString());

	if ( Package != NULL )
	{
		return Package->IsDirty();
	}

	return false;
}

/////////////////////////////////////////
// FFrontendFilter_ReplicatedBlueprint
/////////////////////////////////////////

bool FFrontendFilter_ReplicatedBlueprint::PassesFilter( AssetFilterType InItem ) const
{
	FString TagValue = InItem.TagsAndValues.FindRef("NumReplicatedProperties");
	return !TagValue.IsEmpty() && (FCString::Atoi(*TagValue) > 0);
}

/////////////////////////////////////////
// FFrontendFilter_ShowOtherDevelopers
/////////////////////////////////////////

FFrontendFilter_ShowOtherDevelopers::FFrontendFilter_ShowOtherDevelopers()
{
	BaseDeveloperPath = FPackageName::FilenameToLongPackageName(FPaths::GameDevelopersDir());
	UserDeveloperPath = FPackageName::FilenameToLongPackageName(FPaths::GameUserDeveloperDir());
	bIsOnlyOneDeveloperPathSelected = false;
}

void FFrontendFilter_ShowOtherDevelopers::SetCurrentFilter(const FARFilter& InFilter)
{
	if ( InFilter.PackagePaths.Num() == 1 )
	{
		const FString PackagePath = InFilter.PackagePaths[0].ToString() + TEXT("/");
		
		// If the path starts with the base developer path, and is not the path itself then only one developer path is selected
		bIsOnlyOneDeveloperPathSelected = PackagePath.StartsWith(BaseDeveloperPath) && PackagePath.Len() != BaseDeveloperPath.Len();
	}
	else
	{
		// More or less than one path is selected
		bIsOnlyOneDeveloperPathSelected = false;
	}
}

bool FFrontendFilter_ShowOtherDevelopers::PassesFilter( AssetFilterType InItem ) const
{
	// Never hide developer assets when a single developer folder is selected.
	if ( !bIsOnlyOneDeveloperPathSelected )
	{
		// If selecting multiple folders, the Developers folder/parent folder, or "All Assets", hide assets which are found in the development folder unless they are in the current user's folder
		const FString PackagePath = InItem.PackagePath.ToString() + TEXT("/");
		const bool bPackageInDeveloperFolder = PackagePath.StartsWith(BaseDeveloperPath) && PackagePath.Len() != BaseDeveloperPath.Len();

		if ( bPackageInDeveloperFolder )
		{
			const bool bPackageInUserDeveloperFolder = PackagePath.StartsWith(UserDeveloperPath);
			if ( !bPackageInUserDeveloperFolder )
			{
				return false;
			}
		}
	}

	return true;
}

/////////////////////////////////////////
// FFrontendFilter_ShowRedirectors
/////////////////////////////////////////

FFrontendFilter_ShowRedirectors::FFrontendFilter_ShowRedirectors()
{
	bAreRedirectorsInBaseFilter = false;
	RedirectorClassName = UObjectRedirector::StaticClass()->GetFName();
}

void FFrontendFilter_ShowRedirectors::SetCurrentFilter(const FARFilter& InFilter)
{
	bAreRedirectorsInBaseFilter = InFilter.ClassNames.Contains(UObjectRedirector::StaticClass()->GetFName());
}

bool FFrontendFilter_ShowRedirectors::PassesFilter( AssetFilterType InItem ) const
{
	// Never hide redirectors if they are explicitly searched for
	if ( !bAreRedirectorsInBaseFilter )
	{
		return InItem.AssetClass != RedirectorClassName;
	}

	return true;
}

/////////////////////////////////////////
// FFrontendFilter_InUseByLoadedLevels
/////////////////////////////////////////

FFrontendFilter_InUseByLoadedLevels::FFrontendFilter_InUseByLoadedLevels()
	: bIsCurrentlyActive(false)
{
	FEditorDelegates::MapChange.AddRaw(this, &FFrontendFilter_InUseByLoadedLevels::OnEditorMapChange);
}

FFrontendFilter_InUseByLoadedLevels::~FFrontendFilter_InUseByLoadedLevels()
{
	FEditorDelegates::MapChange.RemoveAll(this);
}

void FFrontendFilter_InUseByLoadedLevels::ActiveStateChanged( bool bActive )
{
	bIsCurrentlyActive = bActive;

	if ( bActive )
	{
		ObjectTools::TagInUseObjects(ObjectTools::SO_LoadedLevels);
	}
}

bool FFrontendFilter_InUseByLoadedLevels::PassesFilter( AssetFilterType InItem ) const
{
	bool bObjectInUse = false;
	
	if ( InItem.IsAssetLoaded() )
	{
		UObject* Asset = InItem.GetAsset();

		const bool bUnreferenced = !Asset->HasAnyMarks( OBJECTMARK_TagExp );
		const bool bIndirectlyReferencedObject = Asset->HasAnyMarks( OBJECTMARK_TagImp );
		const bool bRejectObject =
			Asset->GetOuter() == NULL || // Skip objects with null outers
			Asset->HasAnyFlags( RF_Transient ) || // Skip transient objects (these shouldn't show up in the CB anyway)
			Asset->IsPendingKill() || // Objects that will be garbage collected 
			bUnreferenced || // Unreferenced objects 
			bIndirectlyReferencedObject; // Indirectly referenced objects

		if( !bRejectObject && Asset->HasAnyFlags( RF_Public ) )
		{
			// The object is in use 
			bObjectInUse = true;
		}
	}

	return bObjectInUse;
}

void FFrontendFilter_InUseByLoadedLevels::OnEditorMapChange( uint32 MapChangeFlags )
{
	if ( MapChangeFlags == MapChangeEventFlags::NewMap && bIsCurrentlyActive )
	{
		ObjectTools::TagInUseObjects(ObjectTools::SO_LoadedLevels);
		BroadcastChangedEvent();
	}
}