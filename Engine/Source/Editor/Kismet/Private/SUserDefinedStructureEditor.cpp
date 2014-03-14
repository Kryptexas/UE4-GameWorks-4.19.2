// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "SUserDefinedStructureEditor.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/UnrealEd/Public/Kismet2/StructureEditorUtils.h"
#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "Editor/KismetWidgets//Public/SPinTypeSelector.h"

#define LOCTEXT_NAMESPACE "StructureEditor"

//Creates panel with StructuresList
class FUserDefinedStructuresDetails : public IDetailCustomization
{
public:
	FUserDefinedStructuresDetails(TWeakPtr<FBlueprintEditor> InBlueprintEditor) 
		: BlueprintEditor(InBlueprintEditor) {}

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance(TWeakPtr<FBlueprintEditor> InBlueprintEditor)
	{
		return MakeShareable(new FUserDefinedStructuresDetails(InBlueprintEditor));
	}

	static bool IsPropertyVisible( const UProperty* InProperty )
	{
		return false;
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( class IDetailLayoutBuilder& DetailLayout ) OVERRIDE;

public:
	UBlueprint* GetBlueprint() const
	{
		auto BlueprintEditorPtr = BlueprintEditor.Pin();
		return BlueprintEditorPtr.IsValid() ? BlueprintEditorPtr->GetBlueprintObj() : NULL;
	}

	FBPStructureDescription* FindStructureDescByName(FName Name) const
	{
		if(UBlueprint* Blueprint = GetBlueprint())
		{
			FBPStructureDescription* StructureDesc = Blueprint->UserDefinedStructures.FindByPredicate(FStructureEditorUtils::FFindByNameHelper(Name));
			return StructureDesc;
		}
		return NULL;
	}

	FBPVariableDescription* FindStructureFieldByGuid(FName StructureName, FGuid FieldGuid) const
	{
		if(FBPStructureDescription* StructureDesc = FindStructureDescByName(StructureName))
		{
			FBPVariableDescription* FieldDesc = StructureDesc->Fields.FindByPredicate(FStructureEditorUtils::FFindByGuidHelper(FieldGuid));
			return FieldDesc;
		}
		return NULL;
	}

private:
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
};

//Creates list of all structures
class FUserDefinedStructuresList : public IDetailCustomNodeBuilder, public TSharedFromThis<FUserDefinedStructuresList>
{
public:
	FUserDefinedStructuresList(TWeakPtr<class FUserDefinedStructuresDetails> InStructuresDetails) 
		: StructuresDetails(InStructuresDetails) {}

	void OnChanged()
	{
		OnRegenerateChildren.ExecuteIfBound();
	}

	FReply OnAddNewStructure()
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			FStructureEditorUtils::AddStructure(StructuresDetailsSP->GetBlueprint());
			OnChanged();
		}
		return FReply::Handled();
	}

	/** IDetailCustomNodeBuilder Interface*/
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRegenerateChildren ) OVERRIDE 
	{
		OnRegenerateChildren = InOnRegenerateChildren;
	}
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) OVERRIDE;
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) OVERRIDE {}
	virtual void Tick( float DeltaTime ) OVERRIDE {}
	virtual bool RequiresTick() const OVERRIDE { return false; }
	virtual FName GetName() const OVERRIDE { return NAME_None; }
	virtual bool InitiallyCollapsed() const OVERRIDE { return false; }

private:
	FSimpleDelegate OnRegenerateChildren;
	TWeakPtr<class FUserDefinedStructuresDetails> StructuresDetails;
};

//Represents single structure (List of fields)
class FUserDefinedStructureLayout : public IDetailCustomNodeBuilder, public TSharedFromThis<FUserDefinedStructureLayout>
{
public:
	FUserDefinedStructureLayout(TWeakPtr<class FUserDefinedStructuresDetails> InStructuresDetails, TWeakPtr<class FUserDefinedStructuresList> InStructuresList,FName InStructureName)
		: StructureName(InStructureName)
		, StructuresList(InStructuresList)
		, StructuresDetails(InStructuresDetails) {}

	FName GetStructureName() const
	{
		return StructureName;
	}

	void OnChanged()
	{
		OnRegenerateChildren.ExecuteIfBound();
	}

