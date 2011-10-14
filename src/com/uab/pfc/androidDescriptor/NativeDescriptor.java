package com.uab.pfc.androidDescriptor;

import android.graphics.Bitmap;

public class NativeDescriptor {

	static {
		System.loadLibrary("descriptor");
	}

	public native float []descriptor(String path);
	
	/** 
	 * Adds two integers, returning their sum
	 */
	public native int add( int v1, int v2 );

	/**
	 * Returns Hello World string
	 */
	public native String helloWorld();
}
