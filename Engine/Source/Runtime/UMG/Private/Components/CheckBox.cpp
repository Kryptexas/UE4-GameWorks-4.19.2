// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UCheckBox

UCheckBox::UCheckBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	static const FName StyleName(TEXT("Style"));
	WidgetStyle = PCIP.CreateDefaultSubobject<UCheckBoxWidgetStyle>(this, StyleName);

	SCheckBox::FArguments SlateDefaults;
	WidgetStyle->CheckBoxStyle = *SlateDefaults._Style;

	CheckedState = ESlateCheckBoxState::Unchecked;

	HorizontalAlignment = SlateDefaults._HAlign;
	Padding = SlateDefaults._Padding.Get();

	BorderBackgroundColor = FLinearColor::White;
}

void UCheckBox::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyCheckbox.Reset();
}

TSharedRef<SWidget> UCheckBox::RebuildWidget()
{
	MyCheckbox = SNew(SCheckBox)
		.OnCheckStateChanged( BIND_UOBJECT_DELEGATE(FOnCheckStateChanged, SlateOnCheckStateChangedCallback) )
		.Style(&WidgetStyle->CheckBoxStyle)
		.HAlign( HorizontalAlignment )
		.Padding( Padding )
		.BorderBackgroundColor( BorderBackgroundColor )
		;

	if ( GetChildrenCount() > 0 )
	{
		MyCheckbox->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->TakeWidget() : SNullWidget::NullWidget);
	}
	
	return MyCheckbox.ToSharedRef();
}

void UCheckBox::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	MyCheckbox->SetIsChecked( OPTIONAL_BINDING(ESlateCheckBoxState::Type, CheckedState) );
}

void UCheckBox::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live slot if it already exists
	if ( MyCheckbox.IsValid() )
	{
		MyCheckbox->SetContent(Slot->Content ? Slot->Content->TakeWidget() : SNullWidget::NullWidget);
	}
}

void UCheckBox::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyCheckbox.IsValid() )
	{
		MyCheckbox->SetContent(SNullWidget::NullWidget);
	}
}

bool UCheckBox::IsPressed() const
{
	if ( MyCheckbox.IsValid() )
	{
		return MyCheckbox->IsPressed();
	}

	return false;
}

bool UCheckBox::IsChecked() const
{
	if ( MyCheckbox.IsValid() )
	{
		return MyCheckbox->IsChecked();
	}

	return ( CheckedState == ESlateCheckBoxState::Checked );
}

ESlateCheckBoxState::Type UCheckBox::GetCheckedState() const
{
	if ( MyCheckbox.IsValid() )
	{
		return MyCheckbox->GetCheckedState();
	}

	return CheckedState;
}

void UCheckBox::SetIsChecked(bool InIsChecked)
{
	CheckedState = InIsChecked ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	if ( MyCheckbox.IsValid() )
	{
		MyCheckbox->SetIsChecked(OPTIONAL_BINDING(ESlateCheckBoxState::Type, CheckedState));
	}
}

void UCheckBox::SetCheckedState(ESlateCheckBoxState::Type InCheckedState)
{
	CheckedState = InCheckedState;
	if ( MyCheckbox.IsValid() )
	{
		MyCheckbox->SetIsChecked(OPTIONAL_BINDING(ESlateCheckBoxState::Type, CheckedState));
	}
}

void UCheckBox::SlateOnCheckStateChangedCallback(ESlateCheckBoxState::Type NewState)
{
	//@TODO: Choosing to treat Undetermined as Checked
	const bool bWantsToBeChecked = NewState != ESlateCheckBoxState::Unchecked;

	if (OnCheckStateChanged.IsBound())
	{
		OnCheckStateChanged.Broadcast(bWantsToBeChecked);
	}
	else
	{
		CheckedState = NewState;
	}
}

void UCheckBox::PostLoad()
{
	Super::PostLoad();

	if ( GetLinkerUE4Version() < VER_UE4_DEPRECATE_UMG_STYLE_ASSETS )
	{
		if ( Style_DEPRECATED != nullptr )
		{
			const FCheckBoxStyle* StylePtr = Style_DEPRECATED->GetStyle<FCheckBoxStyle>();
			if ( StylePtr != nullptr )
			{
				WidgetStyle->CheckBoxStyle = *StylePtr;
			}

			Style_DEPRECATED = nullptr;
		}

		if ( UncheckedImage_DEPRECATED != nullptr )
		{
			WidgetStyle->CheckBoxStyle.UncheckedImage = UncheckedImage_DEPRECATED->Brush;
			UncheckedImage_DEPRECATED = nullptr;
		}

		if ( UncheckedHoveredImage_DEPRECATED != nullptr )
		{
			WidgetStyle->CheckBoxStyle.UncheckedHoveredImage = UncheckedHoveredImage_DEPRECATED->Brush;
			UncheckedHoveredImage_DEPRECATED = nullptr;
		}

		if ( UncheckedPressedImage_DEPRECATED != nullptr )
		{
			WidgetStyle->CheckBoxStyle.UncheckedPressedImage = UncheckedPressedImage_DEPRECATED->Brush;
			UncheckedPressedImage_DEPRECATED = nullptr;
		}

		if ( CheckedImage_DEPRECATED != nullptr )
		{
			WidgetStyle->CheckBoxStyle.CheckedImage = CheckedImage_DEPRECATED->Brush;
			CheckedImage_DEPRECATED = nullptr;
		}

		if ( CheckedHoveredImage_DEPRECATED != nullptr )
		{
			WidgetStyle->CheckBoxStyle.CheckedHoveredImage = CheckedHoveredImage_DEPRECATED->Brush;
			CheckedHoveredImage_DEPRECATED = nullptr;
		}

		if ( CheckedPressedImage_DEPRECATED != nullptr )
		{
			WidgetStyle->CheckBoxStyle.CheckedPressedImage = CheckedPressedImage_DEPRECATED->Brush;
			CheckedPressedImage_DEPRECATED = nullptr;
		}

		if ( UndeterminedImage_DEPRECATED != nullptr )
		{
			WidgetStyle->CheckBoxStyle.UndeterminedImage = UndeterminedImage_DEPRECATED->Brush;
			UndeterminedImage_DEPRECATED = nullptr;
		}

		if ( UndeterminedHoveredImage_DEPRECATED != nullptr )
		{
			WidgetStyle->CheckBoxStyle.UndeterminedHoveredImage = UndeterminedHoveredImage_DEPRECATED->Brush;
			UndeterminedHoveredImage_DEPRECATED = nullptr;
		}

		if ( UndeterminedPressedImage_DEPRECATED != nullptr )
		{
			WidgetStyle->CheckBoxStyle.UndeterminedPressedImage = UndeterminedPressedImage_DEPRECATED->Brush;
			UndeterminedPressedImage_DEPRECATED = nullptr;
		}
	}
}

#if WITH_EDITOR

const FSlateBrush* UCheckBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.CheckBox");
}

const FText UCheckBox::GetToolboxCategory()
{
	return LOCTEXT("Common", "Common");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
