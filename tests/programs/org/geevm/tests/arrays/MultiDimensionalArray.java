// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.arrays;

import org.geevm.util.Printer;

public class MultiDimensionalArray {

    public static void main(String[] args) {
        int[][] t = new int[3][3];
        for (int i = 0; i < t.length; i++) {
            for (int j = 0; j < t[i].length; j++) {
                t[i][j] = i * 100 + j;
            }
        }

        for (int i = 0; i < t.length; i++) {
            for (int j = 0; j < t.length; j++) {
                Printer.println(t[i][j]);
            }
        }
        // CHECK: 0
        // CHECK-NEXT: 1
        // CHECK-NEXT: 2
        // CHECK-NEXT: 100
        // CHECK-NEXT: 101
        // CHECK-NEXT: 102
        // CHECK-NEXT: 200
        // CHECK-NEXT: 201
        // CHECK-NEXT: 202
    }

}