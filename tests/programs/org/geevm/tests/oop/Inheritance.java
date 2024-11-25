// RUN: split-file %s %t
// RUN: %compile --no-copy-sources -d %t -m org.geevm.tests.oop.Inheritance %t/org/geevm/tests/oop/Base.java \
// RUN: %t/org/geevm/tests/oop/Derived.java %t/org/geevm/tests/oop/Inheritance.java | FileCheck "%s"

//--- org/geevm/tests/oop/Inheritance.java
package org.geevm.tests.oop;

import org.geevm.util.Printer;

public class Inheritance {

    public static void main(String[] args) {
        Base base = new Base();
        // CHECK: 10
        Printer.println(base.first());
        // CHECK-NEXT: 20
        Printer.println(base.second());

        Base derived = new Derived();
        // CHECK-NEXT: 10
        Printer.println(derived.first());
        // CHECK-NEXT: 40
        Printer.println(derived.second());
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
