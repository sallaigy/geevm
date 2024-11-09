package org.geevm.tests.oop;

import org.geevm.tests.Printer;

public class InheritanceFields {

    public static void main(String[] args) {
        Base base = new Base();
        // Should print '10'
        Printer.println(base.publicFieldInBase);

        Base derived = new Derived();
        // Should print '10'
        Printer.println(derived.publicFieldInBase);

        Derived derived2 = new Derived();
        // Should print '10'
        Printer.println(derived2.publicFieldInBase);
        // Should print '40'
        Printer.println(derived2.fieldInDerived);

    }

}
