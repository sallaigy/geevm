package org.geevm.tests.errors;

public class UnknownNativeMethod {
    public static void main(String[] args) {
        callee();
    }

    public static native void callee();
}
