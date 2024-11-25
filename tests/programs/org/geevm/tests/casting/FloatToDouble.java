// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class FloatToDouble {

    public static void main(String[] args) {
        // CHECK: 42.9
        castAndPrint(42.9f);
        // CHECK-NEXT: -42.9
        castAndPrint(-42.9f);
        // CHECK-NEXT: 0
        castAndPrint(0.0f);
        // CHECK-NEXT: 1.4013e-45
        castAndPrint(Float.MIN_VALUE);
        // CHECK-NEXT: 3.40282e+38
        castAndPrint(Float.MAX_VALUE);
        // CHECK-NEXT: -3.40282e+38
        castAndPrint(-Float.MAX_VALUE);
        // CHECK-NEXT: inf
        castAndPrint(Float.POSITIVE_INFINITY);
        // CHECK-NEXT: -inf
        castAndPrint(Float.NEGATIVE_INFINITY);
        // CHECK-NEXT: nan
        castAndPrint(Float.NaN);
        // CHECK-NEXT: 0.1
        castAndPrint(0.1f);
    }
    
    public static void castAndPrint(float val) {
        Printer.println((double) val);
    }
}
