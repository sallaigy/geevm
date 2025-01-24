// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.exceptions;

import org.geevm.util.Printer;

public class UncaughtException {
    public static void main(String[] args) {
        callee();
        // CHECK: Exception in thread "main" java.lang.IllegalStateException: Exception thrown in callee.
        // CHECK-NEXT: at org.geevm.tests.exceptions.UncaughtException.callee(UncaughtException.java:16)
        // CHECK-NEXT: at org.geevm.tests.exceptions.UncaughtException.main(UncaughtException.java:8)
    }

    public static void callee() {
        if (condition()) {
            throw new IllegalStateException("Exception thrown in callee.");
        }
    }

    public static boolean condition() {
        return true;
    }
}
