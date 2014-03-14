// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A dialog to create a new project or open an existing one
 */
class SGameProjectDialog
	: public SCompoundWidget
{
	// Enumerates available tabs.
	enum ETab
	{
		ProjectsTab,
		NewProjectTab
	};

public:

	SLATE_BEGIN_ARGS(SGameProjectDialog) { }
		SLATE_ARGUMENT(bool, AllowProjectOpening)
		SLATE_ARGUMENT(bool, AllowProjectCreate)
	SLATE_END_ARGS()

public:

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );

public:

	// Begin SWidget overrides

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

	// End SWidget overrides

protected:

	/**
	 * Fades the entire dialog in over a short period of time.
	 */
	void FadeIn( );

	/**
	 * Opens the specified project.
	 *
	 * @param ProjectFile - The project file to open.
	 *
	 * @return true if the project was opened, false otherwise.
	 */
	bool OpenProject( const FString& ProjectFile );

	/**
	 * Shows the 'New Project' tab.
	 */
	void ShowNewProjectTab( );

	/**
	 * Shows the project browser tab.
	 */
	FReply ShowProjectBrowser( );

private:

	// Callback for getting the color of the custom content area.
	FLinearColor HandleCustomContentColorAndOpacity() const;

	// Callback for clicking the 'Marketplace' button.
	FReply HandleMarketplaceTabButtonClicked( );

	// Callback for getting the foreground color of the 'Marketplace' button.
	FSlateColor HandleMarketplaceTabButtonForegroundColor( ) const;

	// Callback for clicking the 'New Project' button.
	FReply HandleNewProjectTabButtonClicked( );

	// Callback for getting the border image of the 'New Project' button.
	const FSlateBrush* HandleNewProjectTabHeaderBorderImage( ) const;

	// Callback for getting the button image of the 'New Project' button.
	const FSlateBrush* HandleNewProjectTabHeaderImage( ) const;

	// Callback for clicking the 'Projects' button.
	FReply HandleProjectsTabButtonClicked( );

	// Callback for getting the border image of the 'Projects' button.
	const FSlateBrush* HandleProjectsTabHeaderBorderImage( ) const;

	// Callback for getting the button image of the 'Projects' button.
	const FSlateBrush* HandleProjectsTabHeaderImage( ) const;

private:

	// Holds the fading animation.
	FCurveSequence FadeAnimation;

	// Holds the fading animation curve.
	FCurveHandle FadeCurve;

	/** The switcher widget to control which screen is in view */
	TSharedPtr<SWidgetSwitcher> ContentAreaSwitcher;
	TSharedPtr<SProjectBrowser> ProjectBrowser;
	TSharedPtr<SNewProjectWizard> NewProjectWizard;
	TSharedPtr<SButton> ProjectsTabButton;
	TSharedPtr<SButton> NewProjectTabButton;
	TSharedPtr<SButton> MarketplaceButton;

	ETab ActiveTab;

	EVisibility ProjectsTabVisibility;
	EVisibility NewProjectTabVisibility;
};
