// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "TutorialStructCustomization.h"
#include "EditorTutorial.h"
#include "STutorialEditableText.h"

#define LOCTEXT_NAMESPACE "TutorialStructCustomization"

TSharedRef<IPropertyTypeCustomization> FTutorialContentCustomization::MakeInstance()
{
	return MakeShareable( new FTutorialContentCustomization );
}

void FTutorialContentCustomization::CustomizeHeader( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	TSharedPtr<IPropertyHandle> TypeProperty = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTutorialContent, Type));
	TSharedPtr<IPropertyHandle> ContentProperty = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTutorialContent, Content));
	TSharedPtr<IPropertyHandle> ExcerptNameProperty = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTutorialContent, ExcerptName));
	TSharedPtr<IPropertyHandle> TextProperty = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTutorialContent, Text));

	// Show and hide various widgets based on current content type
	struct Local
	{
		static EVisibility GetContentVisibility(TSharedPtr<IPropertyHandle> InPropertyHandle)
		{
			check(InPropertyHandle.IsValid());

			uint8 Value = 0;
			if(InPropertyHandle->GetValue(Value) == FPropertyAccess::Success)
			{
				const ETutorialContent::Type EnumValue = (ETutorialContent::Type)Value;
				return (EnumValue == ETutorialContent::UDNExcerpt) ? EVisibility::Visible : EVisibility::Collapsed;
			}

			return EVisibility::Collapsed;
		}

		static EVisibility GetExcerptNameVisibility(TSharedPtr<IPropertyHandle> InPropertyHandle)
		{
			check(InPropertyHandle.IsValid());

			uint8 Value = 0;
			if(InPropertyHandle->GetValue(Value) == FPropertyAccess::Success)
			{
				const ETutorialContent::Type EnumValue = (ETutorialContent::Type)Value;
				return (EnumValue == ETutorialContent::UDNExcerpt) ? EVisibility::Visible : EVisibility::Collapsed;
			}

			return EVisibility::Collapsed;
		}

		static EVisibility GetTextVisibility(TSharedPtr<IPropertyHandle> InPropertyHandle)
		{
			check(InPropertyHandle.IsValid());

			uint8 Value = 0;
			if(InPropertyHandle->GetValue(Value) == FPropertyAccess::Success)
			{
				const ETutorialContent::Type EnumValue = (ETutorialContent::Type)Value;
				return (EnumValue == ETutorialContent::Text) ? EVisibility::Visible : EVisibility::Collapsed;
			}

			return EVisibility::Collapsed;
		}

		static EVisibility GetRichTextVisibility(TSharedPtr<IPropertyHandle> InPropertyHandle)
		{
			check(InPropertyHandle.IsValid());

			uint8 Value = 0;
			if(InPropertyHandle->GetValue(Value) == FPropertyAccess::Success)
			{
				const ETutorialContent::Type EnumValue = (ETutorialContent::Type)Value;
				return (EnumValue == ETutorialContent::RichText) ? EVisibility::Visible : EVisibility::Collapsed;
			}

			return EVisibility::Collapsed;
		}

		static FText GetValueAsText(TSharedPtr<IPropertyHandle> InPropertyHandle)
		{
			FText Text;

			if( InPropertyHandle->GetValueAsFormattedText( Text ) == FPropertyAccess::MultipleValues )
			{
				Text = NSLOCTEXT("PropertyEditor", "MultipleValues", "Multiple Values");
			}

			return Text;
		}

		static void OnTextCommitted( const FText& NewText, ETextCommit::Type /*CommitInfo*/, TSharedPtr<IPropertyHandle> InPropertyHandle )
		{
			InPropertyHandle->SetValueFromFormattedString( NewText.ToString() );
		}

		static void OnTextChanged( const FText& NewText, TSharedPtr<IPropertyHandle> InPropertyHandle )
		{
			InPropertyHandle->SetValueFromFormattedString( NewText.ToString() );
		}
	};

	HeaderRow
	.NameContent()
	[
		ContentProperty->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(250.0f)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			TypeProperty->CreatePropertyValueWidget()
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			.Visibility_Static(&Local::GetContentVisibility, TypeProperty)
			+SHorizontalBox::Slot()
			[
				ContentProperty->CreatePropertyValueWidget()
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			.Visibility_Static(&Local::GetExcerptNameVisibility, TypeProperty)
			+SHorizontalBox::Slot()
			[
				ExcerptNameProperty->CreatePropertyValueWidget()
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			.Visibility_Static(&Local::GetTextVisibility, TypeProperty)
			+SHorizontalBox::Slot()
			[
				TextProperty->CreatePropertyValueWidget()
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			.Visibility_Static(&Local::GetRichTextVisibility, TypeProperty)
			+SHorizontalBox::Slot()
			[
				SNew(STutorialEditableText)
				.Text_Static(&Local::GetValueAsText, TextProperty)
				.OnTextCommitted(FOnTextCommitted::CreateStatic(&Local::OnTextCommitted, TextProperty))
				.OnTextChanged(FOnTextChanged::CreateStatic(&Local::OnTextChanged, TextProperty))
			]
		]
	];
}

void FTutorialContentCustomization::CustomizeChildren( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{

}

/** 'Tooltip' window to indicate what is currently being picked and how to abort the picker (Esc) */
class SWidgetPickerFloatingWindow : public SCompoundWidget
{
	SLATE_BEGIN_ARGS( SWidgetPickerFloatingWindow ) {}

	SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<class IPropertyHandle> InStructPropertyHandle)
	{
		StructPropertyHandle = InStructPropertyHandle;
		ParentWindow = InArgs._ParentWindow;

		ChildSlot
		[
			SNew(SToolTip)
			.Text(this, &SWidgetPickerFloatingWindow::GetPickerStatusText)
		];
	}

	FName GetPickedWidgetName()
	{
		return PickedWidgetName;
	}

private:
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override
	{
		PickedWidgetName = NAME_None;

		FWidgetPath Path = FSlateApplication::Get().LocateWindowUnderMouse(FSlateApplication::Get().GetCursorPos(), FSlateApplication::Get().GetInteractiveTopLevelWindows(), true);

		for(int32 PathIndex = Path.Widgets.Num() - 1; PathIndex >= 0; PathIndex--)
		{
			TSharedRef<SWidget> PathWidget = Path.Widgets[PathIndex].Widget;
			if(PathWidget->GetTag() != NAME_None)
			{
				PickedWidgetName = PathWidget->GetTag();
				break;
			}
		}

		// kind of a hack, but we need to maintain keyboard focus otherwise we wont get our keypress to 'pick'
		FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EKeyboardFocusCause::SetDirectly);
		if(ParentWindow.IsValid())
		{
			// also kind of a hack, but this is the only way at the moment to get a 'cursor decorator' without using the drag-drop code path
			ParentWindow.Pin()->MoveWindowTo(FSlateApplication::Get().GetCursorPos() + FSlateApplication::Get().GetCursorSize());
		}
	}

	FText GetPickerStatusText() const
	{
		return FText::Format(LOCTEXT("TootipHint", "{0} (Esc to pick)"), FText::FromName(PickedWidgetName));
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent) override
	{
		if(InKeyboardEvent.GetKey() == EKeys::Escape)
		{
			TSharedPtr<IPropertyHandle> WrapperIdentifierProperty = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTutorialContentAnchor, WrapperIdentifier));
			WrapperIdentifierProperty->SetValue(PickedWidgetName);

			TSharedPtr<IPropertyHandle> TypeProperty = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTutorialContentAnchor, Type));
			TypeProperty->SetValue((uint8)ETutorialAnchorIdentifier::NamedWidget);
			
			PickedWidgetName = NAME_None;

			if(ParentWindow.IsValid())
			{
				FSlateApplication::Get().RequestDestroyWindow(ParentWindow.Pin().ToSharedRef());
				ParentWindow.Reset();
			}

			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	/** We need to support keyboard focus to process the 'Esc' key */
	virtual bool SupportsKeyboardFocus() const override
	{
		return true;
	}

