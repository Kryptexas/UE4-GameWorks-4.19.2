// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UMenuAnchor

UMenuAnchor::UMenuAnchor(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Placement = MenuPlacement_ComboBox;
}

TSharedRef<SWidget> UMenuAnchor::RebuildWidget()
{
	MyMenuAnchor = SNew(SMenuAnchor)
		.Method(SMenuAnchor::UseCurrentWindow)
		.Placement(Placement)
		.OnGetMenuContent(BIND_UOBJECT_DELEGATE(FOnGetContent, HandleGetMenuContent));

	if ( GetChildrenCount() > 0 )
	{
		MyMenuAnchor->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->GetWidget() : SNullWidget::NullWidget);
	}
	
	return BuildDesignTimeWidget( MyMenuAnchor.ToSharedRef() );
}

void UMenuAnchor::SyncronizeProperties()
{
	Super::SyncronizeProperties();
}

void UMenuAnchor::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live slot if it already exists
	if ( MyMenuAnchor.IsValid() )
	{
		MyMenuAnchor->SetContent(Slot->Content ? Slot->Content->GetWidget() : SNullWidget::NullWidget);
	}
}

void UMenuAnchor::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyMenuAnchor.IsValid() )
	{
		MyMenuAnchor->SetContent(SNullWidget::NullWidget);
	}
}

TSharedRef<SWidget> UMenuAnchor::HandleGetMenuContent()
{
	TSharedPtr<SWidget> SlateMenuWidget;

	if ( OnGetMenuContentEvent.IsBound() )
	{
		UWidget* MenuWidget = OnGetMenuContentEvent.Execute();
		if ( MenuWidget )
		{
			SlateMenuWidget = MenuWidget->GetWidget();
		}
	}

	if ( MenuClass != NULL && !MenuClass->HasAnyClassFlags(CLASS_Abstract) )
	{
		UWidget* MenuWidget = (UWidget*)ConstructObject<UWidget>(MenuClass, GetOuter());
		if ( MenuWidget )
		{
			SlateMenuWidget = MenuWidget->GetWidget();
		}
	}

	return SlateMenuWidget.IsValid() ? SlateMenuWidget.ToSharedRef() : SNullWidget::NullWidget;
}

void UMenuAnchor::SetIsOpen(bool InIsOpen, bool bFocusMenu)
{
	if ( MyMenuAnchor.IsValid() )
	{
		MyMenuAnchor->SetIsOpen(InIsOpen, bFocusMenu);
	}
}

bool UMenuAnchor::IsOpen() const
{
	if ( MyMenuAnchor.IsValid() )
	{
		return MyMenuAnchor->IsOpen();
	}

	return false;
}

#if WITH_EDITOR

const FSlateBrush* UMenuAnchor::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.MenuAnchor");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
