// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace InputConstants
{
	const FMargin PropertyPadding(2.0f, 0.0f, 2.0f, 0.0f);
	const float TextBoxWidth = 200.0f;
	const float ScaleBoxWidth = 50.0f;
}

struct FMappingSet
{
	FName SharedName;
	TArray<TSharedRef<IPropertyHandle>> Mappings;
};

class FActionMappingsNodeBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FActionMappingsNodeBuilder>
{
public:
	FActionMappingsNodeBuilder( IDetailLayoutBuilder* InDetailLayoutBuilder, const TSharedPtr<IPropertyHandle>& InPropertyHandle );

	/** IDetailCustomNodeBuilder interface */
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRebuildChildren  ) OVERRIDE { OnRebuildChildren = InOnRebuildChildren; } 
	virtual bool RequiresTick() const OVERRIDE { return true; }
	virtual void Tick( float DeltaTime ) OVERRIDE;
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) OVERRIDE;
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) OVERRIDE;
	virtual bool InitiallyCollapsed() const OVERRIDE { return true; };
	virtual FName GetName() const OVERRIDE { return NAME_None; }

private:
	void AddActionMappingButton_OnClick();
	void ClearActionMappingButton_OnClick();
	void OnActionMappingNameCommitted(const FText& InName, ETextCommit::Type CommitInfo, const FMappingSet MappingSet);
	void AddActionMappingToGroupButton_OnClick(const FMappingSet MappingSet);
	void RemoveActionMappingGroupButton_OnClick(const FMappingSet MappingSet);

	bool GroupsRequireRebuild() const;
	void RebuildGroupedMappings();
	void RebuildChildren()
	{
		OnRebuildChildren.ExecuteIfBound();
	}

private:
	/** Called to rebuild the children of the detail tree */
	FSimpleDelegate OnRebuildChildren;

	/** Associated detail layout builder */
	IDetailLayoutBuilder* DetailLayoutBuilder;

	/** Property handle to associated action mappings */
	TSharedPtr<IPropertyHandle> ActionMappingsPropertyHandle;

	TArray<FMappingSet> GroupedMappings;
};

class FAxisMappingsNodeBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FAxisMappingsNodeBuilder>
{
public:
	FAxisMappingsNodeBuilder( IDetailLayoutBuilder* InDetailLayoutBuilder, const TSharedPtr<IPropertyHandle>& InPropertyHandle );

	/** IDetailCustomNodeBuilder interface */
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRebuildChildren  ) OVERRIDE { OnRebuildChildren = InOnRebuildChildren; } 
	virtual bool RequiresTick() const OVERRIDE { return true; }
	virtual void Tick( float DeltaTime ) OVERRIDE;
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) OVERRIDE;
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) OVERRIDE;
	virtual bool InitiallyCollapsed() const OVERRIDE { return true; };
	virtual FName GetName() const OVERRIDE { return NAME_None; }

private:
	void AddAxisMappingButton_OnClick();
	void ClearAxisMappingButton_OnClick();
	void OnAxisMappingNameCommitted(const FText& InName, ETextCommit::Type CommitInfo, const FMappingSet MappingSet);
	void AddAxisMappingToGroupButton_OnClick(const FMappingSet MappingSet);
	void RemoveAxisMappingGroupButton_OnClick(const FMappingSet MappingSet);

	bool GroupsRequireRebuild() const;
	void RebuildGroupedMappings();
	void RebuildChildren()
	{
		OnRebuildChildren.ExecuteIfBound();
	}

private:
	/** Called to rebuild the children of the detail tree */
	FSimpleDelegate OnRebuildChildren;

	/** Associated detail layout builder */
	IDetailLayoutBuilder* DetailLayoutBuilder;

	/** Property handle to associated axis mappings */
	TSharedPtr<IPropertyHandle> AxisMappingsPropertyHandle;

	TArray<FMappingSet> GroupedMappings;
};

class FInputSettingsDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** ILayoutDetails interface */
	virtual void CustomizeDetails( class IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
};