// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetReflectorNode.generated.h"

/** 
 * A widget reflector node that contains the interface and basic data required by both live and snapshot nodes 
 */
UCLASS()
class UWidgetReflectorNodeBase : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Initialize this node from the given widget geometry, caching out any data that may be required for future visualization in the widget reflector
	 */
	virtual void Initialize(const FArrangedWidget& InWidgetGeometry);

	/**
	 * @note This function only works for ULiveWidgetReflectorNode instances
	 * @return The live widget that this node is referencing
	 */
	virtual TSharedPtr<SWidget> GetLiveWidget() const;

	/**
	 * @return The type string for the widget we were initialized from
	 */
	virtual FText GetWidgetType() const;
	
	/**
	 * @return The visibility string for the widget we were initialized from
	 */
	virtual FText GetWidgetVisibilityText() const;

	/**
	 * The human readable location for widgets that are defined in C++ is the file and line number
	 * The human readable location for widgets that are defined in UMG is the asset name
	 * @return The fully human readable location for the widget we were initialized from
	 */
	virtual FText GetWidgetReadableLocation() const;

	/**
	 * @return The name of the file that the widget we were initialized from was created from (for C++ widgets)
	 */
	virtual FString GetWidgetFile() const;

	/**
	 * @return The line number of the file that the widget we were initialized from was created from (for C++ widgets)
	 */
	virtual int32 GetWidgetLineNumber() const;

	/**
	 * @return The name of the asset that the widget we were initialized from was created from (for UMG widgets)
	 */
	virtual FName GetWidgetAssetName() const;

	/**
	 * @return The desired size of the widget we were initialized from
	 */
	virtual FVector2D GetWidgetDesiredSize() const;

	/**
	 * @return The foreground color of the widget we were initialized from
	 */
	virtual FSlateColor GetWidgetForegroundColor() const;

	/**
	 * @return The in-memory address of the widget we were initialized from
	 */
	virtual FString GetWidgetAddress() const;

	/**
	 * @return True if the the widget we were initialized from is enabled, false otherwise
	 */
	virtual bool GetWidgetEnabled() const;

	/**
	 * @return The geometry of the widget we were initialized from
	 */
	const FGeometry& GetGeometry() const;

	/**
	 * @return The tint that is applied to text in order to provide visual hints
	 */
	const FLinearColor& GetTint() const;

	/**
	 * Set the tint to the given value
	 */
	void SetTint(const FLinearColor& InTint);

	/**
	 * Add the given node to our list of children for this widget
	 */
	void AddChildNode(UWidgetReflectorNodeBase* InChildNode);

	/**
	 * @return The node entries for the widget's children
	 */
	const TArray<UWidgetReflectorNodeBase*>& GetChildNodes() const;

protected:
	/** The geometry of the widget */
	UPROPERTY()
	FGeometry Geometry;

	/** Node entries for the widget's children */
	UPROPERTY()
	TArray<UWidgetReflectorNodeBase*> ChildNodes;

	/** A tint that is applied to text in order to provide visual hints */
	UPROPERTY()
	FLinearColor Tint;
};

/** 
 * A widget reflector node that holds on to the widget it references so that certain properties can be updated live 
 */
UCLASS()
class ULiveWidgetReflectorNode : public UWidgetReflectorNodeBase
{
	GENERATED_BODY()

public:
	// UWidgetReflectorNodeBase interface
	virtual void Initialize(const FArrangedWidget& InWidgetGeometry) override;
	virtual TSharedPtr<SWidget> GetLiveWidget() const override;
	virtual FText GetWidgetType() const override;
	virtual FText GetWidgetVisibilityText() const override;
	virtual FText GetWidgetReadableLocation() const override;
	virtual FString GetWidgetFile() const override;
	virtual int32 GetWidgetLineNumber() const override;
	virtual FName GetWidgetAssetName() const override;
	virtual FVector2D GetWidgetDesiredSize() const override;
	virtual FSlateColor GetWidgetForegroundColor() const override;
	virtual FString GetWidgetAddress() const override;
	virtual bool GetWidgetEnabled() const override;

private:
	/** The widget this node is watching */
	TWeakPtr<SWidget> Widget;
};

