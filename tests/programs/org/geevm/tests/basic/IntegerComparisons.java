package org.geevm.tests.basic;

import org.geevm.tests.Printer;

public class IntegerComparisons {

    public static void main(String[] args) {
        compare(0, 1); // prints 035
        compare(1, 0); // prints 125
        compare(1, 1); // prints 234 
    }

    private static void compare(int a, int b) {
        if (a < b) {
            Printer.println(0);
        }
        if (a > b) {
            Printer.println(1);
        }
        if (a >= b) {
            Printer.println(2);
        }
        if (a <= b) {
            Printer.println(3);
        }
        if (a == b) {
            Printer.println(4);
        }
        if (a != b) {
            Printer.println(5);
        }
    }

}
