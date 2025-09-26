// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/IntegerLogicalShiftRight#calculateShift(II)I" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerLogicalShiftRight {

    public static void main(String[] args) {
        // CHECK: 0
        Printer.println(calculateShift(1, 1));
        // CHECK-NEXT: 1
        Printer.println(calculateShift(3, 1));
        // CHECK-NEXT: 42
        Printer.println(calculateShift(42, 0));
        // CHECK-NEXT: 2147483647
        Printer.println(calculateShift(-1, 1));
        // CHECK-NEXT: 2147483645
        Printer.println(calculateShift(-5, 1));
        // CHECK-NEXT: 0
        Printer.println(calculateShift(0, 5));
        // CHECK-NEXT: 1
        Printer.println(calculateShift(1, 32));
        // CHECK-NEXT: -1
        Printer.println(calculateShift(-1, 32));
        // CHECK-NEXT: 1073741823
        Printer.println(calculateShift(Integer.MAX_VALUE, 1));
        // CHECK-NEXT: 1073741824
        Printer.println(calculateShift(Integer.MIN_VALUE, 1));
    }

    public static int calculateShift(int x, int y) {
        return x >>> y;
    }

}
