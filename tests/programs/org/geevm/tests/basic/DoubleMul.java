// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class DoubleMul {

    public static void main(String[] args) {
        double nan = Double.NaN;

        // CHECK: 23.625
        Printer.println(mul(10.5, 2.25));
        // CHECK-NEXT: -23.625
        Printer.println(mul(10.5, -2.25));
        // CHECK-NEXT: -23.625
        Printer.println(mul(-10.5, 2.25));
        // CHECK-NEXT: 23.625
        Printer.println(mul(-10.5, -2.25));
        // CHECK-NEXT: nan
        Printer.println(mul(nan, 2.25));
        // CHECK-NEXT: inf
        Printer.println(mul(10.5, Double.POSITIVE_INFINITY));
        // CHECK-NEXT: -inf
        Printer.println(mul(-10.5, Double.POSITIVE_INFINITY));
        // CHECK-NEXT: -inf
        Printer.println(mul(10.5, Double.NEGATIVE_INFINITY));
        // CHECK-NEXT: inf
        Printer.println(mul(-10.5, Double.NEGATIVE_INFINITY));
        // CHECK-NEXT: inf
        Printer.println(mul(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY));
        // CHECK-NEXT: -inf
        Printer.println(mul(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY));
        // CHECK-NEXT: inf
        Printer.println(mul(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY));
        // CHECK-NEXT: inf
        Printer.println(mul(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY));
        // CHECK-NEXT: -nan
        Printer.println(mul(Double.POSITIVE_INFINITY, 0.0));
        // CHECK-NEXT: -nan
        Printer.println(mul(Double.NEGATIVE_INFINITY, 0.0));
    }

    public static double mul(double x, double y) {
        return x * y;
    }

}
