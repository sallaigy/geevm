// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/Fields#check()V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class Fields {

    private int sum = 100;
    private int inc = 2;

    public static void main(String[] args) {
        Fields instance = new Fields();
        instance.check();
    }

    public void check() {
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
