package org.geevm.tests;

public class Printer {

    public static native void println(int value);
    public static native void println(float value);
    public static native void println(long value);
    public static native void println(double value);
    public static native void println(char value);
    public static native void println(boolean value);
    public static native void println(String value);

}