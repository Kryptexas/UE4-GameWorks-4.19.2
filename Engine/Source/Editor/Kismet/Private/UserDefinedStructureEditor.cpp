// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "UserDefinedStructureEditor.h"

#include "PropertyCustomizationHelpers.h"
#include "Editor/KismetWidgets//Public/SPinTypeSelector.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "StructureEditor"


///////////////////////////////////////////////////////////////////////////////////////
// FUserDefinedStructureEditor

const FName FUserDefinedStructureEditor::MemberVariablesTabId( TEXT( "UserDefinedStruct_MemberVariablesEditor" ) );
const FName FUserDefinedStructureEditor::UserDefinedStructureEditorAppIdentifier( TEXT( "UserDefinedStructEditorApp" ) );

void FUserDefinedStructureEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner( MemberVariablesTabId, FOnSpawnTab::CreateSP(this, &FUserDefinedStructureEditor::SpawnStructureTab) )
		.SetDisplayName( LOCTEXT("MemberVariablesEditor", "Member Variables") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
}

void FUserDefinedStructureEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);

	TabManager->UnregisterTabSpawner( MemberVariablesTabId );
}

void FUserDefinedStructureEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UUserDefinedStruct* Struct)
{
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout( "Standalone_UserDefinedStructureEditor_Layout_v1" )
	->AddArea
	(
		FTabManager::NewPrimaryArea() ->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.1f)
			->SetHideTabWell( true )
			->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
		)
		->Split
		(
			FTabManager::NewSplitter()
			->Split
			(
				FTabManager::NewStack()
				->AddTab( MemberVariablesTabId, ETabState::OpenedTab )
			)
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor( Mode, InitToolkitHost, UserDefinedStructureEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, Struct );
}

FUserDefinedStructureEditor::~FUserDefinedStructureEditor()
{
}

FName FUserDefinedStructureEditor::GetToolkitFName() const
{
	return FName("UserDefinedStructureEditor");
}

FText FUserDefinedStructureEditor::GetBaseToolkitName() const
{
	return LOCTEXT( "AppLabel", "Struct Editor" );
}

FText FUserDefinedStructureEditor::GetToolkitName() const
{
	if (1 == GetEditingObjects().Num())
	{
		return FAssetEditorToolkit::GetToolkitName();
	}
	return GetBaseToolkitName();
}

FString FUserDefinedStructureEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("UDStructWorldCentricTabPrefix", "Struct ").ToString();
}

FLinearColor FUserDefinedStructureEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.0f, 0.0f, 1.0f, 0.5f );
}

TSharedRef<SDockTab> FUserDefinedStructureEditor::SpawnStructureTab(const FSpawnTabArgs& Args)
{
	check( Args.GetTabId() == MemberVariablesTabId );

	UUserDefinedStruct* EditedStruct = NULL;
	const auto EditingObjects = GetEditingObjects();
	if (EditingObjects.Num())
	{
		EditedStruct = Cast<UUserDefinedStruct>(EditingObjects[ 0 ]);
	}

	// Create a property view
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs( /*bUpdateFromSelection=*/ false, /*bLockable=*/ false, /*bAllowSearch=*/ false, /*bObjectsUseNameArea=*/ true, /*bHideSelectionTip=*/ true );
	DetailsViewArgs.bHideActorNameArea = true;
	DetailsViewArgs.bShowOptions = false;

	PropertyView = EditModule.CreateDetailView( DetailsViewArgs );

	FOnGetDetailCustomizationInstance LayoutStructDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FUserDefinedStructureDetails::MakeInstance);
	PropertyView->RegisterInstancedCustomPropertyLayout(UUserDefinedStruct::StaticClass(), LayoutStructDetails);

	PropertyView->SetObject(EditedStruct);

	return SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("GenericEditor.Tabs.Properties") )
		.Label( LOCTEXT("UserDefinedStructureEditor", "Structure") )
		.TabColorScale( GetTabColorScale() )
		[
			PropertyView.ToSharedRef()
		];
}

///////////////////////////////////////////////////////////////////////////////////////
// FUserDefinedStructureLayout

//Represents single structure (List of fields)
class FUserDefinedStructureLayout : public IDetailCustomNodeBuilder, public TSharedFromThis<FUserDefinedStructureLayout>
{
public:
	FUserDefinedStructureLayout(TWeakPtr<class FUserDefinedStructureDetails> InStructureDetails) : StructureDetails(InStructureDetails) {}

	void OnChanged()
	{
		OnRegenerateChildren.ExecuteIfBound();
	}

	FReply OnAddNewField()
	{
		auto StructureDetailsSP = StructureDetails.Pin();
		if(StructureDetailsSP.IsValid())
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			const  FEdGraphPinType InitialType(K2Schema->PC_Boolean, TEXT(""), NULL, false, false);
			FStructureEditorUtils::AddVariable(StructureDetailsSP->GetUserDefinedStruct(), InitialType);
			OnChanged();
		}

