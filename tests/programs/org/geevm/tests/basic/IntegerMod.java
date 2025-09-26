// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/IntegerMod#mod(II)I" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerMod {

    public static void main(String[] args) {
        // CHECK: 1
        Printer.println(mod(10, 3));
        // CHECK-NEXT: 1
        Printer.println(mod(10, -3));
        // CHECK-NEXT: -1
        Printer.println(mod(-10, 3));
        // CHECK-NEXT: -1
        Printer.println(mod(-10, -3));
    }

    public static int mod(int x, int y) {
        return x % y;
    }

}
