package org.geevm.tests.arrays;

import org.geevm.tests.Printer;

public class ObjectArrayExceptions {

    public static void main(String[] args) {
        try {
            String[] t = new String[-1];
        } catch (NegativeArraySizeException ex) {
            Printer.println("Caught NegativeArraySizeException");
        }

        try {
            store(null, 5);
        } catch (NullPointerException ex) {
            Printer.println("Caught NullPointerException");
        }

        try {
            store(new String[10], -1);
        } catch (ArrayIndexOutOfBoundsException ex) {
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            store(new String[10], 10);
        } catch (ArrayIndexOutOfBoundsException ex) {
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            store(new String[10], 11);
        } catch (ArrayIndexOutOfBoundsException ex) {
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            Object[] t = new String[10];
            t[0] = new Object();
        } catch (ArrayStoreException ex) {
            Printer.println("Caught ArrayStoreException");
        }

        try {
            load(null, 5);
        } catch (NullPointerException ex) {
            Printer.println("Caught NullPointerException");
        }

        try {
            load(new String[10], -1);
        } catch (ArrayIndexOutOfBoundsException ex) {
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(new String[10], 10);
        } catch (ArrayIndexOutOfBoundsException ex) {
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(new String[10], 11);
        } catch (ArrayIndexOutOfBoundsException ex) {
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }
    }

    public static void store(String[] t, int idx) {
        t[idx] = "hello";
    }

    public static String load(String[] t, int idx) {
        return t[idx];
    }

}