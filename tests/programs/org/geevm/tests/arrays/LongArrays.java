package org.geevm.tests.arrays;

import org.geevm.tests.Printer;

public class LongArrays {

    public static void main(String[] args) {
        long[] t = new long[10];
        for (int i = 0; i < t.length; i++) {
            t[i] = 2 * i;
        }

        for (int i = 0; i < t.length; i++) {
            Printer.println(t[i]);
        }
    }

}