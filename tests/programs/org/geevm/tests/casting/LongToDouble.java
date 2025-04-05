// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/casting/LongToDouble#castAndPrint(J)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class LongToDouble {

    public static void main(String[] args) {
        // CHECK: 12345
        // CHECK-NEXT: -12345
        // CHECK-NEXT: -2.14748e+09
        // CHECK-NEXT: 2.14748e+09
        // CHECK-NEXT: -9.22337e+18
        // CHECK-NEXT: 9.22337e+18
        castAndPrint(12345);
        castAndPrint(-12345);
        castAndPrint((((long) Integer.MIN_VALUE) - 1));
        castAndPrint((((long) Integer.MAX_VALUE) + 1));
        castAndPrint(Long.MIN_VALUE);
        castAndPrint(Long.MAX_VALUE);
    }

    public static void castAndPrint(long val) {
        Printer.println((double) val);
    }

}
