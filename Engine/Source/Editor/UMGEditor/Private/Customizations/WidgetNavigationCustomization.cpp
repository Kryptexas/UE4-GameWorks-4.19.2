// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "PropertyEditing.h"
#include "ObjectEditorUtils.h"
#include "WidgetGraphSchema.h"
#include "ScopedTransaction.h"
#include "BlueprintEditorUtils.h"
#include "Blueprint/WidgetNavigation.h"
#include "WidgetNavigationCustomization.h"

#define LOCTEXT_NAMESPACE "UMG"

// FWidgetNavigationCustomization
////////////////////////////////////////////////////////////////////////////////

void FWidgetNavigationCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FWidgetNavigationCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TWeakPtr<IPropertyHandle> PropertyHandlePtr(PropertyHandle);

	MakeNavRow(PropertyHandlePtr, ChildBuilder, EUINavigation::Left, LOCTEXT("LeftNavigation", "Left"));
	MakeNavRow(PropertyHandlePtr, ChildBuilder, EUINavigation::Right, LOCTEXT("RightNavigation", "Right"));
	MakeNavRow(PropertyHandlePtr, ChildBuilder, EUINavigation::Up, LOCTEXT("UpNavigation", "Up"));
	MakeNavRow(PropertyHandlePtr, ChildBuilder, EUINavigation::Down, LOCTEXT("DownNavigation", "Down"));
}

void FWidgetNavigationCustomization::FillOutChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Generate all the other children
	uint32 NumChildren;
	PropertyHandle->GetNumChildren(NumChildren);

	for ( uint32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++ )
	{
		TSharedRef<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		if ( ChildHandle->IsCustomized() )
		{
			continue;
		}

		if ( ChildHandle->GetProperty() == NULL )
		{
			FillOutChildren(ChildHandle, ChildBuilder, CustomizationUtils);
		}
		else
		{
			ChildBuilder.AddChildProperty(ChildHandle);
		}
	}
}

FReply FWidgetNavigationCustomization::OnCustomizeNavigation(TWeakPtr<IPropertyHandle> PropertyHandle)
{
	TArray<UObject*> OuterObjects;
	PropertyHandle.Pin()->GetOuterObjects(OuterObjects);

	const FScopedTransaction Transaction(LOCTEXT("UpdateNavigation", "Update Navigation"));

	for ( UObject* OuterObject : OuterObjects )
	{
		if ( UWidget* Widget = Cast<UWidget>(OuterObject) )
		{
			if ( !Widget->Navigation )
			{
				Widget->Modify();
				Widget->Navigation = ConstructObject<UWidgetNavigation>(UWidgetNavigation::StaticClass(), Widget);
			}
		}
	}

	return FReply::Handled();
}

FText FWidgetNavigationCustomization::HandleNavigationText(TWeakPtr<IPropertyHandle> PropertyHandle, EUINavigation Nav) const
{
	TArray<UObject*> OuterObjects;
	TSharedPtr<IPropertyHandle> PropertyHandlePtr = PropertyHandle.Pin();
	PropertyHandlePtr->GetOuterObjects(OuterObjects);

	EUINavigationRule Rule = EUINavigationRule::Invalid;
	for (UObject* OuterObject : OuterObjects)
	{
		if (UWidget* Widget = Cast<UWidget>(OuterObject))
		{
			EUINavigationRule CurRule = EUINavigationRule::Escape;
			UWidgetNavigation* WidgetNavigation = Widget->Navigation;
			if (Widget->Navigation != nullptr)
			{
				CurRule = WidgetNavigation->GetNavigationRule(Nav);
			}

			if (Rule != EUINavigationRule::Invalid && CurRule != Rule)
			{
				return LOCTEXT("NavigationMultipleValues", "Multiple Values");
			}
			Rule = CurRule;
		}
	}

	switch (Rule)
	{
	case EUINavigationRule::Escape:
		return LOCTEXT("NavigationEscape", "Escape");
	case EUINavigationRule::Stop:
		return LOCTEXT("NavigationStop", "Stop");
	case EUINavigationRule::Wrap:
		return LOCTEXT("NavigationWrap", "Wrap");
	}

	return FText::GetEmpty();
}

void FWidgetNavigationCustomization::MakeNavRow(TWeakPtr<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, EUINavigation Nav, FText NavName)
{
	ChildBuilder.AddChildContent(NavName)
		.NameContent()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(NavName)
		]
		.ValueContent()
		[
			SNew(SComboButton)
			.HAlign(HAlign_Center)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &FWidgetNavigationCustomization::HandleNavigationText, PropertyHandle, Nav)
			]
			.ContentPadding(FMargin(2.0f, 1.0f))
			.MenuContent()
			[
				MakeNavMenu(PropertyHandle, Nav)
			]
		];
}

