// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/LongAnd#calculateAnd(JJ)J" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongAnd {

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

    public static long calculateAnd(long x, long y) {
        return x & y;
    }

}
