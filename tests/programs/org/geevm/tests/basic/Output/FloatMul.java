// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class FloatAdd {

    public static void main(String[] args) {
        double nan = Double.NaN;

        // CHECK: 23.625
        Printer.println(mul(10.5f, 2.25f));
        // CHECK-NEXT: -23.625
        Printer.println(mul(10.5f, -2.25f));
        // CHECK-NEXT: -23.625
        Printer.println(mul(-10.5f, 2.25f));
        // CHECK-NEXT: 23.625
        Printer.println(mul(-10.5f, -2.25f));
        // CHECK-NEXT: nan
        Printer.println(mul(nan, 2.25f));
        // CHECK-NEXT: +inf
        Printer.println(mul(10.5f, Float.POSITIVE_INFINITY));
        // CHECK-NEXT: -inf
        Printer.println(mul(-10.5f, Float.POSITIVE_INFINITY));
        // CHECK-NEXT: -inf
        Printer.println(mul(10.5f, Float.NEGATIVE_INFINITY));
        // CHECK-NEXT: +inf
        Printer.println(mul(-10.5f, Float.NEGATIVE_INFINITY));
        // CHECK-NEXT: +inf
        Printer.println(mul(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY));
        // CHECK-NEXT: -inf
        Printer.println(mul(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY));
        // CHECK-NEXT: -inf
        Printer.println(mul(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY));
        // CHECK-NEXT: +inf
        Printer.println(mul(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY));
        // CHECK-NEXT: nan
        Printer.println(mul(Float.POSITIVE_INFINITY, 0.0f));
        // CHECK-NEXT: nan
        Printer.println(mul(Float.NEGATIVE_INFINITY, 0.0f));
    }

    public static double add(float x, float y) {
        return x + y;
    }

}
