// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UComboBoxString

UComboBoxString::UComboBoxString(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SComboBox< TSharedPtr<FString> >::FArguments SlateDefaults;
	WidgetStyle = *SlateDefaults._ComboBoxStyle;

	ContentPadding = FMargin(4.0, 2.0);
	MaxListHeight = 450.0f;
	HasDownArrow = true;
}

void UComboBoxString::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyComboBox.Reset();
	ComoboBoxContent.Reset();
}

TSharedRef<SWidget> UComboBoxString::RebuildWidget()
{
	for ( FString& DefaultOptions : DefaultOptions )
	{
		AddOption(DefaultOptions);
	}

	int32 InitialIndex = FindOptionIndex(SelectedOption);
	if ( InitialIndex != -1 )
	{
		CurrentOptionPtr = Options[InitialIndex];
	}

	MyComboBox =
		SNew(SComboBox< TSharedPtr<FString> >)
		.ComboBoxStyle(&WidgetStyle)
		.OptionsSource(&Options)
		.InitiallySelectedItem(CurrentOptionPtr)
		.ContentPadding(ContentPadding)
		.MaxListHeight(MaxListHeight)
		.HasDownArrow(HasDownArrow)
		.OnGenerateWidget(BIND_UOBJECT_DELEGATE(SComboBox< TSharedPtr<FString> >::FOnGenerateWidget, HandleGenerateWidget))
		.OnSelectionChanged(BIND_UOBJECT_DELEGATE(SComboBox< TSharedPtr<FString> >::FOnSelectionChanged, HandleSelectionChanged))
		.OnComboBoxOpening(BIND_UOBJECT_DELEGATE(FOnComboBoxOpening, HandleOpening))
		[
			SAssignNew(ComoboBoxContent, SBox)
		];

	if ( InitialIndex != -1 )
	{
		// Generate the widget for the initially selected widget if needed
		ComoboBoxContent->SetContent(HandleGenerateWidget(CurrentOptionPtr));
	}

	return MyComboBox.ToSharedRef();
}

void UComboBoxString::AddOption(const FString& Option)
{
	Options.Add(MakeShareable(new FString(Option)));
}

bool UComboBoxString::RemoveOption(const FString& Option)
{
	int32 OptionIndex = FindOptionIndex(Option);
	if (Options[OptionIndex] == CurrentOptionPtr)
	{
		CurrentOptionPtr.Reset();
	}
	if ( OptionIndex != -1 )
	{
		Options.RemoveAt(OptionIndex);
		return true;
	}

	return false;
}

int32 UComboBoxString::FindOptionIndex(const FString& Option) const
{
	for ( int32 OptionIndex = 0; OptionIndex < Options.Num(); OptionIndex++ )
	{
		const TSharedPtr<FString>& OptionAtIndex = Options[OptionIndex];

		if ( ( *OptionAtIndex ) == Option )
		{
			return OptionIndex;
		}
	}

	return -1;
}

FString UComboBoxString::GetOptionAtIndex(int32 Index) const
{
	if (Index >= 0 && Index < Options.Num())
	{
		return *(Options[Index]);
	}
	return FString();
}

void UComboBoxString::ClearOptions()
{
	Options.Empty();
}

void UComboBoxString::SetSelectedOption(FString Option)
{
	int32 InitialIndex = FindOptionIndex(Option);
	if (InitialIndex != -1)
	{
		CurrentOptionPtr = Options[InitialIndex];
		ComoboBoxContent->SetContent(HandleGenerateWidget(CurrentOptionPtr));
	}
}

FString UComboBoxString::GetSelectedOption() const
{
	if (CurrentOptionPtr.IsValid())
	{
		return *CurrentOptionPtr;
	}
	return FString();
}

TSharedRef<SWidget> UComboBoxString::HandleGenerateWidget(TSharedPtr<FString> Item) const
{
	// Call the user's delegate to see if they want to generate a custom widget bound to the data source.
	if ( !IsDesignTime() && OnGenerateWidgetEvent.IsBound() )
	{
		UWidget* Widget = OnGenerateWidgetEvent.Execute(*Item);
		if ( Widget != NULL )
		{
			return Widget->TakeWidget();
		}
	}

	// If a row wasn't generated just create the default one, a simple text block of the item's name.
	return SNew(STextBlock).Text(FText::FromString(*Item));
}

void UComboBoxString::HandleSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectionType)
{
	CurrentOptionPtr = Item;

	if ( !IsDesignTime() )
	{
		OnSelectionChanged.Broadcast(*Item, SelectionType);
	}

	// When the selection changes we always generate another widget to represent the content area of the comobox.
	ComoboBoxContent->SetContent( HandleGenerateWidget(Item) );
}

void UComboBoxString::HandleOpening()
{
	OnOpening.Broadcast();
}

#if WITH_EDITOR

const FSlateBrush* UComboBoxString::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.ComboBox");
}

const FText UComboBoxString::GetPaletteCategory()
{
	return LOCTEXT("Input", "Input");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
