// RUN: split-file %s %t
// RUN: %compile --no-copy-sources -d %t -m org.geevm.tests.basic.StaticCallsToAnotherClass %t/org/geevm/tests/basic/MathHelper.java \
// RUN: | FileCheck "%s"

//--- org/geevm/tests/basic/StaticCallsToAnotherClass.java
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class StaticCallsToAnotherClass {

    public static void main(String[] args) {
        int start = 0;
        int sum = 0;

        for (int i = start; i < MathHelper.ten(); i++) {
            sum = MathHelper.inc(sum, i);
        }

        // CHECK: 15
        Printer.println(sum);
    }

}

//--- org/geevm/tests/basic/MathHelper.java
package org.geevm.tests.basic;

class MathHelper {

    public static int ten() {
        return 10;
    }

    public static int inc(int v, int i) {
        if (i % 2 == 0) {
            return v + 1;
        }
        return v + 2;
    }

}
