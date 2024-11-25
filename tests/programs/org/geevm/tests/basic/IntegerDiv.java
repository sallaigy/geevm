// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerDiv {

    public static void main(String[] args) {
        // CHECK: 3
        Printer.println(div(10, 3));
        // CHECK-NEXT: -3
        Printer.println(div(10, -3));
        // CHECK-NEXT: -3
        Printer.println(div(-10, 3));
        // CHECK-NEXT: 3
        Printer.println(div(-10, -3));
    }

    public static int div(int x, int y) {
        return x / y;
    }

}
