// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerCompareZero {

    public static void main(String[] args) {
        // CHECK: 1
        // CHECK-NEXT: 2
        // CHECK-NEXT: 5
        compare(1);
        // CHECK-NEXT: 2
        // CHECK-NEXT: 3
        // CHECK-NEXT: 4
        compare(0);
        // CHECK-NEXT: 0
        // CHECK-NEXT: 3
        // CHECK-NEXT: 5
        compare(-1);
    }

    private static void compare(int a) {
        if (a < 0) {
            Printer.println(0);
        }
        if (a > 0) {
            Printer.println(1);
        }
        if (a >= 0) {
            Printer.println(2);
        }
        if (a <= 0) {
            Printer.println(3);
        }
        if (a == 0) {
            Printer.println(4);
        }
        if (a != 0) {
            Printer.println(5);
        }
    }

}
