// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/LongDiv#div(JJ)J" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongDiv {

    public static void main(String[] args) {
        // CHECK: 3
        Printer.println(div(10, 3));
        // CHECK-NEXT: -3
        Printer.println(div(10, -3));
        // CHECK-NEXT: -3
        Printer.println(div(-10, 3));
        // CHECK-NEXT: 3
        Printer.println(div(-10, -3));
    }

    public static long div(long x, long y) {
        return x / y;
    }

}
