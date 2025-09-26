// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/casting/FloatToInt#castAndPrint(F)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class FloatToInt {

    public static void main(String[] args) {
        // CHECK: 42
        castAndPrint(42.9f);;
        // CHECK-NEXT: -42
        castAndPrint(-42.9f);
        // CHECK-NEXT: 0
        castAndPrint(0.0f);
        // CHECK-NEXT: 2147483647
        castAndPrint(Float.MAX_VALUE);
        // CHECK-NEXT: 0
        castAndPrint(Float.MIN_VALUE);
        // CHECK-NEXT: -2147483648
        castAndPrint(-Float.MAX_VALUE);
        // CHECK-NEXT: 2147483647
        castAndPrint(Float.POSITIVE_INFINITY);
        // CHECK-NEXT: -2147483648
        castAndPrint(Float.NEGATIVE_INFINITY);
        // CHECK-NEXT: 0
        castAndPrint(Float.NaN);
        // CHECK-NEXT: 2147483647
        castAndPrint(2_147_483_647.5f);
        // CHECK-NEXT: -2147483648
        castAndPrint(-2_147_483_648.5f);
    }

    public static void castAndPrint(float val) {
        Printer.println((int) val);
    }
}
