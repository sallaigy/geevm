package org.geevm.tests.basic;

import org.geevm.tests.Printer;

public class IntegerArithmetic {

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

}
