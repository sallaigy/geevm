// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerMul {

    public static void main(String[] args) {
        // CHECK: 30
        Printer.println(mul(10, 3));
    }

    public static int mul(int x, int y) {
        return x * y;
    }

}
