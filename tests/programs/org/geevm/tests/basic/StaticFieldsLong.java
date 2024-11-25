// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class StaticFieldsLong {

    private static final long START = 1;
    private static final long END = 100;
    private static long inc = 1;

    public static void main(String[] args) {
        long sum = 0;

        for (long i = START; i < END; i++) {
            sum += inc;
            if (i % 10 == 0) {
                inc += 1;
            }
        }

        // CHECK: 540
        Printer.println(sum);
    }

}
