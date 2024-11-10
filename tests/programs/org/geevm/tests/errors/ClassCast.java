package org.geevm.tests.errors;

import org.geevm.tests.Printer;

public class ClassCast {
    public static void main(String[] args) {
        Object x = new ClassCast();
        Printer.println((String) x); // Throws ClassCastException
    }
}
