// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
//This file needs to be here so the "ant" build step doesnt fail when looking for a /src folder.

package com.epicgames.ue4;

import android.os.RemoteException;
import android.content.IntentSender.SendIntentException;

import com.android.vending.billing.util.IabHelper;
import com.android.vending.billing.util.IabResult;
import com.android.vending.billing.util.Inventory;
import com.android.vending.billing.util.Purchase;
import com.android.vending.billing.util.Base64;

public class GooglePlayStoreHelper extends StoreHelper
{
	private IabHelper inAppPurchaseHelper;
	
	private GameActivity gameActivity;

	private Logger Log;

	private String ProductKey;
	
	// Iab helper functions for the store interface
	IabHelper.OnConsumeFinishedListener ConsumeProductFinishedCallback =  new IabHelper.OnConsumeFinishedListener() 
	{
		public void onConsumeFinished(Purchase purchase, IabResult result) 
		{
			if (result.isSuccess()) 
			{
				Log.debug("[GooglePlayStoreHelper] - onConsumeFinished: " + (result.isSuccess() ? "Success" : "Failed" ));
				String Receipt = Base64.encode(purchase.getOriginalJson().getBytes());
				nativePurchaseComplete(true, purchase.getSku(), Receipt);
			}
			else
			{
				// Something went wrong, cannot consume product...send back success, but will be unable to purchase again
				Log.debug("[GooglePlayStoreHelper] - ERROR: Could not successfully consume item. Will attempt again on next inventory update.");
				nativePurchaseComplete(false, purchase.getSku(), "");
			}
		}
	};

	public GooglePlayStoreHelper(String InProductKey, GameActivity InGameActivity, final Logger InLog)
	{
		ProductKey = InProductKey;
		gameActivity = InGameActivity;
		Log = InLog;
		SetupIapService();
	}

	private boolean bIsIapSetup = false;
	public void SetupIapService()
	{
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::SetupIapService");

		inAppPurchaseHelper = new IabHelper(gameActivity, ProductKey);

		inAppPurchaseHelper.startSetup(new IabHelper.OnIabSetupFinishedListener() 
		{
			public void onIabSetupFinished(IabResult result)
			{
				Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::onIabSetupFinished");

				bIsIapSetup = result.isSuccess();

				if (bIsIapSetup)
				{
					Log.debug("[GooglePlayStoreHelper] - In-app purchasing helper has been setup");
				}
				else
				{
					Log.debug("[GooglePlayStoreHelper] - Problem setting up In-app purchasing helper: " + result);
				}
			}
		});
	}


	private String[] cachedQueryProductIds;
	private boolean[] cachedQueryConsumableFlags;
	public boolean QueryInAppPurchases(final String[] ProductIds, final boolean[] bConsumableFlags)
	{
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::QueryInAppPurchases");

		// Cache the input parameters for cross threading
		cachedQueryProductIds = ProductIds;
		cachedQueryConsumableFlags = bConsumableFlags;
		
		// We can launch a purchase workflow if we have provided product Ids and the IabHelper is set-up.
		boolean bIsAbleToSendQuery = bIsIapSetup && (cachedQueryProductIds.length > 0 );
		if( bIsAbleToSendQuery )
		{
			IabHelper.QueryInventoryFinishedListener GotInventoryListener = new IabHelper.QueryInventoryFinishedListener()
			{
				public void onQueryInventoryFinished(IabResult result, Inventory inventory)
				{
					if (result.isSuccess())
					{
						Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::onQueryInventoryFinished - Success.");
						String[] titles = new String[cachedQueryProductIds.length];
						String[] descriptions = new String[cachedQueryProductIds.length];
						String[] prices = new String[cachedQueryProductIds.length];

						int Idx = 0;
						for( String productId : cachedQueryProductIds )
						{
							titles[Idx] = inventory.getSkuDetails(productId).getTitle();
							descriptions[Idx] = inventory.getSkuDetails(productId).getDescription();
							prices[Idx] = inventory.getSkuDetails(productId).getPrice();

							++Idx;
						}

						Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::onQueryInventoryFinished - Succeeded.");
						nativeQueryComplete(true, cachedQueryProductIds, titles, descriptions, prices); 
					}
					else
					{
						Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::onQueryInventoryFinished - Failed.");
						nativeQueryComplete(false, null, null, null, null);
					}
				}
			};

			// Run the query pruchases command.
			Log.debug("[GooglePlayStoreHelper] - queryInventoryAsync. - start");
			try {
				inAppPurchaseHelper.queryInventoryAsync(GotInventoryListener);
			} catch(Exception e) {
				Log.debug("[GooglePlayStoreHelper] - Exception caught - " + e.getMessage());
			}
			Log.debug("[GooglePlayStoreHelper] - queryInventoryAsync. - end");

		}
		else
		{
			if( bIsIapSetup == false )
			{
				Log.debug("[GooglePlayStoreHelper] - Failed to launch a query inventory flow, Store Helper is not set-up.");
			}
			else if(cachedQueryProductIds.length <= 0 )
			{
				Log.debug("[GooglePlayStoreHelper] - Failed to launch a query inventory flow, Cannot query without product Ids.");
			}

			nativeQueryComplete(false, null, null, null, null);
		}


		return bIsAbleToSendQuery;
	}

