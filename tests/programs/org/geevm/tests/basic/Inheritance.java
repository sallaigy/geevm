package org.geevm.tests.basic;

import org.geevm.tests.basic.inheritance.Base;
import org.geevm.tests.basic.inheritance.Derived;

public class Inheritance {

    public static void main(String[] args) {
        Base base = new Base();
        // Should print '10'
        Printer.println(base.first());
        // Should print '20'
        Printer.println(base.second());

        Base derived = new Derived();
        // Should print '10'
        Printer.println(derived.first());
        // Should print '40'
        Printer.println(derived.second());
    }

}
