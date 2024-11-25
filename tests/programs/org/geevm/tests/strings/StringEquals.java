// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.strings;

import org.geevm.util.Printer;

public class StringEquals {
    public static void main(String[] args) {
        // CHECK: true
        Printer.println("abc" == "abc");
        // CHECK-NEXT: false
        Printer.println("abc" != "abc");
        // CHECK-NEXT: false
        Printer.println("abc" == "xyz");
        // CHECK-NEXT: true
        Printer.println("abc" != "xyz");
        // CHECK-NEXT: true
        Printer.println("abc".equals("abc"));
        // CHECK-NEXT: false
        Printer.println("abc".equals("xyz"));
        // CHECK-NEXT: true
        Printer.println("abc" == ("a" + "b" + "c").intern());
        // CHECK-NEXT: false
        Printer.println("abc" == ("x" + "y" + "z").intern());
    }
}
