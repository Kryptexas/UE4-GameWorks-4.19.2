// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "HUD.generated.h"

/** List of actors and debug text to draw, @see AddDebugText(), RemoveDebugText(), and DrawDebugTextList() */
USTRUCT()
struct FDebugTextInfo
{
	GENERATED_USTRUCT_BODY()

	/**  AActor to draw DebugText over */
	UPROPERTY()
	class AActor* SrcActor;

	/** Offset from SrcActor.Location to apply */
	UPROPERTY()
	FVector SrcActorOffset;

	/** Desired offset to interpolate to */
	UPROPERTY()
	FVector SrcActorDesiredOffset;

	/** Text to display */
	UPROPERTY()
	FString DebugText;

	/** Time remaining for the debug text, -1.f == infinite */
	UPROPERTY(transient)
	float TimeRemaining;

	/** Duration used to lerp desired offset */
	UPROPERTY()
	float Duration;

	/** Text color */
	UPROPERTY()
	FColor TextColor;

	/** whether the offset should be treated as absolute world location of the string */
	UPROPERTY()
	uint32 bAbsoluteLocation:1;

	/** If the actor moves does the text also move with it? */
	UPROPERTY()
	uint32 bKeepAttachedToActor:1;

	/** When we first spawn store off the original actor location for use with bKeepAttachedToActor */
	UPROPERTY()
	FVector OrigActorLocation;

	/** The Font which to display this as.  Will Default to GetSmallFont()**/
	UPROPERTY()
	class UFont* Font;

	/** Scale to apply to font when rendering */
	UPROPERTY()
	float FontScale;

	FDebugTextInfo()
		: SrcActor(NULL)
		, SrcActorOffset(ForceInit)
		, SrcActorDesiredOffset(ForceInit)
		, TimeRemaining(0)
		, Duration(0)
		, TextColor(ForceInit)
		, bAbsoluteLocation(false)
		, bKeepAttachedToActor(false)
		, OrigActorLocation(ForceInit)
		, Font(NULL)
		, FontScale(1.0f)
	{
	}

};

class ENGINE_API FSimpleReticle
{
public:
	FSimpleReticle()
	{
		SetupReticle( 8.0f, 20.0f );
	}

	void SetupReticle( float Length, float InnerSize )
	{
		HorizontalOffsetMin.Set( InnerSize, 0.0f );
		HorizontalOffsetMax.Set( InnerSize + Length, 0.0f );
		VerticalOffsetMin.Set( 0.0f, InnerSize);
		VerticalOffsetMax.Set( 0.0f, InnerSize + Length );
	}
	void Draw( class FCanvas* InCanvas, FLinearColor InColor );
private:
	FVector2D HorizontalOffsetMin;
	FVector2D HorizontalOffsetMax;
	FVector2D VerticalOffsetMin;
	FVector2D VerticalOffsetMax;	
};

class ENGINE_API FHUDHitBox
{
public:
	/** 
	 * Constructor for a hitbox.
	 * @param	InCoords		Coordinates of top left of hitbox.
	 * @param	InSize			Size of the box.
	 */
	FHUDHitBox( FVector2D InCoords, FVector2D InSize, const FName& InName, bool bInConsumesInput, int32 InPriority );

	/** 
	 * Are the given coordinates within this hitbox.
	 * @param	InCoords		Coordinates to check.
	 * @returns true if coordinates are within this hitbox.
	 */
	bool Contains( FVector2D InCoords ) const;

	/** 
	 * Debug render for this hitbox.
	 * @param	InCanvas		Canvas on which to render.
	 * @param	InColor			Color to render the box.
	 */
	void Draw( class FCanvas* InCanvas, const FLinearColor& InColor );
	
	/** Get the name of this hitbox.  */
	const FName& GetName() const { return Name;};

	/** Should other boxes be processed if this box is clicked.  */
	const bool ConsumesInput() const { return bConsumesInput; };

	/** Get the priority of this hitbox.  */
	const int32 GetPriority() const { return Priority; };

private:
	/** Coordinates of top left of hitbox.  */
	FVector2D	Coords;

	/** Size of this hitbox.  */
	FVector2D	Size;
	
	/** The name of this hitbox.  */
	FName		Name;	

	/** Wether or not this hitbox should prevent hit checks to other hitboxes.  */
	bool bConsumesInput;

	/** The priority of this hitbox. Higher boxes are given priority. */
	int32 Priority;
};

//=============================================================================
// Base class of the heads-up display.
//
//=============================================================================
UCLASS(config=Game, hidecategories=(Rendering,Actor,Input,Replication), notplaceable, transient, dependson=UCanvas, BlueprintType, Blueprintable)
class ENGINE_API AHUD : public AActor
{
	GENERATED_UCLASS_BODY()

