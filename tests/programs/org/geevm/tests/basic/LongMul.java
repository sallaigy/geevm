// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongMul {

    public static void main(String[] args) {
        // CHECK: 30
        Printer.println(mul(10, 3));
    }

    public static long mul(long x, long y) {
        return x * y;
    }

}
