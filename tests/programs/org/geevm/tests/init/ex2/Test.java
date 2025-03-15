// RUN: split-file %s %t
// RUN: %compile --no-copy-sources -d %t -m org.geevm.tests.init.ex2.Test %t/org/geevm/tests/init/ex2/Super.java \
// RUN: %t/org/geevm/tests/init/ex2/Sub.java %t/org/geevm/tests/init/ex2/Test.java | FileCheck "%s"

//--- org/geevm/tests/init/ex2/Super.java
package org.geevm.tests.init.ex2;

import org.geevm.util.Printer;

public class Super {

    static int taxi = 1729;

}

//--- org/geevm/tests/init/ex2/Sub.java
package org.geevm.tests.init.ex2;

import org.geevm.util.Printer;

public class Sub extends Super {
    static { Printer.println("Sub "); }
}


//--- org/geevm/tests/init/ex2/Test.java
package org.geevm.tests.init.ex2;

import org.geevm.util.Printer;

public class Test {
    public static void main(String[] args) {
        // CHECK-NOT: Sub
        // CHECK: 1729
        Printer.println(Sub.taxi);
    }
}
