// RUN: split-file %s %t
// RUN: %compile --no-copy-sources -d %t -m org.geevm.tests.init.ex1.Test %t/org/geevm/tests/init/ex1/One.java \
// RUN: %t/org/geevm/tests/init/ex1/Two.java %t/org/geevm/tests/init/ex1/Super.java %t/org/geevm/tests/init/ex1/Test.java \
// RUN: | FileCheck "%s"

//--- org/geevm/tests/init/ex1/Super.java
package org.geevm.tests.init.ex1;

import org.geevm.util.Printer;

public class Super {

    static { Printer.println("Super"); }

}

//--- org/geevm/tests/init/ex1/One.java
package org.geevm.tests.init.ex1;

import org.geevm.util.Printer;

public class One extends Super {

    static { Printer.println("One"); }

}

//--- org/geevm/tests/init/ex1/Two.java
package org.geevm.tests.init.ex1;

import org.geevm.util.Printer;

public class Two extends Super {

    static { Printer.println("Two"); }

}

//--- org/geevm/tests/init/ex1/Test.java
package org.geevm.tests.init.ex1;

import org.geevm.util.Printer;

// Example from JLS §12.4.1, example 12.4.1-1.
class Test {
    public static void main(String[] args) {
        One o = null;
        Two t = new Two();
        Printer.println((Object)o == (Object)t);
        // CHECK: Super
        // CHECK-NEXT: Two
        // CHECK-NEXT: false
    }
}
