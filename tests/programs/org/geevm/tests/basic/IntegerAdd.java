// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerAdd {

    public static void main(String[] args) {
        // CHECK: 13
        Printer.println(add(10, 3));
        // CHECK: -2147483648
        Printer.println(add(Integer.MAX_VALUE, 1));
    }

    public static int add(int x, int y) {
        return x + y;
    }

}
