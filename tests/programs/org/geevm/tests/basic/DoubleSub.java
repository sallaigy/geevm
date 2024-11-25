// RUN: %compile -d %t "%s" | FileCheck "%s"
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