	/** Pre-defined FColors for convenience. */
	UPROPERTY()
	FColor WhiteColor;

	UPROPERTY()
	FColor GreenColor;

	UPROPERTY()
	FColor RedColor;

	/** PlayerController which owns this HUD. */
	UPROPERTY()
	class APlayerController* PlayerOwner;    

	/** Tells whether the game was paused due to lost focus */
	UPROPERTY(transient)
	uint32 bLostFocusPaused:1;

	/** Whether or not the HUD should be drawn. */
	UPROPERTY(config)
	uint32 bShowHUD:1;    

	/** If true, current ViewTarget shows debug information using its DisplayDebug(). */
	UPROPERTY()
	uint32 bShowDebugInfo:1;    

	/** If true, show hitbox debugging info. */
	UPROPERTY()
	uint32 bShowHitBoxDebugInfo:1;    

	/** If true, native HUD will not draw.  Allows blueprinted HUDs to totally replace an existing HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HUD)
	uint32 bSuppressNativeHUD:1;

	/** If true, render actor overlays. */
	UPROPERTY()
	uint32 bShowOverlays:1;

	/** Holds a list of Actors that need PostRender() calls. */
	UPROPERTY()
	TArray<class AActor*> PostRenderedActors;

	/** Used to calculate delta time between HUD rendering. */
	UPROPERTY(transient)
	float LastHUDRenderTime;

	/** Time since last HUD render. */
	UPROPERTY(transient)
	float RenderDelta;

	/** Array of names specifying what debug info to display for viewtarget actor. */
	UPROPERTY(globalconfig)
	TArray<FName> DebugDisplay;    

protected:
	/** Canvas to Draw HUD on.  Only valid during PostRender() event.  */
	UPROPERTY()
	class UCanvas* Canvas;

	/** 'Foreground' debug canvas, will draw in front of Slate UI. */
	UPROPERTY()
	class UCanvas* DebugCanvas;

private:
	UPROPERTY()
	TArray<struct FDebugTextInfo> DebugTextList;

	/** Areas of the screen where UIBlur is enabled. */
	TArray< FIntRect > UIBlurOverrideRectangles;

public:
	//=============================================================================
	// Utils

	/** hides or shows HUD */
	UFUNCTION(exec)
	virtual void ShowHUD();

	/**
	 * Toggles displaying properties of player's current ViewTarget
	 * DebugType input values supported by base engine include "AI", "physics", "net", "camera", and "collision"
	 */
	UFUNCTION(exec)
	virtual void ShowDebug(FName DebugType = NAME_None);

	UFUNCTION(reliable, client, SealedEvent)
	void AddDebugText(const FString& DebugText, class AActor* SrcActor = NULL, float Duration = 0, FVector Offset = FVector(ForceInit), FVector DesiredOffset = FVector(ForceInit), FColor TextColor = FColor(ForceInit), bool bSkipOverwriteCheck = false, bool bAbsoluteLocation = false, bool bKeepAttachedToActor = false, class UFont* InFont = NULL, float FontScale = 1.0);

	UFUNCTION(reliable, client, SealedEvent)
	void RemoveAllDebugStrings();

	UFUNCTION(reliable, client, SealedEvent)
	void RemoveDebugText(class AActor* SrcActor, bool bLeaveDurationText = false);

	/** Hook to allow blueprints to do custom HUD drawing. @see bSuppressNativeHUD to control HUD drawing in base class. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic)
	virtual void ReceiveDrawHUD(int32 SizeX, int32 SizeY);

	/** Called when a hit box is clicked on. Provides the name associated with that box. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic)
	virtual void ReceiveHitBoxClick(const FName BoxName);

	/** Called when a hit box is unclicked. Provides the name associated with that box. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic)
	virtual void ReceiveHitBoxRelease(const FName BoxName);

	/** Called when a hit box is moused over. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic)
	virtual void ReceiveHitBoxBeginCursorOver(const FName BoxName);

	/** Called when a hit box no longer has the mouse over it. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic)
	virtual void ReceiveHitBoxEndCursorOver(const FName BoxName);

	//=============================================================================
	// Kismet API for simple HUD drawing.

	/**
	 * Returns the width and height of a string.
	 * @param Text				String to draw
	 * @param OutWidth			Returns the width in pixels of the string.
	 * @param OutHeight			Returns the height in pixels of the string.
	 * @param Font				Font to draw text.  If NULL, default font is chosen.
	 * @param Scale				Scale multiplier to control size of the text.
	 */
	UFUNCTION(BlueprintCallable, Category=HUD, meta=(Scale = "1"))
	void GetTextSize(const FString& Text, float& OutWidth, float& OutHeight, class UFont* Font=NULL, float Scale=1.f) const;

