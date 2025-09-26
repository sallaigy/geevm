// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/Constants#main([Ljava/lang/String;)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class Constants {

    public static void main(String[] args) {
        // CHECK: 1
        Printer.println(1);
        // CHECK-NEXT: -1
        Printer.println(-1);
        // CHECK-NEXT: -2
        Printer.println(-2);
        // CHECK-NEXT: 127
        Printer.println(127);
        // CHECK-NEXT: -127
        Printer.println(-127);
        // CHECK-NEXT: 128
        Printer.println(128);
        // CHECK-NEXT: 32767
        Printer.println(32767);
        // CHECK-NEXT: -32768
        Printer.println(-32768);
    }

}
