package org.geevm.tests.basic;

import org.geevm.tests.basic.inheritance.Base;
import org.geevm.tests.basic.inheritance.Derived;

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
