// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/DoubleSub#sub(DD)D" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class DoubleSub {

    public static void main(String[] args) {
        // CHECK: 8.25
        Printer.println(sub(10.5, 2.25));
    }

    public static double sub(double x, double y) {
        return x - y;
    }

}
