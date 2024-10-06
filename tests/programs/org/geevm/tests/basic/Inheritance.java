package org.geevm.tests.basic;

import org.geevm.tests.basic.inheritance.Base;
import org.geevm.tests.basic.inheritance.Derived;

public class Inheritance {

    public static void main(String[] args) {
        Base base = new Base();
        // Should print '10'
        __geevm_print(base.first());
        // Should print '20'
        __geevm_print(base.second());

        Base derived = new Derived();
        // Should print '10'
        __geevm_print(derived.first());
        // Should print '40'
        __geevm_print(derived.second());
    }

    public static native void __geevm_print(int value);

}
