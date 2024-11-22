package org.geevm.tests.arrays;

import org.geevm.tests.Printer;

public class ByteArrays {

    public static void main(String[] args) {
        byte[] t = new byte[10];
        for (int i = 0; i < t.length; i++) {
            t[i] = (byte) (2 * i);
        }

        for (int i = 0; i < t.length; i++) {
            Printer.println(t[i]);
        }
    }
}
