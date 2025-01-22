// RUN: %compile -d %t "%s" 2>& 1 | FileCheck "%s"
package org.geevm.tests.gc;

import org.geevm.util.Printer;

public class GcPreservesObjectHash {

    public static void main(String[] args) {
        Object object = new GcPreservesObjectHash();
        int hash1 = object.hashCode();
        System.gc();
        int hash2 = object.hashCode();

        // CHECK: true
        Printer.println(hash1 == hash2);
    }

}