	FReply OnAddNewField()
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			const  FEdGraphPinType InitialType(K2Schema->PC_Boolean, TEXT(""), NULL, false, false);
			FStructureEditorUtils::AddVariable(StructuresDetailsSP->GetBlueprint(), StructureName, InitialType);
			OnChanged();
		}

		return FReply::Handled();
	}

	FReply OnDuplicateStructure()
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			FStructureEditorUtils::DuplicateStructure(StructuresDetailsSP->GetBlueprint(), StructureName);

			auto StructuresListSP = StructuresList.Pin();
			if(StructuresListSP.IsValid())
			{
				StructuresListSP->OnChanged();
			}
		}

		return FReply::Handled();
	}

	void OnRemoveStruct()
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			FStructureEditorUtils::RemoveStructure(StructuresDetailsSP->GetBlueprint(), StructureName);
			auto StructuresListSP = StructuresList.Pin();
			if(StructuresListSP.IsValid())
			{
				StructuresListSP->OnChanged();
			}
		}
	}

	FText OnGetNameText() const
	{
		return FText::FromName(StructureName);
	}

	void OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			const FString NewNameStr = NewText.ToString();
			if(FStructureEditorUtils::RenameStructure(StructuresDetailsSP->GetBlueprint(), StructureName, NewNameStr))
			{
				StructureName = *NewNameStr;
				OnChanged();
			}
		}
	}

	const FSlateBrush* OnGetStructureStatus() const
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			if(const FBPStructureDescription* StructDesc = StructuresDetailsSP->FindStructureDescByName(GetStructureName()))
			{
				const EBlueprintStructureStatus Status = StructDesc->CompiledStruct ? (EBlueprintStructureStatus)StructDesc->CompiledStruct->Status : BSS_Dirty;
				switch(Status)
				{
				case EBlueprintStructureStatus::BSS_Error:
					return FEditorStyle::GetBrush("Kismet.Status.Error.Small");
				case EBlueprintStructureStatus::BSS_UpToDate:
					return FEditorStyle::GetBrush("Kismet.Status.Good.Small");
				default:
					return FEditorStyle::GetBrush("Kismet.Status.Unknown.Small");
				}
				
			}
		}
		return NULL;
	}

	/** IDetailCustomNodeBuilder Interface*/
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRegenerateChildren ) OVERRIDE 
	{
		OnRegenerateChildren = InOnRegenerateChildren;
	}
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) OVERRIDE;

	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) OVERRIDE 
	{
		NodeRow
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image( this, &FUserDefinedStructureLayout::OnGetStructureStatus )
			]
			+SHorizontalBox::Slot()
			.FillWidth(1)
			.VAlign(VAlign_Center)
			[
				SNew(SEditableTextBox)
				.Text( this, &FUserDefinedStructureLayout::OnGetNameText )
				.OnTextCommitted( this, &FUserDefinedStructureLayout::OnNameTextCommitted )
				.Font( IDetailLayoutBuilder::GetDetailFont() )
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(this, &FUserDefinedStructureLayout::OnRemoveStruct), LOCTEXT("RemoveStructure", "RemoveStructure"))
			]
		];
	}
	virtual void Tick( float DeltaTime ) OVERRIDE {}
	virtual bool RequiresTick() const OVERRIDE { return false; }
	virtual FName GetName() const OVERRIDE { return StructureName; }
	virtual bool InitiallyCollapsed() const OVERRIDE { return true; }

private:
	TWeakPtr<class FUserDefinedStructuresDetails> StructuresDetails;
	TWeakPtr<class FUserDefinedStructuresList> StructuresList;

	FName StructureName;

	FSimpleDelegate OnRegenerateChildren;
};

//Represents single field
class FUserDefinedStructureFieldLayout : public IDetailCustomNodeBuilder, public TSharedFromThis<FUserDefinedStructureFieldLayout>
{
public:
	FUserDefinedStructureFieldLayout(TWeakPtr<class FUserDefinedStructuresDetails> InStructuresDetails, TWeakPtr<class FUserDefinedStructureLayout> InStructureLayout, FGuid InFieldGuid)
		: FieldGuid(InFieldGuid)
		, StructureLayout(InStructureLayout)
		, StructuresDetails(InStructuresDetails) {}

