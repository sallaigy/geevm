package org.geevm.tests.arrays;

import org.geevm.tests.Printer;

public class DoubleArrays {

    public static void main(String[] args) {
        double[] t = new double[10];
        for (int i = 0; i < t.length; i++) {
            t[i] = 1.5 * i;
        }

        for (int i = 0; i < t.length; i++) {
            Printer.println(t[i]);
        }
    }
}
