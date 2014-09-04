// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SDesignerToolBar.h"
#include "DesignerCommands.h"

#define LOCTEXT_NAMESPACE "UMG"

void SDesignerToolBar::Construct( const FArguments& InArgs )
{
	CommandList = InArgs._CommandList;

	ChildSlot
	[
		MakeToolBar(InArgs._Extenders)
	];

	SViewportToolBar::Construct(SViewportToolBar::FArguments());
}

TSharedRef< SWidget > SDesignerToolBar::MakeToolBar(const TSharedPtr< FExtender > InExtenders)
{
	FToolBarBuilder ToolbarBuilder( CommandList, FMultiBoxCustomization::None, InExtenders );

	// Use a custom style
	FName ToolBarStyle = "ViewportMenu";
	ToolbarBuilder.SetStyle(&FEditorStyle::Get(), ToolBarStyle);
	ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);

	// Transform controls cannot be focusable as it fights with the press space to change transform mode feature
	ToolbarBuilder.SetIsFocusable( false );

	ToolbarBuilder.BeginSection("Transform");
	ToolbarBuilder.BeginBlockGroup();
	{
		ToolbarBuilder.AddToolBarButton( FDesignerCommands::Get().LayoutTransform, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), "LayoutTransform" );
		ToolbarBuilder.AddToolBarButton( FDesignerCommands::Get().RenderTransform, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), "RenderTransform" );
	}
	ToolbarBuilder.EndBlockGroup();
	ToolbarBuilder.EndSection();
	
	return ToolbarBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