	/**
	 * Draws a string on the HUD.
	 * @param Text				String to draw
	 * @param TextColor			Color to draw string
	 * @param ScreenX			Screen-space X coordinate of upper left corner of the string.
	 * @param ScreenY			Screen-space Y coordinate of upper left corner of the string.
	 * @param Font				Font to draw text.  If NULL, default font is chosen.
	 * @param Scale				Scale multiplier to control size of the text.
	 * @param bScalePosition	Whether the "Scale" parameter should also scale the position of this draw call.
	 */
	UFUNCTION(BlueprintCallable, Category=HUD, meta=( Scale = "1", bScalePosition = "false"))
	void DrawText(const FString& Text, FLinearColor TextColor, float ScreenX, float ScreenY, class UFont* Font=NULL, float Scale=1.f, bool bScalePosition=false);

	/**
	 * Draws a 2D line on the HUD.
	 * @param StartScreenX		Screen-space X coordinate of start of the line.
	 * @param StartScreenY		Screen-space Y coordinate of start of the line.
	 * @param EndScreenX		Screen-space X coordinate of end of the line.
	 * @param EndScreenY		Screen-space Y coordinate of end of the line.
	 * @param LineColor			Color to draw line
	 */
	UFUNCTION(BlueprintCallable, Category=HUD)
	void DrawLine(float StartScreenX, float StartScreenY, float EndScreenX, float EndScreenY, FLinearColor LineColor);

	/**
	 * Draws a colored untextured quad on the HUD.
	 * @param RectColor			Color of the rect. Can be translucent.
	 * @param ScreenX			Screen-space X coordinate of upper left corner of the quad.
	 * @param ScreenY			Screen-space Y coordinate of upper left corner of the quad.
	 * @param ScreenW			Screen-space width of the quad (in pixels).
	 * @param ScreenH			Screen-space height of the quad (in pixels).
	 */
	UFUNCTION(BlueprintCallable, Category=HUD)
	void DrawRect(FLinearColor RectColor, float ScreenX, float ScreenY, float ScreenW, float ScreenH);

	/**
	 * Draws a textured quad on the HUD.
	 * @param Texture			Texture to draw.
	 * @param ScreenX			Screen-space X coordinate of upper left corner of the quad.
	 * @param ScreenY			Screen-space Y coordinate of upper left corner of the quad.
	 * @param ScreenW			Screen-space width of the quad (in pixels).
	 * @param ScreenH			Screen-space height of the quad (in pixels).
	 * @param TextureU			Texture-space U coordinate of upper left corner of the quad
	 * @param TextureV			Texture-space V coordinate of upper left corner of the quad.
	 * @param TextureUWidth		Texture-space width of the quad (in normalized UV distance).
	 * @param TextureVHeight	Texture-space height of the quad (in normalized UV distance).
	 * @param TintColor			Vertex color for the quad.
	 * @param BlendMode			Controls how to blend this quad with the scene. Translucent by default.
	 * @param Scale				Amount to scale the entire texture (horizontally and vertically)
	 * @param bScalePosition	Whether the "Scale" parameter should also scale the position of this draw call.
	 * @param Rotation			Amount to rotate this quad
	 * @param RotPivot			Location (as proportion of quad, 0-1) to rotate about
	 */
	UFUNCTION(BlueprintCallable, Category=HUD, meta=(Scale = "1", bScalePosition = "false", AdvancedDisplay = "9"))
	void DrawTexture(UTexture* Texture, float ScreenX, float ScreenY, float ScreenW, float ScreenH, float TextureU, float TextureV, float TextureUWidth, float TextureVHeight, FLinearColor TintColor=FLinearColor::White, EBlendMode BlendMode=BLEND_Translucent, float Scale=1.f, bool bScalePosition=false, float Rotation=0.f, FVector2D RotPivot=FVector2D::ZeroVector);

	/**
	 * Draws a textured quad on the HUD. Assumes 1:1 texel density.
	 * @param Texture			Texture to draw.
	 * @param ScreenX			Screen-space X coordinate of upper left corner of the quad.
	 * @param ScreenY			Screen-space Y coordinate of upper left corner of the quad.
	 * @param Scale				Scale multiplier to control size of the text.
	 * @param bScalePosition	Whether the "Scale" parameter should also scale the position of this draw call.
	 */
	UFUNCTION(BlueprintCallable, Category=HUD, meta=(Scale = "1", bScalePosition = "false"))
	void DrawTextureSimple(UTexture* Texture, float ScreenX, float ScreenY, float Scale=1.f, bool bScalePosition=false);

