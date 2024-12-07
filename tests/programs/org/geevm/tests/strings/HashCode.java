// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.strings;

import org.geevm.util.Printer;

public class HashCode {
    public static void main(String[] args) {
        String x = "abc";
        String y = "xyz";
        Integer z = 10;

        Printer.println(x.hashCode() != 0);
        Printer.println(x.hashCode() == x.hashCode());
        Printer.println(x.hashCode() == y.hashCode());
        Printer.println(x.hashCode() == z.hashCode());

        // CHECK: true
        // CHECK-NEXT: true
        // CHECK-NEXT: false
        // CHECK-NEXT: false
    }
}
