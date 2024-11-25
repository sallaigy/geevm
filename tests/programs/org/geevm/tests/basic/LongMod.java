// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongMod {

    public static void main(String[] args) {
        // CHECK: 1
        Printer.println(mod(10, 3));
        // CHECK-NEXT: 1
        Printer.println(mod(10, -3));
        // CHECK-NEXT: -1
        Printer.println(mod(-10, 3));
        // CHECK-NEXT: -1
        Printer.println(mod(-10, -3));
    }

    public static long mod(long x, long y) {
        return x % y;
    }

}
