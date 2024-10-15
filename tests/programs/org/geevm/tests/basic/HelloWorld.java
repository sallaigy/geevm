package org.geevm.tests.basic;

public class HelloWorld {
    public static void main(String[] args) {
        __geevm_print("Hello World!");
    }

    private static native void __geevm_print(String value);
}
