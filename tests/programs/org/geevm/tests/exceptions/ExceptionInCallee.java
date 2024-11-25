// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.exceptions;

import org.geevm.util.Printer;

public class ExceptionInCallee {
    public static void main(String[] args) {
        try {
            callee();
        } catch (Exception ex) {
            // CHECK: Caught exception: Exception thrown in callee.
            Printer.println("Caught exception: " + ex.getMessage());
        }
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
