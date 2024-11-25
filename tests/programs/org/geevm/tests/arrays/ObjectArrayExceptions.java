// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.arrays;

import org.geevm.util.Printer;

public class ObjectArrayExceptions {

    public static void main(String[] args) {
        try {
            String[] t = new String[-1];
        } catch (NegativeArraySizeException ex) {
            // CHECK: Caught NegativeArraySizeException
            Printer.println("Caught NegativeArraySizeException");
        }

        try {
            store(null, 5);
        } catch (NullPointerException ex) {
            // CHECK-NEXT: Caught NullPointerException
            Printer.println("Caught NullPointerException");
        }

        try {
            store(new String[10], -1);
        } catch (ArrayIndexOutOfBoundsException ex) {
            // CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            store(new String[10], 10);
        } catch (ArrayIndexOutOfBoundsException ex) {
            // CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            store(new String[10], 11);
        } catch (ArrayIndexOutOfBoundsException ex) {
            // CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            Object[] t = new String[10];
            t[0] = new Object();
        } catch (ArrayStoreException ex) {
            // CHECK-NEXT: Caught ArrayStoreException
            Printer.println("Caught ArrayStoreException");
        }

        try {
            load(null, 5);
        } catch (NullPointerException ex) {
            // CHECK-NEXT: Caught NullPointerException
            Printer.println("Caught NullPointerException");
        }

        try {
            load(new String[10], -1);
        } catch (ArrayIndexOutOfBoundsException ex) {
            // CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(new String[10], 10);
        } catch (ArrayIndexOutOfBoundsException ex) {
            // CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(new String[10], 11);
        } catch (ArrayIndexOutOfBoundsException ex) {
            // CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
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