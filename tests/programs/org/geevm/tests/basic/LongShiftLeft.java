// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/LongShiftLeft#calculateShift(JJ)J" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongShiftLeft {

    public static void main(String[] args) {
        // CHECK: 2
        Printer.println(calculateShift(1, 1));
        // CHECK-NEXT: 8
        Printer.println(calculateShift(1, 3));
        // CHECK-NEXT: 1
        Printer.println(calculateShift(1, 0));
        // CHECK-NEXT: -4
        Printer.println(calculateShift(-2, 1));
        // CHECK-NEXT: 4294967296
        Printer.println(calculateShift(1, 32));
        // CHECK-NEXT: 8589934592
        Printer.println(calculateShift(1, 33));
        // CHECK-NEXT: 2147483648
        Printer.println(calculateShift(1 << 30, 1));
        // CHECK-NEXT: 1
        Printer.println(calculateShift(1, 64));
        // CHECK-NEXT: 2
        Printer.println(calculateShift(1, 65));
        // CHECK-NEXT: 0
        Printer.println(calculateShift(0, 10));
        // CHECK-NEXT: -2
        Printer.println(calculateShift(Long.MAX_VALUE, 1));
        // CHECK-NEXT: 0
        Printer.println(calculateShift(Long.MIN_VALUE, 1));
    }

    public static long calculateShift(long x, long y) {
        return x << y;
    }

}
