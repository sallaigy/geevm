// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.arrays;

import org.geevm.util.Printer;

public class CharArrays {

    public static void main(String[] args) {
        char[] t = new char[10];
        for (int i = 0; i < t.length; i++) {
            t[i] = (char) ('0' + i);
        }

        for (int i = 0; i < t.length; i++) {
            Printer.println(t[i]);
        }
        // CHECK: 0
        // CHECK-NEXT: 1
        // CHECK-NEXT: 2
        // CHECK-NEXT: 3
        // CHECK-NEXT: 4
        // CHECK-NEXT: 5
        // CHECK-NEXT: 6
        // CHECK-NEXT: 7
        // CHECK-NEXT: 8
        // CHECK-NEXT: 9
    }

}