	/**
	 * Draws a material-textured quad on the HUD.
	 * @param Material			Material to use
	 * @param ScreenX			Screen-space X coordinate of upper left corner of the quad.
	 * @param ScreenY			Screen-space Y coordinate of upper left corner of the quad.
	 * @param ScreenW			Screen-space width of the quad (in pixels).
	 * @param ScreenH			Screen-space height of the quad (in pixels).
	 * @param MaterialU			Texture-space U coordinate of upper left corner of the quad
	 * @param MaterialV			Texture-space V coordinate of upper left corner of the quad.
	 * @param MaterialUWidth	Texture-space width of the quad (in normalized UV distance).
	 * @param MaterialVHeight	Texture-space height of the quad (in normalized UV distance).
	 * @param Scale				Amount to scale the entire texture (horizontally and vertically)
	 * @param bScalePosition	Whether the "Scale" parameter should also scale the position of this draw call.
	 * @param Rotation			Amount to rotate this quad
	 * @param RotPivot			Location (as proportion of quad, 0-1) to rotate about
	 */
	UFUNCTION(BlueprintCallable, Category=HUD, meta=(Scale = "1", bScalePosition = "false", AdvancedDisplay = "9"))
	void DrawMaterial(UMaterialInterface* Material, float ScreenX, float ScreenY, float ScreenW, float ScreenH, float MaterialU, float MaterialV, float MaterialUWidth, float MaterialVHeight, float Scale=1.f, bool bScalePosition=false, float Rotation=0.f, FVector2D RotPivot=FVector2D::ZeroVector);

	/**
	 * Draws a material-textured quad on the HUD.  Assumes UVs such that the entire material is shown.
	 * @param Material			Material to use
	 * @param ScreenX			Screen-space X coordinate of upper left corner of the quad.
	 * @param ScreenY			Screen-space Y coordinate of upper left corner of the quad.
	 * @param ScreenW			Screen-space width of the quad (in pixels).
	 * @param ScreenH			Screen-space height of the quad (in pixels).
	 * @param Scale				Amount to scale the entire texture (horizontally and vertically)
	 * @param bScalePosition	Whether the "Scale" parameter should also scale the position of this draw call.
	 */
	UFUNCTION(BlueprintCallable, Category=HUD, meta=(Scale = "1", bScalePosition = "false"))
	void DrawMaterialSimple(UMaterialInterface* Material, float ScreenX, float ScreenY, float ScreenW, float ScreenH, float Scale=1.f, bool bScalePosition=false);

	/** Transforms a 3D world-space vector into 2D screen coordinates */
	UFUNCTION(BlueprintPure, Category=HUD)
	FVector Project(FVector Location);
	
	/**
	 * Add a hitbox to the hud
	 * @param Position			Coordinates of the top left of the hit box.
	 * @param Size				Size of the hit box.
	 * @param Name				Name of the hit box.
	 * @param bConsumesInput	Whether click processing should continue if this hit box is clicked.
	 * @param Priority			The priority of the box used for layering. Larger values are considered first.  Equal values are considered in the order they were added.
	 */
	UFUNCTION(BlueprintCallable, Category=HUD, meta=(InPriority="0"))
	void AddHitBox(FVector2D Position, FVector2D Size, FName Name, bool bConsumesInput, int32 Priority = 0);

protected:
	FSimpleReticle	Reticle;

	/** Returns the PlayerController for this HUD's player.	 */
	UFUNCTION(BlueprintCallable, Category=HUD)
	APlayerController* GetOwningPlayerController() const;

	/** Returns the Pawn for this HUD's player.	 */
	UFUNCTION(BlueprintCallable, Category=HUD)
	APawn* GetOwningPawn() const;

public:
	/**
	 * Draws a colored line between two points
	 * @param Start - start of line
	 * @param End - end if line
	 * @param LineColor
	 */
	void Draw3DLine(FVector Start, FVector End, FColor LineColor);

	/**
	 * Draws a colored line between two points
	 * @param X1 - start of line x
	 * @param Y1 - start of line y
	 * @param X2 - end of line x
	 * @param Y3 - end of line y
	 * @param LineColor
	 */
	void Draw2DLine(int32 X1, int32 Y1, int32 X2, int32 Y2, FColor LineColor);

	/** Set the canvas and debug canvas to use during drawing */
	void SetCanvas(class UCanvas* InCanvas, class UCanvas* InDebugCanvas);

