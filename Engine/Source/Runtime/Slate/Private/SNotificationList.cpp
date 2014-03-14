// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"


/////////////////////////////////////////////////
// SNotificationExtendable

/** Contains the standard functionality for a notification to inherit from*/
class SNotificationExtendable : public SNotificationItem
{
public:
	virtual ~SNotificationExtendable()
	{
		// Just in case, make sure we have left responsive mode when getting cleaned up
		if ( ThrottleHandle.IsValid() )
		{
			FSlateThrottleManager::Get().LeaveResponsiveMode( ThrottleHandle );
		}
	}
	
	
	/** Sets the text for message element */
	virtual void SetText( const TAttribute< FText >& InText ) OVERRIDE
	{
		Text = InText;
		MyTextBlock->SetText( Text );
	}

	virtual ECompletionState GetCompletionState() const OVERRIDE
	{
		return CompletionState;
	}

	virtual void SetCompletionState(ECompletionState State) OVERRIDE
	{
		CompletionState = State;

		if (State == CS_Success || State == CS_Fail)
		{
			CompletionStateAnimation = FCurveSequence();
			GlowCurve = CompletionStateAnimation.AddCurve(0.f, 0.75f);
			CompletionStateAnimation.Play();
		}
	}

	virtual void ExpireAndFadeout() OVERRIDE
	{
		FadeAnimation = FCurveSequence();
		// Add some space for the expire time
		FadeAnimation.AddCurve(FadeOutDuration.Get(), ExpireDuration.Get());
		// Add the actual fade curve
		FadeCurve = FadeAnimation.AddCurve(0.f, FadeOutDuration.Get());
		FadeAnimation.PlayReverse();
	}

	/** Begins the fadein of this message */
	void Fadein( const bool bAllowThrottleWhenFrameRateIsLow )
	{
		// Make visible
		SetVisibility(EVisibility::Visible);

		// Play Fadein animation
		FadeAnimation = FCurveSequence();
		FadeCurve = FadeAnimation.AddCurve(0.f, FadeInDuration.Get());
		FadeAnimation.Play();

		// Scale up/flash animation
		IntroAnimation = FCurveSequence();
		ScaleCurveX = IntroAnimation.AddCurve(0.2f, 0.3f, ECurveEaseFunction::QuadOut);
		ScaleCurveY = IntroAnimation.AddCurve(0.f, 0.2f);
		GlowCurve = IntroAnimation.AddCurve(0.5f, 0.55f, ECurveEaseFunction::QuadOut);
		IntroAnimation.Play();

		// When a fade in occurs, we need a high framerate for the animation to look good
		if( FadeInDuration.Get() > KINDA_SMALL_NUMBER && bAllowThrottleWhenFrameRateIsLow && !ThrottleHandle.IsValid() )
		{
			if( !FSlateApplication::Get().IsRunningAtTargetFrameRate() )
			{
				ThrottleHandle = FSlateThrottleManager::Get().EnterResponsiveMode();
			}
		}
	}

	/** Begins the fadeout of this message */
	virtual void Fadeout() OVERRIDE
	{
		// Start fade animation
		FadeAnimation = FCurveSequence();
		FadeCurve = FadeAnimation.AddCurve(0.f, FadeOutDuration.Get());
		FadeAnimation.PlayReverse();
	}

	void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

		const bool bIsFadingOut = FadeAnimation.IsInReverse();
		const bool bIsCurrentlyPlaying = FadeAnimation.IsPlaying();
		const bool bIsIntroPlaying = IntroAnimation.IsPlaying();

		if ( !bIsCurrentlyPlaying && bIsFadingOut )
		{
			// Reset the Animation
			FadeoutComplete();
		}

		if ( !bIsIntroPlaying && ThrottleHandle.IsValid() )
		{
			// Leave responsive mode once the intro finishes playing
			FSlateThrottleManager::Get().LeaveResponsiveMode( ThrottleHandle );
		}
	}

