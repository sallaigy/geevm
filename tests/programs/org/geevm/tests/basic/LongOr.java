// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/LongOr#calculateOr(JJ)J" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongOr {

    public static void main(String[] args) {
        // CHECK: 1
        Printer.println(calculateOr(0, 1));
        // CHECK-NEXT: 1
        Printer.println(calculateOr(1, 1));
        // CHECK-NEXT: 3
        Printer.println(calculateOr(2, 1));
        // CHECK-NEXT: 2
        Printer.println(calculateOr(2, 2));
        // CHECK-NEXT: 3
        Printer.println(calculateOr(3, 2));
        // CHECK-NEXT: 12
        Printer.println(calculateOr(12, 8));
        // CHECK-NEXT: 15
        Printer.println(calculateOr(0, 15));
        // CHECK-NEXT: 15
        Printer.println(calculateOr(10, 5));
        // CHECK-NEXT: -16
        Printer.println(calculateOr(-16, -16));
        // CHECK-NEXT: -1
        Printer.println(calculateOr(-16, 15));
    }

    public static long calculateOr(long x, long y) {
        return x | y;
    }

}
