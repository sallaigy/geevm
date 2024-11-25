// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerComparisons {

    public static void main(String[] args) {
         // CHECK: 0
         // CHECK-NEXT: 3
         // CHECK-NEXT: 5
        compare(0, 1);
         // CHECK-NEXT: 1
         // CHECK-NEXT: 2
         // CHECK-NEXT: 5
        compare(1, 0);
         // CHECK-NEXT: 2
         // CHECK-NEXT: 3
         // CHECK-NEXT: 4
        compare(1, 1);
    }

    private static void compare(int a, int b) {
        if (a < b) {
            Printer.println(0);
        }
        if (a > b) {
            Printer.println(1);
        }
        if (a >= b) {
            Printer.println(2);
        }
        if (a <= b) {
            Printer.println(3);
        }
        if (a == b) {
            Printer.println(4);
        }
        if (a != b) {
            Printer.println(5);
        }
    }

}
