package org.geevm.tests.casting;

import org.geevm.tests.Printer;

public class FloatToInt {

    public static void main(String[] args) {
        castAndPrint(42.9f); // Expect 42
        castAndPrint(-42.9f); // Expect -42
        castAndPrint(0.0f); // Expect 0
        castAndPrint(Float.MAX_VALUE); // Expect Integer.MAX_VALUE
        castAndPrint(Float.MIN_VALUE); // Expect 0
        castAndPrint(-Float.MAX_VALUE); // Expect Integer.MIN_VALUE
        castAndPrint(Float.POSITIVE_INFINITY); // Expect Integer.MAX_VALUE
        castAndPrint(Float.NEGATIVE_INFINITY); // Expect Integer.MIN_VALUE
        castAndPrint(Float.NaN); // Expect 0
        castAndPrint(2_147_483_647.5f); // Expect 2_147_483_647
        castAndPrint(-2_147_483_648.5f); // Expect -2_147_483_648
    }

    public static void castAndPrint(float val) {
        Printer.println((int) val);
    }
}
