// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ScreenShotChangeListNode.h: Declares the FScreenShotChangeListNode class.
=============================================================================*/

#pragma once

class FScreenShotPlatformNode : public FScreenShotBaseNode
{
public:

	/**
	* Construct test data given a name.
	*
	* @param InName  The Name of this test data item.
	* @param InName  The Name of this asset.
	*/
	FScreenShotPlatformNode( const FString& InName, const FString& InAssetName = "" )
	: FScreenShotBaseNode( InName, InAssetName )
	{}

public:

	// Begin IScreenShotData interface

	virtual void AddScreenShotData( const FScreenShotDataItem& InScreenDataItem ) OVERRIDE 
	{
		FString ScreenShotNumber = FString::Printf( TEXT( "CL #%d"), InScreenDataItem.ChangeListNumber );
		TSharedRef<FScreenShotBaseNode> ChildNode = MakeShareable( new FScreenShotBaseNode( ScreenShotNumber, InScreenDataItem.AssetName) );
		Children.Add( ChildNode );
	};


	virtual EScreenShotDataType::Type GetScreenNodeType() OVERRIDE 
	{ 
		return EScreenShotDataType::SSDT_Platform; 
	};


	virtual bool SetFilter( TSharedPtr< ScreenShotFilterCollection > ScreenFilter ) OVERRIDE 
	{
		FilteredChildren.Empty();

		bool bPassesFilter = ScreenFilter->PassesAllFilters( SharedThis( this ) );
		if( bPassesFilter )
		{
			for ( int32 Index = 0; Index < Children.Num(); Index++ )
			{
				FilteredChildren.Add( Children[Index] );
			}
		}

		return bPassesFilter;
	};


	// End IScreenShotData interface
};

