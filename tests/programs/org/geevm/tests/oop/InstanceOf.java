// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/oop/InstanceOf#main([Ljava/lang/String;)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.oop;

import org.geevm.util.Printer;

public class InstanceOf {

    public static void main(String[] args) {
        Object value = "";

        // CHECK: true
        Printer.println(value instanceof String);
        // CHECK-NEXT: true
        Printer.println(value instanceof Object);
        // CHECK-NEXT: false
        Printer.println(value instanceof InstanceOf);
    }

}
