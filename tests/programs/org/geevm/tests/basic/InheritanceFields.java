package org.geevm.tests.basic;

import org.geevm.tests.basic.inheritance.Base;
import org.geevm.tests.basic.inheritance.Derived;

public class InheritanceFields {

    public static void main(String[] args) {
        Base base = new Base();
        // Should print '10'
        __geevm_print(base.publicFieldInBase);

        Base derived = new Derived();
        // Should print '10'
        __geevm_print(derived.publicFieldInBase);

        Derived derived2 = new Derived();
        // Should print '10'
        __geevm_print(derived2.publicFieldInBase);
        // Should print '40'
        __geevm_print(derived2.fieldInDerived);

    }

    public static native void __geevm_print(int value);

}
