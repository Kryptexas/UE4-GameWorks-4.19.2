// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/** The view model for the SAddContentWidget. */
class FAddContentWidgetViewModel : public TSharedFromThis<FAddContentWidgetViewModel>
{
public:
	DECLARE_DELEGATE(FOnCategoriesChanged)
	DECLARE_DELEGATE(FOnContentSourcesChanged);
	DECLARE_DELEGATE(FOnSelectedContentSourceChanged);
	DECLARE_DELEGATE(FOnAddListChanged);

	typedef TTextFilter<TSharedPtr<FContentSourceViewModel>> ContentSourceTextFilter;

	/** Creates a shared reference to a new view model. */
	static TSharedRef<FAddContentWidgetViewModel> CreateShared();

	/** Gets the view models for the current set of content source categories. */
	const TArray<FCategoryViewModel>* GetCategories();

	/** Sets the delegate which should be executed when the set of categories changes. */
	void SetOnCategoriesChanged(FOnCategoriesChanged OnCategoriesChangedIn);

	/** Gets the view model for the currently selected category.  Only content sources which match
		the selected category will be returned by GetContentSources(). */
	FCategoryViewModel GetSelectedCategory();

	/** Sets the currently selected category view model. Only content sources which match the selected
		category will be returned by GetContentSources(). */
	void SetSelectedCategory(FCategoryViewModel SelectedCategoryIn);

	/** Sets search text which should be used to filter the content sources. */
	void SetSearchText(FText SearchTextIn);

	/** Gets a filtered array of content sources which match both the selected category and the search
		text if it has been set. */
	const TArray<TSharedPtr<FContentSourceViewModel>>* GetContentSources();

	/** Sets the delegate which should be executed when the current set of content sources returned by
		GetContentSources changes. */
	void SetOnContentSourcesChanged(FOnContentSourcesChanged OnContentSourcesChangedIn);

	/** Gets the currently selected content source. */
	TSharedPtr<FContentSourceViewModel> GetSelectedContentSource();

	/** Sets the currently selected content source. */
	void SetSelectedContentSource(TSharedPtr<FContentSourceViewModel> SelectedContentSourceIn);

	/** Sets the delegate which should be executed when the selected content source changes. */
	void SetOnSelectedContentSourceChanged(FOnSelectedContentSourceChanged OnSelectedContentSourceChangedIn);

	/** Gets the array of content sources which the user has selected for addition to the project. */
	const TArray<TSharedPtr<FContentSourceViewModel>>* GetAddList();

	/** Adds a content source to the array of content sources which the user has selected for addition to the project. */
	void AppendToAddList(TSharedPtr<FContentSourceViewModel> ContentSourceToAppend);

	/** Removes a content source from the array of content sources which the user has selected for addition to the project. */
	void RemoveFromAddList(TSharedPtr<FContentSourceViewModel> ContentSourceToRemove);

	/** Sets the delegate which should be executed when the contents of the array returned by GetAddList changes. */
	void SetOnAddListChanged(FOnAddListChanged OnAddListChangedIn);

private:
	FAddContentWidgetViewModel();
	void Initialize();

	/** Builds view models for all available content sources. */
	void BuildContentSourceViewModels();

	/** Filters the current set of content sources based on the selected category and the search text. */
	void FilterContentSourceViewModels();

	/** Converts a content source item to an array of strings for processing by the TTextFilter. */
	void TransformContentSourceToStrings(TSharedPtr<FContentSourceViewModel> Item, OUT TArray< FString >& Array);

	/** Handles notification from the content source providers when their content source arrays change. */
	void ContentSourcesChanged();

private:
	/** The array of content source providers who's content sources are being displayed. */
	TArray<TSharedPtr<IContentSourceProvider>> ContentSourceProviders;

	/** The view models for the available categories. */
	TArray<FCategoryViewModel> Categories;

	/** A combined array of all content sources from all content source providers. */
	TArray<TSharedPtr<FContentSourceViewModel>> ContentSourceViewModels;

	/** A filtered array of content sources based on the currently selected category and the search text. */
	TArray<TSharedPtr<FContentSourceViewModel>> FilteredContentSourceViewModels;

	/** An array of the content sources which the user has selected for addition to the project. */
	TArray<TSharedPtr<FContentSourceViewModel>> AddList;

	/** A map which keeps track of the currently selected content source for each category. */
	TMap<FCategoryViewModel, TSharedPtr<FContentSourceViewModel>> CategoryToSelectedContentSourceMap;

	/** The view model for the currently selected category. */
	FCategoryViewModel SelectedCategory;

	/** The current search text. */
	FText SearchText;

	/** The delegate which is executed when the available categories change. */
	FOnCategoriesChanged OnCategoriesChanged;

	/** The delegate which is executed when the filtered content sources change. */
	FOnContentSourcesChanged OnContentSourcesChanged;

	/** The delegate which is executed when the currently selected content source changes. */
	FOnSelectedContentSourceChanged OnSelectedContentSourceChanged;

	/** The delegate which is executed when the contents of the array of content souces to add
		changes. */
	FOnAddListChanged OnAddListChanged;

	/** The filter which is used to filter the content sources based on the search text. */
	TSharedPtr<ContentSourceTextFilter> ContentSourceFilter;
};