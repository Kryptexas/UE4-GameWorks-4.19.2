// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EdGraph/EdGraphNode.h" // for ENodeTitleType
#include "EdGraph/EdGraphSchema.h"

/*******************************************************************************
 * FNodeTextCache
 ******************************************************************************/

/** 
 * Constructing FText strings every frame can be costly, so this struct provides 
 * a way to easily cache those strings (node title, tooptip, etc.) for reuse.
 */
struct FNodeTextCache
{
public:
	FNodeTextCache() :
		CacheRefreshID(0)
	{}

	/**
	 * Checks if the title is out of date
	 *
	 * @param InOwningNode		Node that owns this title
	 */
	FORCEINLINE bool IsOutOfDate(const UEdGraphNode* InOwningNode) const
	{
		check(InOwningNode);
		return CachedText.IsEmpty() || (InOwningNode->GetSchema()->IsCacheVisualizationOutOfDate(CacheRefreshID));
	}

	/**
	 * Checks if the title is out of date
	 *
	 * @param InText			Text to cache
	 * @param InOwningNode		Node that owns this title
	 */
	FORCEINLINE void SetCachedText(FText const& InText, const UEdGraphNode* InOwningNode) const
	{
		check(InOwningNode);
		CachedText = InText;
		CacheRefreshID = InOwningNode->GetSchema()->GetCurrentVisualizationCacheID();
	}

	/** */
	FORCEINLINE FText& GetCachedText() const
	{
		return CachedText;
	}

	/** */
	FORCEINLINE void MarkDirty() const
	{
		Clear();
		CacheRefreshID = 0;
	}

	/** */
	FORCEINLINE void Clear() const
	{
		CachedText = FText::GetEmpty();
	}

	/** */
	FORCEINLINE operator FText&() const
	{
		return GetCachedText();
	}

private:
	/** Mutable so that SetCachedText() can remain const (callable by the node's GetNodeTitle() method)*/
	mutable FText CachedText;

	/** ID to check if the title should be considered dirty due to outside conditions that may require the title to refresh */
	mutable int32 CacheRefreshID;
};

/*******************************************************************************
 * FNodeTitleTextTable
 ******************************************************************************/

struct FNodeTitleTextTable
{
public:
	/**
	 * Checks if the title of the passed type is cached
	 *
	 * @param InTitleType		Type of title to be examined
	 * @param InOwningNode		Node that owns this title
	 */
	FORCEINLINE bool IsTitleCached(ENodeTitleType::Type InTitleType, const UEdGraphNode* InOwningNode) const
	{
		return !CachedNodeTitles[InTitleType].IsOutOfDate(InOwningNode);
	}

	/**
	 * Caches the specific title type
	 *
	 * @param InTitleType		Type of title to be examined
	 * @param InText			Text to cache
	 * @param InOwningNode		Node that owns this title
	 */
	FORCEINLINE void SetCachedTitle(ENodeTitleType::Type InTitleType, FText const& InText, const UEdGraphNode* InOwningNode) const
	{
		CachedNodeTitles[InTitleType].SetCachedText(InText, InOwningNode);
	}

	/** */
	FORCEINLINE FText& GetCachedTitle(ENodeTitleType::Type TitleType) const
	{
		return CachedNodeTitles[TitleType].GetCachedText();
	}

	/** */
	FORCEINLINE void MarkDirty() const
	{
		for (uint32 TitleIndex = 0; TitleIndex < ENodeTitleType::MAX_TitleTypes; ++TitleIndex)
		{
			CachedNodeTitles[TitleIndex].MarkDirty();
		}
	}

	/** */
	FORCEINLINE FText& operator[](ENodeTitleType::Type TitleIndex) const
	{
		return GetCachedTitle(TitleIndex);
	}

private:
	/** */
	FNodeTextCache CachedNodeTitles[ENodeTitleType::MAX_TitleTypes];
};

/*******************************************************************************
* FNodeTextTable
******************************************************************************/

struct FNodeTextTable : FNodeTitleTextTable
{
public:
	/**
	 * Checks if there is text cached
	 *
	 * @param InOwningNode		Node that owns this title
	 */
	FORCEINLINE bool IsTooltipCached(const UEdGraphNode* InOwningNode) const
	{
		return !CachedTooltip.IsOutOfDate(InOwningNode);
	}

	/**
	 * Caches the tooltip
	 *
	 * @param InText			Text to cache
	 * @param InOwningNode		Node that owns this title
	 */
	FORCEINLINE void SetCachedTooltip(FText const& InText, const UEdGraphNode* InOwningNode) const
	{
		CachedTooltip.SetCachedText(InText, InOwningNode);
	}

	/** */
	FORCEINLINE FText GetCachedTooltip() const
	{
		return CachedTooltip.GetCachedText();
	}

	/** */
	FORCEINLINE void MarkDirty() const
	{
		FNodeTitleTextTable::MarkDirty();
		CachedTooltip.MarkDirty();
	}

private:
	/** */
	FNodeTextCache CachedTooltip;
};
