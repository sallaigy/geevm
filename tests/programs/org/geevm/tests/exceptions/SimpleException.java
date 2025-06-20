// RUN: %compile -d %t "%s" | FileCheck "%s"
// RUN: %compile -d %t -f "-Xjit org/geevm/tests/exceptions/SimpleException#main([Ljava/lang/String;)V" "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.exceptions;

import org.geevm.util.Printer;

public class SimpleException {
    public static void main(String[] args) {
        try {
            if (condition()) {
                throw new IllegalStateException("Exception thrown and caught in the same method.");
            }
        } catch (Exception ex) {
            // CHECK: Caught exception: Exception thrown and caught in the same method.
            Printer.println("Caught exception: " + ex.getMessage());
        }
    }

    public static boolean condition() {
        return true;
    }
}
