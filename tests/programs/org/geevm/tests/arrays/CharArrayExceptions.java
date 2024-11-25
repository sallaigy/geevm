// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.arrays;

import org.geevm.util.Printer;

public class CharArrayExceptions {

    public static void main(String[] args) {
        try {
            char[] t = new char[-1];
        } catch (NegativeArraySizeException ex) {
            //CHECK: Caught NegativeArraySizeException
            Printer.println("Caught NegativeArraySizeException");
        }

        try {
            store(null, 5);
        } catch (NullPointerException ex) {
            //CHECK-NEXT: Caught NullPointerException
            Printer.println("Caught NullPointerException");
        }

        try {
            store(new char[10], -1);
        } catch (ArrayIndexOutOfBoundsException ex) {
            //CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            store(new char[10], 10);
        } catch (ArrayIndexOutOfBoundsException ex) {
            //CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            store(new char[10], 11);
        } catch (ArrayIndexOutOfBoundsException ex) {
            //CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(null, 5);
        } catch (NullPointerException ex) {
            //CHECK-NEXT: Caught NullPointerException
            Printer.println("Caught NullPointerException");
        }

        try {
            load(new char[10], -1);
        } catch (ArrayIndexOutOfBoundsException ex) {
            //CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(new char[10], 10);
        } catch (ArrayIndexOutOfBoundsException ex) {
            //CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(new char[10], 11);
        } catch (ArrayIndexOutOfBoundsException ex) {
            //CHECK-NEXT: Caught ArrayIndexOutOfBoundsException
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }
    }

    public static void store(char[] t, int idx) {
        t[idx] = 10;
    }

    public static int load(char[] t, int idx) {
        return t[idx];
    }

}