		return FReply::Handled();
	}

	const FSlateBrush* OnGetStructureStatus() const
	{
		auto StructureDetailsSP = StructureDetails.Pin();
		if(StructureDetailsSP.IsValid())
		{
			if(auto Struct = StructureDetailsSP->GetUserDefinedStruct())
			{
				switch(Struct->Status.GetValue())
				{
				case EUserDefinedStructureStatus::UDSS_Error:
					return FEditorStyle::GetBrush("Kismet.Status.Error.Small");
				case EUserDefinedStructureStatus::UDSS_UpToDate:
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

	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) OVERRIDE {}
	virtual void Tick( float DeltaTime ) OVERRIDE {}
	virtual bool RequiresTick() const OVERRIDE { return false; }
	virtual FName GetName() const OVERRIDE 
	{ 
		auto StructureDetailsSP = StructureDetails.Pin();
		if(StructureDetailsSP.IsValid())
		{
			if(auto Struct = StructureDetailsSP->GetUserDefinedStruct())
			{
				return Struct->GetFName();
			}
		}
		return NAME_None; 
	}
	virtual bool InitiallyCollapsed() const OVERRIDE { return false; }

private:
	TWeakPtr<class FUserDefinedStructureDetails> StructureDetails;

	FSimpleDelegate OnRegenerateChildren;
};

///////////////////////////////////////////////////////////////////////////////////////
// FUserDefinedStructureFieldLayout

//Represents single field
class FUserDefinedStructureFieldLayout : public IDetailCustomNodeBuilder, public TSharedFromThis<FUserDefinedStructureFieldLayout>
{
public:
	FUserDefinedStructureFieldLayout(TWeakPtr<class FUserDefinedStructureDetails> InStructureDetails, TWeakPtr<class FUserDefinedStructureLayout> InStructureLayout, FGuid InFieldGuid)
		: FieldGuid(InFieldGuid)
		, StructureLayout(InStructureLayout)
		, StructureDetails(InStructureDetails) {}

	void OnChanged()
	{
		OnRegenerateChildren.ExecuteIfBound();
	}

	FText OnGetNameText() const
	{
		auto StructureDetailsSP = StructureDetails.Pin();
		if(StructureDetailsSP.IsValid())
		{
			return FText::FromString(FStructureEditorUtils::GetVariableDisplayName(StructureDetailsSP->GetUserDefinedStruct(), FieldGuid));
		}
		return FText::GetEmpty();
	}

	void OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
	{
		auto StructureDetailsSP = StructureDetails.Pin();
		if(StructureDetailsSP.IsValid())
		{
			const FString NewNameStr = NewText.ToString();
			FStructureEditorUtils::RenameVariable(StructureDetailsSP->GetUserDefinedStruct(), FieldGuid, NewNameStr);
			OnChanged();
		}
	}

	FEdGraphPinType OnGetPinInfo() const
	{
		auto StructureDetailsSP = StructureDetails.Pin();
		if(StructureDetailsSP.IsValid())
		{
			if(const FStructVariableDescription* FieldDesc = StructureDetailsSP->FindStructureFieldByGuid(FieldGuid))
			{
				return FieldDesc->ToPinType();
			}
		}
		return FEdGraphPinType();
	}

	void PinInfoChanged(const FEdGraphPinType& PinType)
	{
		auto StructureDetailsSP = StructureDetails.Pin();
		if(StructureDetailsSP.IsValid())
		{
			FStructureEditorUtils::ChangeVariableType(StructureDetailsSP->GetUserDefinedStruct(), FieldGuid, PinType);
			OnChanged();
		}
	}

	void OnPrePinInfoChange(const FEdGraphPinType& PinType)
	{

	}

	void OnRemovField()
	{
		auto StructureDetailsSP = StructureDetails.Pin();
		if(StructureDetailsSP.IsValid())
		{
			FStructureEditorUtils::RemoveVariable(StructureDetailsSP->GetUserDefinedStruct(), FieldGuid);
			auto StructureLayoutSP = StructureLayout.Pin();
			if(StructureLayoutSP.IsValid())
			{
				StructureLayoutSP->OnChanged();
			}
		}
	}

	bool IsRemoveButtonEnabled()
	{
		auto StructureDetailsSP = StructureDetails.Pin();
		if (StructureDetailsSP.IsValid())
		{
			if (auto UDStruct = StructureDetailsSP->GetUserDefinedStruct())
			{
				return (FStructureEditorUtils::GetVarDesc(UDStruct).Num() > 1);
			}
		}
		return false;
	}

	FText OnGetArgDefaultValueText() const
	{
		auto StructureDetailsSP = StructureDetails.Pin();
		if(StructureDetailsSP.IsValid())
		{
			if(const FStructVariableDescription* FieldDesc = StructureDetailsSP->FindStructureFieldByGuid(FieldGuid))
			{
				return FText::FromString(FieldDesc->DefaultValue);
			}
		}
		return FText();
	}

	void OnArgDefaultValueCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
	{
		auto StructureDetailsSP = StructureDetails.Pin();
		if(StructureDetailsSP.IsValid())
		{
			FStructureEditorUtils::ChangeVariableDefaultValue(StructureDetailsSP->GetUserDefinedStruct(), FieldGuid, NewText.ToString());
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
		auto StructureDetailsSP = StructureDetails.Pin();
		if(StructureDetailsSP.IsValid())
		{
			auto FieldDesc = StructureDetailsSP->FindStructureFieldByGuid(FieldGuid);
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
			auto Child = PinTypeNode->Children[ChildIndex];
			if(Child.IsValid())
			{
				auto ScriptStruct = Cast<const UScriptStruct>(Child->GetPinType(false).PinSubCategoryObject.Get());
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
		auto StructureDetailsSP = StructureDetails.Pin();
		if(StructureDetailsSP.IsValid() && K2Schema)
		{
			K2Schema->GetVariableTypeTree(TypeTree, bAllowExec, bAllowWildcard);
			const auto Parent = StructureDetailsSP->GetUserDefinedStruct();
			// THE TREE HAS ONLY 2 LEVELS
			for (auto PinTypePtr : TypeTree)
			{
				if (PinTypePtr.IsValid() && (K2Schema->PC_Struct == PinTypePtr->GetPinType(false).PinCategory))
				{
					RemoveInvalidSubTypes(PinTypePtr, Parent);
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
				PropertyCustomizationHelpers::MakeClearButton(
					FSimpleDelegate::CreateSP(this, &FUserDefinedStructureFieldLayout::OnRemovField),
					LOCTEXT("RemoveVariable", "Remove member variable"),
					TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FUserDefinedStructureFieldLayout::IsRemoveButtonEnabled)))
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
	virtual FName GetName() const OVERRIDE { return FName(*FieldGuid.ToString()); }
	virtual bool InitiallyCollapsed() const OVERRIDE { return true; }

private:
	TWeakPtr<class FUserDefinedStructureDetails> StructureDetails;

	TWeakPtr<class FUserDefinedStructureLayout> StructureLayout;

	FGuid FieldGuid;

	FSimpleDelegate OnRegenerateChildren;
};

///////////////////////////////////////////////////////////////////////////////////////
// FUserDefinedStructureLayout
void FUserDefinedStructureLayout::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) 
{
	ChildrenBuilder.AddChildContent(FString())
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
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.Text(LOCTEXT("NewStructureField", "New Variable").ToString())
					.OnClicked(this, &FUserDefinedStructureLayout::OnAddNewField)
				]
		];

	auto StructureDetailsSP = StructureDetails.Pin();
	if(StructureDetailsSP.IsValid())
	{
		if(auto Struct = StructureDetailsSP->GetUserDefinedStruct())
		{
			for (auto& VarDesc : FStructureEditorUtils::GetVarDesc(Struct))
			{
				TSharedRef<class FUserDefinedStructureFieldLayout> VarLayout = MakeShareable(new FUserDefinedStructureFieldLayout(StructureDetails,  SharedThis(this), VarDesc.VarGuid));
				ChildrenBuilder.AddChildCustomBuilder(VarLayout);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// FUserDefinedStructureDetails

UUserDefinedStruct* FUserDefinedStructureDetails::GetUserDefinedStruct()
{
	return UserDefinedStruct.Get();
}

FStructVariableDescription* FUserDefinedStructureDetails::FindStructureFieldByGuid(FGuid Guid)
{
	if (auto Struct = GetUserDefinedStruct())
	{
		return FStructureEditorUtils::GetVarDesc(Struct).FindByPredicate(FStructureEditorUtils::FFindByGuidHelper<FStructVariableDescription>(Guid));
	}
	return NULL;
}

void FUserDefinedStructureDetails::CustomizeDetails( class IDetailLayoutBuilder& DetailLayout )
{
	const TArray<TWeakObjectPtr<UObject>> Objects = DetailLayout.GetDetailsView().GetSelectedObjects();
	check(Objects.Num() > 0);

	if (Objects.Num() == 1)
	{
		UserDefinedStruct = CastChecked<UUserDefinedStruct>(Objects[0].Get());
		IDetailCategoryBuilder& StructureCategory = DetailLayout.EditCategory("Structure", LOCTEXT("Structure", "Structure").ToString());

		Layout = MakeShareable(new FUserDefinedStructureLayout(SharedThis(this)));
		StructureCategory.AddCustomBuilder(Layout.ToSharedRef());
	}

	FStructureEditorUtils::FStructEditorManager::Get().AddListener(this);
}

void FUserDefinedStructureDetails::OnChanged(const class UUserDefinedStruct* Struct)
{
	if (Struct && (GetUserDefinedStruct() == Struct))
	{
		if (Layout.IsValid())
		{
			Layout->OnChanged();
		}
	}
}

FUserDefinedStructureDetails::~FUserDefinedStructureDetails()
{
	FStructureEditorUtils::FStructEditorManager::Get().RemoveListener(this);
}

#undef LOCTEXT_NAMESPACE