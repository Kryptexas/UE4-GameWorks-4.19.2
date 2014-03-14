// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SlateStyleRegistry.h"

TMap< FName, const ISlateStyle* > FSlateStyleRegistry::SlateStyleRepository;

void FSlateStyleRegistry::RegisterSlateStyle( const ISlateStyle& InSlateStyle )
{
	const FName& SlateStyleName = InSlateStyle.GetStyleSetName();
	check( SlateStyleName.IsValid() );
	check( !FindSlateStyle( SlateStyleName ) );

	SlateStyleRepository.Add( SlateStyleName, &InSlateStyle );

	if ( FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer().IsValid() )
	{
		FSlateApplication::Get().GetRenderer()->LoadStyleResources( InSlateStyle );
	}
}

void FSlateStyleRegistry::UnRegisterSlateStyle( const ISlateStyle& InSlateStyle )
{
	const FName& SlateStyleName = InSlateStyle.GetStyleSetName();
	check( SlateStyleName.IsValid() );

	SlateStyleRepository.Remove( SlateStyleName );
}

void FSlateStyleRegistry::UnRegisterSlateStyle(const FName StyleSetName)
{
	check(StyleSetName.IsValid());
	SlateStyleRepository.Remove(StyleSetName);
}

const ISlateStyle* FSlateStyleRegistry::FindSlateStyle( const FName& InSlateStyleName )
{
	const ISlateStyle** SlateStylePtr = SlateStyleRepository.Find( InSlateStyleName );
	return (SlateStylePtr) ? *SlateStylePtr : nullptr;
}

void FSlateStyleRegistry::GetAllResources( TArray< const FSlateBrush* >& OutResources )
{
	/* Iterate the style chunks and collect their resources */
	for( auto It = SlateStyleRepository.CreateConstIterator(); It; ++It )
	{
		It->Value->GetResources( OutResources );
	}
}