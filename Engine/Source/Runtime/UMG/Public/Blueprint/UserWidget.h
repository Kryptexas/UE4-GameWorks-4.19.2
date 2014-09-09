// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Geometry.h"
#include "Engine/GameInstance.h"
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
USTRUCT(BlueprintType)
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
	void AddToViewport(bool bModal = false, bool bShowCursor = false);

	/*  */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void RemoveFromViewport();

	/**
	 * Sets the widgets position in the viewport.
	 */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void SetPositionInViewport(FVector2D Position);

	/*  */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void SetDesiredSizeInViewport(FVector2D Size);

	/*  */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void SetAnchorsInViewport(FAnchors Anchors);

	/*  */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void SetAlignmentInViewport(FVector2D Alignment);

	/*  */
	UFUNCTION(BlueprintCallable, Category="User Interface|Viewport")
	void SetZOrderInViewport(int32 ZOrder);

	/*  */
	UFUNCTION(BlueprintPure, Category="Appearance")
	bool GetIsVisible() const;

	UFUNCTION(BlueprintPure, Category="Appearance")
	TEnumAsByte<ESlateVisibility::Type> GetVisiblity() const;

	/** Sets the player context associated with this UI. */
	void SetPlayerContext(FLocalPlayerContext InPlayerContext);

	/** Gets the player context associated with this UI. */
	const FLocalPlayerContext& GetPlayerContext() const;

	/** Gets the local player associated with this UI. */
	UFUNCTION(BlueprintCallable, Category="Player")
	class ULocalPlayer* GetOwningLocalPlayer() const;

	/** Gets the player controller associated with this UI. */
	UFUNCTION(BlueprintCallable, Category="Player")
	class APlayerController* GetOwningPlayer() const;

	/** Gets the player pawn associated with this UI. */
	UFUNCTION(BlueprintCallable, Category="Player")
	class APawn* GetOwningPlayerPawn() const;

	/** Called when the widget is constructed */
	UFUNCTION(BlueprintNativeEvent, Category="User Interface")
	void Construct();

	UFUNCTION(BlueprintNativeEvent, Category="User Interface")
	void Tick(FGeometry MyGeometry, float InDeltaTime);

	//TODO UMG HitTest

	UFUNCTION(BlueprintNativeEvent, Category="User Interface | Painting")
	void OnPaint(UPARAM(ref) FPaintContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, Category="Keyboard")
	FEventReply OnKeyboardFocusReceived(FGeometry MyGeometry, FKeyboardFocusEvent InKeyboardFocusEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Keyboard")
	void OnKeyboardFocusLost(FKeyboardFocusEvent InKeyboardFocusEvent);
	//UFUNCTION(BlueprintNativeEvent, Category="User Interface")
	//void OnKeyboardFocusChanging(FWeakWidgetPath PreviousFocusPath, FWidgetPath NewWidgetPath);

	UFUNCTION(BlueprintNativeEvent, Category="Keyboard")
	FEventReply OnKeyChar(FGeometry MyGeometry, FCharacterEvent InCharacterEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Keyboard")
	FEventReply OnPreviewKeyDown(FGeometry MyGeometry, FKeyboardEvent InKeyboardEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Keyboard")
	FEventReply OnKeyDown(FGeometry MyGeometry, FKeyboardEvent InKeyboardEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Keyboard")
	FEventReply OnKeyUp(FGeometry MyGeometry, FKeyboardEvent InKeyboardEvent);

	UFUNCTION(BlueprintNativeEvent, Category="Mouse")
	FEventReply OnMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Mouse")
	FEventReply OnPreviewMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Mouse")
	FEventReply OnMouseButtonUp(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Mouse")
	FEventReply OnMouseMove(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Mouse")
	void OnMouseEnter(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Mouse")
	void OnMouseLeave(const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Mouse")
	FEventReply OnMouseWheel(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Mouse")
	FEventReply OnMouseButtonDoubleClick(FGeometry InMyGeometry, const FPointerEvent& InMouseEvent);

	//UFUNCTION(BlueprintNativeEvent, Category="Mouse")
	//FCursorReply OnCursorQuery(FGeometry MyGeometry, const FPointerEvent& CursorEvent) const;

	UFUNCTION(BlueprintNativeEvent, Category="Drag and Drop")
	void OnDragDetected(FGeometry MyGeometry, const FPointerEvent& PointerEvent, UDragDropOperation*& Operation);

	UFUNCTION(BlueprintNativeEvent, Category="Drag and Drop")
	void OnDragCancelled(const FPointerEvent& PointerEvent, UDragDropOperation* Operation);
	
	UFUNCTION(BlueprintNativeEvent, Category="Drag and Drop")
	void OnDragEnter(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation* Operation);
	UFUNCTION(BlueprintNativeEvent, Category="Drag and Drop")
	void OnDragLeave(FPointerEvent PointerEvent, UDragDropOperation* Operation);
	UFUNCTION(BlueprintNativeEvent, Category="Drag and Drop")
	bool OnDragOver(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation* Operation);
	UFUNCTION(BlueprintNativeEvent, Category="Drag and Drop")
	bool OnDrop(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation* Operation);

	UFUNCTION(BlueprintNativeEvent, Category="Gamepad Input")
	FEventReply OnControllerButtonPressed(FGeometry MyGeometry, FControllerEvent ControllerEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Gamepad Input")
	FEventReply OnControllerButtonReleased(FGeometry MyGeometry, FControllerEvent ControllerEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Gamepad Input")
	FEventReply OnControllerAnalogValueChanged(FGeometry MyGeometry, FControllerEvent ControllerEvent);

	UFUNCTION(BlueprintNativeEvent, Category="Touch Input")
	FEventReply OnTouchGesture(FGeometry MyGeometry, const FPointerEvent& GestureEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Touch Input")
	FEventReply OnTouchStarted(FGeometry MyGeometry, const FPointerEvent& InTouchEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Touch Input")
	FEventReply OnTouchMoved(FGeometry MyGeometry, const FPointerEvent& InTouchEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Touch Input")
	FEventReply OnTouchEnded(FGeometry MyGeometry, const FPointerEvent& InTouchEvent);
	UFUNCTION(BlueprintNativeEvent, Category="Touch Input")
	FEventReply OnMotionDetected(FGeometry MyGeometry, FMotionEvent InMotionEvent);

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
	TSharedRef<SWidget> MakeViewportWidget(bool bModal, bool bShowCursor, TSharedPtr<SWidget>& UserSlateWidget);

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
	virtual const FText GetToolboxCategory() override;
	// End UWidget interface
#endif

public:

	/** Called when the visibility changes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="Behavior")
	bool bSupportsKeyboardFocus;

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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	FMargin GetFullScreenOffset() const;
	FAnchors GetViewportAnchors() const;
	FVector2D GetFullScreenAlignment() const;
	int32 GetFullScreenZOrder() const;

private:
	FAnchors ViewportAnchors;
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
		return nullptr;
	}

	// Assign the outer to the game instance if it exists, otherwise use the world
	UObject* Outer = World->GetGameInstance() ? StaticCast<UObject*>(World->GetGameInstance()) : StaticCast<UObject*>(World);
	ULocalPlayer* Player = World->GetFirstLocalPlayerFromController();
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
		return nullptr;
	}

	// Assign the outer to the game instance if it exists, otherwise use the player controller's world
	UWorld* World = OwningPlayer->GetWorld();
	UObject* Outer = World->GetGameInstance() ? StaticCast<UObject*>(World->GetGameInstance()) : StaticCast<UObject*>(World);
	UUserWidget* NewWidget = ConstructObject<UUserWidget>(UserWidgetClass, Outer);
	
	NewWidget->SetPlayerContext(FLocalPlayerContext(OwningPlayer));
	NewWidget->Initialize();

	return Cast<T>(NewWidget);
}