	FName GetStructureName() const
	{
		auto StructureLayoutSP = StructureLayout.Pin();
		if(StructureLayoutSP.IsValid())
		{
			return StructureLayoutSP->GetStructureName();
		}
		return NAME_None;
	}

	void OnChanged()
	{
		OnRegenerateChildren.ExecuteIfBound();
	}

	FText OnGetNameText() const
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			return FText::FromString(FStructureEditorUtils::GetDisplayName(StructuresDetailsSP->GetBlueprint(), GetStructureName(), FieldGuid));
		}
		return FText::GetEmpty();
	}

	void OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			const FString NewNameStr = NewText.ToString();
			FStructureEditorUtils::RenameVariable(StructuresDetailsSP->GetBlueprint(), GetStructureName(), FieldGuid, NewNameStr);
			OnChanged();
		}
	}

	FEdGraphPinType OnGetPinInfo() const
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			if(const FBPVariableDescription* FieldDesc = StructuresDetailsSP->FindStructureFieldByGuid(GetStructureName(), FieldGuid))
			{
				return FieldDesc->VarType;
			}
		}
		return FEdGraphPinType();
	}

	void PinInfoChanged(const FEdGraphPinType& PinType)
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			FStructureEditorUtils::ChangeVariableType(StructuresDetailsSP->GetBlueprint(), GetStructureName(), FieldGuid, PinType);
			OnChanged();
		}
	}

	void OnPrePinInfoChange(const FEdGraphPinType& PinType)
	{

	}

	void OnRemovField()
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			FStructureEditorUtils::RemoveVariable(StructuresDetailsSP->GetBlueprint(), GetStructureName(), FieldGuid);
			auto StructureLayoutSP = StructureLayout.Pin();
			if(StructureLayoutSP.IsValid())
			{
				StructureLayoutSP->OnChanged();
			}
		}
	}

	FText OnGetArgDefaultValueText() const
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			if(const FBPVariableDescription* FieldDesc = StructuresDetailsSP->FindStructureFieldByGuid(GetStructureName(), FieldGuid))
			{
				return FText::FromString(FieldDesc->DefaultValue);
			}
		}
		return FText();
	}

	void OnArgDefaultValueCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			FStructureEditorUtils::ChangeVariableDefaultValue(StructuresDetailsSP->GetBlueprint(), GetStructureName(), FieldGuid, NewText.ToString());
			OnChanged();
		}
	}

	/** IDetailCustomNodeBuilder Interface*/
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRegenerateChildren ) OVERRIDE 
	{
		OnRegenerateChildren = InOnRegenerateChildren;
	}

	EVisibility GetErrorIconVisibility()
	{
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid())
		{
			auto FieldDesc = StructuresDetailsSP->FindStructureFieldByGuid(GetStructureName(), FieldGuid);
			if (FieldDesc && FieldDesc->bInvalidMember)
			{
				return EVisibility::Visible;
			}
		}

		return EVisibility::Collapsed;
	}

	void RemoveInvalidSubTypes(TSharedPtr<UEdGraphSchema_K2::FPinTypeTreeInfo> PinTypeNode, const UStruct* Parent) const
	{
		for (int32 ChildIndex = 0; ChildIndex < PinTypeNode->Children.Num();)
		{
			TSharedPtr<UEdGraphSchema_K2::FPinTypeTreeInfo> Child = PinTypeNode->Children[ChildIndex];
			if(Child.IsValid())
			{
				const UScriptStruct* ScriptStruct = Cast<const UScriptStruct>(Child->PinType.PinSubCategoryObject.Get());
				if (ScriptStruct)
				{
					if (FStructureEditorUtils::EStructureError::Ok != FStructureEditorUtils::IsStructureValid(ScriptStruct, Parent))
					{
						PinTypeNode->Children.RemoveAt(ChildIndex);
						continue;
					}
				}
			}
			++ChildIndex;
		}
	}

	void GetFilteredVariableTypeTree( TArray< TSharedPtr<UEdGraphSchema_K2::FPinTypeTreeInfo> >& TypeTree, bool bAllowExec, bool bAllowWildcard ) const
	{
		auto K2Schema = GetDefault<UEdGraphSchema_K2>();
		auto StructuresDetailsSP = StructuresDetails.Pin();
		if(StructuresDetailsSP.IsValid() && K2Schema)
		{
			auto FieldDesc = StructuresDetailsSP->FindStructureDescByName(GetStructureName());
			if (FieldDesc)
			{
				K2Schema->GetVariableTypeTree(TypeTree, bAllowExec, bAllowWildcard);

				// THE TREE HAS ONLY 2 LEVELS
				for (auto TypeTreeIter = TypeTree.CreateIterator(); TypeTreeIter; ++TypeTreeIter)
				{
					TSharedPtr<UEdGraphSchema_K2::FPinTypeTreeInfo> PinTypePtr = *TypeTreeIter;
					if (PinTypePtr.IsValid() && (K2Schema->PC_Struct == PinTypePtr->PinType.PinCategory))
					{
						RemoveInvalidSubTypes(PinTypePtr, FieldDesc->CompiledStruct);
					}
				}
			}
		}
	}

	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) OVERRIDE 
	{
		auto K2Schema = GetDefault<UEdGraphSchema_K2>();

		TSharedPtr<SImage> ErrorIcon;

		NodeRow
		.NameContent()
		[
			SNew(SHorizontalBox)

			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SAssignNew(ErrorIcon, SImage)
				.Image( FEditorStyle::GetBrush("Icons.Error") )
			]

			+SHorizontalBox::Slot()
			.FillWidth(1)
			.VAlign(VAlign_Center)
			[
				SNew(SEditableTextBox)
				.Text( this, &FUserDefinedStructureFieldLayout::OnGetNameText )
				.OnTextCommitted( this, &FUserDefinedStructureFieldLayout::OnNameTextCommitted )
				.Font( IDetailLayoutBuilder::GetDetailFont() )
			]
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1)
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SPinTypeSelector, FGetPinTypeTree::CreateSP(this, &FUserDefinedStructureFieldLayout::GetFilteredVariableTypeTree))
				.TargetPinType(this, &FUserDefinedStructureFieldLayout::OnGetPinInfo)
				.OnPinTypePreChanged(this, &FUserDefinedStructureFieldLayout::OnPrePinInfoChange)
				.OnPinTypeChanged(this, &FUserDefinedStructureFieldLayout::PinInfoChanged)
				.Schema(K2Schema)
				.bAllowExec(false)
				.bAllowWildcard(false)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(this, &FUserDefinedStructureFieldLayout::OnRemovField), LOCTEXT("RemoveVariable", "Remove member variable"))
			]
		];

		if (ErrorIcon.IsValid())
		{
			ErrorIcon->SetVisibility(
				TAttribute<EVisibility>::Create(
					TAttribute<EVisibility>::FGetter::CreateSP(
						this, &FUserDefinedStructureFieldLayout::GetErrorIconVisibility)));
		}
	}

	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) OVERRIDE 
	{
		ChildrenBuilder.AddChildContent( *LOCTEXT( "FunctionArgDetailsDefaultValue", "Default Value" ).ToString() )
		.NameContent()
		[
			SNew(STextBlock)
			.Text( LOCTEXT( "DefaultValue", "Default Value" ) )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
		]
		.ValueContent()
		[
			SNew(SEditableTextBox)
			.Text( this, &FUserDefinedStructureFieldLayout::OnGetArgDefaultValueText )
			.OnTextCommitted( this, &FUserDefinedStructureFieldLayout::OnArgDefaultValueCommitted )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
		];
	}

	virtual void Tick( float DeltaTime ) OVERRIDE {}
	virtual bool RequiresTick() const OVERRIDE { return false; }
	virtual FName GetName() const OVERRIDE { return NAME_None; } // TODO?
	virtual bool InitiallyCollapsed() const OVERRIDE { return true; }

