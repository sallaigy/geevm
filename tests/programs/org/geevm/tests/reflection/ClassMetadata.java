// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.reflection;

import org.geevm.util.Printer;

public class ClassMetadata {

    public static void main(String[] args) throws ClassNotFoundException {
        // CHECK: org.geevm.tests.reflection.ClassMetadata
        Printer.println(ClassMetadata.class.getName());

        ClassMetadata object = new ClassMetadata();
        // CHECK-NEXT: org.geevm.tests.reflection.ClassMetadata
        Printer.println(object.getClass().getName());

        Class<?> cls = Class.forName("org.geevm.tests.reflection.ClassMetadata");
        // CHECK-NEXT: org.geevm.tests.reflection.ClassMetadata
        Printer.println(cls.getName());
    }
}
