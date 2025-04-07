// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/StaticFieldsLoop#main([Ljava/lang/String;)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class StaticFieldsLoop {

    private static final int START = 1;
    private static final int END = 100;
    private static int inc = 1;

    public static void main(String[] args) {
        int sum = 0;

        for (int i = START; i < END; i++) {
            sum += inc;
            if (i % 10 == 0) {
                inc += 1;
            }
        }

        // CHECK: 540
        Printer.println(sum);
    }

}
