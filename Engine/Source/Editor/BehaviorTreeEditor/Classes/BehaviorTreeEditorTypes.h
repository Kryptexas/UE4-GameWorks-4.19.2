// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeEditorTypes.generated.h"

struct FAbortDrawHelper
{
	uint16 AbortStart;
	uint16 AbortEnd;
	uint16 SearchStart;
	uint16 SearchEnd;

	FAbortDrawHelper() : AbortStart(MAX_uint16), AbortEnd(MAX_uint16), SearchStart(MAX_uint16), SearchEnd(MAX_uint16) {}
};

struct FClassData
{
	FClassData() {}
	FClassData(UClass* InClass, const FString& InDeprecatedMessage) : 
		bIsHidden(0), bHideParent(0), Class(InClass), DeprecatedMessage(InDeprecatedMessage)
	{}
	FClassData(const FString& InAssetName, const FString& InGeneratedClassPackage, const FString& InClassName) :
		bIsHidden(0), bHideParent(0), AssetName(InAssetName), GeneratedClassPackage(InGeneratedClassPackage), ClassName(InClassName) 
	{}

	FString ToString() const;
	FString GetClassName() const;
	UClass* GetClass();
	bool IsAbstract() const;

	FORCEINLINE bool IsDeprecated() const { return DeprecatedMessage.Len() > 0; }
	FORCEINLINE FString GetDeprecatedMessage() const { return DeprecatedMessage; }

	/** set when child class masked this one out (e.g. always use game specific class instead of engine one) */
	uint32 bIsHidden : 1;

	/** set when class wants to hide parent class from selection (just one class up hierarchy) */
	uint32 bHideParent : 1;

private:

	/** pointer to uclass */
	TWeakObjectPtr<UClass> Class;

	/** path to class if it's not loaded yet */
	FString AssetName;
	FString GeneratedClassPackage;

	/** resolved name of class from asset data */
	FString ClassName;

	/** message for deprecated class */
	FString DeprecatedMessage;
};

struct FClassDataNode
{
	FClassData Data;
	FString ParentClassName;

	TSharedPtr<FClassDataNode> ParentNode;
	TArray<TSharedPtr<FClassDataNode> > SubNodes;
};

struct FClassBrowseHelper
{
	FClassBrowseHelper();
	~FClassBrowseHelper();

	static void GatherClasses(const UClass* BaseClass, TArray<FClassData>& AvailableClasses);
	static FString GetDeprecationMessage(const UClass* Class);

	void OnAssetAdded(const class FAssetData& AssetData);
	void OnAssetRemoved(const class FAssetData& AssetData);
	void InvalidateCache();

private:

	TSharedPtr<FClassDataNode> RootNode;
	
	TSharedPtr<FClassDataNode> CreateClassDataNode(const class FAssetData& AssetData);
	TSharedPtr<FClassDataNode> FindBaseClassNode(TSharedPtr<FClassDataNode> Node, const FString& ClassName);
	void FindAllSubClasses(TSharedPtr<FClassDataNode> Node, TArray<FClassData>& AvailableClasses);

	UClass* FindAssetClass(const FString& GeneratedClassPackage, const FString& AssetName);
	void BuildClassGraph();
	void AddClassGraphChildren(TSharedPtr<FClassDataNode> Node, TArray<TSharedPtr<FClassDataNode> >& NodeList);

	bool IsHidingParentClass(UClass* Class);
};

struct FCompareNodeXLocation
{
	FORCEINLINE bool operator()(const UEdGraphPin& A, const UEdGraphPin& B) const
	{
		return A.GetOwningNode()->NodePosX < B.GetOwningNode()->NodePosX;
	}
};

namespace ESubNode
{
	enum Type {
		Decorator,
		Service
	};
}

struct FNodeBounds
{
	FVector2D Position;
	FVector2D Size;

	FNodeBounds(FVector2D InPos, FVector2D InSize)
	{
		Position = InPos;
		Size = InSize;
	}
};

UCLASS()
class UBehaviorTreeEditorTypes : public UObject
{
	GENERATED_UCLASS_BODY()

	static const FString PinCategory_MultipleNodes;
	static const FString PinCategory_SingleComposite;
	static const FString PinCategory_SingleTask;
	static const FString PinCategory_SingleNode;
};
