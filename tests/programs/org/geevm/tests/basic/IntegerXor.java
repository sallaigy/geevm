// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerXor {

    public static void main(String[] args) {
        // CHECK: 1
        xorAndPrint(0, 1);
        // CHECK-NEXT: 0
        xorAndPrint(1, 1);
        // CHECK-NEXT: 3
        xorAndPrint(2, 1);
        // CHECK-NEXT: 0
        xorAndPrint(2, 2);
        // CHECK-NEXT: 1
        xorAndPrint(3, 2);
        // CHECK-NEXT: 4
        xorAndPrint(12, 8);
        // CHECK-NEXT: 15
        xorAndPrint(0, 15);
        // CHECK-NEXT: 15
        xorAndPrint(10, 5);
        // CHECK-NEXT: 0
        xorAndPrint(-16, -16);
        // CHECK-NEXT: -1
        xorAndPrint(-16, 15);
    }

    public static void xorAndPrint(int x, int y) {
        Printer.println(x ^ y);
    }

}
