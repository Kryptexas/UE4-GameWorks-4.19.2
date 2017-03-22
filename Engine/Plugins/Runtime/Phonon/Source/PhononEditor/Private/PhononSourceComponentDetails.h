//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "IDetailCustomization.h"
#include "Input/Reply.h"

class UPhononSourceComponent;
class IDetailLayoutBuilder;

namespace Phonon
{
	class FPhononSourceComponentDetails : public IDetailCustomization
	{
	public:
		FPhononSourceComponentDetails();

		static TSharedRef<IDetailCustomization> MakeInstance();

	private:
		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

		FReply OnBakePropagation();

		TWeakObjectPtr<UPhononSourceComponent> PhononSourceComponent;

		TSharedPtr<class FTickableNotification> BakePropagationTickable;
	};
} 