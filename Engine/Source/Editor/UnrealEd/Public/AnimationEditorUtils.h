// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#ifndef __AnimationEditorUtils_h__
#define __AnimationEditorUtils_h__

/** dialog to prompt users to decide an animation asset name */
class SCreateAnimationAssetDlg : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SCreateAnimationAssetDlg)
	{
	}
		SLATE_ARGUMENT(FText, DefaultAssetPath)
	SLATE_END_ARGS()

		SCreateAnimationAssetDlg()
		: UserResponse(EAppReturnType::Cancel)
	{
	}

	void Construct(const FArguments& InArgs);

public:
	/** Displays the dialog in a blocking fashion */
	EAppReturnType::Type ShowModal();

	/** Gets the resulting asset path */
	FString GetAssetPath();

	/** Gets the resulting asset name */
	FString GetAssetName();

	/** Gets the resulting full asset path (path+'/'+name) */
	FString GetFullAssetPath();

protected:
	void OnPathChange(const FString& NewPath);
	void OnNameChange(const FText& NewName, ETextCommit::Type CommitInfo);
	FReply OnButtonClick(EAppReturnType::Type ButtonID);

	bool ValidatePackage();

	EAppReturnType::Type UserResponse;
	FText AssetPath;
	FText AssetName;

	static FText LastUsedAssetPath;
};

/** Defines FCanExecuteAction delegate interface.  Returns true when an action is able to execute. */
DECLARE_DELEGATE_OneParam(FAnimAssetCreated, TArray<class UObject*>);

//Animation editor utility functions
namespace AnimationEditorUtils
{
	UNREALED_API void CreateAnimationAssets(const TArray<TWeakObjectPtr<USkeleton>>& Skeletons, TSubclassOf<UAnimationAsset> AssetClass, const FString& InPrefix, FAnimAssetCreated AssetCreated );
	UNREALED_API void FillCreateAssetMenu(FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USkeleton>> Skeletons, FAnimAssetCreated AssetCreated, bool bInContentBrowser=true);
} // namespace AnimationEditorUtils

#endif //__AnimationEditorUtils_h__
