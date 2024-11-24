package org.geevm.tests.casting;

import org.geevm.tests.Printer;

public class DoubleToFloat {

    public static void main(String[] args) {
        castAndPrint(42.9); 
        castAndPrint(-42.9);
        castAndPrint(0.0);
        castAndPrint(Double.MIN_VALUE);
        castAndPrint(Double.MAX_VALUE);
        castAndPrint(-Double.MAX_VALUE);
        castAndPrint(Double.POSITIVE_INFINITY);
        castAndPrint(Double.NEGATIVE_INFINITY);
        castAndPrint(Double.NaN);
        castAndPrint(0.1);
    }

    public static void castAndPrint(double val) {
        Printer.println((float) val);
    }
}
