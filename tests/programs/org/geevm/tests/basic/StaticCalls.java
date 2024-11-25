// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class StaticCalls {

    public static void main(String[] args) {
        int start = 0;
        int end = 10;
        int sum = 0;

        for (int i = start; i < max(); i++) {
            sum = inc(sum, i);
        }

        // CHECK: 15
        Printer.println(sum);
    }

    private static int max() {
        return 10;
    }

    private static int inc(int v, int i) {
        if (i % 2 == 0) {
            return v + 1;
        }
        return v + 2;
    }

}
