// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class FloatAdd {

    public static void main(String[] args) {
        float nan = Float.NaN;

        // CHECK: 12.75
        Printer.println(add(10.5f, 2.25f));
        // CHECK-NEXT: nan
        Printer.println(add(nan, 2.25f));
        // CHECK-NEXT: nan
        Printer.println(add(10.5f, nan));
        // CHECK-NEXT: -nan
        Printer.println(add(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY));
        // CHECK-NEXT: inf
        Printer.println(add(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY));
        // CHECK-NEXT: -inf
        Printer.println(add(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY));
        // CHECK-NEXT: 0
        Printer.println(add(+0.0f, -0.0f));
        // CHECK-NEXT: 0
        Printer.println(add(+0.0f, +0.0f));
        // CHECK-NEXT: -0
        Printer.println(add(-0.0f, -0.0f));
        // CHECK-NEXT: 2.25
        Printer.println(add(+0.0f, 2.25f));
        // CHECK-NEXT: 2.25
        Printer.println(add(-0.0f, 2.25f));
        // CHECK-NEXT: 0
        Printer.println(add(-2.25f, 2.25f));
        // CHECK-NEXT: nan
        Printer.println(add(10.5f, nan));
    }

    public static float add(float x, float y) {
        return x + y;
    }

}
