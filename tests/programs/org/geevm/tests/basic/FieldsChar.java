// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/FieldsChar#check()V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class FieldsChar {

    private char sum = 'a';
    private int inc = 2;

    public static void main(String[] args) {
        FieldsChar instance = new FieldsChar();
        instance.check();
    }

    public void check() {
        // CHECK: a
        Printer.println(sum);

        sum = 'a';
        // CHECK-NEXT: a
        Printer.println(sum);

        sum += inc;
        // CHECK-NEXT: c
        Printer.println(sum);
    }
}