	String cachedPurchaseProductId;
	boolean cachedPurchaseConsumableFlag;
	public boolean BeginPurchase(final String ProductId, final boolean bConsumableFlag)
	{
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::BeginPurchase");
	
		// Cache the input parameters for cross threading
		cachedPurchaseProductId = ProductId;
		cachedPurchaseConsumableFlag = bConsumableFlag;

		// We can launch a purchase workflow if we have a provided product Id and the IabHelper is set-up.
		boolean bIsValidProductId = (cachedPurchaseProductId.length() > 0);
		if( bIsIapSetup && bIsValidProductId )
		{
			IabHelper.OnIabPurchaseFinishedListener PurchaseFinishedCallback = new IabHelper.OnIabPurchaseFinishedListener() 
			{
				public void onIabPurchaseFinished(IabResult result, Purchase purchase) 
				{
					if(result.isSuccess())
					{
						// Consumable items must be consumed on the server prior to provisioning the item to the game
						if(cachedPurchaseConsumableFlag == true)
						{
							Log.debug("[GooglePlayStoreHelper] - Purchase Success. Requesting Item be consumed.");
							inAppPurchaseHelper.consumeAsync(purchase, ConsumeProductFinishedCallback);
						}
						// Non consumable can immediately notify the game of the purchase
						else
						{
							Log.debug("[GooglePlayStoreHelper] - Purchase Success.");
							String Receipt = Base64.encode(purchase.getOriginalJson().getBytes());
							nativePurchaseComplete(false, cachedPurchaseProductId, Receipt);
						}
					}
					else
					{
						Log.debug("[GooglePlayStoreHelper] - Purchase Unsuccessful. " + result.getMessage());
						nativePurchaseComplete(false, cachedPurchaseProductId, "" );
					} 
				}
			};
			
			Log.debug("[GooglePlayStoreHelper] - launchPurchaseFlow. - start");
			try {
				inAppPurchaseHelper.launchPurchaseFlow(gameActivity, cachedPurchaseProductId, cachedPurchaseConsumableFlag ? 10001 : 10002, PurchaseFinishedCallback, "");
			} catch(Exception e) {
				Log.debug("[GooglePlayStoreHelper] - Exception caught - " + e.getMessage());
			}
			Log.debug("[GooglePlayStoreHelper] - launchPurchaseFlow. - end");
		}
		else
		{
			if( bIsIapSetup == false )
			{
				Log.debug("[GooglePlayStoreHelper] - Failed to launch a purchase flow, Store Helper is not set-up.");
			}
			else if( bIsValidProductId == false )
			{
				Log.debug("[GooglePlayStoreHelper] - Failed to launch a purchase flow, Cannot purchase without a valid product Id.");
			}

			nativePurchaseComplete(false, ProductId, "" );
		}
		
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::BeginPurchase - " + ((bIsIapSetup && bIsValidProductId) ? "true" : "false"));
		return bIsIapSetup && bIsValidProductId;
	}

	public boolean IsAllowedToMakePurchases()
	{
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::IsAllowedToMakePurchases");
		return bIsIapSetup;
	}

	public native void nativeQueryComplete(boolean bSuccess, String[] productIDs, String[] titles, String[] descriptions, String[] prices );

	public native void nativePurchaseComplete(boolean bSuccess, String ProductId, String ReceiptData);
}

