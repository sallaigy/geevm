// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongOr {

    public static void main(String[] args) {
        // CHECK: 1
        orAndPrint(0, 1);
        // CHECK-NEXT: 1
        orAndPrint(1, 1);
        // CHECK-NEXT: 3
        orAndPrint(2, 1);
        // CHECK-NEXT: 2
        orAndPrint(2, 2);
        // CHECK-NEXT: 3
        orAndPrint(3, 2);
        // CHECK-NEXT: 12
        orAndPrint(12, 8);
        // CHECK-NEXT: 15
        orAndPrint(0, 15);
        // CHECK-NEXT: 15
        orAndPrint(10, 5);
        // CHECK-NEXT: -16
        orAndPrint(-16, -16);
        // CHECK-NEXT: -1
        orAndPrint(-16, 15);
    }

    public static void orAndPrint(long x, long y) {
        Printer.println(x | y);
    }

}
