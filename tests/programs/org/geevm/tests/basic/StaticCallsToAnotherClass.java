package org.geevm.tests.basic;

public class StaticCallsToAnotherClass {

    public static void main(String[] args) {
        int start = 0;
        int sum = 0;

        for (int i = start; i < MathHelper.ten(); i++) {
            sum = MathHelper.inc(sum, i);
        }

        // Should print 15
        __geevm_print(sum);
    }

    public static native void __geevm_print(int value);

}