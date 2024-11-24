package org.geevm.tests.basic;

import org.geevm.tests.Printer;

public class IntegerArithmetic {

    public static void main(String[] args) {
        Printer.println(add(10, 3)); // 13
        Printer.println(add(Integer.MAX_VALUE, 1)); // -2147483648
        Printer.println(sub(10, 3)); // 7
        Printer.println(sub(Integer.MIN_VALUE, 1)); // 2147483647
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
        Printer.println(neg(Integer.MIN_VALUE)); // -2147483648
        Printer.println(neg(Integer.MAX_VALUE)); // -2147483647
    }

    public static int add(int x, int y) {
        return x + y;
    }

    public static int sub(int x, int y) {
        return x - y;
    }

    public static int mul(int x, int y) {
        return x * y;
    }

    public static int div(int x, int y) {
        return x / y;
    }

    public static int mod(int x, int y) {
        return x % y;
    }

    public static int neg(int x) {
        return -x;
    }

}
