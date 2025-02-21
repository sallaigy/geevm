// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.strings;

import java.nio.charset.StandardCharsets;

public class Unicode {
    public static void main(String[] args) {
        String str = "Hello, 世界!";

        // CHECK: 10
        System.out.println(str.length());
        // CHECK-NEXT: 10
        System.out.println(str.codePointCount(0, str.length()));
        // CHECK-NEXT: 14
        System.out.println(str.getBytes(StandardCharsets.UTF_8).length);
        for (char c : str.toCharArray()) {
            // CHECK-NEXT: Hello, 世界!
            System.out.print(c);
        }
        System.out.println();

        String flags = "\uD83C\uDDE8\uD83C\uDDE6";
        // CHECK-NEXT: 4
        System.out.println(flags.length());
        // CHECK-NEXT: 2
        System.out.println(flags.codePointCount(0, flags.length()));
        // CHECK-NEXT: 8
        System.out.println(flags.getBytes(StandardCharsets.UTF_8).length);
    }
}
