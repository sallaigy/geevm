package org.geevm.tests.basic;

public class IntegerCompareZero {

    public static void main(String[] args) {
        compare(1); // prints 125
        compare(0); // prints 234
        compare(-1); // prints 035
    }

    private static void compare(int a) {
        if (a < 0) {
            Printer.println(0);
        }
        if (a > 0) {
            Printer.println(1);
        }
        if (a >= 0) {
            Printer.println(2);
        }
        if (a <= 0) {
            Printer.println(3);
        }
        if (a == 0) {
            Printer.println(4);
        }
        if (a != 0) {
            Printer.println(5);
        }
    }

}
