package org.geevm.tests.casting;

import org.geevm.tests.Printer;

public class LongToFloat {

    public static void main(String[] args) {
        // 12345.0
        // -12345.0
        // -2.1474836E9
        // 2.1474836E9
        // -9.223372E18
        // 9.223372E18
        castAndPrint(12345);
        castAndPrint(-12345);
        castAndPrint((((long) Integer.MIN_VALUE) - 1));
        castAndPrint((((long) Integer.MAX_VALUE) + 1));
        castAndPrint(Long.MIN_VALUE);
        castAndPrint(Long.MAX_VALUE);
    }

    public static void castAndPrint(long val) {
        Printer.println((float) val);
    }

}
