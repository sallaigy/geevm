package org.geevm.tests.basic;

public class IntegerComparisons {

    public static void main(String[] args) {
        compare(0, 1); // prints 035
        compare(1, 0); // prints 125
        compare(1, 1); // prints 234 
    }

    private static void compare(int a, int b) {
        if (a < b) {
            __geevm_print(0);
        }
        if (a > b) {
            __geevm_print(1);
        }
        if (a >= b) {
            __geevm_print(2);
        }
        if (a <= b) {
            __geevm_print(3);
        }
        if (a == b) {
            __geevm_print(4);
        }
        if (a != b) {
            __geevm_print(5);
        }
    }

    public static native void __geevm_print(int value);

}
