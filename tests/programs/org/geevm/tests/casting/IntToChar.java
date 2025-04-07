// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/casting/IntToChar#castAndPrint(I)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class IntToChar {

    public static void main(String[] args) {
        // CHECK: A
        int normalValue = 65;
        castAndPrint(normalValue);

        // CHECK-NEXT: B
        int overflowValue = 65602;
        castAndPrint(overflowValue);

        // CHECK-NEXT: ﾾ
        int underflowValue = -65602;
        castAndPrint(underflowValue);
    }

    public static void castAndPrint(int val) {
        Printer.println((char) val);
    }
}
