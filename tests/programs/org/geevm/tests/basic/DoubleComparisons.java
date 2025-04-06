// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/DoubleComparisons#compare(DD)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class DoubleComparisons {

    public static void main(String[] args) {
        // CHECK: a < b
        // CHECK-NEXT: a <= b
        // CHECK-NEXT: a != b
        compare(1.2, 2.5f);
        // CHECK-NEXT: a > b
        // CHECK-NEXT: a >= b
        // CHECK-NEXT: a != b
        compare(2.5f, 1.2);
        // CHECK-NEXT: a >= b
        // CHECK-NEXT: a <= b
        // CHECK-NEXT: a == b
        compare(1.2, 1.2);
        // CHECK-NEXT: a != b
        compare(Double.NaN, 1.f);
        // CHECK-NEXT: a != b
        compare(1.f, Double.NaN);
        // CHECK-NEXT: a != b
        compare(Double.NaN, Double.NaN);
    }

    private static void compare(double a, double b) {
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
