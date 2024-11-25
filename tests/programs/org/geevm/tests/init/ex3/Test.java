// RUN: split-file %s %t
// RUN: %compile --no-copy-sources -d %t -m org.geevm.tests.init.ex3.Test %t/org/geevm/tests/init/ex3/I.java \
// RUN: %t/org/geevm/tests/init/ex3/J.java %t/org/geevm/tests/init/ex3/K.java | FileCheck "%s"

//--- org/geevm/tests/init/ex3/I.java
package org.geevm.tests.init.ex3;

import org.geevm.util.Printer;

public interface I {
    int i = 1, ii = Test.out("ii", 2);
}

//--- org/geevm/tests/init/ex3/J.java
package org.geevm.tests.init.ex3;

import org.geevm.util.Printer;

public interface J extends I {
    int j = Test.out("j", 3), jj = Test.out("jj", 4);
}

//--- org/geevm/tests/init/ex3/K.java
package org.geevm.tests.init.ex3;

import org.geevm.util.Printer;

public interface K extends J {
    int k = Test.out("k", 5);
}

//--- org/geevm/tests/init/ex3/Test.java
package org.geevm.tests.init.ex3;

import org.geevm.util.Printer;

public class Test {
    public static void main(String[] args) {
        // CHECK: 1
        // CHECK-NEXT: j=3
        // CHECK-NEXT: jj=4
        // CHECK-NEXT: 3
        Printer.println(J.i);
        Printer.println(K.j);
    }
    static int out(String s, int i) {
        Printer.println(s + "=" + i);
        return i;
    }
}
