package com.chaoyi.myapplication;

public class OpenSLPlay {
    static {
        System.loadLibrary("openslplay");
    }

    public native void playSound(String input);
}
