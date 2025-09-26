// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/LongSub#sub(JJ)J" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class LongSub {

    public static void main(String[] args) {
        // CHECK: 7
        Printer.println(sub(10, 3));
        // CHECK: 9223372036854775807
        Printer.println(sub(Long.MIN_VALUE, 1));
    }

    public static long sub(long x, long y) {
        return x - y;
    }

}
