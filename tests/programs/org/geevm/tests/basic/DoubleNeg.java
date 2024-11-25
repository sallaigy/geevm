// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class DoubleNeg {

    public static void main(String[] args) {
        double nan = Double.NaN;

        // CHECK: -10.5
        Printer.println(neg(10.5));
        // CHECK-NEXT: 10.5
        Printer.println(neg(-10.5));
        // CHECK-NEXT: -0
        Printer.println(neg(+0.0));
        // CHECK-NEXT: 0
        Printer.println(neg(-0.0));
        // CHECK-NEXT: -4.94066e-324
        Printer.println(neg(Double.MIN_VALUE));
        // CHECK-NEXT: -1.79769e+308
        Printer.println(neg(Double.MAX_VALUE));
        // CHECK-NEXT: -nan
        Printer.println(neg(Double.NaN));
        // CHECK-NEXT: inf
        Printer.println(neg(Double.NEGATIVE_INFINITY));
        // CHECK-NEXT: -inf
        Printer.println(neg(Double.POSITIVE_INFINITY));
    }

    public static double neg(double x) {
        return -x;
    }

}
