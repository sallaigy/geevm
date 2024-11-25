// RUN: split-file %s %t
// RUN: %compile --no-copy-sources -d %t -m org.geevm.tests.oop.InheritanceFields %t/org/geevm/tests/oop/Base.java \
// RUN: %t/org/geevm/tests/oop/Derived.java %t/org/geevm/tests/oop/InheritanceFields.java | FileCheck "%s"

//--- org/geevm/tests/oop/InheritanceFields.java
package org.geevm.tests.oop;

import org.geevm.util.Printer;

public class InheritanceFields {

    public static void main(String[] args) {
        Base base = new Base();
        // CHECK: 10
        Printer.println(base.publicFieldInBase);

        Base derived = new Derived();
        // CHECK-NEXT: 10
        Printer.println(derived.publicFieldInBase);

        Derived derived2 = new Derived();
        // CHECK-NEXT: 10
        Printer.println(derived2.publicFieldInBase);
        // CHECK-NEXT: 40
        Printer.println(derived2.fieldInDerived);

    }

}

//--- org/geevm/tests/oop/Base.java
package org.geevm.tests.oop;

public class Base {

    public int publicFieldInBase = 10;

    public int first() {
        return 10;
    }

    public int second() {
        return 20;
    }
}

//--- org/geevm/tests/oop/Derived.java
package org.geevm.tests.oop;

public class Derived extends Base {

    public int fieldInDerived = 40;

    public int second() {
        return 40;
    }
}
