// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/**
 * An asset filter for filtering based on if the asset is in another developer's folder.  
 */
class FOtherDevelopersAssetFilter : public IFilter< AssetFilterType >, public TSharedFromThis<FOtherDevelopersAssetFilter>
{
public:
	/**
	 * Default constructor.
	 */
	FOtherDevelopersAssetFilter();

	/**
	 * Destructor.
	 */
	~FOtherDevelopersAssetFilter();

	/**
	 * Returns whether the specified Item passes the Filter's restrictions
	 *
	 * @param InItem - The asset to filter.
	 *
	 * @return true if passes the filter, otherwise false.
	 */
	virtual bool PassesFilter( AssetFilterType InItem ) const;

	/** Broadcasts anytime the restrictions of the Filter changes */
	DECLARE_DERIVED_EVENT( FOtherDevelopersAssetFilter, IFilter<AssetFilterType>::FChangedEvent, FChangedEvent );
	virtual FChangedEvent& OnChanged() { return ChangedEvent; }

	/**
	 * Sets if we should filter out assets from other developers
	 * @param value - the value to change the flag to.
	 */
	void SetShowOtherDeveloperAssets(bool value);

	/**
	 * Gets if we should filter out assets from other developers
	 * @return true if other developers assets will pass the filter, otherwise false.
	 */
	bool GetShowOtherDeveloperAssets() const;

private:

	// The base developer path
	FString BaseDeveloperPath;
	// The user's developer path
	FString UserDeveloperPath;

	// Do we need to filter out other developers from the returned assets?
	bool ShowOtherDeveloperAssets;

	// Fired when the ShowOtherDeveloperAssets flag changes.
	FChangedEvent ChangedEvent;
};