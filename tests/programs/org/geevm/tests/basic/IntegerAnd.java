// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/IntegerAnd#calculateAnd(II)I" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerAnd {

    public static void main(String[] args) {
        // CHECK: 0
        Printer.println(calculateAnd(0, 1));
        // CHECK-NEXT: 1
        Printer.println(calculateAnd(1, 1));
        // CHECK-NEXT: 0
        Printer.println(calculateAnd(2, 1));
        // CHECK-NEXT: 2
        Printer.println(calculateAnd(2, 2));
        // CHECK-NEXT: 2
        Printer.println(calculateAnd(3, 2));
        // CHECK-NEXT: 8
        Printer.println(calculateAnd(12, 8));
        // CHECK-NEXT: 0
        Printer.println(calculateAnd(0, 15));
        // CHECK-NEXT: 0
        Printer.println(calculateAnd(10, 5));
        // CHECK-NEXT: -16
        Printer.println(calculateAnd(-16, -16));
        // CHECK-NEXT: 0
        Printer.println(calculateAnd(-16, 15));
    }

    public static int calculateAnd(int x, int y) {
        return x & y;
    }

}
