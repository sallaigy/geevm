// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.system;

public class HelloWorldSystem {
    public static void main(String[] args) {
        // CHECK: Hello World!
        System.out.println("Hello World!");
    }
}
