// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/FloatSub#sub(FF)F" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class FloatSub {

    public static void main(String[] args) {
        float nan = Float.NaN;

        // CHECK: 8.25
        Printer.println(sub(10.5f, 2.25f));
        // CHECK-NEXT: nan
        Printer.println(sub(nan, 2.25f));
        // CHECK-NEXT: nan
        Printer.println(sub(10.5f, nan));
        // CHECK-NEXT: inf
        Printer.println(sub(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY));
        // CHECK-NEXT: -nan
        Printer.println(sub(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY));
        // CHECK-NEXT: nan
        Printer.println(sub(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY));
        // CHECK-NEXT: 0
        Printer.println(sub(+0.0f, -0.0f));
        // CHECK-NEXT: 0
        Printer.println(sub(+0.0f, +0.0f));
        // CHECK-NEXT: 0
        Printer.println(sub(-0.0f, -0.0f));
        // CHECK-NEXT: -2.25
        Printer.println(sub(+0.0f, 2.25f));
        // CHECK-NEXT: -2.25
        Printer.println(sub(-0.0f, 2.25f));
        // CHECK-NEXT: -4.5
        Printer.println(sub(-2.25f, 2.25f));
        // CHECK-NEXT: nan
        Printer.println(sub(10.5f, nan));
    }

    public static float sub(float x, float y) {
        return x - y;
    }

}
