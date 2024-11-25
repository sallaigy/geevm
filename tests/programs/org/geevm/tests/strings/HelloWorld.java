// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.strings;

import org.geevm.util.Printer;

public class HelloWorld {
    public static void main(String[] args) {
        // CHECK: Hello World!
        Printer.println("Hello World!");
    }
}
