package org.geevm.tests.casting;

import org.geevm.tests.Printer;

public class IntToFloat {

    public static void main(String[] args) {
        castAndPrint(12345); // 12345.0
        castAndPrint(-12345); // -12345.0
        castAndPrint(16777216); // 1.6777216E7
        castAndPrint(16777217); // 1.6777216E7
        castAndPrint(Integer.MAX_VALUE); // 2.1474836E9
        castAndPrint(Integer.MIN_VALUE); // -2.1474836E9
    }

    public static void castAndPrint(int val) {
        Printer.println((float) val);
    }
}