protected:
	/** A fadeout has completed */
	void FadeoutComplete()
	{
		// Make sure we are no longer fading
		FadeAnimation = FCurveSequence();
		FadeCurve = FCurveHandle();

		// Clear the complete state to hide all the images/throbber
		SetCompletionState(CS_None);

		// Make sure we have left responsive mode
		if ( ThrottleHandle.IsValid() )
		{
			FSlateThrottleManager::Get().LeaveResponsiveMode( ThrottleHandle );
		}

		// Clear reference
		if( MyList.IsValid() )
		{
			MyList.Pin()->NotificationItemFadedOut(SharedThis(this));
		}
	}

	/** Gets the current color along the fadeout curve */
	FSlateColor GetContentColor() const
	{
		return GetContentColorRaw();
	}

	/** Gets the current color along the fadeout curve */
	FLinearColor GetContentColorRaw() const
	{
		// if we have a parent window, we need to make that transparent, rather than
		// this widget
		if(MyList.IsValid() && MyList.Pin()->ParentWindowPtr.IsValid())
		{
			MyList.Pin()->ParentWindowPtr.Pin()->SetOpacity( FadeCurve.GetLerp() );
			return FLinearColor(1,1,1,1);
		}
		else
		{
			return FMath::Lerp( FLinearColor(1,1,1,0), FLinearColor(1,1,1,1), FadeCurve.GetLerp() );
		}
	}

	/** Gets the color of the glow effect */
	FSlateColor GetGlowColor() const
	{
		float GlowAlpha = 1.0f - GlowCurve.GetLerp();
		
		if (GlowAlpha == 1.0f)
		{
			GlowAlpha = 0.0f;
		}

		switch (CompletionState)
		{
		case CS_Success: return FLinearColor(0,1,0,GlowAlpha);
		case CS_Fail: return FLinearColor(1,0,0,GlowAlpha);
		default: return FLinearColor(1,1,1,GlowAlpha);
		}
	}

	/** Gets the scale for the entire item */
	FVector2D GetItemScale() const
	{
		return FVector2D(  ScaleCurveX.GetLerp(), ScaleCurveY.GetLerp() );
	}

	/** Gets the visibility for the throbber */
	EVisibility GetThrobberVisibility() const
	{
		return CompletionState == CS_Pending ? EVisibility::Visible : EVisibility::Collapsed;
	}

	EVisibility GetSuccessImageVisibility() const
	{
		return CompletionState == CS_Success ? EVisibility::Visible : EVisibility::Collapsed;
	}

	EVisibility GetFailImageVisibility() const
	{
		return CompletionState == CS_Fail ? EVisibility::Visible : EVisibility::Collapsed;
	}

public:
	/** The SNotificationList that is displaying this Item */
	TWeakPtr<SNotificationList> MyList;

protected:

	/** The text displayed in this text block */
	TAttribute< FText > Text;

	/** The fade in duration for this element */
	TAttribute< float > FadeInDuration;

	/** The fade out duration for this element */
	TAttribute< float > FadeOutDuration;

	/** The duration before a fadeout for this element */
	TAttribute< float > ExpireDuration;

	/** The text displayed in this element */
	TSharedPtr<STextBlock> MyTextBlock;

	/** The completion state of this message */	
	ECompletionState CompletionState;

	/** The fading animation */
	FCurveSequence FadeAnimation;
	FCurveHandle FadeCurve;

	/** The intro animation */
	FCurveSequence IntroAnimation;
	FCurveHandle ScaleCurveX;
	FCurveHandle ScaleCurveY;
	FCurveHandle GlowCurve;

	/** The completion state change animation */
	FCurveSequence CompletionStateAnimation;

	/** Handle to a throttle request made to ensure the intro animation is smooth in low FPS situations */
	FThrottleRequest ThrottleHandle;
};


/////////////////////////////////////////////////
// SNotificationItemImpl

/** A single line in the event message list with additional buttons*/
class SNotificationItemImpl : public SNotificationExtendable
{
public:
	SLATE_BEGIN_ARGS( SNotificationItemImpl )
		: _Text()
		, _Image()	
		, _FadeInDuration(0.5f)
		, _FadeOutDuration(2.f)
		, _ExpireDuration(1.f)
		, _WidthOverride(FOptionalSize())
	{}

		/** The text displayed in this text block */
		SLATE_ATTRIBUTE( FText, Text )
		/** Setup information for the buttons on the notification */ 
		SLATE_ATTRIBUTE(TArray<FNotificationButtonInfo>, ButtonDetails)
		/** The icon image to display next to the text */
		SLATE_ATTRIBUTE( const FSlateBrush*, Image )
		/** The fade in duration for this element */
		SLATE_ATTRIBUTE( float, FadeInDuration )
		/** The fade out duration for this element */
		SLATE_ATTRIBUTE( float, FadeOutDuration )
		/** The duration before a fadeout for this element */
		SLATE_ATTRIBUTE( float, ExpireDuration )
		/** Controls whether or not to add the animated throbber */
		SLATE_ATTRIBUTE( bool, bUseThrobber)
		/** Controls whether or not to display the success and fail icons */
		SLATE_ATTRIBUTE( bool, bUseSuccessFailIcons)
		/** When true the larger bolder font will be used to display the message */
		SLATE_ATTRIBUTE( bool, bUseLargeFont)
		/** When set this forces the width of the box, used to stop resizeing on text change */
		SLATE_ATTRIBUTE( FOptionalSize, WidthOverride )
		/** When set this will display as a hyperlink on the right side of the notification. */
		SLATE_EVENT( FSimpleDelegate, Hyperlink)
		/** Text to display for the hyperlink (if Hyperlink is valid) */
		SLATE_ATTRIBUTE( FText, HyperlinkText );

