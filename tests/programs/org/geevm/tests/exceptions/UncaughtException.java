// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.exceptions;

import org.geevm.util.Printer;

public class UncaughtException {
    public static void main(String[] args) {
        callee();
        // CHECK: Exception java.lang.IllegalStateException: 'Exception thrown in callee.'
        // CHECK-NEXT: at org.geevm.tests.exceptions.UncaughtException.callee(Unknown Source)
        // CHECK-NEXT: at org.geevm.tests.exceptions.UncaughtException.main(Unknown Source)
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
