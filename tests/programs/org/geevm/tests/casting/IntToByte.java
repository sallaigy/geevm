package org.geevm.tests.casting;

import org.geevm.tests.Printer;

public class IntToByte {

    public static void main(String[] args) {
        int normalValue = 100;
        castAndPrint(normalValue); // 100

        int overflowValue = 130;
        castAndPrint(overflowValue); // -126

        int underflowValue = -130;
        castAndPrint(underflowValue); // 126
    }

    public static void castAndPrint(int val) {
        Printer.println((byte) val);
    }
}