private:
	/** Handle to the property struct */
	TSharedPtr<class IPropertyHandle> StructPropertyHandle;

	/** Handle to the window that contains this widget */
	TWeakPtr<SWindow> ParentWindow;

	/** The widget name we are picking */
	FName PickedWidgetName;
};

/** Widget used to launch a 'picking' session */
class SWidgetPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SWidgetPicker ) {}

	SLATE_END_ARGS()

	~SWidgetPicker()
	{
		// kill the picker window as well if this widget is going away - that way we dont get dangling refs to the property
		if(PickerWindow.IsValid() && FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().RequestDestroyWindow(PickerWindow.Pin().ToSharedRef());
			PickerWindow.Reset();
			PickerWidget.Reset();
		}
	}

	void Construct(const FArguments& InArgs, TSharedRef<class IPropertyHandle> InStructPropertyHandle, IPropertyTypeCustomizationUtils* InStructCustomizationUtils)
	{
		StructPropertyHandle = InStructPropertyHandle;

		ChildSlot
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SWidgetPicker::HandlePickerStatusText)
				.Font(InStructCustomizationUtils->GetRegularFont())
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
				.OnClicked( this, &SWidgetPicker::OnClicked )
				.ContentPadding(4.0f)
				.ForegroundColor( FSlateColor::UseForeground() )
				.IsFocusable(false)
				[
					SNew( SImage )
					.Image( FEditorStyle::GetBrush("PropertyWindow.Button_PickActorInteractive") )
					.ColorAndOpacity( FSlateColor::UseForeground() )
				]
			]
		];
	}

