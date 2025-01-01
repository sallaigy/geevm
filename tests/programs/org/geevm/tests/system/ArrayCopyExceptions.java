// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.system;

public class ArrayCopyExceptions {

    public static void main(String[] args) {
        int[] a = new int[] {1, 2, 3};
        int[] b = new int[3];

        try {
            System.arraycopy(null, 0, b, 0, 3);
        } catch (NullPointerException e) {
            // CHECK: null
            System.out.println(e.getMessage());
        }

        try {
            System.arraycopy(a, -1, b, 0, 3);
        } catch (IndexOutOfBoundsException e) {
            // CHECK-NEXT: arraycopy: source index -1 out of bounds for int[3]
            System.out.println(e.getMessage());
        }

        try {
            System.arraycopy(a, 0, b, -1, 3);
        } catch (IndexOutOfBoundsException e) {
            // CHECK-NEXT: arraycopy: destination index -1 out of bounds for int[3]
            System.out.println(e.getMessage());
        }

        try {
            System.arraycopy(a, 100, b, 0, 3);
        } catch (IndexOutOfBoundsException e) {
            // CHECK-NEXT: arraycopy: last source index 103 out of bounds for int[3]
            System.out.println(e.getMessage());
        }

        try {
            System.arraycopy(a, 0, b, 100, 3);
        } catch (IndexOutOfBoundsException e) {
            // CHECK-NEXT: arraycopy: last destination index 103 out of bounds for int[3]
            System.out.println(e.getMessage());
        }

        try {
            System.arraycopy(a, 0, b, 0, -1);
        } catch (IndexOutOfBoundsException e) {
            // CHECK-NEXT: arraycopy: length -1 is negative
            System.out.println(e.getMessage());
        }

        try {
            System.arraycopy(a, 0, b, 0, 100);
        } catch (IndexOutOfBoundsException e) {
            // CHECK-NEXT: arraycopy: last source index 100 out of bounds for int[3]
            System.out.println(e.getMessage());
        }
    }

}