/** 
 * A widget reflector node that holds the widget information from a snapshot at a given point in time 
 */
UCLASS()
class USnapshotWidgetReflectorNode : public UWidgetReflectorNodeBase
{
	GENERATED_BODY()

public:
	// UWidgetReflectorNodeBase interface
	virtual void Initialize(const FArrangedWidget& InWidgetGeometry) override;
	virtual FText GetWidgetType() const override;
	virtual FText GetWidgetVisibilityText() const override;
	virtual FText GetWidgetReadableLocation() const override;
	virtual FString GetWidgetFile() const override;
	virtual int32 GetWidgetLineNumber() const override;
	virtual FName GetWidgetAssetName() const override;
	virtual FVector2D GetWidgetDesiredSize() const override;
	virtual FSlateColor GetWidgetForegroundColor() const override;
	virtual FString GetWidgetAddress() const override;
	virtual bool GetWidgetEnabled() const override;

private:
	/** The type string of the widget at the point it was passed to Initialize */
	UPROPERTY()
	FText CachedWidgetType;

	/** The visibility string of the widget at the point it was passed to Initialize */
	UPROPERTY()
	FText CachedWidgetVisibilityText;

	/** The human readable location (source file for C++ widgets, asset name for UMG widgets) of the widget at the point it was passed to Initialize */
	UPROPERTY()
	FText CachedWidgetReadableLocation;

	/** The name of the file that the widget was created from at the point it was passed to Initialize (for C++ widgets) */
	UPROPERTY()
	FString CachedWidgetFile;

	/** The line number of the file that the widget was created from at the point it was passed to Initialize (for C++ widgets) */
	UPROPERTY()
	int32 CachedWidgetLineNumber;

	/** The name of the asset that the widget was created from at the point it was passed to Initialize (for UMG widgets) */
	UPROPERTY()
	FName CachedWidgetAssetName;

	/** The desired size of the widget at the point it was passed to Initialize */
	UPROPERTY()
	FVector2D CachedWidgetDesiredSize;

	/** The foreground color of the widget at the point it was passed to Initialize */
	UPROPERTY()
	FSlateColor CachedWidgetForegroundColor;

	/** The in-memory address of the widget at the point it was passed to Initialize */
	UPROPERTY()
	FString CachedWidgetAddress;

	/** The enabled state of the widget at the point it was passed to Initialize */
	UPROPERTY()
	bool CachedWidgetEnabled;
};


class FWidgetReflectorNodeUtils
{
public:
	/**
	 * Create a single node referencing a live widget.
	 *
	 * @param InNodeClass The type of widget reflector node to create
	 * @param InWidgetGeometry Optional widget and associated geometry which this node should represent
	 */
	static ULiveWidgetReflectorNode* NewLiveNode(const FArrangedWidget& InWidgetGeometry = FArrangedWidget(SNullWidget::NullWidget, FGeometry()));

	/**
	 * Create nodes for the supplied widget and all their children such that they reference a live widget
	 * Note that we include both visible and invisible children!
	 * 
	 * @param InNodeClass The type of widget reflector node to create
	 * @param InWidgetGeometry Widget and geometry whose children to capture in the snapshot.
	 */
	static ULiveWidgetReflectorNode* NewLiveNodeTreeFrom(const FArrangedWidget& InWidgetGeometry);

	/**
	 * Create a single node referencing a snapshot of its current state.
	 *
	 * @param InNodeClass The type of widget reflector node to create
	 * @param InWidgetGeometry Optional widget and associated geometry which this node should represent
	 */
	static USnapshotWidgetReflectorNode* NewSnapshotNode(const FArrangedWidget& InWidgetGeometry = FArrangedWidget(SNullWidget::NullWidget, FGeometry()));

