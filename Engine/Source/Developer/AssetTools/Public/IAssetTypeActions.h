// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace EAssetTypeCategories
{
	enum Type
	{
		None					= 0x00,
		Basic					= 0x01,
		Animation				= 0x02,
		MaterialsAndTextures	= 0x04,
		Sounds					= 0x08,
		Physics					= 0x10,
		Misc					= 0x20
	};
}

namespace EAssetTypeActivationMethod
{
	enum Type
	{
		DoubleClicked,
		EnterPressed,
		SpacePressed
	};
}

class IToolkitHost;

/* Revision information for a single revision of a file in source control */
struct FRevisionInfo
{
	int32		Revision;
	int32		Changelist;
	FDateTime	Date;	
};

/** AssetTypeActions provide actions and other information about asset types */
class IAssetTypeActions : public TSharedFromThis<IAssetTypeActions>
{
public:
	/** Virtual destructor */
	virtual ~IAssetTypeActions(){}

	/** Returns the name of this type */
	virtual FText GetName() const = 0;

	/** Checks to see if the specified object is handled by this type. */
	virtual UClass* GetSupportedClass() const = 0;

	/** Returns the color associated with this type */
	virtual FColor GetTypeColor() const = 0;

	/** Returns true if this class can supply actions for InObjects. */
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const = 0;

	/** Generates a menubuilder for the specified objects. */
	virtual void GetActions( const TArray<UObject*>& InObjects, class FMenuBuilder& MenuBuilder ) = 0;

	/** Opens the asset editor for the specified objects. If EditWithinLevelEditor is valid, the world-centric editor will be used. */
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) = 0;

	/** Performs asset type specific activation for the supplied assets. This happens when the user double clicks, presses enter, or presses space. */
	virtual void AssetsActivated( const TArray<UObject*>& InObjects, EAssetTypeActivationMethod::Type ActivationType ) = 0;

	/** Returns true if this class can be used as a filter in the content browser */
	virtual bool CanFilter() = 0;

	/** Returns the categories that this asset type. The return value is one or more flags from EAssetTypeCategories.  */
	virtual uint32 GetCategories() = 0;

	/** @return True if we should force world-centric mode for newly-opened assets */
	virtual bool ShouldForceWorldCentric() = 0;

	/** Performs asset-specific diff on the supplied asset */
	virtual void PerformAssetDiff(UObject* OldAsset, UObject* NewAsset, const struct FRevisionInfo& OldRevision, const struct FRevisionInfo& NewRevision) const = 0;

	/** Returns the thumbnail info for the specified asset, if it has one. */
	virtual class UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const = 0;

	/** Returns additional tooltip information for the specified asset, if it has any (otherwise return the null widget) */
	virtual FText GetAssetDescription(const class FAssetData& AssetData) const = 0;
};
