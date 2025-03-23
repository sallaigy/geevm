// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.basic;

import org.geevm.util.Printer;

public class Count {

    public static void main(String[] args) {
        int start = 0;
        int end = 10;
        int sum = 0;

        for (int i = start; i < end; i++) {
            sum += 1;
        }

        // CHECK: 10
        Printer.println(sum);
    }

}
