package org.geevm.tests.arrays;

import org.geevm.tests.Printer;

public class ShortArrays {

    public static void main(String[] args) {
        short[] t = new short[10];
        for (int i = 0; i < t.length; i++) {
            t[i] = (short) (2 * i);
        }

        for (int i = 0; i < t.length; i++) {
            Printer.println(t[i]);
        }
    }
}
