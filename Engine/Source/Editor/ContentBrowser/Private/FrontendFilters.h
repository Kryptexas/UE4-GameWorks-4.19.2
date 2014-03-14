// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#define LOCTEXT_NAMESPACE "ContentBrowser"


typedef const FAssetData& AssetFilterType;
typedef TFilterCollection<AssetFilterType> AssetFilterCollectionType;


class FFrontendFilter : public IFilter<AssetFilterType>
{
public:
	/** Returns the system name for this filter */
	virtual FString GetName() const = 0;

	/** Returns the human readable name for this filter */
	virtual FText GetDisplayName() const = 0;

	/** Returns the tooltip for this filter, shown in the filters menu */
	virtual FText GetToolTipText() const = 0;

	/** Returns the color this filter button will be when displayed as a button */
	virtual FLinearColor GetColor() const { return FLinearColor(0.6f, 0.6f, 0.6f, 1); }

	/** Returns the name of the icon to use in menu entries */
	virtual FName GetIconName() const { return NAME_None; }

	/** Returns true if the filter should be in the list when disabled and not in the list when enabled */
	virtual bool IsInverseFilter() const { return false; }

	/** Invoke to set the ARFilter that is currently used to filter assets in the asset view */
	virtual void SetCurrentFilter(const FARFilter& InBaseFilter) { }

	/** Notification that the filter became active or inactive */
	virtual void ActiveStateChanged(bool bActive) { }

	// IFilter implementation
	DECLARE_DERIVED_EVENT( FFrontendFilter, IFilter<AssetFilterType>::FChangedEvent, FChangedEvent );
	virtual FChangedEvent& OnChanged() OVERRIDE { return ChangedEvent; }

protected:
	void BroadcastChangedEvent() const { ChangedEvent.Broadcast(); }

private:
	FChangedEvent ChangedEvent;
};

/** A filter that displays only checked out assets */
class FFrontendFilter_CheckedOut : public FFrontendFilter
{
public:
	FFrontendFilter_CheckedOut();

	// FFrontendFilter implementation
	virtual FString GetName() const OVERRIDE { return TEXT("CheckedOut"); }
	virtual FText GetDisplayName() const OVERRIDE { return LOCTEXT("FrontendFilter_CheckedOut", "Checked Out"); }
	virtual FText GetToolTipText() const OVERRIDE { return LOCTEXT("FrontendFilter_CheckedOutTooltip", "Show only assets that you have checked out or pending for add."); }
	virtual void ActiveStateChanged(bool bActive) OVERRIDE;

	// IFilter implementation
	virtual bool PassesFilter( AssetFilterType InItem ) const OVERRIDE;

private:
	
	/** Request the source control status for this filter */
	void RequestStatus();

	/** Callback when source control operation has completed */
	void SourceControlOperationComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);
};

/** A filter that displays only modified assets */
class FFrontendFilter_Modified : public FFrontendFilter
{
public:
	// FFrontendFilter implementation
	virtual FString GetName() const OVERRIDE { return TEXT("Modified"); }
	virtual FText GetDisplayName() const OVERRIDE { return LOCTEXT("FrontendFilter_Modified", "Modified"); }
	virtual FText GetToolTipText() const OVERRIDE { return LOCTEXT("FrontendFilter_ModifiedTooltip", "Show only assets that have been modified and not yet saved."); }

	// IFilter implementation
	virtual bool PassesFilter( AssetFilterType InItem ) const OVERRIDE;
};

/** A filter that displays blueprints that have replicated properties */
class FFrontendFilter_ReplicatedBlueprint : public FFrontendFilter
{
public:

	// FFrontendFilter implementation
	virtual FString GetName() const OVERRIDE { return TEXT("ReplicatedBlueprint"); }
	virtual FText GetDisplayName() const OVERRIDE { return LOCTEXT("FFrontendFilter_ReplicatedBlueprint", "Replicated Blueprints"); }
	virtual FText GetToolTipText() const OVERRIDE { return LOCTEXT("FFrontendFilter_ReplicatedBlueprintToolTip", "Show only blueprints with replicated properties."); }

	// IFilter implementation
	virtual bool PassesFilter( AssetFilterType InItem ) const OVERRIDE;
};

/** An inverse filter that allows display of content in developer folders that are not the current user's */
class FFrontendFilter_ShowOtherDevelopers : public FFrontendFilter
{
public:
	/** Constructor */
	FFrontendFilter_ShowOtherDevelopers();

	// FFrontendFilter implementation
	virtual FString GetName() const OVERRIDE { return TEXT("ShowOtherDevelopers"); }
	virtual FText GetDisplayName() const OVERRIDE { return LOCTEXT("FrontendFilter_ShowOtherDevelopers", "Other Developers"); }
	virtual FText GetToolTipText() const OVERRIDE { return LOCTEXT("FrontendFilter_ShowOtherDevelopersTooltip", "Allow display of assets in developer folders that aren't yours."); }
	virtual bool IsInverseFilter() const OVERRIDE { return true; }
	virtual void SetCurrentFilter(const FARFilter& InFilter) OVERRIDE;

	// IFilter implementation
	virtual bool PassesFilter( AssetFilterType InItem ) const OVERRIDE;

private:
	FString BaseDeveloperPath;
	FString UserDeveloperPath;
	bool bIsOnlyOneDeveloperPathSelected;
};

/** An inverse filter that allows display of object redirectors */
class FFrontendFilter_ShowRedirectors : public FFrontendFilter
{
public:
	/** Constructor */
	FFrontendFilter_ShowRedirectors();

	// FFrontendFilter implementation
	virtual FString GetName() const OVERRIDE { return TEXT("ShowRedirectors"); }
	virtual FText GetDisplayName() const OVERRIDE { return LOCTEXT("FrontendFilter_ShowRedirectors", "Show Redirectors"); }
	virtual FText GetToolTipText() const OVERRIDE { return LOCTEXT("FrontendFilter_ShowRedirectorsToolTip", "Allow display of Redirectors."); }
	virtual bool IsInverseFilter() const OVERRIDE { return true; }
	virtual void SetCurrentFilter(const FARFilter& InFilter) OVERRIDE;

	// IFilter implementation
	virtual bool PassesFilter( AssetFilterType InItem ) const OVERRIDE;

private:
	bool bAreRedirectorsInBaseFilter;
	FName RedirectorClassName;
};

/** A filter that only displays assets used by loaded levels */
class FFrontendFilter_InUseByLoadedLevels : public FFrontendFilter
{
public:
	/** Constructor/Destructor */
	FFrontendFilter_InUseByLoadedLevels();
	~FFrontendFilter_InUseByLoadedLevels();

	// FFrontendFilter implementation
	virtual FString GetName() const OVERRIDE { return TEXT("InUseByLoadedLevels"); }
	virtual FText GetDisplayName() const OVERRIDE { return LOCTEXT("FrontendFilter_InUseByLoadedLevels", "In Use By Level"); }
	virtual FText GetToolTipText() const OVERRIDE { return LOCTEXT("FrontendFilter_InUseByLoadedLevelsToolTip", "Show only assets that are currently in use by any loaded level."); }
	virtual void ActiveStateChanged(bool bActive) OVERRIDE;

	// IFilter implementation
	virtual bool PassesFilter( AssetFilterType InItem ) const OVERRIDE;

	/** Handler for when maps change in the editor */
	void OnEditorMapChange( uint32 MapChangeFlags );

private:
	bool bIsCurrentlyActive;
};


#undef LOCTEXT_NAMESPACE
