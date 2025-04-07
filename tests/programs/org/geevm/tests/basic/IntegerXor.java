// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/IntegerXor#calculateXor(II)I" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerXor {

    public static void main(String[] args) {
        // CHECK: 1
        Printer.println(calculateXor(0, 1));
        // CHECK-NEXT: 0
        Printer.println(calculateXor(1, 1));
        // CHECK-NEXT: 3
        Printer.println(calculateXor(2, 1));
        // CHECK-NEXT: 0
        Printer.println(calculateXor(2, 2));
        // CHECK-NEXT: 1
        Printer.println(calculateXor(3, 2));
        // CHECK-NEXT: 4
        Printer.println(calculateXor(12, 8));
        // CHECK-NEXT: 15
        Printer.println(calculateXor(0, 15));
        // CHECK-NEXT: 15
        Printer.println(calculateXor(10, 5));
        // CHECK-NEXT: 0
        Printer.println(calculateXor(-16, -16));
        // CHECK-NEXT: -1
        Printer.println(calculateXor(-16, 15));
    }

    public static int calculateXor(int x, int y) {
        return x ^ y;
    }

}
