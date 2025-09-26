// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/DoubleMod#mod(DD)D" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class DoubleMod {

    public static void main(String[] args) {
        double nan = Double.NaN;

        // CHECK: 1.5
        Printer.println(mod(10.5, 2.25));
        // CHECK-NEXT: 1.5
        Printer.println(mod(10.5, -2.25));
        // CHECK-NEXT: -1.5
        Printer.println(mod(-10.5, 2.25));
        // CHECK-NEXT: -1.5
        Printer.println(mod(-10.5, -2.25));

    }

    public static double mod(double x, double y) {
        return x % y;
    }

}
