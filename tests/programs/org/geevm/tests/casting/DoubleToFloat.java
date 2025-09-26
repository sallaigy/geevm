// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/casting/DoubleToFloat#castAndPrint(D)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class DoubleToFloat {

    public static void main(String[] args) {
        // CHECK: 42.9
        castAndPrint(42.9);
        // CHECK-NEXT: -42.9
        castAndPrint(-42.9);
        // CHECK-NEXT: 0
        castAndPrint(0.0);
        // CHECK-NEXT: 0
        castAndPrint(Double.MIN_VALUE);
        // CHECK-NEXT: inf
        castAndPrint(Double.MAX_VALUE);
        // CHECK-NEXT: -inf
        castAndPrint(-Double.MAX_VALUE);
        // CHECK-NEXT: inf
        castAndPrint(Double.POSITIVE_INFINITY);
        // CHECK-NEXT: -inf
        castAndPrint(Double.NEGATIVE_INFINITY);
        // CHECK-NEXT: nan
        castAndPrint(Double.NaN);
        // CHECK-NEXT: 0.1
        castAndPrint(0.1);
    }

    public static void castAndPrint(double val) {
        Printer.println((float) val);
    }
}
