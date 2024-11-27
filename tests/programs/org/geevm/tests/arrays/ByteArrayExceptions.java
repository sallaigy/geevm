// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.arrays;

import org.geevm.util.Printer;

public class ByteArrayExceptions {

    public static void main(String[] args) {
        try {
            byte[] t = new byte[-1];
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
            store(new byte[10], -1);
        } catch (ArrayIndexOutOfBoundsException ex) {
            // CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            store(new byte[10], 10);
        } catch (ArrayIndexOutOfBoundsException ex) {
            // CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            store(new byte[10], 11);
        } catch (ArrayIndexOutOfBoundsException ex) {
            // CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(null, 5);
        } catch (NullPointerException ex) {
            // CHECK-NEXT: Caught NullPointerException
            Printer.println("Caught NullPointerException");
        }

        try {
            load(new byte[10], -1);
        } catch (ArrayIndexOutOfBoundsException ex) {
            // CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(new byte[10], 10);
        } catch (ArrayIndexOutOfBoundsException ex) {
            // CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(new byte[10], 11);
        } catch (ArrayIndexOutOfBoundsException ex) {
            // CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }
    }

    public static void store(byte[] t, int idx) {
        t[idx] = 10;
    }

    public static byte load(byte[] t, int idx) {
        return t[idx];
    }

}