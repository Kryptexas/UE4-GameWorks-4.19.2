// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class KISMET_API FDetailsDiff
{
public:
	FDetailsDiff( const UObject* InObject, const TMap< FName, const UProperty* >& InPropertyMap, const TSet< FName >& InIdenticalProperties );

	void HighlightProperty( FName PropertyName );
	TSharedRef< SWidget > DetailsWidget();

private:
	TMap< FName, const UProperty* > PropertyMap;
	TSharedPtr< class IDetailsView > DetailsView;
};
