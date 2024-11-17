package org.geevm.tests.init.ex3;

import org.geevm.tests.Printer;

public class Test {
    public static void main(String[] args) {
        Printer.println(J.i);
        Printer.println(K.j);
    }
    static int out(String s, int i) {
        Printer.println(s + "=" + i);
        return i;
    }
}
