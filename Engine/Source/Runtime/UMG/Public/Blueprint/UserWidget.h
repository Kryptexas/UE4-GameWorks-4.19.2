// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Geometry.h"
#include "UserWidget.generated.h"

static FGeometry NullGeometry;
static FSlateRect NullRect;
static FSlateWindowElementList NullElementList;
static FWidgetStyle NullStyle;

class UWidgetAnimation;

/**
 * The state passed into OnPaint that we can expose as a single painting structure to blueprints to
 * allow script code to override OnPaint behavior.
 */
USTRUCT()
struct UMG_API FPaintContext
{
	GENERATED_USTRUCT_BODY()

public:

	/** Don't ever use this constructor.  Needed for code generation. */
	FPaintContext()
		: AllottedGeometry(NullGeometry)
		, MyClippingRect(NullRect)
		, OutDrawElements(NullElementList)
		, LayerId(0)
		, InWidgetStyle(NullStyle)
		, bParentEnabled(true)
		, MaxLayer(0)
	{ }

	FPaintContext(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled)
		: AllottedGeometry(AllottedGeometry)
		, MyClippingRect(MyClippingRect)
		, OutDrawElements(OutDrawElements)
		, LayerId(LayerId)
		, InWidgetStyle(InWidgetStyle)
		, bParentEnabled(bParentEnabled)
		, MaxLayer(LayerId)
	{
	}

	/** We override the assignment operator to allow generated code to compile with the const ref member. */
	void operator=( const FPaintContext& Other )
	{
		const_cast<FGeometry&>( AllottedGeometry ) = Other.AllottedGeometry;
		const_cast<FSlateRect&>( MyClippingRect ) = Other.MyClippingRect;
		OutDrawElements = Other.OutDrawElements;
		LayerId = Other.LayerId;
		const_cast<FWidgetStyle&>( InWidgetStyle ) = Other.InWidgetStyle;
		bParentEnabled = Other.bParentEnabled;
	}

public:

	const FGeometry& AllottedGeometry;
	const FSlateRect& MyClippingRect;
	FSlateWindowElementList& OutDrawElements;
	int32 LayerId;
	const FWidgetStyle& InWidgetStyle;
	bool bParentEnabled;

	int32 MaxLayer;
};

class UUMGSequencePlayer;

//TODO UMG If you want to host a widget that's full screen there may need to be a SWindow equivalent that you spawn it into.

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConstructEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVisibilityChangedEvent, ESlateVisibility::Type, Visibility);

/**
 * The user widget is extensible by users through the WidgetBlueprint.
 */
UCLASS(Abstract, editinlinenew, BlueprintType, Blueprintable, meta=( Category="User Controls" ))
class UMG_API UUserWidget : public UWidget
{
	GENERATED_UCLASS_BODY()
public:
	//UObject interface
	virtual void PostInitProperties() override;
	virtual class UWorld* GetWorld() const override;
	virtual void PostEditImport() override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	// End of UObject interface

	void Initialize();

	//UVisual interface
	virtual void ReleaseNativeWidget() override;
	// End of UVisual interface

	/*  */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void AddToViewport(bool bAbsoluteLayout = false, bool bModal = false, bool bShowCursor = false);

	/*  */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void RemoveFromViewport();

	/*  */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void SetOffsetInViewport(FVector2D DesiredOffset);

	/*  */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void SetDesiredSizeInViewport(FVector2D DesiredSize);

	/*  */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void SetAnchorsInViewport(FVector2D Anchors);

	/*  */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void SetAlignmentInViewport(FVector2D Alignment);

	/*  */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void SetZOrderInViewport(int32 ZOrder);

	/*  */
	UFUNCTION(BlueprintPure, Category="Appearance")
	bool GetIsVisible();

	UFUNCTION(BlueprintPure, Category="Appearance")
	TEnumAsByte<ESlateVisibility::Type> GetVisiblity();

	/** Sets the player context associated with this UI. */
	void SetPlayerContext(FLocalPlayerContext InPlayerContext);

	/** Gets the player context associated with this UI. */
	const FLocalPlayerContext& GetPlayerContext() const;

	/** Gets the local player associated with this UI. */
	UFUNCTION(BlueprintCallable, Category="Player")
	class ULocalPlayer* GetLocalPlayer() const;

	/** Gets the player controller associated with this UI. */
	UFUNCTION(BlueprintCallable, Category="Player")
	class APlayerController* GetPlayerController() const;

	/** Called when the widget is constructed */
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	void Construct();

	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	void Tick(FGeometry MyGeometry, float InDeltaTime);

	//TODO UMG HitTest

	UFUNCTION(BlueprintImplementableEvent, Category="User Interface | Painting")
	void OnPaint(UPARAM(ref) FPaintContext& Context) const;

