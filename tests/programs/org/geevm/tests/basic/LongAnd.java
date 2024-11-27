// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongAnd {

    public static void main(String[] args) {
        // CHECK: 0
        andAndPrint(0, 1);
        // CHECK-NEXT: 1
        andAndPrint(1, 1);
        // CHECK-NEXT: 0
        andAndPrint(2, 1);
        // CHECK-NEXT: 2
        andAndPrint(2, 2);
        // CHECK-NEXT: 2
        andAndPrint(3, 2);
        // CHECK-NEXT: 8
        andAndPrint(12, 8);
        // CHECK-NEXT: 0
        andAndPrint(0, 15);
        // CHECK-NEXT: 0
        andAndPrint(10, 5);
        // CHECK-NEXT: -16
        andAndPrint(-16, -16);
        // CHECK-NEXT: 0
        andAndPrint(-16, 15);
    }

    public static void andAndPrint(long x, long y) {
        Printer.println(x & y);
    }

}
