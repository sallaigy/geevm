package org.geevm.tests.exceptions;

import org.geevm.tests.Printer;

public class UncaughtException {
    public static void main(String[] args) {
        callee();
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
