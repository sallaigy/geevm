// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/casting/LongToInt#calculateCast(J)I" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class LongToInt {

    public static void main(String[] args) {
        // CHECK: 100
        Printer.println(calculateCast(100));
        // CHECK-NEXT: 2147483647
        Printer.println(calculateCast((((long) Integer.MIN_VALUE) - 1)));
        // CHECK-NEXT: -2147483648
        Printer.println(calculateCast((((long) Integer.MAX_VALUE) + 1)));
        // CHECK-NEXT: 0
        Printer.println(calculateCast(Long.MIN_VALUE));
        // CHECK-NEXT: -1
        Printer.println(calculateCast(Long.MAX_VALUE));
    }

    public static int calculateCast(long val) {
        return (int) val;
    }

}
