//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "IPhononEditorModule.h"
#include "SlateStyle.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPhononEditor, Log, All);

class FComponentVisualizer;
class FExtender;
class FMenuBuilder;
class FUICommandList;

namespace Phonon
{
	class FPhononEditorModule : public IPhononEditorModule
	{
	public:
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;

		TSharedRef<FExtender> OnExtendLevelEditorBuildMenu(const TSharedRef<FUICommandList> CommandList);
		void CreateBuildMenu(FMenuBuilder& Builder);

		void ExportScene();
		bool IsReadyToExportScene() const;

		void BakeReverb();
		bool IsReadyToBakeReverb() const;

		void RegisterComponentVisualizer(const FName ComponentClassName, TSharedPtr<FComponentVisualizer> Visualizer);

	private:
		TSharedPtr<class FTickableNotification> ExportSceneTickable;
		TSharedPtr<class FTickableNotification> BakeReverbTickable;
		TSharedPtr<FSlateStyleSet> PhononStyleSet;

		TArray<FName> RegisteredComponentClassNames;
	};
}