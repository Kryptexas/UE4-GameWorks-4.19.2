// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "SOutputLogDialog.h"
#include "SHyperlink.h"

void SOutputLogDialog::Open( const FText& InTitle, const FText& InHeader, const FText& InLog, const FText& InFooter )
{
	TSharedRef<SWindow> ModalWindow = SNew(SWindow)
		.Title( InTitle )
		.SizingRule(ESizingRule::Autosized)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMinimize(false) 
		.SupportsMaximize(false);

	TSharedRef<SOutputLogDialog> MessageBox = SNew(SOutputLogDialog)
		.ParentWindow(ModalWindow)
		.Header( InHeader )
		.Log( InLog )
		.Footer( InFooter );

	ModalWindow->SetContent( MessageBox );

	GEditor->EditorAddModalWindow(ModalWindow);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SOutputLogDialog::Construct( const FArguments& InArgs )
{
	ParentWindow = InArgs._ParentWindow.Get();
	ParentWindow->SetWidgetToFocusOnActivate(SharedThis(this));

	FSlateFontInfo MessageFont( FEditorStyle::GetFontStyle("StandardDialog.LargeFont"));
	Header = InArgs._Header;
	Log = InArgs._Log;
	Footer = InArgs._Footer;

	MaxWidth = FSlateApplication::Get().GetPreferredWorkArea().GetSize().X * 0.8f;

	TSharedPtr<SUniformGridPanel> ButtonBox;

	this->ChildSlot
		[	
			SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)

					+SVerticalBox::Slot()
						.HAlign(HAlign_Fill)
						.AutoHeight()
						.Padding(12.0f)
						[
							SNew(STextBlock)
								.Text(Header)
								.Visibility(Header.Get().IsEmptyOrWhitespace()? EVisibility::Hidden : EVisibility::Visible)
								.Font(MessageFont)
								.WrapTextAt(MaxWidth - 50.0f)
						]

					+SVerticalBox::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						.FillHeight(1.0f)
						.MaxHeight(550)
						.Padding(12.0f, 0.0f, 12.0f, 12.0f)
						[
							SNew(SMultiLineEditableTextBox)
								.Style(FEditorStyle::Get(), "Log.TextBox")
								.TextStyle(FEditorStyle::Get(), "Log.Normal")
								.ForegroundColor(FLinearColor::Gray)
								.Text(FText::TrimTrailing(Log.Get()))
								.IsReadOnly(true)
								.AlwaysShowScrollbars(true)
						]

					+SVerticalBox::Slot()
						.HAlign(HAlign_Fill)
						.AutoHeight()
						.Padding(12.0f, 0.0f, 12.0f, Footer.Get().IsEmptyOrWhitespace()? 0.0f : 12.0f)
						[
							SNew(STextBlock)
								.Text(Footer)
								.Visibility(Footer.Get().IsEmptyOrWhitespace()? EVisibility::Hidden : EVisibility::Visible)
								.Font(MessageFont)
								.WrapTextAt(MaxWidth - 50.0f)
						]

					+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(12.0f, 0.0f, 12.0f, 12.0f)
						[
							SNew(SHorizontalBox)

							+SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.HAlign(HAlign_Left)
								.VAlign(VAlign_Center)
								.Padding(0.0f)
								[
									SNew(SHyperlink)
										.OnNavigate(this, &SOutputLogDialog::HandleCopyMessageHyperlinkNavigate)
										.Text( NSLOCTEXT("SOutputLogDialog", "CopyMessageHyperlink", "Copy Message") )
										.ToolTipText( NSLOCTEXT("SOutputLogDialog", "CopyMessageTooltip", "Copy the text in this message to the clipboard (CTRL+C)") )
								]

							+SHorizontalBox::Slot()
								.AutoWidth()
								.HAlign(HAlign_Right)
								.VAlign(VAlign_Center)
								.Padding(0.f)
								[
									SNew( SButton )
										.Text( NSLOCTEXT("SOutputLogDialog", "Ok", "Ok") )
										.OnClicked( this, &SOutputLogDialog::HandleButtonClicked )
										.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
										.HAlign(HAlign_Center)
								]
						]
				]
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply SOutputLogDialog::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{
	if( InKeyEvent.GetKey() == EKeys::Escape )
	{
		return HandleButtonClicked();
	}
	else if (InKeyEvent.GetKey() == EKeys::C && InKeyEvent.IsControlDown())
	{
		CopyMessageToClipboard();
		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}

bool SOutputLogDialog::SupportsKeyboardFocus() const
{
	return true;
}

FVector2D SOutputLogDialog::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	FVector2D DesiredSize = SCompoundWidget::ComputeDesiredSize(LayoutScaleMultiplier);
	DesiredSize.X = FMath::Min(DesiredSize.X, MaxWidth);
	return DesiredSize;
}

void SOutputLogDialog::CopyMessageToClipboard( )
{
	FString FullMessage = FString::Printf(TEXT("%s") LINE_TERMINATOR LINE_TERMINATOR TEXT("%s") LINE_TERMINATOR LINE_TERMINATOR TEXT("%s"), *Header.Get().ToString(), *Log.Get().ToString(), *Footer.Get().ToString()).Trim();
	FPlatformMisc::ClipboardCopy( *FullMessage );
}

FReply SOutputLogDialog::HandleButtonClicked( )
{
	ParentWindow->RequestDestroyWindow();
	return FReply::Handled();
}

void SOutputLogDialog::HandleCopyMessageHyperlinkNavigate( )
{
	CopyMessageToClipboard();
}
