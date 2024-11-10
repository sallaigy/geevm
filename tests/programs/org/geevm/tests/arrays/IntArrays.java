package org.geevm.tests.arrays;

import org.geevm.tests.Printer;

public class IntArrays {

    public static void main(String[] args) {
        int[] t = new int[10];
        for (int i = 0; i < t.length; i++) {
            t[i] = i * 2;
        }

        for (int i = 0; i < t.length; i++) {
            Printer.println(t[i]);
        }
    }

}