private:
	TWeakPtr<class FUserDefinedStructuresDetails> StructuresDetails;

	TWeakPtr<class FUserDefinedStructureLayout> StructureLayout;

	FGuid FieldGuid;

	FSimpleDelegate OnRegenerateChildren;
};

void FUserDefinedStructureLayout::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) 
{
	ChildrenBuilder.AddChildContent(FString())
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		[
			SNew(SButton)
			.Text(LOCTEXT("DuplicateStructure", "Duplicate").ToString())
			.OnClicked(this, &FUserDefinedStructureLayout::OnDuplicateStructure)
		]
		+SHorizontalBox::Slot()
		.HAlign(HAlign_Right)
		[
			SNew(SButton)
			.Text(LOCTEXT("NewStructureField", "New Variable").ToString())
			.OnClicked(this, &FUserDefinedStructureLayout::OnAddNewField)
		]
	];

	auto StructuresDetailsSP = StructuresDetails.Pin();
	if(StructuresDetailsSP.IsValid())
	{
		FBPStructureDescription* StructDesc = StructuresDetailsSP->FindStructureDescByName(StructureName);
		if(StructDesc)
		{
			for (auto VarDescIter = StructDesc->Fields.CreateConstIterator(); VarDescIter; ++VarDescIter)
			{
				TSharedRef<class FUserDefinedStructureFieldLayout> VarLayout = MakeShareable(new FUserDefinedStructureFieldLayout(StructuresDetails,  SharedThis(this), (*VarDescIter).VarGuid));
				ChildrenBuilder.AddChildCustomBuilder(VarLayout);
			}
		}
	}
}

