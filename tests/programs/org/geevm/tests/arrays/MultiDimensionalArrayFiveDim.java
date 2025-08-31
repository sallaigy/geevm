// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/arrays/MultiDimensionalArrayFiveDim#main([Ljava/lang/String;)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.arrays;

public class MultiDimensionalArrayFiveDim {

    public static void main(String[] args) {
        int[][][][][] t = new int[6][5][4][3][2];
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

        System.out.println(t[5][4][3][2][1]);
        // CHECK: 54321
    }

}
