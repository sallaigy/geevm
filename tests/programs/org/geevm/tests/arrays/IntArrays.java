// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.arrays;

import org.geevm.util.Printer;

public class IntArrays {

    public static void main(String[] args) {
        int[] t = new int[10];
        for (int i = 0; i < t.length; i++) {
            t[i] = i * 2;
        }

        for (int i = 0; i < t.length; i++) {
            Printer.println(t[i]);
        }
        // CHECK: 0
        // CHECK-NEXT: 2
        // CHECK-NEXT: 4
        // CHECK-NEXT: 6
        // CHECK-NEXT: 8
        // CHECK-NEXT: 10
        // CHECK-NEXT: 12
        // CHECK-NEXT: 14
        // CHECK-NEXT: 16
        // CHECK-NEXT: 18
    }

}