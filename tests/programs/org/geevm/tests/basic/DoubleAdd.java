// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/DoubleAdd#add(DD)D" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class DoubleAdd {

    public static void main(String[] args) {
        double nan = Double.NaN;

        // CHECK: 12.75
        Printer.println(add(10.5, 2.25));
        // CHECK-NEXT: nan
        Printer.println(add(nan, 2.25));
        // CHECK-NEXT: nan
        Printer.println(add(10.5, nan));
        // CHECK-NEXT: -nan
        Printer.println(add(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY));
        // CHECK-NEXT: inf
        Printer.println(add(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY));
        // CHECK-NEXT: -inf
        Printer.println(add(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY));
        // CHECK-NEXT: 0
        Printer.println(add(+0.0, -0.0));
        // CHECK-NEXT: 0
        Printer.println(add(+0.0, +0.0));
        // CHECK-NEXT: -0
        Printer.println(add(-0.0, -0.0));
        // CHECK-NEXT: 2.25
        Printer.println(add(+0.0, 2.25));
        // CHECK-NEXT: 2.25
        Printer.println(add(-0.0, 2.25));
        // CHECK-NEXT: 0
        Printer.println(add(-2.25, 2.25));
        // CHECK-NEXT: nan
        Printer.println(add(10.5, nan));
    }

    public static double add(double x, double y) {
        return x + y;
    }

}
