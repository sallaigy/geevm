// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class IntToFloat {

    public static void main(String[] args) {
        // CHECK: 12345
        castAndPrint(12345);
        // CHECK-NEXT: -12345
        castAndPrint(-12345);
        // CHECK-NEXT: 1.67772e+07
        castAndPrint(16777216);
        // CHECK-NEXT: 1.67772e+07
        castAndPrint(16777217);
        // CHECK-NEXT: 2.14748e+09
        castAndPrint(Integer.MAX_VALUE);
        // CHECK-NEXT: -2.14748e+09
        castAndPrint(Integer.MIN_VALUE);
    }

    public static void castAndPrint(int val) {
        Printer.println((float) val);
    }
}
