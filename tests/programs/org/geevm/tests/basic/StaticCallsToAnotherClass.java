package org.geevm.tests.basic;

public class StaticCallsToAnotherClass {

    public static void main(String[] args) {
        int start = 0;
        int sum = 0;

        for (int i = start; i < MathHelper.ten(); i++) {
            sum = MathHelper.inc(sum, i);
        }

        // Should print 15
        Printer.println(sum);
    }

}