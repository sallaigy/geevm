// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.casting;

import org.geevm.util.Printer;

public class IntToByte {

    public static void main(String[] args) {
        // CHECK: 100
        castAndPrint(100);
        // CHECK-NEXT: -126
        castAndPrint(130);
        // CHECK-NEXT: 126
        castAndPrint(-130);
    }

    public static void castAndPrint(int val) {
        Printer.println((byte) val);
    }
}
