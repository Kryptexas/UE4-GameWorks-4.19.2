// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateStyle.h"
#include "GCObject.h"

class FAssetData;
class UObject;
class UCurveFloat;
class UCurveVector;
class UCurveLinearColor;

class ENGINE_API FSlateGameResources : public FSlateStyleSet, public FGCObject
{
public:	

	/** Create a new Slate resource set that is scoped to the ScopeToDirectory.
	 *
	 * All paths will be formed as InBasePath + "/" + AssetPath. Lookups will be restricted to ScopeToDirectory.
	 * e.g. Resources.Initialize( "/Game/Widgets/SFortChest", "/Game/Widgets" )
	 *      Resources.GetBrush( "SFortChest/SomeBrush" );         // Will look up "/Game/Widgets" + "/" + "SFortChest/SomeBrush" = "/Game/Widgets/SFortChest/SomeBrush"
	 *      Resources.GetBrush( "SSomeOtherWidget/SomeBrush" );   // Will fail because "/Game/Widgets/SSomeOtherWidget/SomeBrush" is outside directory to which we scoped.
	 */
	static TSharedRef<FSlateGameResources> New( const FName& InStyleSetName, const FString& ScopeToDirectory, const FString& InBasePath = FString() );

	/**
	 * Construct a style chunk.
	 * @param InStyleSetName The name used to identity this style set
	 */
	FSlateGameResources( const FName& InStyleSetName );

	virtual ~FSlateGameResources();

	/** @See New() */
	void Initialize( const FString& ScopeToDirectory, const FString& InBasePath );

	/**
	 * Populate an array of FSlateBrush with resources consumed by this style chunk.
	 * @param OutResources - the array to populate.
	 */
	virtual void GetResources( TArray< const FSlateBrush* >& OutResources ) const OVERRIDE;

	virtual void SetContentRoot( const FString& InContentRootDir ) OVERRIDE;

	virtual const FSlateBrush* GetBrush( const FName PropertyName, const ANSICHAR* Specifier = NULL ) const OVERRIDE;

	UCurveFloat* GetCurveFloat( const FName AssetName ) const;
	UCurveVector* GetCurveVector( const FName AssetName ) const;
	UCurveLinearColor* GetCurveLinearColor( const FName AssetName ) const;

protected:

	virtual const FSlateWidgetStyle* GetWidgetStyleInternal( const FName DesiredTypeName, const FName StyleName ) const OVERRIDE;

	virtual void Log( ISlateStyle::EStyleMessageSeverity Severity, const FText& Message ) const OVERRIDE;

	virtual void Log( const TSharedRef< class FTokenizedMessage >& Message ) const;

	/** Callback registered to the Asset Registry to be notified when an asset is added. */
	void AddAsset(const FAssetData& InAddedAssetData);

	/** Callback registered to the Asset Registry to be notified when an asset is removed. */
	void RemoveAsset(const FAssetData& InRemovedAssetData);

	bool ShouldCache( const FAssetData& InAssetData );

	void AddAssetToCache( UObject* InStyleObject, bool bEnsureUniqueness );

	void RemoveAssetFromCache( const FAssetData& AssetData );

	FName GenerateMapName( const FAssetData& AssetData );

	FName GenerateMapName( UObject* StyleObject );

	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

private:

	TMap< FName, UObject* > UIResources;
	FString BasePath;
	bool HasBeenInitialized;
};