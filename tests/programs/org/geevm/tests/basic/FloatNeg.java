// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class FloatNeg {

    public static void main(String[] args) {
        // CHECK: -10.5
        Printer.println(neg(10.5f));
        // CHECK-NEXT: 10.5
        Printer.println(neg(-10.5f));
        // CHECK-NEXT: -0
        Printer.println(neg(+0.0f));
        // CHECK-NEXT: 0
        Printer.println(neg(-0.0f));
        // CHECK-NEXT: -1.4013e-45
        Printer.println(neg(Float.MIN_VALUE));
        // CHECK-NEXT: -3.40282e+38
        Printer.println(neg(Float.MAX_VALUE));
        // CHECK-NEXT: -nan
        Printer.println(neg(Float.NaN));
        // CHECK-NEXT: inf
        Printer.println(neg(Float.NEGATIVE_INFINITY));
        // CHECK-NEXT: -inf
        Printer.println(neg(Float.POSITIVE_INFINITY));
    }

    public static float neg(float x) {
        return -x;
    }

}
