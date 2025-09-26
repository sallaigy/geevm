// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/casting/DoubleToLong#castAndPrint(D)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class DoubleToLong {

    public static void main(String[] args) {
        // CHECK: 42
        castAndPrint(42.9f);
        // CHECK-NEXT: -42
        castAndPrint(-42.9f);
        // CHECK-NEXT: 0
        castAndPrint(0.0f);
        // CHECK-NEXT: 9223372036854775807
        castAndPrint(Double.MAX_VALUE);
        // CHECK-NEXT: 0
        castAndPrint(Double.MIN_VALUE);
        // CHECK-NEXT: -9223372036854775808
        castAndPrint(-Double.MAX_VALUE);
        // CHECK-NEXT: 9223372036854775807
        castAndPrint(Double.POSITIVE_INFINITY);
        // CHECK-NEXT: -9223372036854775808
        castAndPrint(Double.NEGATIVE_INFINITY);
        // CHECK-NEXT: 0
        castAndPrint(Double.NaN);
        // CHECK-NEXT: 9223372036854775807
        castAndPrint(9_223_372_036_854_775_807.5f);
        // CHECK-NEXT: -9223372036854775808
        castAndPrint(-9_223_372_036_854_775_808.5f);
    }

    public static void castAndPrint(double val) {
        Printer.println((long) val);
    }
}
