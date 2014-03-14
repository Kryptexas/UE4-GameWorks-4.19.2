// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

struct FPropertyInfo
{
public:

	bool operator==( const FPropertyInfo& Other ) const 
	{
		return Property == Other.Property && ArrayIndex == Other.ArrayIndex;
	}

	TWeakObjectPtr< UProperty > Property;
	int32 ArrayIndex;
};


class FPropertyPath
{
public:

	static TSharedRef< FPropertyPath > CreateEmpty()
	{
		return MakeShareable( new FPropertyPath() );
	}

	static TSharedRef< FPropertyPath > Create( const TWeakObjectPtr< UProperty >& Property )
	{
		TSharedRef< FPropertyPath > NewPath = MakeShareable( new FPropertyPath() );

		FPropertyInfo NewPropInfo;
		NewPropInfo.Property = Property;
		NewPropInfo.ArrayIndex = INDEX_NONE;

		NewPath->Properties.Add( NewPropInfo );

		return NewPath;
	}


	int32 GetNumProperties() const 
	{ 
		return Properties.Num(); 
	}

	const FPropertyInfo& GetPropertyInfo( int32 index ) const
	{
		return Properties[ index ];
	}

	const FPropertyInfo& GetLeafMostProperty() const
	{
		return Properties[ Properties.Num() - 1 ];
	}

	const FPropertyInfo& GetRootProperty() const
	{
		return Properties[ 0 ];
	}

	TSharedRef< FPropertyPath > ExtendPath( const FPropertyInfo& NewLeaf )
	{
		TSharedRef< FPropertyPath > NewPath = MakeShareable( new FPropertyPath() );

		NewPath->Properties = Properties;
		NewPath->Properties.Add( NewLeaf );
		
		return NewPath;
	}

	TSharedRef< FPropertyPath > ExtendPath( const TSharedRef< FPropertyPath >& Extension )
	{
		TSharedRef< FPropertyPath > NewPath = MakeShareable( new FPropertyPath() );

		NewPath->Properties = Properties;

		for (int Index = Extension->GetNumProperties() - 1; Index >= 0 ; Index--)
		{
			NewPath->Properties.Add( Extension->GetPropertyInfo( Index ) );
		}

		return NewPath;
	}

	TSharedRef< FPropertyPath > TrimPath( const int32 AmountToTrim )
	{
		TSharedRef< FPropertyPath > NewPath = MakeShareable( new FPropertyPath() );

		NewPath->Properties = Properties;

		for (int Count = 0; Count < AmountToTrim; Count++)
		{
			NewPath->Properties.Pop();
		}

		return NewPath;
	}

	TSharedRef< FPropertyPath > TrimRoot( const int32 AmountToTrim )
	{
		TSharedRef< FPropertyPath > NewPath = MakeShareable( new FPropertyPath() );

		for (int Count = AmountToTrim; Count < Properties.Num(); Count++)
		{
			NewPath->Properties.Add( Properties[ Count ] );
		}

		return NewPath;
	}

	FString ToString() const
	{
		FString NewDisplayName;
		bool FirstAddition = true;
		for (int PropertyIndex = 0; PropertyIndex < Properties.Num(); PropertyIndex++)
		{
			const FPropertyInfo& PropInfo = Properties[ PropertyIndex ];
			if ( !(PropInfo.Property->IsA( UArrayProperty::StaticClass() ) && PropertyIndex != Properties.Num() - 1 ) )
			{
				if ( !FirstAddition )
				{
					NewDisplayName += TEXT( "->" );
				}

				NewDisplayName += PropInfo.Property->GetFName().ToString();

				if ( PropInfo.ArrayIndex != INDEX_NONE )
				{
					NewDisplayName += FString::Printf( TEXT( "[%d]" ), PropInfo.ArrayIndex );
				}

				FirstAddition = false;
			}
		}

		return NewDisplayName;
	}

	/**
	 * Add another property to be associated with this path
	 */
	void AddProperty( FPropertyInfo& InProperty )
	{
		Properties.Add( InProperty );
	}


	static bool AreEqual( const TSharedRef< FPropertyPath >& PathLhs, const TSharedRef< FPropertyPath >& PathRhs )
	{
		bool Result = false;

		if ( PathLhs->GetNumProperties() == PathRhs->GetNumProperties() )
		{
			bool FoundMatch = true;
			for( int Index = PathLhs->GetNumProperties() - 1; Index >= 0; --Index )
			{
				const FPropertyInfo& LhsPropInfo = PathLhs->GetPropertyInfo( Index );
				const FPropertyInfo& RhsPropInfo = PathRhs->GetPropertyInfo( Index );

				if ( LhsPropInfo.Property != RhsPropInfo.Property ||
					LhsPropInfo.ArrayIndex != RhsPropInfo.ArrayIndex )
				{
					FoundMatch = false;
					break;
				}
			}

			if ( FoundMatch )
			{
				Result = true;
			}
		}

		return Result;
	}

private:

	TArray< FPropertyInfo > Properties;
};