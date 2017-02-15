// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneObjectBindingPtrCustomization.h"
#include "IPropertyUtilities.h"
#include "MovieSceneBindingOwnerInterface.h"
#include "MovieSceneSequence.h"
#include "MovieScene.h"
#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"


#define LOCTEXT_NAMESPACE "MovieSceneObjectBindingPtrCustomization"


namespace
{
	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, FGuid);

	class SObjectBindingTree : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SObjectBindingTree){}
			SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, TWeakObjectPtr<UMovieSceneSequence> InSequence)
		{
			OnSelectionChangedEvent = InArgs._OnSelectionChanged;
			WeakSequence = InSequence;

			UMovieScene* MovieScene = WeakSequence->GetMovieScene();
			if (!MovieScene)
			{
				ChildSlot
				[
					SNew(STextBlock).Text(LOCTEXT("InvalidSequence", "Invalid sequence"))
				];
				return;
			}

			const int32 Count = MovieScene->GetPossessableCount();
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(Index);
				if (!WeakSequence.Get()->CanRebindPossessable(Possessable))
				{
					continue;
				}

				FGuid ParentGuid = Possessable.GetParent();
				Hierarchy.FindOrAdd(ParentGuid).Add(MakeShared<FGuid>(Possessable.GetGuid()));
			}

			RootGuids = Hierarchy.FindRef(FGuid());

			ChildSlot
			[
				SNew(STreeView<TSharedPtr<FGuid>>)
				.TreeItemsSource(&RootGuids)
				.OnGenerateRow(this, &SObjectBindingTree::OnGenerateRowWidget)
				.OnGetChildren(this, &SObjectBindingTree::OnGetChildrenForTree)
				.OnSelectionChanged(this, &SObjectBindingTree::OnTreeSelectionChanged)
			];
		}

		TSharedRef<ITableRow> OnGenerateRowWidget(TSharedPtr<FGuid> InGuid, const TSharedRef<STableViewBase>& OwnerTable)
		{
			UMovieSceneSequence* Sequence = WeakSequence.Get();
			UMovieScene* MovieScene = Sequence ? Sequence->GetMovieScene() : nullptr;
			FText DisplayString = MovieScene ? MovieScene->GetObjectDisplayName(*InGuid) : FText();

			return SNew(STableRow<TSharedPtr<FGuid>>, OwnerTable)
			[
				SNew(STextBlock).Text(DisplayString)
			];
		}

		void OnGetChildrenForTree(TSharedPtr<FGuid> ParentId, TArray<TSharedPtr<FGuid>>& OutChildren)
		{
			OutChildren = Hierarchy.FindRef(*ParentId);
		}

		void OnTreeSelectionChanged(TSharedPtr<FGuid> SelectedGuid, ESelectInfo::Type)
		{
			OnSelectionChangedEvent.ExecuteIfBound(*SelectedGuid);
			FSlateApplication::Get().DismissAllMenus();
		}

	private:
		
		FOnSelectionChanged OnSelectionChangedEvent;

		TArray<TSharedPtr<FGuid>> RootGuids;
		TMap<FGuid, TArray<TSharedPtr<FGuid>>> Hierarchy;
		TWeakObjectPtr<UMovieSceneSequence> WeakSequence;
	};
}

void FMovieSceneObjectBindingPtrCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructProperty = PropertyHandle;
	GuidProperty = StructProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMovieSceneObjectBindingPtr, Guid));

	check(GuidProperty.IsValid());

	HeaderRow
	.NameContent()
	[
		StructProperty->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(SComboButton)
		.OnGetMenuContent(this, &FMovieSceneObjectBindingPtrCustomization::GetBindingMenu)
		.ContentPadding(FMargin(4.0, 2.0))
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(this, &FMovieSceneObjectBindingPtrCustomization::ResolveBindingNameInSequence)
			.Font(CustomizationUtils.GetRegularFont())
		]
	];
}
IMovieSceneBindingOwnerInterface* FMovieSceneObjectBindingPtrCustomization::GetInterface() const
{
	TArray<UObject*> OuterObjects;
	StructProperty->GetOuterObjects(OuterObjects);

	if (OuterObjects.Num() != 1)
	{
		return nullptr;
	}

	for ( UObject* NextOuter = OuterObjects[0]; NextOuter; NextOuter = NextOuter->GetOuter() )
	{
		if (IMovieSceneBindingOwnerInterface* Result = Cast<IMovieSceneBindingOwnerInterface>(NextOuter))
		{
			return Result;
		}
	}
	return nullptr;
}

UMovieSceneSequence* FMovieSceneObjectBindingPtrCustomization::GetSequence() const
{
	IMovieSceneBindingOwnerInterface* OwnerInterface = GetInterface();
	return OwnerInterface ? OwnerInterface->RetrieveOwnedSequence() : nullptr;
}

FGuid FMovieSceneObjectBindingPtrCustomization::GetGuid() const
{
	TArray<void*> Guids;
	GuidProperty->AccessRawData(Guids);

	return ensure(Guids.Num() == 1) ? *static_cast<FGuid*>(Guids[0]) : FGuid();
}

void FMovieSceneObjectBindingPtrCustomization::SetBindingId(FGuid InGuid)
{
	TArray<void*> Guids;
	GuidProperty->AccessRawData(Guids);

	if (ensure(Guids.Num() == 1))
	{
		*static_cast<FGuid*>(Guids[0]) = InGuid;
	}
}

FText FMovieSceneObjectBindingPtrCustomization::ResolveBindingNameInSequence() const
{
	UMovieSceneSequence* Sequence = GetSequence();
	UMovieScene* MovieScene = Sequence ? Sequence->GetMovieScene() : nullptr;

	FText DisplayString = MovieScene ? MovieScene->GetObjectDisplayName(GetGuid()) : FText();
	return DisplayString.IsEmpty() ? LOCTEXT("UnresolvedBinding", "Unresolved Binding") : DisplayString;
}

TSharedRef<SWidget> FMovieSceneObjectBindingPtrCustomization::GetBindingMenu()
{
	return
		SNew(SObjectBindingTree, GetSequence())
		.OnSelectionChanged(this, &FMovieSceneObjectBindingPtrCustomization::SetBindingId);
}

#undef LOCTEXT_NAMESPACE
