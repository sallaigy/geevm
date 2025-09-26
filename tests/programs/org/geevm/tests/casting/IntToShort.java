// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/casting/IntToShort#castAndPrint(I)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class IntToShort {

    public static void main(String[] args) {
        int normalValue = 100;
        castAndPrint(normalValue); // CHECK: 100

        int overflowValue = 35000;
        castAndPrint(overflowValue); // CHECK-NEXT: -30536

        int underflowValue = -35000;
        castAndPrint(underflowValue); // CHECK-NEXT: 30536
    }

    public static void castAndPrint(int val) {
        Printer.println((short) val);
    }
}
