package org.geevm.tests.casting;

import org.geevm.tests.Printer;

public class DoubleToLong {

    public static void main(String[] args) {
        castAndPrint(42.9f); // Expect 42
        castAndPrint(-42.9f); // Expect -42
        castAndPrint(0.0f); // Expect 0
        castAndPrint(Double.MAX_VALUE); // Expect Long.MAX_VALUE
        castAndPrint(Double.MIN_VALUE); // Expect 0
        castAndPrint(-Double.MAX_VALUE); // Expect Long.MIN_VALUE
        castAndPrint(Double.POSITIVE_INFINITY); // Expect Long.MAX_VALUE
        castAndPrint(Double.NEGATIVE_INFINITY); // Expect Long.MIN_VALUE
        castAndPrint(Double.NaN); // Expect 0
        castAndPrint(9_223_372_036_854_775_807.5f); // Expect 9223372036854775807 (Long.MAX_VALUE)
        castAndPrint(-9_223_372_036_854_775_808.5f); // Expect -9223372036854775808 (Long.MIN_VALUE)
    }

    public static void castAndPrint(double val) {
        Printer.println((long) val);
    }
}