	SLATE_END_ARGS()
	/**
	 * Constructs this widget
	 *
	 * @param InArgs    Declaration from which to construct the widget
	*/
	void Construct( const FArguments& InArgs )
	{
		CompletionState = CS_None;

		Text = InArgs._Text;
		FadeInDuration = InArgs._FadeInDuration;
		FadeOutDuration = InArgs._FadeOutDuration;
		ExpireDuration = InArgs._ExpireDuration;

		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("NotificationList.ItemBackground"))
			.BorderBackgroundColor( this, &SNotificationItemImpl::GetContentColor )
			.ColorAndOpacity(this, &SNotificationItemImpl::GetContentColorRaw )
			.DesiredSizeScale( this, &SNotificationItemImpl::GetItemScale )
			[
				SNew(SBorder)
				.Padding( FMargin(5) )
				.BorderImage(FCoreStyle::Get().GetBrush("NotificationList.ItemBackground_Border"))
				.BorderBackgroundColor( this, &SNotificationItemImpl::GetGlowColor)
				[
					ConstructInternals(InArgs)
				]
			]
		];
	}
	
	/*
	 * Returns the internals of the notification
	 */
	TSharedRef<SHorizontalBox> ConstructInternals( const FArguments& InArgs ) 
	{
		Hyperlink = InArgs._Hyperlink;
		HyperlinkText = InArgs._HyperlinkText;

		TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

		// Notification image
		HorizontalBox->AddSlot()
		.AutoWidth()
		.Padding(10.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		[
			SNew(SImage)
			.Image( InArgs._Image )
		];

		// Set font to default case.
		FSlateFontInfo font = FCoreStyle::Get().GetFontStyle(TEXT("NotificationList.FontBold"));

		// only override if requested
		if (!InArgs._bUseLargeFont.Get())
		{
			font = FCoreStyle::Get().GetFontStyle(TEXT("NotificationList.FontLight"));
		}

		// Build Text box
		HorizontalBox->AddSlot()
		.AutoWidth()
		.Padding(20.f, 0.f, 20.f, 0.f)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		[
			SNew(SBox)
			.WidthOverride(InArgs._WidthOverride)
			[
				SAssignNew(MyTextBlock, STextBlock)
				.Text(Text)
				.Font(font)
			]
		];

		if (InArgs._bUseThrobber.Get())
		{
			// Build pending throbber
			HorizontalBox->AddSlot()
			.AutoWidth()
			.Padding(45.f, 0.f, 45.f, 0.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SThrobber)
				.Visibility( this, &SNotificationItemImpl::GetThrobberVisibility )
			];
		}

		if (InArgs._bUseSuccessFailIcons.Get())
		{
			// Build success image
			HorizontalBox->AddSlot()
			.AutoWidth()
			.Padding(5.f, 0.f, 45.f, 0.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image( FCoreStyle::Get().GetBrush("NotificationList.SuccessImage") )
				.Visibility( this, &SNotificationItemImpl::GetSuccessImageVisibility )
			];

			// Build fail image
			HorizontalBox->AddSlot()
			.AutoWidth()
			.Padding(5.f, 0.f, 45.f, 0.f )
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image( FCoreStyle::Get().GetBrush("NotificationList.FailImage") )
				.Visibility( this, &SNotificationItemImpl::GetFailImageVisibility )
			];
		}

		// Adds any buttons that were passed in.
		for (int32 idx = 0; idx < InArgs._ButtonDetails.Get().Num(); idx++)
		{
			FNotificationButtonInfo button = InArgs._ButtonDetails.Get()[idx];

			HorizontalBox->AddSlot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(2)
			[
				SNew(SButton)
				.Text(button.Text)
				.ToolTipText(button.ToolTip)
				.OnClicked(this, &SNotificationItemImpl::OnButtonClicked, button.Callback) 
				.Visibility( this, &SNotificationItemImpl::GetButtonVisibility, button.VisibilityOnNone, button.VisibilityOnPending, button.VisibilityOnSuccess, button.VisibilityOnFail )
			];
		}

		// Adds a hyperlink, but only visible when bound
		HorizontalBox->AddSlot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(2)
			[
				SNew(SHyperlink)
					.Text(this, &SNotificationItemImpl::GetHyperlinkText)
					.OnNavigate(this, &SNotificationItemImpl::OnHyperlinkClicked)
					.Visibility(this, &SNotificationItemImpl::GetHyperlinkVisibility)
			];	

		return HorizontalBox;
	}

	/** Sets the text and delegate for the hyperlink */
	virtual void SetHyperlink( const FSimpleDelegate& InHyperlink, const TAttribute< FText >& InHyperlinkText = TAttribute< FText >() ) OVERRIDE
	{
		Hyperlink = InHyperlink;

		// Only replace the text if specified
		if ( InHyperlinkText.IsBound() )
		{
			HyperlinkText = InHyperlinkText;
		}
	}

