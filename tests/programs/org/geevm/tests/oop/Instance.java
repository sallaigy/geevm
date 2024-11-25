// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.oop;

import org.geevm.util.Printer;

public class Instance {

    public Instance() {
    }

    public static void main(String[] args) {
        Instance instance = new Instance();
        int sum = instance.getNumber();

        // CHECK: 42
        Printer.println(sum);
    }

    private final int getNumber() {
        return 42;
    }

}
