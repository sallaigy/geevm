package org.geevm.tests.casting;

import org.geevm.tests.Printer;

public class IntToLong {

    public static void main(String[] args) {
        int minVal = Integer.MIN_VALUE;
        int maxVal = Integer.MAX_VALUE;
        int basic = 100;

        castAndPrint(basic); // 100
        castAndPrint(minVal); // -2147483648
        castAndPrint(maxVal); // 2147483647
    }

    public static void castAndPrint(int val) {
        Printer.println((long) val);
    }

}
