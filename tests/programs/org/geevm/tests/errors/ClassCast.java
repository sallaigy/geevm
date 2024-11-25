// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.errors;

import org.geevm.util.Printer;

public class ClassCast {
    public static void main(String[] args) {
        Object x = new ClassCast();
        Printer.println((String) x);
    // CHECK: Exception java.lang.ClassCastException: 'class org.geevm.tests.errors.ClassCast cannot be cast to class java.lang.String'
    // CHECK-NEXT: at org.geevm.tests.errors.ClassCast.main(Unknown Source)
    }
}
