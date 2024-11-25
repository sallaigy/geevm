// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class LongToInt {

    public static void main(String[] args) {
        // CHECK: 100
        castAndPrint(100);
        // CHECK-NEXT: 2147483647
        castAndPrint((((long) Integer.MIN_VALUE) - 1));
        // CHECK-NEXT: -2147483648
        castAndPrint((((long) Integer.MAX_VALUE) + 1));
        // CHECK-NEXT: 0
        castAndPrint(Long.MIN_VALUE);
        // CHECK-NEXT: -1
        castAndPrint(Long.MAX_VALUE);
    }

    public static void castAndPrint(long val) {
        Printer.println((int) val);
    }

}
