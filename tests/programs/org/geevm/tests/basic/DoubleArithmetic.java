package org.geevm.tests.basic;

import org.geevm.tests.Printer;

public class DoubleArithmetic {

    public static void main(String[] args) {
        double nan = Double.NaN;

        Printer.println(add(10.5, 2.25)); // 12.75
        Printer.println(add(nan, 2.25)); // nan
        Printer.println(add(10.5, nan)); // nan
        Printer.println(add(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY)); // -nan
        Printer.println(add(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY)); // +inf
        Printer.println(add(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY)); // -inf
        Printer.println(add(+0.0, -0.0)); // +0.0
        Printer.println(add(+0.0, +0.0)); // +0.0
        Printer.println(add(-0.0, -0.0)); // -0.0
        Printer.println(add(+0.0, 2.25)); // 2.25
        Printer.println(add(-0.0, 2.25)); // 2.25
        Printer.println(add(-2.25, 2.25)); // +0.0
        Printer.println(add(10.5, nan)); // nan

        Printer.println(sub(10.5, 2.25)); // 8.25

        Printer.println(mul(10.5, 2.25)); // 23.625
        Printer.println(mul(10.5, -2.25)); // -23.625
        Printer.println(mul(-10.5, 2.25)); // -23.625
        Printer.println(mul(-10.5, -2.25)); // 23.625
        Printer.println(mul(nan, 2.25)); // nan
        Printer.println(mul(10.5, Double.POSITIVE_INFINITY)); // +inf
        Printer.println(mul(-10.5, Double.POSITIVE_INFINITY)); // -inf
        Printer.println(mul(10.5, Double.NEGATIVE_INFINITY)); // -inf
        Printer.println(mul(-10.5, Double.NEGATIVE_INFINITY)); // +inf
        Printer.println(mul(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY)); // +inf
        Printer.println(mul(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY)); // -inf
        Printer.println(mul(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY)); // -inf
        Printer.println(mul(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY)); // +inf
        Printer.println(mul(Double.POSITIVE_INFINITY, 0.0)); // nan
        Printer.println(mul(Double.NEGATIVE_INFINITY, 0.0)); // nan

        Printer.println(div(10.5, 2.25)); // 4.6666665
        Printer.println(div(nan, 2.25)); // nan
        Printer.println(div(10.5, nan)); // nan
        Printer.println(div(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY)); // -nan
        Printer.println(div(10.5, Double.NEGATIVE_INFINITY)); // -0
        Printer.println(div(10.5, Double.POSITIVE_INFINITY)); // 0
        Printer.println(div(Double.POSITIVE_INFINITY, 2.25)); // +inf
        Printer.println(div(Double.NEGATIVE_INFINITY, 2.25)); // -inf

        Printer.println(mod(10.5, 2.25)); // 1.5
    }

    public static double add(double x, double y) {
        return x + y;
    }

    public static double sub(double x, double y) {
        return x - y;
    }

    public static double mul(double x, double y) {
        return x * y;
    }

    public static double div(double x, double y) {
        return x / y;
    }

    public static double mod(double x, double y) {
        return x % y;
    }

}
