// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/FloatComparisons#compare(FF)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class FloatComparisons {

    public static void main(String[] args) {
        // CHECK: a < b
        // CHECK-NEXT: a <= b
        // CHECK-NEXT: a != b
        compare(1.2f, 2.5f);
        // CHECK-NEXT: a > b
        // CHECK-NEXT: a >= b
        // CHECK-NEXT: a != b
        compare(2.5f, 1.2f);
        // CHECK-NEXT: a >= b
        // CHECK-NEXT: a <= b
        // CHECK-NEXT: a == b
        compare(1.2f, 1.2f);
        // CHECK-NEXT: a != b
        compare(Float.NaN, 1.f);
        // CHECK-NEXT: a != b
        compare(1.f, Float.NaN);
        // CHECK-NEXT: a != b
        compare(Float.NaN, Float.NaN);
    }

    private static void compare(float a, float b) {
        if (a < b) {
            Printer.println("a < b");
        }
        if (a > b) {
            Printer.println("a > b");
        }
        if (a >= b) {
            Printer.println("a >= b");
        }
        if (a <= b) {
            Printer.println("a <= b");
        }
        if (a == b) {
            Printer.println("a == b");
        }
        if (a != b) {
            Printer.println("a != b");
        }
    }

}
