// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.arrays;

import org.geevm.util.Printer;

public class MultiDimensionalArrayFiveDim {

    public static void main(String[] args) {
        int[][][][][] t = new int[9][8][7][6][5];
        for (int i = 0; i < t.length; i++) {
            for (int j = 0; j < t[i].length; j++) {
                for (int k = 0; k < t[i][j].length; k++) {
                    for (int l = 0; l < t[i][j][k].length; l++) {
                        for (int m = 0; m < t[i][j][k][l].length; m++) {
                            t[i][j][k][l][m] = i * 10000 + j * 1000 + k * 100 + l * 10 + m;
                        }
                    }
                }
            }
        }

        Printer.println(t[5][4][3][2][1]);
        // CHECK: 54321
    }

}