package org.geevm.tests.casting;

import org.geevm.tests.Printer;

public class DoubleToInt {

    public static void main(String[] args) {
        castAndPrint(42.9);
        castAndPrint(-42.9);
        castAndPrint(0.0f);
        castAndPrint(Double.MAX_VALUE);
        castAndPrint(Double.MIN_VALUE);
        castAndPrint(-Double.MAX_VALUE);
        castAndPrint(Double.POSITIVE_INFINITY);
        castAndPrint(Double.NEGATIVE_INFINITY);
        castAndPrint(Double.NaN);
        castAndPrint(2_147_483_647.5);
        castAndPrint(-2_147_483_648.5);
    }

    public static void castAndPrint(double val) {
        Printer.println((int) val);
    }
}
