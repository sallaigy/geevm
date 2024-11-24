package org.geevm.tests.casting;

import org.geevm.tests.Printer;

public class LongToInt {

    public static void main(String[] args) {
        castAndPrint(100); // 100
        castAndPrint((((long) Integer.MIN_VALUE) - 1)); // 2147483647
        castAndPrint((((long) Integer.MAX_VALUE) + 1)); // -2147483648
        castAndPrint(Long.MIN_VALUE); // 0
        castAndPrint(Long.MAX_VALUE); // -1
    }

    public static void castAndPrint(long val) {
        Printer.println((int) val);
    }

}
