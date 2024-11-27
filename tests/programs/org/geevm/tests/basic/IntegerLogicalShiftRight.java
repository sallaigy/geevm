// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerLogicalShiftRight {

    public static void main(String[] args) {
        // CHECK: 0
        shiftAndPrint(1, 1);
        // CHECK-NEXT: 1
        shiftAndPrint(3, 1);
        // CHECK-NEXT: 42
        shiftAndPrint(42, 0);
        // CHECK-NEXT: 2147483647
        shiftAndPrint(-1, 1);
        // CHECK-NEXT: 2147483645
        shiftAndPrint(-5, 1);
        // CHECK-NEXT: 0
        shiftAndPrint(0, 5);
        // CHECK-NEXT: 1
        shiftAndPrint(1, 32);
        // CHECK-NEXT: -1
        shiftAndPrint(-1, 32);
        // CHECK-NEXT: 1073741823
        shiftAndPrint(Integer.MAX_VALUE, 1);
        // CHECK-NEXT: 1073741824
        shiftAndPrint(Integer.MIN_VALUE, 1);
    }
    public static void shiftAndPrint(int x, int y) {
        Printer.println(x >>> y);
    }

}
