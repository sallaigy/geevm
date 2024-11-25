// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.arrays;

import org.geevm.util.Printer;

public class DoubleArrays {

    public static void main(String[] args) {
        double[] t = new double[10];
        for (int i = 0; i < t.length; i++) {
            t[i] = 1.5 * i;
        }

        for (int i = 0; i < t.length; i++) {
            Printer.println(t[i]);
        }
        // CHECK: 0
        // CHECK-NEXT: 1.5
        // CHECK-NEXT: 3
        // CHECK-NEXT: 4.5
        // CHECK-NEXT: 6
        // CHECK-NEXT: 7.5
        // CHECK-NEXT: 9
        // CHECK-NEXT: 10.5
        // CHECK-NEXT: 12
        // CHECK-NEXT: 13.5
    }
}
