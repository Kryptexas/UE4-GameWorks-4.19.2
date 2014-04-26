// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SWizard.h: Declares the SWizard class.
=============================================================================*/

#pragma once


/**
 * Delegate type for getting a page's enabled state.
 *
 * The first parameter is the page index.
 */
DECLARE_DELEGATE_OneParam(FOnWizardPageIsEnabled, int32)


/**
 * Implements a wizard widget.
 */
class SLATE_API SWizard
	: public SCompoundWidget
{
public:

	/**
	 * Implements a wizard page.
	 */
	class SLATE_API FWizardPage
	{
	public:

		SLATE_BEGIN_ARGS(FWizardPage)
			: _ButtonContent()
			, _CanShow(true)
			, _PageContent()
		{ }

			/** Holds the content of the button to be shown in the wizard's page selector. */
			SLATE_NAMED_SLOT(FArguments, ButtonContent)

			/** Holds a flag indicating whether this page can be activated (default = true). */
			SLATE_ATTRIBUTE(bool, CanShow)

			/** Exposes a delegate to be invoked when this page is being activated. */
			SLATE_EVENT(FSimpleDelegate, OnEnter)

			/** Exposes a delegate to be invoked when this page is being deactivated. */
			SLATE_EVENT(FSimpleDelegate, OnLeave)

			/** Holds the page content. */
			SLATE_DEFAULT_SLOT(FArguments, PageContent)

		SLATE_END_ARGS()


	public:

		/**
		 * Creates and initializes a new instance.
		 *
		 * @param InArgs - The arguments for the page.
		 */
		FWizardPage( const FArguments& InArgs )
			: ButtonContent(InArgs._ButtonContent)
			, Showable(InArgs._CanShow)
			, OnEnterDelegate(InArgs._OnEnter)
			, OnLeaveDelegate(InArgs._OnLeave)
			, PageContent(InArgs._PageContent)
		{ }


	public:

		/**
		 * Checks whether the page can be shown.
		 *
		 * @return bool - true if the page can be shown, false otherwise.
		 */
		bool CanShow( ) const
		{
			return Showable.Get();
		}

		/**
		 * Gets the button content.
		 *
		 * @return The button content widget.
		 */
		const TSharedRef<SWidget>& GetButtonContent( ) const
		{
			return ButtonContent.Widget;
		}

		/**
		 * Gets the page content.
		 *
		 * @return TAlwaysValidWidget - The page contents.
		 */
		const TSharedRef<SWidget>& GetPageContent( ) const
		{
			return PageContent.Widget;
		}

		/**
		 * Gets a delegate to be invoked when this page is being entered.
		 *
		 * @return FSimpleDelegate - The delegate.
		 */
		FSimpleDelegate& OnEnter( )
		{
			return OnEnterDelegate;
		}

		/**
		 * Gets a delegate to be invoked when this page is being left.
		 *
		 * @return FSimpleDelegate - The delegate.
		 */
		FSimpleDelegate& OnLeave( )
		{
			return OnLeaveDelegate;
		}


	private:

		// Holds the button content.
		TAlwaysValidWidget ButtonContent;

		// Holds a flag indicating whether this page can be activated.
		TAttribute<bool> Showable;

		// Holds a delegate to be invoked when the page is activated.
		FSimpleDelegate OnEnterDelegate;

		// Holds a delegate to be invoked when the page is deactivated.
		FSimpleDelegate OnLeaveDelegate;

		// Holds the page content.
		TAlwaysValidWidget PageContent;
	};


public:

	SLATE_BEGIN_ARGS(SWizard)
		: _CanFinish(true)
		, _FinishButtonText(NSLOCTEXT("SWizard", "DefaultFinishButtonText", "Finish").ToString())
		, _FinishButtonToolTip(NSLOCTEXT("SWizard", "DefaultFinishButtonTooltip", "Finish the wizard").ToString())
		, _InitialPageIndex(0)
		, _DesiredSize(FVector2D(0, 0))
		, _ShowPageList(true)
		, _ShowCancelButton(true)
	{ }

		SLATE_SUPPORTS_SLOT_WITH_ARGS(FWizardPage)

		/** Holds a flag indicating whether the 'Finish' button is enabled. */
		SLATE_ATTRIBUTE(bool, CanFinish)

		/** Holds the label text of the wizard's 'Finish' button. */
		SLATE_TEXT_ATTRIBUTE(FinishButtonText)

		/** Holds the tool tip text of the wizard's 'Finish' button. */
		SLATE_TEXT_ATTRIBUTE(FinishButtonToolTip)

		/** Holds the index of the initial page to be displayed (0 = default). */
		SLATE_ATTRIBUTE(int32, InitialPageIndex)

		/** Holds the desired size. */
		SLATE_ATTRIBUTE(FVector2D, DesiredSize)

		/** Exposes a delegate to be invoked when the wizard is canceled. */
		SLATE_EVENT(FSimpleDelegate, OnCanceled)

		/** Exposes a delegate to be invoked when the wizard is finished. */
		SLATE_EVENT(FSimpleDelegate, OnFinished)

		/** Exposes a delegate to be invoked when the wizard's 'Back' button is clicked. This button is only visible if this delegate is set and only on the first page. */
		SLATE_EVENT(FOnClicked, OnFirstPageBackClicked)

		/** Exposes a delegate to be invoked when the wizard's 'Next' button is clicked. */
		SLATE_EVENT(FOnClicked, OnNextClicked)

		/** Exposes a delegate to be invoked when the wizard's 'Previous' button is clicked. */
		SLATE_EVENT(FOnClicked, OnPrevClicked)

		/** Holds a flag indicating whether the page list should be shown (default = true). */
		SLATE_ARGUMENT(bool, ShowPageList)

		/** Holds a flag indicating whether the cancel button should be shown (default = true). */
		SLATE_ARGUMENT(bool, ShowCancelButton)

	SLATE_END_ARGS()


public:

	/**
	 * Checks whether the page with the specified index can be shown.
	 *
	 * @param PageIndex - The index of the page to check.
	 *
	 * @return bool - true if the page can be shown, false otherwise.
	 */
	bool CanShowPage( int32 PageIndex ) const;

	/**
	 * Construct this widget.
	 *
	 * @param InArgs - The declaration data for this widget.
	 */
	void Construct( const FArguments& InArgs );

	/**
	 * Gets the number of pages that this wizard contains.
	 *
	 * @return The number of pages.
	 */
	int32 GetNumPages( ) const
	{
		return WidgetSwitcher->GetNumWidgets();
	}

	/**
	 * Gets the index of the specified wizard page widget.
	 *
	 * @param PageWidget - The page widget to get the index for.
	 *
	 * @return The index of the page, or INDEX_NONE if not found.
	 */
	int32 GetPageIndex( const TSharedRef<SWidget>& PageWidget ) const
	{
		return WidgetSwitcher->GetWidgetIndex(PageWidget);
	}

	/**
	 * Attempts to show the page with the specified index.
	 *
	 * @param PageIndex - The index of the page to show.
	 */
	void ShowPage( int32 PageIndex );


public:

	// Begin SCompoundWidget interface

	virtual FVector2D ComputeDesiredSize( ) const OVERRIDE;

	// End SCompoundWidget interface


public:

	/**
	 * Returns a new slot for a page.
	 *
	 * @param PageName - The name of the page.
	 *
	 * @return A new slot.
	 */
	static FWizardPage::FArguments Page( )
	{
		FWizardPage::FArguments Args;
		return Args;
	}


private:

	// Callback for clicking the 'Cancel' button.
	FReply HandleCancelButtonClicked( );

	// Callback for clicking the 'Launch' button.
	FReply HandleFinishButtonClicked( );

	// Callback for clicking the 'Next' button.
	FReply HandleNextButtonClicked( );

	// Callback for getting the enabled state of the 'Next' button.
	bool HandleNextButtonIsEnabled( ) const;

	// Callback for getting the visibility of the 'Next' button.
	EVisibility HandleNextButtonVisibility( ) const;

	// Callback for getting the checked state of a page button.
	void HandlePageButtonCheckStateChanged( ESlateCheckBoxState::Type NewState, int32 PageIndex );

	// Callback for clicking a page button.
	ESlateCheckBoxState::Type HandlePageButtonIsChecked( int32 PageIndex ) const;

	// Callback for getting the enabled state of a page button.
	bool HandlePageButtonIsEnabled( int32 PageIndex ) const;

	// Callback for clicking the 'Previous' button.
	FReply HandlePrevButtonClicked( );

	// Callback for getting the enabled state of the 'Previous' button.
	bool HandlePrevButtonIsEnabled( ) const;

	// Callback for getting the visibility of the 'Previous' button.
	EVisibility HandlePrevButtonVisibility( ) const;


private:

	// Holds the wizard's desired size.
	FVector2D DesiredSize;

	// Holds the collection of wizard pages.
	TIndirectArray<FWizardPage> Pages;

	// Holds the widget switcher.
	TSharedPtr<SWidgetSwitcher> WidgetSwitcher;


private:

	// Holds a delegate to be invoked when the 'Cancel' button has been clicked.
	FSimpleDelegate OnCanceled;

	// Holds a delegate to be invoked when the 'Finish' button has been clicked.
	FSimpleDelegate OnFinished;

	// Holds a delegate to be invoked when the 'Next' button has been clicked.
	FOnClicked OnNextClicked;

	// Holds a delegate to be invoked when the 'Previous' button has been clicked.
	FOnClicked OnPrevClicked;

	// Holds a delegate to be invoked when the 'Previous' button has been clicked on the first page.
	FOnClicked OnFirstPageBackClicked;
};
