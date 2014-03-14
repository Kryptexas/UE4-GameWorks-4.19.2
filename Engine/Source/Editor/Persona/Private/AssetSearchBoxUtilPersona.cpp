// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PersonaPrivatePCH.h"
#include "AssetSearchBoxUtilPersona.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "PropertyHandle.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "IDetailsView.h"
#include "SAssetSearchBox.h"

void SAssetSearchBoxForBones::Construct( const FArguments& InArgs, const class UObject* Outer, TSharedPtr<class IPropertyHandle> BoneNameProperty )
{
	check(Outer);

	// get the currently chosen bone/socket, if any
	const FString PropertyName = BoneNameProperty->GetPropertyDisplayName();
	FString CurValue;
	BoneNameProperty->GetValue(CurValue);
	if( CurValue == FString("None") )
	{
		CurValue.Empty();
	}

	const USkeleton* Skeleton = NULL;

	TArray<FString> PossibleSuggestions;
	if( const USkeletalMesh* SkeletalMesh = Cast<const USkeletalMesh>(Outer) )
	{
		if( InArgs._IncludeSocketsForSuggestions.Get() )
		{
			for( auto SocketIt=const_cast<USkeletalMesh*>(SkeletalMesh)->GetMeshOnlySocketList().CreateConstIterator(); SocketIt; ++SocketIt )
			{
				PossibleSuggestions.Add((*SocketIt)->SocketName.ToString());
			}
		}
		Skeleton = SkeletalMesh->Skeleton;
	}
	else
	{
		Skeleton = Cast<const USkeleton>(Outer);
	}
	if( Skeleton && InArgs._IncludeSocketsForSuggestions.Get() )
	{
		for( auto SocketIt=Skeleton->Sockets.CreateConstIterator(); SocketIt; ++SocketIt )
		{
			PossibleSuggestions.Add((*SocketIt)->SocketName.ToString());
		}
	}
	check(Skeleton);

	const TArray<FMeshBoneInfo>& Bones = Skeleton->GetReferenceSkeleton().GetRefBoneInfo();
	for( auto BoneIt=Bones.CreateConstIterator(); BoneIt; ++BoneIt )
	{
		PossibleSuggestions.Add(BoneIt->Name.ToString());
	}

	// create the asset search box
	ChildSlot
	[
		SNew(SAssetSearchBox)
		.InitialText(FText::FromString(CurValue))
		.HintText(InArgs._HintText)
		.OnTextCommitted(InArgs._OnTextCommitted)
		.PossibleSuggestions(PossibleSuggestions)
		.DelayChangeNotificationsWhileTyping( true )
		.MustMatchPossibleSuggestions(InArgs._MustMatchPossibleSuggestions)
	];
}
