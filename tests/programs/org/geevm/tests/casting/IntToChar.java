package org.geevm.tests.casting;

import org.geevm.tests.Printer;

public class IntToChar {

    public static void main(String[] args) {
        int normalValue = 65;
        castAndPrint(normalValue); // A

        int overflowValue = 65602;
        castAndPrint(overflowValue); // B

        int underflowValue = -65602;
        castAndPrint(underflowValue); // ﾾ
    }

    public static void castAndPrint(int val) {
        Printer.println((char) val);
    }
}
