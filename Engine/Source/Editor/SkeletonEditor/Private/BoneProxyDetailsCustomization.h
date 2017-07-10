// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IDetailCustomization.h"

class IPropertyHandle;
class UBoneProxy;
class UDebugSkelMeshComponent;
struct FAnimNode_ModifyBone;

class FBoneProxyDetailsCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FBoneProxyDetailsCustomization);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	/** Handle reset to defaults visibility */
	bool IsResetLocationVisible(TSharedPtr<IPropertyHandle> InPropertyHandle, UBoneProxy* InBoneProxy);

	/** Handle reset to defaults visibility */
	bool IsResetRotationVisible(TSharedPtr<IPropertyHandle> InPropertyHandle, UBoneProxy* InBoneProxy);

	/** Handle reset to defaults visibility */
	bool IsResetScaleVisible(TSharedPtr<IPropertyHandle> InPropertyHandle, UBoneProxy* InBoneProxy);

	/** Handle resetting defaults */
	void HandleResetLocation(TSharedPtr<IPropertyHandle> InPropertyHandle, UBoneProxy* InBoneProxy);

	/** Handle resetting defaults */
	void HandleResetRotation(TSharedPtr<IPropertyHandle> InPropertyHandle, UBoneProxy* InBoneProxy);

	/** Handle resetting defaults */
	void HandleResetScale(TSharedPtr<IPropertyHandle> InPropertyHandle, UBoneProxy* InBoneProxy);

	/** Remove any modification node if it has no effect */
	void RemoveUnnecessaryModifications(UDebugSkelMeshComponent* Component, FAnimNode_ModifyBone& ModifyBone);
};
