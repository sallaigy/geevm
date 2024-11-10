package org.geevm.tests.arrays;

import org.geevm.tests.Printer;

public class ObjectArrays {

    public static void main(String[] args) {
        String[] t = new String[10];
        for (int i = 0; i < t.length; i++) {
            t[i] = "#" + String.valueOf(i);
        }

        for (int i = 0; i < t.length; i++) {
            Printer.println(t[i]);
        }
    }

}