// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DragDropOperation.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDragDropMulticast, UDragDropOperation*, Operation);


/**
 * This class is the base drag drop operation for UMG, extend it to add additional data and add new functionality.
 */
UCLASS()
class UMG_API UDragDropOperation : public UObject
{
	GENERATED_UCLASS_BODY()
	
public:
	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drag and Drop", meta=( ExposeOnSpawn="true" ))
	FString Tag;
	
	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drag and Drop", meta=( ExposeOnSpawn="true" ))
	UObject* Payload;
	
	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drag and Drop", meta=( ExposeOnSpawn="true" ))
	class UWidget* DefaultDragVisual;

	/**  */
	UPROPERTY(BlueprintAssignable)
	FOnDragDropMulticast OnDrop;

	/**  */
	UPROPERTY(BlueprintAssignable)
	FOnDragDropMulticast OnDropCanceled;

	/**  */
	UPROPERTY(BlueprintAssignable)
	FOnDragDropMulticast OnDragged;

	/**  */
	UFUNCTION(BlueprintNativeEvent, Category="Drag and Drop")
	void Drop(const FPointerEvent& PointerEvent);

	/**  */
	UFUNCTION(BlueprintNativeEvent, Category="Drag and Drop")
	void DropCanceled(const FPointerEvent& PointerEvent);

	/**  */
	UFUNCTION(BlueprintNativeEvent, Category="Drag and Drop")
	void Dragged(const FPointerEvent& PointerEvent);
};
