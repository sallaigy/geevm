// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/casting/IntToLong#castAndPrint(I)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class IntToLong {

    public static void main(String[] args) {
        int minVal = Integer.MIN_VALUE;
        int maxVal = Integer.MAX_VALUE;
        int basic = 100;

        castAndPrint(basic); // CHECK: 100
        castAndPrint(minVal); // CHECK-NEXT: -2147483648
        castAndPrint(maxVal); // CHECK-NEXT: 2147483647
    }

    public static void castAndPrint(int val) {
        Printer.println((long) val);
    }

}
