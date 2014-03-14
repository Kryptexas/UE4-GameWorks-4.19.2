// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "ObjectEditorUtils.h"

#if WITH_EDITOR
namespace FObjectEditorUtils
{

	FString GetCategory( const UProperty* InProperty )
	{
		static const FName NAME_Category(TEXT("Category"));
		if (InProperty->HasMetaData(NAME_Category))
		{
			return InProperty->GetMetaData(NAME_Category);
		}
		else
		{
			return FString();
		}
	}


	FName GetCategoryFName( const UProperty* InProperty )
	{
		FName CategoryName(NAME_None);
		if( InProperty )
		{
			CategoryName = *GetCategory( InProperty );
		}
		return CategoryName;
	}

	bool IsFunctionHiddenFromClass( const UFunction* InFunction,const UClass* Class )
	{
		bool bResult = false;
		if( InFunction )
		{
			bResult = Class->IsFunctionHidden( *InFunction->GetName() );

			static const FName FunctionCategory(TEXT("Category")); // FBlueprintMetadata::MD_FunctionCategory
			if( !bResult && InFunction->HasMetaData( FunctionCategory ) )
			{
				const FString& ClassCategory = InFunction->GetMetaData(FunctionCategory);				
				bResult = Class->IsCategoryHidden( *ClassCategory );
			}
		}
		return bResult;
	}

	bool IsVariableCategoryHiddenFromClass( const UProperty* InVariable,const UClass* Class )
	{
		bool bResult = false;
		if( InVariable && Class )
		{
			bResult = Class->IsCategoryHidden( *GetCategory(InVariable) );
		}
		return bResult;
	}
}

#endif // WITH_EDITOR
