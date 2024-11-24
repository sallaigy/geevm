package org.geevm.tests.basic;

import org.geevm.tests.Printer;

public class FloatArithmetic {

    public static void main(String[] args) {
        float nan = Float.NaN;

        Printer.println(add(10.5f, 2.25f)); // 12.75
        Printer.println(add(nan, 2.25f)); // nan
        Printer.println(add(10.5f, nan)); // nan
        Printer.println(add(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY)); // -nan
        Printer.println(add(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY)); // +inf
        Printer.println(add(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY)); // -inf
        Printer.println(add(+0.0f, -0.0f)); // +0.0f
        Printer.println(add(+0.0f, +0.0f)); // +0.0f
        Printer.println(add(-0.0f, -0.0f)); // -0.0f
        Printer.println(add(+0.0f, 2.25f)); // 2.25f
        Printer.println(add(-0.0f, 2.25f)); // 2.25f
        Printer.println(add(-2.25f, 2.25f)); // +0.0f
        Printer.println(add(10.5f, nan)); // nan

        Printer.println(sub(10.5f, 2.25f)); // 8.25

        Printer.println(mul(10.5f, 2.25f)); // 23.625
        Printer.println(mul(10.5f, -2.25f)); // -23.625
        Printer.println(mul(-10.5f, 2.25f)); // -23.625
        Printer.println(mul(-10.5f, -2.25f)); // 23.625
        Printer.println(mul(nan, 2.25f)); // nan
        Printer.println(mul(10.5f, Float.POSITIVE_INFINITY)); // +inf
        Printer.println(mul(-10.5f, Float.POSITIVE_INFINITY)); // -inf
        Printer.println(mul(10.5f, Float.NEGATIVE_INFINITY)); // -inf
        Printer.println(mul(-10.5f, Float.NEGATIVE_INFINITY)); // +inf
        Printer.println(mul(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY)); // +inf
        Printer.println(mul(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY)); // -inf
        Printer.println(mul(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY)); // -inf
        Printer.println(mul(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY)); // +inf
        Printer.println(mul(Float.POSITIVE_INFINITY, 0.0f)); // nan
        Printer.println(mul(Float.NEGATIVE_INFINITY, 0.0f)); // nan

        Printer.println(div(10.5f, 2.25f)); // 4.6666665
        Printer.println(div(nan, 2.25f)); // nan
        Printer.println(div(10.5f, nan)); // nan
        Printer.println(div(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY)); // -nan
        Printer.println(div(10.5f, Float.NEGATIVE_INFINITY)); // -0
        Printer.println(div(10.5f, Float.POSITIVE_INFINITY)); // 0
        Printer.println(div(Float.POSITIVE_INFINITY, 2.25f)); // +inf
        Printer.println(div(Float.NEGATIVE_INFINITY, 2.25f)); // -inf

        Printer.println(mod(10.5f, 2.25f)); // 1.5
        Printer.println(mod(10.5f, -2.25f)); // 1.5
        Printer.println(mod(-10.5f, 2.25f)); // -1.5
        Printer.println(mod(-10.5f, -2.25f)); // -1.5

        Printer.println(neg(10.5f)); // -10.5
        Printer.println(neg(-10.5f)); // 10.5
        Printer.println(neg(+0.0f)); // -0.0
        Printer.println(neg(-0.0f)); // 0.0
        Printer.println(neg(Float.MIN_VALUE)); // -1.4E-45
        Printer.println(neg(Float.MAX_VALUE)); // -3.4028235E38
        Printer.println(neg(Float.NaN)); // NaN
        Printer.println(neg(Float.NEGATIVE_INFINITY)); // Infinity
        Printer.println(neg(Float.POSITIVE_INFINITY)); // -Infinity
    }

    public static float add(float x, float y) {
        return x + y;
    }

    public static float sub(float x, float y) {
        return x - y;
    }

    public static float mul(float x, float y) {
        return x * y;
    }

    public static float div(float x, float y) {
        return x / y;
    }

    public static float mod(float x, float y) {
        return x % y;
    }

    public static float neg(float x) {
        return -x;
    }

}
