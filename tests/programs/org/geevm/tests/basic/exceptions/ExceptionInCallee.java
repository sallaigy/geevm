package org.geevm.tests.basic.exceptions;

import org.geevm.tests.basic.Printer;

public class ExceptionInCallee {
    public static void main(String[] args) {
        try {
            callee();
        } catch (Exception ex) {
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
