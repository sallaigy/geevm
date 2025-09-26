// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/LongNeg#neg(J)J" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongNeg {

    public static void main(String[] args) {
        // CHECK: -10
        Printer.println(neg(10));
        // CHECK-NEXT: 10
        Printer.println(neg(-10));
        // CHECK-NEXT: 0
        Printer.println(neg(0));
        // CHECK-NEXT: -9223372036854775808
        Printer.println(neg(Long.MIN_VALUE));
        // CHECK-NEXT: -9223372036854775807
        Printer.println(neg(Long.MAX_VALUE));
    }

    public static long neg(long x) {
        return -x;
    }

}
