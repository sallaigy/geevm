package org.geevm.tests.init.ex1;

import org.geevm.tests.Printer;

// Example from JLS §12.4.1, example 12.4.1-1.
class Test {
    public static void main(String[] args) {
        One o = null;
        Two t = new Two();
        Printer.println((Object)o == (Object)t);
    }
}