	/**
	 * Create nodes for the supplied widget and all their children such that they reference a snapshot of their current state
	 * Note that we include both visible and invisible children!
	 * 
	 * @param InNodeClass The type of widget reflector node to create
	 * @param InWidgetGeometry Widget and geometry whose children to capture in the snapshot.
	 */
	static USnapshotWidgetReflectorNode* NewSnapshotNodeTreeFrom(const FArrangedWidget& InWidgetGeometry);

private:
	/**
	 * Create a single node.
	 *
	 * @param InNodeClass The type of widget reflector node to create
	 * @param InWidgetGeometry Optional widget and associated geometry which this node should represent
	 */
	static UWidgetReflectorNodeBase* NewNode(const TSubclassOf<UWidgetReflectorNodeBase>& InNodeClass, const FArrangedWidget& InWidgetGeometry = FArrangedWidget(SNullWidget::NullWidget, FGeometry()));

	/**
	 * Create nodes for the supplied widget and all their children
	 * Note that we include both visible and invisible children!
	 * 
	 * @param InNodeClass The type of widget reflector node to create
	 * @param InWidgetGeometry Widget and geometry whose children to capture in the snapshot.
	 */
	static UWidgetReflectorNodeBase* NewNodeTreeFrom(const TSubclassOf<UWidgetReflectorNodeBase>& InNodeClass, const FArrangedWidget& InWidgetGeometry);

public:
	/**
	 * Locate all the widgets from a widget path in a list of nodes and their children.
	 * @note This only really works for live nodes, as the snapshot nodes may no longer exist, or not even be local to this machine
	 *
	 * @param CandidateNodes A list of FReflectorNodes that represent widgets.
	 * @param WidgetPathToFind We want to find all reflector nodes corresponding to widgets in this path
	 * @param SearchResult An array that gets results put in it
	 * @param NodeIndexToFind Index of the widget in the path that we are currently looking for; we are done when we've found all of them
	 */
	static void FindLiveWidgetPath(const TArray<UWidgetReflectorNodeBase*>& CandidateNodes, const FWidgetPath& WidgetPathToFind, TArray<UWidgetReflectorNodeBase*>& SearchResult, int32 NodeIndexToFind = 0);

public:
	/**
	 * @return The type string for the given widget
	 */
	static FText GetWidgetType(const TSharedPtr<SWidget>& InWidget);
	
	/**
	 * @return The current visibility string for the given widget
	 */
	static FText GetWidgetVisibilityText(const TSharedPtr<SWidget>& InWidget);
	
	/**
	 * The human readable location for widgets that are defined in C++ is the file and line number
	 * The human readable location for widgets that are defined in UMG is the asset name
	 * @return The fully human readable location for the given widget
	 */
	static FText GetWidgetReadableLocation(const TSharedPtr<SWidget>& InWidget);
	
	/**
	 * @return The name of the file that this widget was created from (for C++ widgets)
	 */
	static FString GetWidgetFile(const TSharedPtr<SWidget>& InWidget);
	
	/**
	 * @return The line number of the file that this widget was created from (for C++ widgets)
	 */
	static int32 GetWidgetLineNumber(const TSharedPtr<SWidget>& InWidget);

	/**
	 * @return The name of the asset that this widget was created from (for UMG widgets)
	 */
	static FName GetWidgetAssetName(const TSharedPtr<SWidget>& InWidget);

	/**
	 * @return The current desired size of the given widget
	 */
	static FVector2D GetWidgetDesiredSize(const TSharedPtr<SWidget>& InWidget);

	/**
	 * @return The in-memory address of the widget, converted to a string
	 */
	static FString GetWidgetAddress(const TSharedPtr<SWidget>& InWidget);

	/**
	 * @return The current foreground color of the given widget
	 */
	static FSlateColor GetWidgetForegroundColor(const TSharedPtr<SWidget>& InWidget);

	/**
	 * @return True if the given widget is currently enabled, false otherwise
	 */
	static bool GetWidgetEnabled(const TSharedPtr<SWidget>& InWidget);
};
