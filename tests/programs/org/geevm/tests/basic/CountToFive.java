// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/Basic/CountToFive#count()I" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class CountToFive {

    public static void main(String[] args) {
        int x = count();

        // CHECK: 5
        Printer.println(x);
    }

    private static int count() {
        int x = 0;
        x = x + 2;
        x = x * 2;
        x = x + 1;

        return x;
    }

}
