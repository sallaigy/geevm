// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerNeg {

    public static void main(String[] args) {
        // CHECK: -10
        Printer.println(neg(10));
        // CHECK-NEXT: 10
        Printer.println(neg(-10));
        // CHECK-NEXT: 0
        Printer.println(neg(0));
        // CHECK-NEXT: -2147483648
        Printer.println(neg(Integer.MIN_VALUE));
        // CHECK-NEXT: -2147483647
        Printer.println(neg(Integer.MAX_VALUE));
    }

    public static int neg(int x) {
        return -x;
    }

}
