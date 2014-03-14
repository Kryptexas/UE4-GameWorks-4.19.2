// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OutputLogPrivatePCH.h"
#include "OutputLogModule.h"
#include "SDebugConsole.h"


namespace DebugConsoleDefs
{
	// How many seconds to animate when console is summoned
	static const float IntroAnimationDuration = 0.25f;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SDebugConsole::Construct( const FArguments& InArgs, const EDebugConsoleStyle::Type InStyle, FOutputLogModule* OutputLogModule )
{
	CurrentStyle = InStyle;

	check( OutputLogModule != NULL );
	ChildSlot.Widget = 
		SNew( SVerticalBox )
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew( SVerticalBox )
					.Visibility( this, &SDebugConsole::MakeVisibleIfLogIsShown )
					
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding( 10.0f )
					[
						SNew(SBox)
						.HeightOverride( 200.0f )
						[
							SNew( SBorder )
								.BorderImage( FEditorStyle::GetBrush( "DebugConsole.Background" ) )
								.ColorAndOpacity( this, &SDebugConsole::GetAnimatedColorAndOpacity )
								.BorderBackgroundColor( this, &SDebugConsole::GetAnimatedSlateColor )
								[
									SNew( SSpacer )
								]
						]
					]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 10.0f )
			[
				SNew(SBox)
				.HeightOverride( 26.0f )
				[
					SNew( SBorder )
						.BorderImage( FEditorStyle::GetBrush( "Console.Background" ) )
						.ColorAndOpacity( this, &SDebugConsole::GetAnimatedColorAndOpacity )
						.BorderBackgroundColor( this, &SDebugConsole::GetAnimatedSlateColor )
						[
							SNew( SHorizontalBox )
								+SHorizontalBox::Slot()
									.AutoWidth()
									.Padding( 3.0f )
								[
									SNew( STextBlock )
										.Text( NSLOCTEXT("Console", "ConsoleLabel", "Console >").ToString() )
								]
								+SHorizontalBox::Slot()
									.Padding( 3.0f )
									.FillWidth( 1.0f )
								[
									OutputLogModule->MakeConsoleInputBox( EditableTextBox )
								]
						]
				]
			];

	// Kick off intro animation
	AnimCurve = FCurveSequence();
	AnimCurve.AddCurve( 0.0f, DebugConsoleDefs::IntroAnimationDuration, ECurveEaseFunction::QuadOut );
	AnimCurve.Play();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SDebugConsole::SDebugConsole()
	: CurrentStyle( EDebugConsoleStyle::Compact )
{
}


void SDebugConsole::SetFocusToEditableText()
{
	FWidgetPath WidgetToFocusPath;
	FSlateApplication::Get().GeneratePathToWidgetChecked( EditableTextBox.ToSharedRef(), WidgetToFocusPath );
	if( WidgetToFocusPath.IsValid() )
	{
		FSlateApplication::Get().SetKeyboardFocus( WidgetToFocusPath, EKeyboardFocusCause::SetDirectly );
	}
}


EVisibility SDebugConsole::MakeVisibleIfLogIsShown() const
{
	return CurrentStyle == EDebugConsoleStyle::WithLog ? EVisibility::Visible : EVisibility::Collapsed;
}


FLinearColor SDebugConsole::GetAnimatedColorAndOpacity() const
{
	return FLinearColor( 1.0f, 1.0f, 1.0f, AnimCurve.GetLerp() );
}


FSlateColor SDebugConsole::GetAnimatedSlateColor() const
{
	return FSlateColor( GetAnimatedColorAndOpacity() );
}
