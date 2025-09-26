// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/ObjectComparisons#compare(Ljava/lang/Object;Ljava/lang/Object;)V,org/geevm/tests/basic/ObjectComparisons#compareToNull(Ljava/lang/Object;)V," "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class ObjectComparisons {
    public static void main(String[] args) {
         // CHECK: a != b
        compare("x", "y");
         // CHECK-NEXT: a != b
        compare("y", "x");
         // CHECK-NEXT: a == b
        compare("x", "x");
        // CHECK-NEXT: x is null
        compareToNull(null);
        // CHECK-NEXT: x is not null
        compareToNull("x");
    }

    private static void compare(Object a, Object b) {
        if (a == b) {
            Printer.println("a == b");
        }

        if (a != b) {
            Printer.println("a != b");
        }
    }

    private static void compareToNull(Object x) {
        if (x == null) {
            Printer.println("x is null");
        }

        if (x != null) {
            Printer.println("x is not null");
        }
    }
}
