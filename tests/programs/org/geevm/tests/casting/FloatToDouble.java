package org.geevm.tests.casting;

import org.geevm.tests.Printer;

public class FloatToDouble {

    public static void main(String[] args) {
        castAndPrint(42.9f); // Expect 42.9
        castAndPrint(-42.9f); // Expect -42.9
        castAndPrint(0.0f); // Expect 0.0
        castAndPrint(Float.MIN_VALUE); // Expect 1.4E-45 (Smallest positive float)
        castAndPrint(Float.MAX_VALUE); // Expect 3.4028235E38 (Largest positive float)
        castAndPrint(-Float.MAX_VALUE); // Expect -3.4028235E38 (Largest negative float)
        castAndPrint(Float.POSITIVE_INFINITY); // Expect Infinity
        castAndPrint(Float.NEGATIVE_INFINITY); // Expect -Infinity
        castAndPrint(Float.NaN); // Expect NaN
        castAndPrint(0.1f); // Expect approximately 0.1 due to precision limits of float
    }
    
    public static void castAndPrint(float val) {
        Printer.println((double) val);
    }
}
