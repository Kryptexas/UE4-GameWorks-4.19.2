package com.google.vr.sdk.samples.transition;
import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.content.Intent;
import com.google.vr.ndk.base.DaydreamApi;

public class GVRTransition2DActivity extends Activity
{
	private static final int EXIT_REQUEST_CODE = 777;
	public static GVRTransition2DActivity activity = null;
	public static GVRTransition2DActivity getActivity() {
		return GVRTransition2DActivity.activity;
	}

	protected void onCreate(Bundle savedInstanceState) {
		this.requestWindowFeature(Window.FEATURE_NO_TITLE);
		this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.dialog);
		Button returnButton = (Button)findViewById(R.id.return_button);
		returnButton.setVisibility(View.GONE);
		GVRTransition2DActivity.activity = this;
	}

	protected void onStart() {
		super.onStart();
		GVRTransitionHelper.onTransitionTo2D(this);
	}

	public void showReturnButton() {
		Button returnButton = (Button)findViewById(R.id.return_button);
		returnButton.setVisibility(View.VISIBLE);
	}

	public void returnToVR(View view) {
		finish();
	}
}