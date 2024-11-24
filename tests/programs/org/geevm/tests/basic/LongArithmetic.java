package org.geevm.tests.basic;

import org.geevm.tests.Printer;

public class LongArithmetic {

    public static void main(String[] args) {
        Printer.println(add(10, 3)); // 13
        Printer.println(add(Long.MAX_VALUE, 1)); // -9223372036854775808
        Printer.println(sub(10, 3)); // 7
        Printer.println(sub(Long.MIN_VALUE, 1)); // 9223372036854775807
        Printer.println(mul(10, 3)); // 30
        Printer.println(div(10, 3)); // 3
        Printer.println(div(10, -3)); // -3
        Printer.println(div(-10, 3)); // -3
        Printer.println(div(-10, -3)); // 3
        Printer.println(mod(10, 3)); // 1
        Printer.println(mod(10, -3)); // 1
        Printer.println(mod(-10, 3)); // -1
        Printer.println(mod(-10, -3)); // -1
        Printer.println(neg(10)); // -10
        Printer.println(neg(-10)); // 10
        Printer.println(neg(0)); // 0
        Printer.println(neg(Long.MIN_VALUE)); // -9223372036854775808
        Printer.println(neg(Long.MAX_VALUE)); // -9223372036854775807
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

    public static long neg(long x) {
        return -x;
    }

}
