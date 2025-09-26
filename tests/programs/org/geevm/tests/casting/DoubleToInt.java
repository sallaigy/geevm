// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/casting/DoubleToInt#castAndPrint(D)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class DoubleToInt {

    public static void main(String[] args) {
        // CHECK: 42
        castAndPrint(42.9);
        // CHECK-NEXT: -42
        castAndPrint(-42.9);
        // CHECK-NEXT: 0
        castAndPrint(0.0f);
        // CHECK-NEXT: 2147483647
        castAndPrint(Double.MAX_VALUE);
        // CHECK-NEXT: 0
        castAndPrint(Double.MIN_VALUE);
        // CHECK-NEXT: -2147483648
        castAndPrint(-Double.MAX_VALUE);
        // CHECK-NEXT: 2147483647
        castAndPrint(Double.POSITIVE_INFINITY);
        // CHECK-NEXT: -2147483648
        castAndPrint(Double.NEGATIVE_INFINITY);
        // CHECK-NEXT: 0
        castAndPrint(Double.NaN);
        // CHECK-NEXT: 2147483647
        castAndPrint(2_147_483_647.5);
        // CHECK-NEXT: -2147483648
        castAndPrint(-2_147_483_648.5);
    }

    public static void castAndPrint(double val) {
        Printer.println((int) val);
    }
}
