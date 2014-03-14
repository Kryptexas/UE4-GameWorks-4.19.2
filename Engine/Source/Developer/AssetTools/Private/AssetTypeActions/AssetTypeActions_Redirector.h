// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FRedirectorRefs;

class FAssetTypeActions_Redirector : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Redirector", "Redirector"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(128, 128, 128); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UObjectRedirector::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void AssetsActivated( const TArray<UObject*>& InObjects, EAssetTypeActivationMethod::Type ActivationType ) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Misc; }

private:
	/** Handler for when FindTarget is selected */
	void ExecuteFindTarget(TArray<TWeakObjectPtr<UObjectRedirector>> Objects);

	/** Handler for when FixUp is selected */
	void ExecuteFixUp(TArray<TWeakObjectPtr<UObjectRedirector>> Objects);

	/** Syncs the content browser to the destination objects for all the supplied redirectors */
	void FindTargets(const TArray<UObjectRedirector*>& Redirectors) const;

	/** Loads and saves all referencers to the supplied redirectors, then deletes the redirectors if all references to them were removed */
	void FixupReferencers(TArray<TWeakObjectPtr<UObjectRedirector>> Objects) const;

	/** Fills out the Referencing packages for all the redirectors described in AssetsToPopulate */
	void PopulateRedirectorReferencers(TArray<FRedirectorRefs>& RedirectorsToPopulate) const;

	/** Updates the source control status of the packages containing the assets to rename */
	bool UpdatePackageStatus(const TArray<FRedirectorRefs>& RedirectorsToFix) const;

	/** 
	  * Loads all referencing packages to redirectors in RedirectorsToFix, finds redirectors whose references can
	  * not be fixed up, and returns a list of referencing packages to save.
	  */
	void LoadReferencingPackages(TArray<FRedirectorRefs>& RedirectorsToFix, TArray<UPackage*>& OutReferencingPackagesToSave) const;

	/** 
	  * Prompts to check out referencing packages and marks assets whose referencing packages were not checked out to not fix the redirector.
	  * Trims PackagesToSave when necessary.
	  * Returns true if the user opted to continue the operation or no dialog was required.
	  */
	bool CheckOutReferencingPackages(TArray<FRedirectorRefs>& RedirectorsToFix, TArray<UPackage*>& InOutReferencingPackagesToSave) const;

	/** Finds any read only packages and removes them from the save list. Redirectors referenced by these packages will not be fixed up. */ 
	void DetectReadOnlyPackages(TArray<FRedirectorRefs>& RedirectorsToFix, TArray<UPackage*>& InOutReferencingPackagesToSave) const;

	/** Saves all the referencing packages and updates SCC state */
	void SaveReferencingPackages(const TArray<UPackage*>& ReferencingPackagesToSave) const;

	/** Deletes redirectors that are valid to delete */
	void DeleteRedirectors(TArray<FRedirectorRefs>& RedirectorsToFix) const;

	/** Report any failures that may have happened during the rename */
	void ReportFailures(const TArray<FRedirectorRefs>& RedirectorsToFix) const;
};