void FUserDefinedStructuresList::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder )
{
	auto StructuresDetailsSP = StructuresDetails.Pin();
	if(StructuresDetailsSP.IsValid())
	{
		const UBlueprint* Blueprint = StructuresDetailsSP->GetBlueprint();
		if(Blueprint)
		{
			ChildrenBuilder.AddChildContent(LOCTEXT( "NewStructureRow", "New Structure" ).ToString())
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.Text(LOCTEXT("NewStructure", "New Structure").ToString())
					.OnClicked(this, &FUserDefinedStructuresList::OnAddNewStructure)
				]
			];

			for (auto StructDescIter = Blueprint->UserDefinedStructures.CreateConstIterator(); StructDescIter; ++StructDescIter)
			{
				TSharedRef<class FUserDefinedStructureLayout> StructureLayout = MakeShareable(new FUserDefinedStructureLayout(StructuresDetails, SharedThis(this), (*StructDescIter).Name));
				ChildrenBuilder.AddChildCustomBuilder(StructureLayout);
			}
		}
	}
}

void FUserDefinedStructuresDetails::CustomizeDetails( class IDetailLayoutBuilder& DetailLayout )
{
	const UBlueprint* Blueprint = GetBlueprint();
	if(NULL != Blueprint)
	{
		IDetailCategoryBuilder& StructureCategory = DetailLayout.EditCategory("Structures", LOCTEXT("Structures", "Structures").ToString());
		auto StructuresList = MakeShareable(new FUserDefinedStructuresList(SharedThis(this)));
		StructureCategory.AddCustomBuilder(StructuresList);
	}
}

void SUserDefinedStructureEditor::Construct(const FArguments& InArgs, TWeakPtr<FBlueprintEditor> InBlueprintEditor)
{
	auto BlueprintEditor = InBlueprintEditor.Pin();
	if(BlueprintEditor.IsValid())
	{
		// Create a property view
		FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		FDetailsViewArgs DetailsViewArgs( /*bUpdateFromSelection=*/ false, /*bLockable=*/ false, /*bAllowSearch=*/ true, /*bObjectsUseNameArea=*/ true, /*bHideSelectionTip=*/ true, BlueprintEditor.Get() );
		auto DetailsViewSP = EditModule.CreateDetailView( DetailsViewArgs );
		DetailsViewSP->SetIsPropertyVisibleDelegate( FIsPropertyVisible::CreateStatic( &FUserDefinedStructuresDetails::IsPropertyVisible ) );
		DetailsViewSP->SetGenericLayoutDetailsDelegate( FOnGetDetailCustomizationInstance::CreateStatic( &FUserDefinedStructuresDetails::MakeInstance, InBlueprintEditor ) );

		this->ChildSlot
		[
			DetailsViewSP
		];

		DetailsViewSP->SetObject(NULL, true);
	}
}

#undef LOCTEXT_NAMESPACE