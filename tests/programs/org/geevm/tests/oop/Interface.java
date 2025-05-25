// RUN: split-file %s %t
// RUN: %compile --no-copy-sources -d %t -m org.geevm.tests.oop.Interface %t/org/geevm/tests/oop/BaseInterface.java \
// RUN: %t/org/geevm/tests/oop/Derived.java %t/org/geevm/tests/oop/Interface.java | FileCheck "%s"
// RUN: %compile --no-copy-sources -d %t -m org.geevm.tests.oop.Interface -f "-Xjit org/geevm/tests/oop/Interface#main([Ljava/lang/String;)V" \
// RUN: %t/org/geevm/tests/oop/BaseInterface.java %t/org/geevm/tests/oop/Derived.java %t/org/geevm/tests/oop/Interface.java | FileCheck "%s"

//--- org/geevm/tests/oop/Interface.java
package org.geevm.tests.oop;

import org.geevm.util.Printer;

public class Interface {

    public static void main(String[] args) {
        BaseInterface derived = new Derived();
        // CHECK: 10
        Printer.println(derived.first());
        // CHECK-NEXT: 40
        Printer.println(derived.second());
    }
}

//--- org/geevm/tests/oop/BaseInterface.java
package org.geevm.tests.oop;

public interface BaseInterface {

    int first();

    int second();
}

//--- org/geevm/tests/oop/Derived.java
package org.geevm.tests.oop;

public class Derived implements BaseInterface {

    @Override
    public int first() {
        return 10;
    }

    @Override
    public int second() {
        return 40;
    }
}
