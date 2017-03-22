//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "IDetailCustomization.h"
#include "Input/Reply.h"

class IDetailLayoutBuilder;
class APhononProbeVolume;

namespace Phonon
{
	class FPhononProbeVolumeDetails : public IDetailCustomization
	{
	public:
		static TSharedRef<IDetailCustomization> MakeInstance();

	private:
		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

		FReply OnGenerateProbes();

		TWeakObjectPtr<APhononProbeVolume> PhononProbeVolume;
	};
}