	virtual void PostInitializeComponents() OVERRIDE;

	/** draw overlays for actors that were rendered this tick and have added themselves to the PostRenderedActors array	*/
	virtual void DrawActorOverlays(FVector Viewpoint, FRotator ViewRotation);

	/** Called in PostInitializeComponents or postprocessing chain has changed (happens because of the worldproperties can define it's own chain and this one is set late). */
	virtual void NotifyBindPostProcessEffects();

	/************************************************************************************************************
	 Actor Render - These functions allow for actors in the world to gain access to the hud and render
	 information on it.
	************************************************************************************************************/

	/** remove an actor from the PostRenderedActors array */
	virtual void RemovePostRenderedActor(class AActor* A);

	/** add an actor to the PostRenderedActors array */
	virtual void AddPostRenderedActor(class AActor* A);

	/**
	 * check if we should be display debug information for particular types of debug messages
	 * @param DebugType - type of debug message
	 * @result bool - true if it should be displayed
	 */
	virtual bool ShouldDisplayDebug(FName DebugType);

	/** 
	 * Entry point for basic debug rendering on the HUD.  Activated and controlled via the "showdebug" console command.  
	 * Can be overridden to display custom debug per-game. 
	 */
	virtual void ShowDebugInfo(float& YL, float& YPos);

	/** PostRender is the main draw loop. */
	virtual void PostRender();

	/** The Main Draw loop for the hud.  Gets called before any messaging.  Should be subclassed */
	virtual void DrawHUD();

	//=============================================================================
	// Messaging.
	//=============================================================================

	/** Display current messages */
	virtual void DrawText(const FString& Text, FVector2D Position, class UFont* TextFont, FVector2D FontScale, FColor TextColor);
	
	/** @return UFont* from given FontSize index */
	virtual class UFont* GetFontFromSizeIndex(int32 FontSize) const;

	/**
	 *	Pauses or unpauses the game due to main window's focus being lost.
	 *	@param Enable - tells whether to enable or disable the pause state
	 */
	virtual void OnLostFocusPause(bool bEnable);

	/**
	 * Iterate through list of debug text and draw it over the associated actors in world space.
	 * Also handles culling null entries, and reducing the duration for timed debug text.
	 */
	void DrawDebugTextList();

	/** Gives the HUD a chance to display project-specific data when taking a "bug" screen shot.	 */
	virtual void HandleBugScreenShot() { }

	/** Array of hitboxes for this frame. */
	TArray< FHUDHitBox >	HitBoxMap;
	
	/** Array of hitboxes that have been hit for this frame. */
	TArray< class FHUDHitBox* >	HitBoxHits;

	/** Set of hitbox (by name) that are currently moused over */
	TSet< FName > HitBoxesOver;

	/**
	 * Debug renderer for this frames hitboxes.
	 * @param	Canvas	Canvas on which to render debug boxes.
	 */
	void RenderHitBoxes( FCanvas* InCanvas );
	
	/**
	 * Update the list of hitboxes and dispatch events for any hits.
	 * @param	InEventType	Input event that triggered the call.
	 */
	bool UpdateAndDispatchHitBoxClickEvents(EInputEvent InEventType);
	
	/**
	 * Update a the list of hitboxes that have been hit this frame.
	 * @param	Canvas	Canvas on which to render debug boxes.
	 */
	void UpdateHitBoxCandidates( const FVector2D& InHitLocation );
	
	/** Have any hitboxes been hit this frame. */
	bool AnyCurrentHitBoxHits() const;	

	/**
	 * Find the first hitbox containing the given coordinates.
	 * @param	InHitLocation Coordinates to check
	 * @return returns the hitbox at the given coordinates or NULL if none match.
	 */
	const FHUDHitBox* GetHitBoxAtCoordinates( const FVector2D& InHitLocation ) const;

	/**
	 * Return the hitbox with the given name
	 * @param	InName	Name of required hitbox
	 * @return returns the hitbox with the given name NULL if none match.
	 */
	const FHUDHitBox* GetHitBoxWithName( const FName& InName ) const;

	//=============================================================================
	// UIBlur overrides
	//=============================================================================

	/** Empty the list of UIBlur rectangles  */
	void ClearUIBlurOverrideRects();

	/** Add a rectangle in which UIBlur will be enabled */
	void AddUIBlurOverrideRect( const FIntRect& UIBlurOverrideRect );

	/** Get the array of rectangles in which UIBlur enabled */
	const TArray<FIntRect>& GetUIBlurRectangles() const;


protected:
	bool IsCanvasValid_WarnIfNot() const;
};



