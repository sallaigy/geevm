// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/errors/ClassCast#main([Ljava/lang/String;)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.errors;

import org.geevm.util.Printer;

public class ClassCast {
    public static void main(String[] args) {
        Object x = new ClassCast();
        Printer.println((String) x);
    // CHECK: Exception in thread "main" java.lang.ClassCastException: class org.geevm.tests.errors.ClassCast cannot be cast to class java.lang.String
    // CHECK-NEXT: at org.geevm.tests.errors.ClassCast.main(ClassCast.java:10)
    }
}
