// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.errors;

import org.geevm.util.Printer;

public class FieldAccessNullPointer {

    public int field;

    public static void main(String[] args) {
        try {
            FieldAccessNullPointer self = null;
            Printer.println(self.field);
        } catch (NullPointerException ex) {
            // CHECK: Caught NullPointerException
            Printer.println("Caught NullPointerException");
        }

        try {
            FieldAccessNullPointer self = null;
            self.field = 10;
        } catch (NullPointerException ex) {
            // CHECK-NEXT: Caught NullPointerException
            Printer.println("Caught NullPointerException");
        }
    }
}
