// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.arrays;

import org.geevm.util.Printer;

public class MultiDimensionalObjectArray {

    private static class MyClass {
        public int x;

        public MyClass(int x) {
            this.x = x;
        }
    }

    public static void main(String[] args) {
        MyClass[][][] t = new MyClass[3][3][3];
        for (int i = 0; i < t.length; i++) {
            for (int j = 0; j < t[i].length; j++) {
                for (int k = 0; k < t[i][j].length; k++) {
                    t[i][j][k] = new MyClass(i * 1000 + j * 100 + k * 10);
                }
            }
        }

        Printer.println(t[2][1][2].x);
        // CHECK: 2120
    }

}