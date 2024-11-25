// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class StaticCallsLong {

    public static void main(String[] args) {
        long start = 0l;
        long end = 10l;
        long sum = 0l;

        for (long i = start; i < max(); i++) {
            sum = inc(sum, i);
        }

        // CHECK: 15
        Printer.println(sum);
    }

    private static long max() {
        return 10;
    }

    private static long inc(long v, long i) {
        if (i % 2 == 0) {
            return v + 1;
        }
        return v + 2;
    }

}