private:
	FReply OnClicked()
	{
		// launch a picker window
		if(!PickerWindow.IsValid())
		{
			TSharedRef<SWindow> NewWindow = SWindow::MakeCursorDecorator();
			NewWindow->MoveWindowTo(FSlateApplication::Get().GetCursorPos());
			PickerWindow = NewWindow;

			NewWindow->SetContent(
				SAssignNew(PickerWidget, SWidgetPickerFloatingWindow, StructPropertyHandle.ToSharedRef())
				.ParentWindow(NewWindow)
				);

			TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
			check(RootWindow.IsValid());
			FSlateApplication::Get().AddWindowAsNativeChild(NewWindow, RootWindow.ToSharedRef());

			FIntroTutorials& IntroTutorials = FModuleManager::Get().GetModuleChecked<FIntroTutorials>("IntroTutorials");
			IntroTutorials.OnIsPicking().BindSP(this, &SWidgetPicker::OnIsPicking);
		}

		return FReply::Handled();
	}

	FText HandlePickerStatusText() const
	{
		FName WidgetName = NAME_None;
		TSharedPtr<IPropertyHandle> WrapperIdentifierProperty = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTutorialContentAnchor, WrapperIdentifier));

		// Some of the tags contain a GUID, we dont want to see that here
		FString WidgetValue;
		WrapperIdentifierProperty->GetValue(WidgetValue);
		return MakeFriendlyStringFromName(WidgetValue);
	}

	FText MakeFriendlyStringFromName(const FString& WidgetName) const
	{
		// We will likely have meta data for this eventually. For now just parse a comma delimited string which currently will either be a name or ident,name,UID
		if (WidgetName == "None")
		{
			return FText::FromName(*WidgetName);
		}
		TArray<FString> Tokens;
		WidgetName.ParseIntoArray(&Tokens, TEXT(","),true);
		if (Tokens.Num() == 3)
		{
			return FText::FromName(*Tokens[1]);
		}
		else if (Tokens.Num() == 3)
		{
			return FText::FromName(*Tokens[0]);
		}
		return FText::FromName(*WidgetName);
	}

	bool OnIsPicking(FName& OutWidgetNameToHighlight) const
	{
		if(PickerWidget.IsValid())
		{
			OutWidgetNameToHighlight = PickerWidget.Pin()->GetPickedWidgetName();
			return true;
		}
		
		return false;
	}

private:
	/** Picker window widget */
	TWeakPtr<SWidgetPickerFloatingWindow> PickerWidget;

	/** Picker window */
	TWeakPtr<SWindow> PickerWindow;

	/** Handle to the struct we are customizing */
	TSharedPtr<class IPropertyHandle> StructPropertyHandle;
};

TSharedRef<IPropertyTypeCustomization> FTutorialContentAnchorCustomization::MakeInstance()
{
	return MakeShareable( new FTutorialContentAnchorCustomization );
}

void FTutorialContentAnchorCustomization::CustomizeHeader( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	TSharedPtr<IPropertyHandle> DrawHighlightProperty = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTutorialContentAnchor, bDrawHighlight));

	HeaderRow
	.NameContent()
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			InStructPropertyHandle->CreatePropertyNameWidget()
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			DrawHighlightProperty->CreatePropertyNameWidget()
		]
	]
	.ValueContent()
	.MinDesiredWidth(250.0f)
	.MaxDesiredWidth(500.0f)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			SNew(SWidgetPicker, InStructPropertyHandle, &StructCustomizationUtils)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			DrawHighlightProperty->CreatePropertyValueWidget()
		]
	];
}

void FTutorialContentAnchorCustomization::CustomizeChildren( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{

}

#undef LOCTEXT_NAMESPACE