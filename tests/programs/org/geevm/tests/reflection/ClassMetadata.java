package org.geevm.tests.reflection;

import org.geevm.tests.Printer;

public class ClassMetadata {

    public static void main(String[] args) throws ClassNotFoundException {
        Printer.println(ClassMetadata.class.getName());

        ClassMetadata object = new ClassMetadata();
        Printer.println(object.getClass().getName());

        Class<?> cls = Class.forName("org.geevm.tests.reflection.ClassMetadata");
        Printer.println(cls.getName());
    }

}
