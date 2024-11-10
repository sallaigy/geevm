package org.geevm.tests.arrays;

import org.geevm.tests.Printer;

public class IntArrayExceptions {

    public static void main(String[] args) {
        try {
            int[] t = new int[-1];
        } catch (NegativeArraySizeException ex) {
            Printer.println("Caught NegativeArraySizeException");
        }

        try {
            store(null, 5);
        } catch (NullPointerException ex) {
            Printer.println("Caught NullPointerException");
        }

        try {
            store(new int[10], -1);
        } catch (ArrayIndexOutOfBoundsException ex) {
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            store(new int[10], 10);
        } catch (ArrayIndexOutOfBoundsException ex) {
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            store(new int[10], 11);
        } catch (ArrayIndexOutOfBoundsException ex) {
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(null, 5);
        } catch (NullPointerException ex) {
            Printer.println("Caught NullPointerException");
        }

        try {
            load(new int[10], -1);
        } catch (ArrayIndexOutOfBoundsException ex) {
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(new int[10], 10);
        } catch (ArrayIndexOutOfBoundsException ex) {
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }

        try {
            load(new int[10], 11);
        } catch (ArrayIndexOutOfBoundsException ex) {
            Printer.println("Caught ArrayIndexOutOfBoundsException");
        }
    }

    public static void store(int[] t, int idx) {
        t[idx] = 10;
    }

    public static int load(int[] t, int idx) {
        return t[idx];
    }

}