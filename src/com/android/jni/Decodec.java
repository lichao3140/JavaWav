package com.android.jni;

public class Decodec {
	
	public native static int newdecode(String fileName);
	
	static{
		System.loadLibrary("JavaWav");
	}
}
