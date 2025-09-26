// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/IntegerMul#mul(II)I" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerMul {

    public static void main(String[] args) {
        // CHECK: 30
        Printer.println(mul(10, 3));
        // CHECK-NEXT: 2
        Printer.println(mul(2147483647, 2));
    }

    public static int mul(int x, int y) {
        return x * y;
    }

}
