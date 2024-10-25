package org.geevm.tests.basic.strings;

import org.geevm.tests.basic.Printer;

public class StringEquals {
    public static void main(String[] args) {
        // true
        Printer.println("abc" == "abc");
        // false
        Printer.println("abc" != "abc");
        // false
        Printer.println("abc" == "xyz");
        // true
        Printer.println("abc" != "xyz");
        // true
        Printer.println("abc".equals("abc"));
        // false
        Printer.println("abc".equals("xyz"));
        // true
        Printer.println("abc" == ("a" + "b" + "c").intern());
        // false
        Printer.println("abc" == ("x" + "y" + "z").intern());
    }
}
