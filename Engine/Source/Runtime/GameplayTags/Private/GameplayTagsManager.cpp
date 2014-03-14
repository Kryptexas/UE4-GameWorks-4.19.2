// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsModulePrivatePCH.h"
//#include "AssetRegistryModule.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#endif


UGameplayTagsManager::UGameplayTagsManager(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

const UDataTable* UGameplayTagsManager::LoadGameplayTagTable( FString TagTableName )
{
	if (!IsRunningCommandlet() && !GameplayTagTable && !TagTableName.IsEmpty())
	{
		GameplayTagTable = LoadObject<UDataTable>(NULL, *TagTableName, NULL, LOAD_None, NULL);
#if WITH_EDITOR
		// Hook into notifications for object re-imports so that the gameplay tag tree can be reconstructed if the table changes
		if (GIsEditor && GameplayTagTable)
		{
			GEditor->OnObjectReimported().AddUObject(this, &UGameplayTagsManager::OnObjectReimported);
		}
#endif
	}
	return GameplayTagTable;
}

// this is here because the tag tree doesn't start with a node and recursion can't be done until the first node is found
void UGameplayTagsManager::GetAllNodesForTag_Recurse(TArray<FString>& Tags, int32 CurrentTagDepth, TSharedPtr<FGameplayTagNode> CurrentTagNode, TArray< TSharedPtr<FGameplayTagNode> >& OutTagArray)
{
	CurrentTagDepth++;
	if(Tags.Num() > CurrentTagDepth)
	{
		// search for the subsequent tags in the hierarchy
		TArray< TSharedPtr<FGameplayTagNode> > CurrentChildrenTags;
		CurrentChildrenTags = CurrentTagNode->GetChildTagNodes();
		for(int32 TagIdx = 0; TagIdx < CurrentChildrenTags.Num(); TagIdx++)
		{
			FString CurrentTagName = CurrentChildrenTags[TagIdx].Get()->GetSimpleTag().ToString();
			if(CurrentTagName == Tags[CurrentTagDepth])
			{
				CurrentTagNode = CurrentChildrenTags[TagIdx];
				OutTagArray.Add(CurrentTagNode);
				GetAllNodesForTag_Recurse(Tags, CurrentTagDepth, CurrentTagNode, OutTagArray);
				break;
			}
		}
	}
}

void UGameplayTagsManager::GetAllNodesForTag( const FString& Tag, TArray< TSharedPtr<FGameplayTagNode> >& OutTagArray )
{
	TArray<FString> Tags;

	OutTagArray.Empty();
	if(Tag.ParseIntoArray( &Tags, TEXT( "." ), true ) > 0)
	{
		int32 CurrentTagDepth = 0;
		// find the first node of the tag
		TSharedPtr<FGameplayTagNode> CurrentTagNode = NULL;
		for(int32 TagIdx = 0; TagIdx < GameplayRootTags.Num(); TagIdx++)
		{
			FString TagName = GameplayRootTags[TagIdx].Get()->GetSimpleTag().ToString();
			if(TagName == Tags[CurrentTagDepth])
			{
				CurrentTagNode = GameplayRootTags[TagIdx];
				break;
			}
		}

		if(CurrentTagNode.IsValid())
		{
			// add it to the list of tags
			OutTagArray.Add(CurrentTagNode);
			GetAllNodesForTag_Recurse(Tags, CurrentTagDepth, CurrentTagNode, OutTagArray);
		}
	}	
}

struct FCompareFGameplayTagNodeByTag
{
	FORCEINLINE bool operator()( const TSharedPtr<FGameplayTagNode>& A, const TSharedPtr<FGameplayTagNode>& B ) const
	{
		return (A->GetSimpleTag().Compare(B->GetSimpleTag())) < 0;
	}
};

void UGameplayTagsManager::ConstructGameplayTagTree()
{
	if (GameplayRootTags.Num() == 0 && GameplayTagTable)
	{
		TSet<FName> RootTags;

		static const FString ContextString(TEXT("UNKNOWN"));
		const int32 NumRows = GameplayTagTable->RowMap.Num();
		for (int32 RowIdx = 0; RowIdx < NumRows; ++RowIdx)
		{
			FGameplayTagTableRow* TagRow = GameplayTagTable->FindRow<FGameplayTagTableRow>(*FString::Printf(TEXT("%d"), RowIdx), ContextString);
			if (TagRow)
			{
				// Split the tag text on the "." delimiter to establish tag depth and then insert each tag into the
				// gameplay tag tree
				TArray<FString> SubTags;
				TagRow->Tag.ParseIntoArray(&SubTags, TEXT("."), true);

				if (SubTags.Num() > 0)
				{
					int32 InsertionIdx = InsertTagIntoNodeArray(*SubTags[0], NULL, GameplayRootTags, SubTags.Num() == 1 ? TagRow->CategoryText : FText());
					TSharedPtr<FGameplayTagNode> CurNode = GameplayRootTags[InsertionIdx];

					for (int32 SubTagIdx = 1; SubTagIdx < SubTags.Num(); ++SubTagIdx)
					{
						TArray< TSharedPtr<FGameplayTagNode> >& ChildTags = CurNode.Get()->GetChildTagNodes();
						FText Description;
						if(SubTagIdx == SubTags.Num() - 1)
						{
							Description = TagRow->CategoryText;
						}
						InsertionIdx = InsertTagIntoNodeArray(*SubTags[SubTagIdx], CurNode, ChildTags, Description);

						CurNode = ChildTags[InsertionIdx];
					}
				}
			}
		}
		GameplayRootTags.Sort(FCompareFGameplayTagNodeByTag());
	}
}

UGameplayTagsManager::~UGameplayTagsManager()
{
	DestroyGameplayTagTree();
}

void UGameplayTagsManager::DestroyGameplayTagTree()
{
	if (GameplayRootTags.Num() > 0)
	{
		for (int32 RootTagIdx = 0; RootTagIdx < GameplayRootTags.Num(); ++RootTagIdx)
		{
			GameplayRootTags[RootTagIdx]->ResetNode();
		}
	}
	GameplayRootTags.Empty();
}

int32 UGameplayTagsManager::InsertTagIntoNodeArray(FName Tag, TWeakPtr<FGameplayTagNode> ParentNode, TArray< TSharedPtr<FGameplayTagNode> >& NodeArray, FText CategoryDescription)
{
	int32 InsertionIdx = INDEX_NONE;

	// See if the tag is already in the array
	for (int32 CurIdx = 0; CurIdx < NodeArray.Num(); ++CurIdx)
	{
		if (NodeArray[CurIdx].IsValid() && NodeArray[CurIdx].Get()->GetSimpleTag() == Tag)
		{
			InsertionIdx = CurIdx;
			break;
		}
	}

	// Insert the tag at the end of the array if not already found
	if (InsertionIdx == INDEX_NONE)
	{
		InsertionIdx = NodeArray.Add(MakeShareable(new FGameplayTagNode(Tag, ParentNode, CategoryDescription)));
	}

	return InsertionIdx;
}


int32 UGameplayTagsManager::GetBestTagCategoryDescription(FString Tag, FText& OutDescription)
{
	// get all the nodes that make up this tag
	TArray< TSharedPtr<FGameplayTagNode> > TagItems;
	GetAllNodesForTag(Tag, TagItems);
	
	// find the deepest tag in the list (last is deepest based on how this array was constructed)
	TSharedPtr<FGameplayTagNode> BestDescriptionNode;
	for(int32 TagIdx = TagItems.Num() - 1; TagIdx >= 0; TagIdx--)
	{
		OutDescription = TagItems[TagIdx]->GetCategoryDescription();
		if(!OutDescription.IsEmpty())
		{
			return TagIdx;
		}
	}

	return -1;
}

#if WITH_EDITOR
void UGameplayTagsManager::GetFilteredGameplayRootTags( const FString& InFilterString, TArray< TSharedPtr<FGameplayTagNode> >& OutTagArray )
{
	TArray<FString> Filters;

	OutTagArray.Empty();
	if( InFilterString.ParseIntoArray( &Filters, TEXT( "," ), true ) > 0 )
	{
		for( int32 iTag = 0; iTag < GameplayRootTags.Num(); ++iTag )
		{
			FString RootTagName = GameplayRootTags[iTag].Get()->GetSimpleTag().ToString();

			// Check if this Tag is in the filter list
			for( int32 iFilter = 0; iFilter < Filters.Num(); ++iFilter )
			{
				if( RootTagName.Equals( Filters[iFilter] ) == true )
				{
					OutTagArray.Add( GameplayRootTags[iTag] );
				}
			}
		}
	}
	else
	{
		// No Filters just return them all
		OutTagArray = GameplayRootTags;
	}
}

void UGameplayTagsManager::OnObjectReimported(UObject* InObject)
{
	// Re-construct the gameplay tag tree if the base table is re-imported
	if (GIsEditor && !IsRunningCommandlet() && InObject && InObject == GameplayTagTable)
	{
		DestroyGameplayTagTree();
		ConstructGameplayTagTree();
	}
}

#endif // WITH_EDITOR

FGameplayTagTableRow::FGameplayTagTableRow(FGameplayTagTableRow const& Other)
{
	*this = Other;
}

FGameplayTagTableRow& FGameplayTagTableRow::operator=(FGameplayTagTableRow const& Other)
{
	// Guard against self-assignment
	if (this == &Other)
	{
		return *this;
	}

	Tag = Other.Tag;

	return *this;
}

bool FGameplayTagTableRow::operator==(FGameplayTagTableRow const& Other) const
{
	return (Tag == Other.Tag);
}

bool FGameplayTagTableRow::operator!=(FGameplayTagTableRow const& Other) const
{
	return (Tag != Other.Tag);
}

FGameplayTagNode::FGameplayTagNode(FName InTag, TWeakPtr<FGameplayTagNode> InParentNode, FText InCategoryDescription)
	: Tag(InTag)
	, CompleteTag(NAME_None)
	, CategoryDescription(InCategoryDescription)
	, ParentNode(InParentNode)
{
	TArray<FName> Tags;

	Tags.Add(InTag);

	TWeakPtr<FGameplayTagNode> CurNode = InParentNode;
	while (CurNode.IsValid())
	{
		Tags.Add(CurNode.Pin()->GetSimpleTag());
		CurNode = CurNode.Pin()->GetParentTagNode();
	}

	FString CompleteTagString;
	for (int32 TagIdx = Tags.Num() - 1; TagIdx >= 0; --TagIdx)
	{
		CompleteTagString += Tags[TagIdx].ToString();
		if (TagIdx > 0)
		{
			CompleteTagString += TEXT(".");
		}
	}

	CompleteTag = FName(*CompleteTagString);
}

FName FGameplayTagNode::GetCompleteTag() const
{
	return CompleteTag;
}

FName FGameplayTagNode::GetSimpleTag() const
{
	return Tag;
}

FText FGameplayTagNode::GetCategoryDescription() const
{
	return CategoryDescription;
}

TArray< TSharedPtr<FGameplayTagNode> >& FGameplayTagNode::GetChildTagNodes()
{
	return ChildTags;
}

const TArray< TSharedPtr<FGameplayTagNode> >& FGameplayTagNode::GetChildTagNodes() const
{
	return ChildTags;
}

TWeakPtr<FGameplayTagNode> FGameplayTagNode::GetParentTagNode() const
{
	return ParentNode;
}

void FGameplayTagNode::ResetNode()
{
	Tag = NAME_None;
	CompleteTag = NAME_None;

	for (int32 ChildIdx = 0; ChildIdx < ChildTags.Num(); ++ChildIdx)
	{
		ChildTags[ChildIdx]->ResetNode();
	}

	ChildTags.Empty();
	ParentNode.Reset();
}
