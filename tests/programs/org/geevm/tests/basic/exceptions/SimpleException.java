package org.geevm.tests.basic.exceptions;

import org.geevm.tests.basic.Printer;

public class SimpleException {
    public static void main(String[] args) {
        try {
            if (condition()) {
                throw new IllegalStateException("Exception thrown and caught in the same method.");
            }
        } catch (Exception ex) {
            Printer.println("Caught exception: " + ex.getMessage());
        }
    }

    public static boolean condition() {
        return true;
    }
}
