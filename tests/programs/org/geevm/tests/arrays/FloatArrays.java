package org.geevm.tests.arrays;

import org.geevm.tests.Printer;

public class FloatArrays {

    public static void main(String[] args) {
        float[] t = new float[10];
        for (int i = 0; i < t.length; i++) {
            t[i] = 1.5f * i;
        }

        for (int i = 0; i < t.length; i++) {
            Printer.println(t[i]);
        }
    }

}
