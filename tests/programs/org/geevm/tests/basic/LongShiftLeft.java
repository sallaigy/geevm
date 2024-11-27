// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongShiftLeft {

    public static void main(String[] args) {
        // CHECK: 2
        shiftAndPrint(1, 1);
        // CHECK-NEXT: 8
        shiftAndPrint(1, 3);
        // CHECK-NEXT: 1
        shiftAndPrint(1, 0);
        // CHECK-NEXT: -4
        shiftAndPrint(-2, 1);
        // CHECK-NEXT: 4294967296
        shiftAndPrint(1, 32);
        // CHECK-NEXT: 8589934592
        shiftAndPrint(1, 33);
        // CHECK-NEXT: 2147483648
        shiftAndPrint(1 << 30, 1);
        // CHECK-NEXT: 1
        shiftAndPrint(1, 64);
        // CHECK-NEXT: 2
        shiftAndPrint(1, 65);
        // CHECK-NEXT: 0
        shiftAndPrint(0, 10);
        // CHECK-NEXT: -2
        shiftAndPrint(Long.MAX_VALUE, 1);
        // CHECK-NEXT: 0
        shiftAndPrint(Long.MIN_VALUE, 1);
    }

    public static void shiftAndPrint(long x, long y) {
        Printer.println(x << y);
    }

}
