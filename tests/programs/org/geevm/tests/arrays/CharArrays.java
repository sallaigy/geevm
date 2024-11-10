package org.geevm.tests.arrays;

import org.geevm.tests.Printer;

public class CharArrays {

    public static void main(String[] args) {
        char[] t = new char[10];
        for (int i = 0; i < t.length; i++) {
            t[i] = (char) ('0' + i);
        }

        for (int i = 0; i < t.length; i++) {
            Printer.println(t[i]);
        }
    }

}