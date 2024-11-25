// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class FloatMod {

    public static void main(String[] args) {
        float nan = Float.NaN;

        // CHECK: 1.5
        Printer.println(mod(10.5f, 2.25f));
        // CHECK-NEXT: 1.5
        Printer.println(mod(10.5f, -2.25f));
        // CHECK-NEXT: -1.5
        Printer.println(mod(-10.5f, 2.25f));
        // CHECK-NEXT: -1.5
        Printer.println(mod(-10.5f, -2.25f));
    }

    public static float mod(float x, float y) {
        return x % y;
    }

}