TSharedRef<class SWidget> FWidgetNavigationCustomization::MakeNavMenu(TWeakPtr<IPropertyHandle> PropertyHandle, EUINavigation Nav)
{
	// create build configurations menu
	FMenuBuilder MenuBuilder(true, NULL);
	{
		FUIAction EscapeAction(FExecuteAction::CreateSP(this, &FWidgetNavigationCustomization::HandleNavMenuEntryClicked, PropertyHandle, Nav, EUINavigationRule::Escape));
		MenuBuilder.AddMenuEntry(LOCTEXT("NavigationRuleEscape", "Escape"), LOCTEXT("NavigationRuleEscapeHint", "Navigation is allowed to escape the bounds of this widget."), FSlateIcon(), EscapeAction);

		FUIAction StopAction(FExecuteAction::CreateSP(this, &FWidgetNavigationCustomization::HandleNavMenuEntryClicked, PropertyHandle, Nav, EUINavigationRule::Stop));
		MenuBuilder.AddMenuEntry(LOCTEXT("NavigationRuleStop", "Stop"), LOCTEXT("NavigationRuleStopHint", "Navigation stops at the bounds of this widget."), FSlateIcon(), StopAction);

		FUIAction WrapAction(FExecuteAction::CreateSP(this, &FWidgetNavigationCustomization::HandleNavMenuEntryClicked, PropertyHandle, Nav, EUINavigationRule::Wrap));
		MenuBuilder.AddMenuEntry(LOCTEXT("NavigationRuleWrap", "Wrap"), LOCTEXT("NavigationRuleWrapHint", "Navigation will wrap to the opposite bound of this object."), FSlateIcon(), WrapAction);

		//FUIAction ExplicitAction(FExecuteAction::CreateSP(this, &FWidgetNavigationCustomization::HandleNavMenuEntryClicked, PropertyHandle, Nav, EUINavigationRule::Explicit));
		//MenuBuilder.AddMenuEntry(LOCTEXT("NavigationRuleExplicit", "Explicit"), LOCTEXT("NavigationRuleExplicitHint", "Navigation will go to a specified widget."), FSlateIcon(), ExplicitAction);

		//FUIAction CustomAction(FExecuteAction::CreateSP(this, &FWidgetNavigationCustomization::HandleNavMenuEntryClicked, PropertyHandle, Nav, EUINavigationRule::Custom));
		//MenuBuilder.AddMenuEntry(LOCTEXT("NavigationRuleCustom", "Custom"), LOCTEXT("NavigationRuleCustomHint", "Custom function can determine what widget is navigated to."), FSlateIcon(), CustomAction);
	}

	return MenuBuilder.MakeWidget();
}

// Callback for clicking a menu entry for a navigations rule.
void FWidgetNavigationCustomization::HandleNavMenuEntryClicked(TWeakPtr<IPropertyHandle> PropertyHandle, EUINavigation Nav, EUINavigationRule Rule)
{
	TArray<UObject*> OuterObjects;
	TSharedPtr<IPropertyHandle> PropertyHandlePtr = PropertyHandle.Pin();
	PropertyHandlePtr->GetOuterObjects(OuterObjects);

	const FScopedTransaction Transaction(LOCTEXT("InitializeNavigation", "Initialize Navigation"));

	for (UObject* OuterObject : OuterObjects)
	{
		if (UWidget* Widget = Cast<UWidget>(OuterObject))
		{
			FWidgetReference WidgetReference = Editor.Pin()->GetReferenceFromPreview(Widget);

			SetNav(WidgetReference.GetPreview(), Nav, Rule);
			SetNav(WidgetReference.GetTemplate(), Nav, Rule);
		}
	}
}

void FWidgetNavigationCustomization::SetNav(UWidget* Widget, EUINavigation Nav, EUINavigationRule Rule)
{
	Widget->Modify();

	UWidgetNavigation* WidgetNavigation = Widget->Navigation;
	if (!Widget->Navigation)
	{
		WidgetNavigation = ConstructObject<UWidgetNavigation>(UWidgetNavigation::StaticClass(), Widget);
	}

	switch (Nav)
	{
	case EUINavigation::Left:
		WidgetNavigation->Left.Rule = Rule;
		break;
	case EUINavigation::Right:
		WidgetNavigation->Right.Rule = Rule;
		break;
	case EUINavigation::Up:
		WidgetNavigation->Up.Rule = Rule;
		break;
	case EUINavigation::Down:
		WidgetNavigation->Down.Rule = Rule;
		break;
	}

	if ( WidgetNavigation->IsDefault() )
	{
		// If the navigation rules are all set to the defaults, remove the navigation
		// information from the widget.
		Widget->Navigation = nullptr;
	}
	else
	{
		Widget->Navigation = WidgetNavigation;
	}
}

#undef LOCTEXT_NAMESPACE
