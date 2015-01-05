// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UWidgetNavigation

UWidgetNavigation::UWidgetNavigation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR

EUINavigationRule UWidgetNavigation::GetNavigationRule(EUINavigation Nav)
{
	switch ( Nav )
	{
	case EUINavigation::Up:
		return Up.Rule;
	case EUINavigation::Down:
		return Down.Rule;
	case EUINavigation::Left:
		return Left.Rule;
	case EUINavigation::Right:
		return Right.Rule;
	}
	return EUINavigationRule::Escape;
}

#endif

void UWidgetNavigation::UpdateMetaData(TSharedRef<FNavigationMetaData> MetaData)
{
	UpdateMetaDataEntry(MetaData, Up, EUINavigation::Up);
	UpdateMetaDataEntry(MetaData, Down, EUINavigation::Down);
	UpdateMetaDataEntry(MetaData, Left, EUINavigation::Left);
	UpdateMetaDataEntry(MetaData, Right, EUINavigation::Right);
}

bool UWidgetNavigation::IsDefault() const
{
	return Up.Rule == EUINavigationRule::Escape &&
		Down.Rule == EUINavigationRule::Escape &&
		Left.Rule == EUINavigationRule::Escape &&
		Right.Rule == EUINavigationRule::Escape;
}

void UWidgetNavigation::UpdateMetaDataEntry(TSharedRef<FNavigationMetaData> MetaData, const FWidgetNavigationData & NavData, EUINavigation Nav)
{
	switch ( NavData.Rule )
	{
	case EUINavigationRule::Escape:
		MetaData->SetNavigationEscape(Nav);
		break;
	case EUINavigationRule::Stop:
		MetaData->SetNavigationStop(Nav);
		break;
	case EUINavigationRule::Wrap:
		MetaData->SetNavigationWrap(Nav);
		break;
	}
}
