// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class IntegerSub {

    public static void main(String[] args) {
        // CHECK: 7
        Printer.println(sub(10, 3));
        // CHECK: 2147483647
        Printer.println(sub(Integer.MIN_VALUE, 1));
    }

    public static int sub(int x, int y) {
        return x - y;
    }

}
