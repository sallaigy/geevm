// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/casting/FloatToLong#castAndPrint(F)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class FloatToLong {

    public static void main(String[] args) {
        // CHECK: 42
        castAndPrint(42.9f);
        // CHECK-NEXT: -42
        castAndPrint(-42.9f);
        // CHECK-NEXT: 0
        castAndPrint(0.0f);
        // CHECK-NEXT: 9223372036854775807
        castAndPrint(Float.MAX_VALUE);
        // CHECK-NEXT: 0
        castAndPrint(Float.MIN_VALUE);
        // CHECK-NEXT: -9223372036854775808
        castAndPrint(-Float.MAX_VALUE);
        // CHECK-NEXT: 9223372036854775807
        castAndPrint(Float.POSITIVE_INFINITY);
        // CHECK-NEXT: -9223372036854775808
        castAndPrint(Float.NEGATIVE_INFINITY);
        // CHECK-NEXT: 0
        castAndPrint(Float.NaN);
        // CHECK-NEXT: 9223372036854775807
        castAndPrint(9_223_372_036_854_775_807.5f);
        // CHECK-NEXT: -9223372036854775808
        castAndPrint(-9_223_372_036_854_775_808.5f);
    }

    public static void castAndPrint(float val) {
        Printer.println((long) val);
    }
}
