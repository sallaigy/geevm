// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/DoubleDiv#div(DD)D" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class DoubleDiv {

    public static void main(String[] args) {
        double nan = Double.NaN;

        // CHECK: 4.66667
        Printer.println(div(10.5, 2.25));
        // CHECK-NEXT: nan
        Printer.println(div(nan, 2.25));
        // CHECK-NEXT: nan
        Printer.println(div(10.5, nan));
        // CHECK-NEXT: -nan
        Printer.println(div(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY));
        // CHECK-NEXT: -0
        Printer.println(div(10.5, Double.NEGATIVE_INFINITY));
        // CHECK-NEXT: 0
        Printer.println(div(10.5, Double.POSITIVE_INFINITY));
        // CHECK-NEXT: inf
        Printer.println(div(Double.POSITIVE_INFINITY, 2.25));
        // CHECK-NEXT: -inf
        Printer.println(div(Double.NEGATIVE_INFINITY, 2.25));
    }

    public static double div(double x, double y) {
        return x / y;
    }

}
