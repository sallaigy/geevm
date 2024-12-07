// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.arrays;

import org.geevm.util.Printer;

public class BooleanArrays {

    public static void main(String[] args) {
        boolean[] t = new boolean[10];
        for (int i = 0; i < t.length; i++) {
            t[i] = i % 2 == 0 ? true : false;
        }

        for (int i = 0; i < t.length; i++) {
            Printer.println(t[i]);
        }
        // CHECK: true
        // CHECK-NEXT: false
        // CHECK-NEXT: true
        // CHECK-NEXT: false
        // CHECK-NEXT: true
        // CHECK-NEXT: false
        // CHECK-NEXT: true
        // CHECK-NEXT: false
        // CHECK-NEXT: true
        // CHECK-NEXT: false
    }
}
