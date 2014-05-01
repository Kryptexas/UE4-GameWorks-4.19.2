// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UserWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVisibilityChangedEvent, ESlateVisibility::Type, Visibility);

UCLASS(Abstract, hideCategories=(Object, Actor))
class UMG_API AUserWidget : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	TEnumAsByte<ESlateVisibility::Type> Visiblity;

	/** Called when the visibility changes. */
	UPROPERTY(BlueprintAssignable)
	FOnVisibilityChangedEvent OnVisibilityChanged;

	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	uint32 bShowCursorWhenVisible : 1;

	// AActor interface
	virtual void PostActorCreated() OVERRIDE;
	virtual void Destroyed() OVERRIDE;
	virtual void RerunConstructionScripts() OVERRIDE;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) OVERRIDE;
	// End of AActor interface

	/*  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void Show();

	/*  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void Hide();

	UFUNCTION(BlueprintPure, Category="Appearance")
	bool GetIsVisible();

	UFUNCTION(BlueprintPure, Category="Appearance")
	TEnumAsByte<ESlateVisibility::Type> GetVisiblity();

	USlateWrapperComponent* GetWidgetHandle(TSharedRef<SWidget> InWidget);

	TSharedRef<SWidget> GetRootWidget();

	TSharedPtr<SWidget> GetWidgetFromName(const FString& Name) const;
	USlateWrapperComponent* GetHandleFromName(const FString& Name) const;


private:
	TSharedPtr<SWidget> RootWidget;
	TMap< TWeakPtr<SWidget>, USlateWrapperComponent* > WidgetToComponent;

private:
	void RebuildWrapperWidget();
};
