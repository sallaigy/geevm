// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/IntegerComparisons#compare(II)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerComparisons {

    public static void main(String[] args) {
         // CHECK: a < b
         // CHECK-NEXT: a <= b
         // CHECK-NEXT: a != b
        compare(0, 1);
         // CHECK-NEXT: a > b
         // CHECK-NEXT: a >= b
         // CHECK-NEXT: a != b
        compare(1, 0);
         // CHECK-NEXT: a >= b
         // CHECK-NEXT: a <= b
         // CHECK-NEXT: a == b
        compare(1, 1);
    }

    private static void compare(int a, int b) {
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
