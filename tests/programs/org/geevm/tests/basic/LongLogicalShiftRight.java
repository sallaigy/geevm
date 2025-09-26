// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/LongLogicalShiftRight#calculateShift(JJ)J" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongLogicalShiftRight {

    public static void main(String[] args) {
        // CHECK: 0
        Printer.println(calculateShift(1, 1));
        // CHECK-NEXT: 1
        Printer.println(calculateShift(3, 1));
        // CHECK-NEXT: 42
        Printer.println(calculateShift(42, 0));
        // CHECK-NEXT: 9223372036854775807
        Printer.println(calculateShift(-1, 1));
        // CHECK-NEXT: 9223372036854775805
        Printer.println(calculateShift(-5, 1));
        // CHECK-NEXT: 0
        Printer.println(calculateShift(0, 5));
        // CHECK-NEXT: 0
        Printer.println(calculateShift(1, 32));
        // CHECK-NEXT: 4294967295
        Printer.println(calculateShift(-1, 32));
        // CHECK-NEXT: 1
        Printer.println(calculateShift(1, 64));
        // CHECK-NEXT: -1
        Printer.println(calculateShift(-1, 64));
        // CHECK-NEXT: 4611686018427387903
        Printer.println(calculateShift(Long.MAX_VALUE, 1));
        // CHECK-NEXT: 4611686018427387904
        Printer.println(calculateShift(Long.MIN_VALUE, 1));
    }

    public static long calculateShift(long x, long y) {
        return x >>> y;
    }

}
