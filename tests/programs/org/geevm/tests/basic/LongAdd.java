// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongAdd {

    public static void main(String[] args) {
        // CHECK: 13
        Printer.println(add(10, 3));
        // CHECK: -9223372036854775808
        Printer.println(add(Long.MAX_VALUE, 1));
    }

    public static long add(long x, long y) {
        return x + y;
    }

}
