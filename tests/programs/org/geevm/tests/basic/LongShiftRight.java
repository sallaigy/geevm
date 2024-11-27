// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongShiftRight {

    public static void main(String[] args) {
        // CHECK: 0
        shiftAndPrint(1, 1);
        // CHECK-NEXT: 1
        shiftAndPrint(3, 1);
        // CHECK-NEXT: 42
        shiftAndPrint(42, 0);
        // CHECK-NEXT: -1
        shiftAndPrint(-1, 1);
        // CHECK-NEXT: -3
        shiftAndPrint(-5, 1);
        // CHECK-NEXT: 0
        shiftAndPrint(0, 5);
        // CHECK-NEXT: 0
        shiftAndPrint(1, 32);
        // CHECK-NEXT: -1
        shiftAndPrint(-1, 32);
        // CHECK-NEXT: 1
        shiftAndPrint(1, 64);
        // CHECK-NEXT: -1
        shiftAndPrint(-1, 64);
        // CHECK-NEXT: 4611686018427387903
        shiftAndPrint(Long.MAX_VALUE, 1);
        // CHECK-NEXT: -4611686018427387904
        shiftAndPrint(Long.MIN_VALUE, 1);
    }
    public static void shiftAndPrint(long x, long y) {
        Printer.println(x >> y);
    }

}
