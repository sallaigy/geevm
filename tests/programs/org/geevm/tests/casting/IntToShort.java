package org.geevm.tests.casting;

import org.geevm.tests.Printer;

public class IntToShort {

    public static void main(String[] args) {
        int normalValue = 100;
        castAndPrint(normalValue); // 100

        int overflowValue = 35000;
        castAndPrint(overflowValue); // -30536

        int underflowValue = -35000;
        castAndPrint(underflowValue); // 30536
    }

    public static void castAndPrint(int val) {
        Printer.println((short) val);
    }
}