protected:

	/* Used to determine whether the button is visible */
	EVisibility GetButtonVisibility( const EVisibility VisibilityOnNone, const EVisibility VisibilityOnPending, const EVisibility VisibilityOnSuccess, const EVisibility VisibilityOnFail ) const
	{
		switch ( CompletionState )
		{
		case SNotificationItem::CS_None: return VisibilityOnNone;
		case SNotificationItem::CS_Pending: return VisibilityOnPending;
		case SNotificationItem::CS_Success: return VisibilityOnSuccess;
		case SNotificationItem::CS_Fail: return VisibilityOnFail;
		default:
			check( false );
			return EVisibility::Visible;
		}
	}

	/* Used to determine whether the hyperlink is visible */
	EVisibility GetHyperlinkVisibility() const
	{
		return Hyperlink.IsBound() ? EVisibility::Visible : EVisibility::Collapsed;
	}

	/* Used to fetch the text to display in the hyperlink */
	FText GetHyperlinkText() const
	{
		return HyperlinkText.Get();
	}

private:

	/* Used as a wrapper for the callback, this means any code calling it does not require access to FReply type */
	FReply OnButtonClicked(FSimpleDelegate InCallback)
	{
		InCallback.ExecuteIfBound();
		return FReply::Handled();
	}

	/* Execute the delegate for the hyperlink, if bound */
	void OnHyperlinkClicked() const
	{
		Hyperlink.ExecuteIfBound();
	}

protected:

	/** When set this will display as a hyperlink on the right side of the notification. */
	FSimpleDelegate Hyperlink;

	/** Text to display for the hyperlink message */
	TAttribute< FText > HyperlinkText;
};

///////////////////////////////////////////////////
//// SNotificationList
TSharedRef<SNotificationItem> SNotificationList::AddNotification(const FNotificationInfo& Info)
{
	EVisibility visibility = EVisibility::HitTestInvisible;
	static const FSlateBrush* CachedImage = FCoreStyle::Get().GetBrush("NotificationList.DefaultMessage");

	// Notifications with buttons need to be hit test enabled.
	if (Info.ButtonDetails.Num() > 0)
	{
		visibility = EVisibility::Collapsed;
	}

	// Create notification.
	TSharedRef<SNotificationExtendable> NewItem =
		SNew(SNotificationItemImpl)
		.Text(Info.Text)
		.ButtonDetails(Info.ButtonDetails)
		.Image((Info.Image != NULL) ? Info.Image : CachedImage)
		.FadeInDuration(Info.FadeInDuration)
		.ExpireDuration(Info.ExpireDuration)
		.FadeOutDuration(Info.FadeOutDuration)
		.bUseThrobber(Info.bUseThrobber)
		.bUseSuccessFailIcons(Info.bUseSuccessFailIcons)
		.bUseLargeFont(Info.bUseLargeFont)
		.WidthOverride(Info.WidthOverride)
		.Visibility(visibility)
		.Hyperlink(Info.Hyperlink)
		.HyperlinkText(Info.HyperlinkText);
		
	NewItem->MyList = SharedThis(this);

	MessageItemBoxPtr->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		[
			NewItem
		];

	NewItem->Fadein( Info.bAllowThrottleWhenFrameRateIsLow );

	if (Info.bFireAndForget)
	{
		NewItem->ExpireAndFadeout();
	}

	return NewItem;
}

void SNotificationList::NotificationItemFadedOut (const TSharedRef<SNotificationItem>& NotificationItem)
{
	if( ParentWindowPtr.IsValid() )
	{
		// If we are in a single-window per notification situation, we dont want to 
		// remove the NotificationItem straight away, rather we will flag us as done
		// and wait for the FSlateNotificationManager to release the parent window.
		bDone = true;
	}
	else
	{
		// This should remove the last non-local reference to this SNotificationItem.
		// Since there may be many local references on the call stack we are not checking
		// if the reference is unique.
		MessageItemBoxPtr->RemoveSlot(NotificationItem);
	}
}

void SNotificationList::Construct(const FArguments& InArgs)
{
	bDone = false;

	ChildSlot
	[
		SAssignNew(MessageItemBoxPtr, SVerticalBox)
	];
}
