// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/IntegerShiftLeft#calculateShift(II)I" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerShiftLeft {

    public static void main(String[] args) {
        // CHECK: 2
        Printer.println(calculateShift(1, 1));
        // CHECK-NEXT: 8
        Printer.println(calculateShift(1, 3));
        // CHECK-NEXT: 1
        Printer.println(calculateShift(1, 0));
        // CHECK-NEXT: -4
        Printer.println(calculateShift(-2, 1));
        // CHECK-NEXT: 1
        Printer.println(calculateShift(1, 32));
        // CHECK-NEXT: 2
        Printer.println(calculateShift(1, 33));
        // CHECK-NEXT: -2147483648
        Printer.println(calculateShift(1 << 30, 1));
        // CHECK-NEXT: 0
        Printer.println(calculateShift(0, 10));
        // CHECK-NEXT: -2
        Printer.println(calculateShift(Integer.MAX_VALUE, 1));
        // CHECK-NEXT: 0
        Printer.println(calculateShift(Integer.MIN_VALUE, 1));
    }

    public static int calculateShift(int x, int y) {
        return x << y;
    }

}
