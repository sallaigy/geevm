package org.geevm.tests.basic;

import org.geevm.tests.Printer;

public class LongArithmetic {

    public static void main(String[] args) {
        Printer.println(add(10, 3)); // 13
        Printer.println(sub(10, 3)); // 7
        Printer.println(mul(10, 3)); // 30
        Printer.println(div(10, 3)); // 3
        Printer.println(div(10, -3)); // -3
        Printer.println(div(-10, 3)); // -3
        Printer.println(div(-10, -3)); // 3
        Printer.println(mod(10, 3)); // 1
        Printer.println(mod(10, -3)); // 1
        Printer.println(mod(-10, 3)); // -1
        Printer.println(mod(-10, -3)); // -1
    }

    public static long add(long x, long y) {
        return x + y;
    }

    public static long sub(long x, long y) {
        return x - y;
    }

    public static long mul(long x, long y) {
        return x * y;
    }

    public static long div(long x, long y) {
        return x / y;
    }

    public static long mod(long x, long y) {
        return x % y;
    }

}
