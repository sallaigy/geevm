// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/FloatDiv#div(FF)F" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class FloatDiv {

    public static void main(String[] args) {
        float nan = Float.NaN;

        // CHECK: 4.66667
        Printer.println(div(10.5f, 2.25f));
        // CHECK-NEXT: nan
        Printer.println(div(nan, 2.25f));
        // CHECK-NEXT: nan
        Printer.println(div(10.5f, nan));
        // CHECK-NEXT: -nan
        Printer.println(div(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY));
        // CHECK-NEXT: -0
        Printer.println(div(10.5f, Float.NEGATIVE_INFINITY));
        // CHECK-NEXT: 0
        Printer.println(div(10.5f, Float.POSITIVE_INFINITY));
        // CHECK-NEXT: inf
        Printer.println(div(Float.POSITIVE_INFINITY, 2.25f));
        // CHECK-NEXT: -inf
        Printer.println(div(Float.NEGATIVE_INFINITY, 2.25f));
    }

    public static float div(float x, float y) {
        return x / y;
    }

}
