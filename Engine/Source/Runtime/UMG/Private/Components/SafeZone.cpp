// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "Components/SafeZone.h"
#include "Components/SafeZoneSlot.h"
#include "Layout/SSafeZone.h"

#define LOCTEXT_NAMESPACE "UMG"

USafeZone::USafeZone()
{
	bCanHaveMultipleChildren = false;
}

#if WITH_EDITOR
const FSlateBrush* USafeZone::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush( "Widget.SafeZone" );
}

const FText USafeZone::GetPaletteCategory()
{
	return LOCTEXT( "Panel", "Panel" );
}
#endif

void USafeZone::OnSlotAdded( UPanelSlot* Slot )
{
	Super::OnSlotAdded( Slot );

	UpdateWidgetProperties();
}

void USafeZone::OnSlotRemoved( UPanelSlot* Slot )
{
	Super::OnSlotRemoved( Slot );

	if ( MySafeZone.IsValid() )
	{
		MySafeZone->SetContent( SNullWidget::NullWidget );
	}
}

UClass* USafeZone::GetSlotClass() const
{
	return USafeZoneSlot::StaticClass();
}

void USafeZone::UpdateWidgetProperties()
{
	if ( MySafeZone.IsValid() && GetChildrenCount() > 0 )
	{
		USafeZoneSlot* SafeSlot = CastChecked< USafeZoneSlot >( Slots[ 0 ] );

		MySafeZone->SetTitleSafe( SafeSlot->bIsTitleSafe );
		MySafeZone->SetHAlign( SafeSlot->HAlign );
		MySafeZone->SetVAlign( SafeSlot->VAlign );
		MySafeZone->SetPadding( SafeSlot->Padding );
	}
}

TSharedRef<SWidget> USafeZone::RebuildWidget()
{
	USafeZoneSlot* SafeSlot = Slots.Num() > 0 ? Cast< USafeZoneSlot >( Slots[ 0 ] ) : nullptr;

	MySafeZone = SNew( SSafeZone )
		.IsTitleSafe( SafeSlot ? SafeSlot->bIsTitleSafe : false )
		.HAlign( SafeSlot ? SafeSlot->HAlign : HAlign_Fill )
		.VAlign( SafeSlot ? SafeSlot->VAlign : VAlign_Fill )
		.Padding( SafeSlot ? SafeSlot->Padding : FMargin() )
		[
			GetChildAt( 0 ) ? GetChildAt( 0 )->TakeWidget() : SNullWidget::NullWidget
		];

	return BuildDesignTimeWidget( MySafeZone.ToSharedRef() );
}

#undef LOCTEXT_NAMESPACE