	UFUNCTION(BlueprintImplementableEvent, Category="Keyboard")
	FEventReply OnKeyboardFocusReceived(FGeometry MyGeometry, FKeyboardFocusEvent InKeyboardFocusEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Keyboard")
	void OnKeyboardFocusLost(FKeyboardFocusEvent InKeyboardFocusEvent);
	//UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	//void OnKeyboardFocusChanging(FWeakWidgetPath PreviousFocusPath, FWidgetPath NewWidgetPath);

	UFUNCTION(BlueprintImplementableEvent, Category="Keyboard")
	FEventReply OnKeyChar(FGeometry MyGeometry, FCharacterEvent InCharacterEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Keyboard")
	FEventReply OnPreviewKeyDown(FGeometry MyGeometry, FKeyboardEvent InKeyboardEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Keyboard")
	FEventReply OnKeyDown(FGeometry MyGeometry, FKeyboardEvent InKeyboardEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Keyboard")
	FEventReply OnKeyUp(FGeometry MyGeometry, FKeyboardEvent InKeyboardEvent);

	UFUNCTION(BlueprintImplementableEvent, Category="Mouse")
	FEventReply OnMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Mouse")
	FEventReply OnPreviewMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Mouse")
	FEventReply OnMouseButtonUp(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Mouse")
	FEventReply OnMouseMove(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Mouse")
	void OnMouseEnter(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Mouse")
	void OnMouseLeave(const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Mouse")
	FEventReply OnMouseWheel(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	//UFUNCTION(BlueprintImplementableEvent, Category="Mouse")
	//FCursorReply OnCursorQuery(FGeometry MyGeometry, const FPointerEvent& CursorEvent) const;
	UFUNCTION(BlueprintImplementableEvent, Category="Mouse")
	FEventReply OnMouseButtonDoubleClick(FGeometry InMyGeometry, const FPointerEvent& InMouseEvent);

	UFUNCTION(BlueprintImplementableEvent, Category="Drag and Drop")
	void OnDragDetected(FGeometry MyGeometry, const FPointerEvent& MouseEvent, UDragDropOperation*& Operation);
	
	UFUNCTION(BlueprintImplementableEvent, Category="Drag and Drop")
	void OnDragEnter(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation* Operation);
	UFUNCTION(BlueprintImplementableEvent, Category="Drag and Drop")
	void OnDragLeave(FPointerEvent PointerEvent, UDragDropOperation* Operation);
	UFUNCTION(BlueprintImplementableEvent, Category="Drag and Drop")
	bool OnDragOver(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation* Operation);
	UFUNCTION(BlueprintImplementableEvent, Category="Drag and Drop")
	bool OnDrop(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation* Operation);

	UFUNCTION(BlueprintImplementableEvent, Category="Gamepad Input")
	FEventReply OnControllerButtonPressed(FGeometry MyGeometry, FControllerEvent ControllerEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Gamepad Input")
	FEventReply OnControllerButtonReleased(FGeometry MyGeometry, FControllerEvent ControllerEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Gamepad Input")
	FEventReply OnControllerAnalogValueChanged(FGeometry MyGeometry, FControllerEvent ControllerEvent);

	UFUNCTION(BlueprintImplementableEvent, Category="Touch Input")
	FEventReply OnTouchGesture(FGeometry MyGeometry, const FPointerEvent& GestureEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Touch Input")
	FEventReply OnTouchStarted(FGeometry MyGeometry, const FPointerEvent& InTouchEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Touch Input")
	FEventReply OnTouchMoved(FGeometry MyGeometry, const FPointerEvent& InTouchEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Touch Input")
	FEventReply OnTouchEnded(FGeometry MyGeometry, const FPointerEvent& InTouchEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="Touch Input")
	FEventReply OnMotionDetected(FGeometry MyGeometry, FMotionEvent InMotionEvent);

	UFUNCTION(BlueprintCallable, Category="User Interface|Drag and Drop")
	UDragDropOperation* CreateDragDropOperation(TSubclassOf<UDragDropOperation> Operation);

	//virtual bool OnVisualizeTooltip(const TSharedPtr<SWidget>& TooltipContent);

	/**
	 * Plays an animation in this widget
	 * 
	 * @param The name of the animation to play
	 */
	UFUNCTION(BlueprintCallable, Category="User Interface|Animation")
	void PlayAnimation(const UWidgetAnimation* InAnimation);

	/**
	 * Stops an already running animation in this widget
	 * 
	 * @param The name of the animation to stop
	 */
	UFUNCTION(BlueprintCallable, Category="User Interface|Animation")
	void StopAnimation(const UWidgetAnimation* InAnimation);

	/** Called when a sequence player is finished playing an animation */
	void OnAnimationFinishedPlaying(UUMGSequencePlayer& Player );

	/** @returns The UObject wrapper for a given SWidget */
	UWidget* GetWidgetHandle(TSharedRef<SWidget> InWidget);

	/** Creates a fullscreen host widget, that wraps this widget. */
	TSharedRef<SWidget> MakeViewportWidget(bool bAbsoluteLayout, bool bModal, bool bShowCursor, TSharedPtr<SWidget>& UserSlateWidget);

	/** @returns The root UObject widget wrapper */
	UWidget* GetRootWidgetComponent();

	/** @returns The slate widget corresponding to a given name */
	TSharedPtr<SWidget> GetWidgetFromName(const FString& Name) const;

	/** @returns The uobject widget corresponding to a given name */
	UWidget* GetHandleFromName(const FString& Name) const;

	/** Ticks this widget and forwards to the underlying widget to tick */
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime );

#if WITH_EDITOR
	// UWidget interface
	virtual const FSlateBrush* GetEditorIcon() override;
	// End UWidget interface
#endif

public:

	/** Called when the visibility changes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="Behavior")
	bool bSupportsKeyboardFocus;

	/** Called when the visibility changes. */
	UPROPERTY(BlueprintAssignable)
	FOnVisibilityChangedEvent OnVisibilityChanged;

	UPROPERTY()
	FMargin Padding;

	/** How much space this slot should occupy in the direction of the panel. */
	UPROPERTY()
	FSlateChildSize Size;

	/**
	* Horizontal pivot position
	*  Given a top aligned slot, where '+' represents the
	*  anchor point defined by PositionAttr.
	*
	*   Left				Center				Right
	+ _ _ _ _            _ _ + _ _          _ _ _ _ +
	|		  |		   | 		   |	  |		    |
	| _ _ _ _ |        | _ _ _ _ _ |	  | _ _ _ _ |
	*
	*  Note: FILL is NOT supported in absolute layout
	*/
	UPROPERTY()
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/**
	* Vertical pivot position
	*   Given a left aligned slot, where '+' represents the
	*   anchor point defined by PositionAttr.
	*
	*   Top					Center			  Bottom
	*	+_ _ _ _ _		 _ _ _ _ _		 _ _ _ _ _
	*	|         |		| 		  |		|		  |
	*	|         |     +		  |		|		  |
	*	| _ _ _ _ |		| _ _ _ _ |		+ _ _ _ _ |
	*
	*  Note: FILL is NOT supported in absolute layout
	*/
	UPROPERTY()
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	/** The components contained in this user widget. */
	UPROPERTY(Transient)
	TArray<UWidget*> Components;

	/** The widget tree contained inside this user widget initialized by the blueprint */
	UPROPERTY(Transient)
	class UWidgetTree* WidgetTree;

	/** All the sequence players currently playing */
	UPROPERTY(Transient)
	TArray<UUMGSequencePlayer*> ActiveSequencePlayers;

	/** List of sequence players to cache and clean up when safe */
	UPROPERTY(Transient)
	TArray<UUMGSequencePlayer*> StoppedSequencePlayers;

	UPROPERTY(Transient)
	UInputComponent* InputComponent;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	FMargin GetFullScreenOffset() const;
	FAnchors GetViewportAnchors() const;
	FVector2D GetFullScreenAlignment() const;
	int32 GetFullScreenZOrder() const;

private:
	FVector2D ViewportAnchors;
	FMargin ViewportOffsets;
	FVector2D ViewportAlignment;
	int32 ViewportZOrder;

	TWeakPtr<class SViewportWidgetHost> FullScreenWidget;

	FLocalPlayerContext PlayerContext;

	bool bInitialized;

	mutable UWorld* CachedWorld;
};

template< class T >
T* CreateWidget(UWorld* World, UClass* UserWidgetClass)
{
	if ( !UserWidgetClass->IsChildOf(UUserWidget::StaticClass()) )
	{
		// TODO UMG Error?
		return NULL;
	}

	ULocalPlayer* Player = World->GetFirstLocalPlayerFromController();

	UObject* Outer = ( Player == NULL ) ? StaticCast<UObject*>(World) : StaticCast<UObject*>(Player);
	UUserWidget* NewWidget = ConstructObject<UUserWidget>(UserWidgetClass, Outer);
	NewWidget->SetPlayerContext(FLocalPlayerContext(Player));
	NewWidget->Initialize();

	return Cast<T>(NewWidget);
}

template< class T >
T* CreateWidget(APlayerController* OwningPlayer, UClass* UserWidgetClass)
{
	if ( !UserWidgetClass->IsChildOf(UUserWidget::StaticClass()) )
	{
		// TODO UMG Error?
		return NULL;
	}

	UUserWidget* NewWidget = ConstructObject<UUserWidget>(UserWidgetClass, OwningPlayer);
	NewWidget->SetPlayerContext(FLocalPlayerContext(OwningPlayer));
	NewWidget->Initialize();

	return Cast<T>(NewWidget);
}
