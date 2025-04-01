// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/StaticFields#main([Ljava/lang/String;)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class StaticFields {

    private static int sum = 100;
    private static int inc = 2;

    public static void main(String[] args) {
        // CHECK: 100
        Printer.println(sum);

        sum = 200;
        // CHECK-NEXT: 200
        Printer.println(sum);

        sum += inc;
        // CHECK-NEXT: 202
        Printer.println(sum);
    }

}
