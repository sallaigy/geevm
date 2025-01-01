// RUN: %compile -d %t "%s"  | FileCheck "%s"
package org.geevm.tests.system;

import java.util.Arrays;

public class ArrayCopy {
    public static void main(String[] args) {
        int[] intArrA = new int[] {1, 2, 3};
        int[] intArrB = new int[3];

        System.arraycopy(intArrA, 0, intArrB, 0, 3);
        // CHECK: [1, 2, 3]
        System.out.println(Arrays.toString(intArrA));

        // CHECK-NEXT: [1, 2, 3]
        System.out.println(Arrays.toString(intArrB));

        System.arraycopy(intArrA, 1, intArrB, 0, 1);

        // CHECK-NEXT: [2, 2, 3]
        System.out.println(Arrays.toString(intArrB));

        char[] charArrA = new char[] {'a', 'b', 'c', 'd'};
        char[] charArrB = new char[] {'x', 'x', 'x', 'x', 'x', 'x', 'x'};

        System.arraycopy(charArrA, 0, charArrB, 1, 4);

        // CHECK-NEXT: [x, a, b, c, d, x, x]
        System.out.println(Arrays.toString(charArrB));

        byte[] byteArrA = new byte[] {1, 2, 3};
        byte[] byteArrB = new byte[5];

        System.arraycopy(byteArrA, 0, byteArrB, 2, byteArrA.length);

        // CHECK-NEXT: [0, 0, 1, 2, 3]
        System.out.println(Arrays.toString(byteArrB));

        short[] shortArrA = new short[] {1, 2, 3};
        short[] shortArrB = new short[5];

        System.arraycopy(shortArrA, 0, shortArrB, 2, shortArrA.length);

        // CHECK-NEXT: [0, 0, 1, 2, 3]
        System.out.println(Arrays.toString(shortArrB));

        long[] longArrA = new long[] {1, 2, 3};
        long[] longArrB = new long[5];

        System.arraycopy(longArrA, 0, longArrB, 2, longArrA.length);

        // CHECK-NEXT: [0, 0, 1, 2, 3]
        System.out.println(Arrays.toString(longArrB));

        float[] floatArrA = new float[] {1.0f, 2.0f, 3.0f};
        float[] floatArrB = new float[5];

        System.arraycopy(floatArrA, 0, floatArrB, 2, floatArrA.length);

        // CHECK-NEXT: [0.0, 0.0, 1.0, 2.0, 3.0]
        System.out.println(Arrays.toString(floatArrB));

        double[] doubleArrA = new double[] {1.0, 2.0, 3.0};
        double[] doubleArrB = new double[5];

        System.arraycopy(doubleArrA, 0, doubleArrB, 2, doubleArrA.length);

        // CHECK-NEXT: [0.0, 0.0, 1.0, 2.0, 3.0]
        System.out.println(Arrays.toString(doubleArrB));

        boolean[] boolArrA = new boolean[] {true, false, true};
        boolean[] boolArrB = new boolean[5];

        System.arraycopy(boolArrA, 0, boolArrB, 2, boolArrA.length);

        // CHECK-NEXT: [false, false, true, false, true]
        System.out.println(Arrays.toString(boolArrB));

        String[] stringArrA = new String[] {"hello", "world", "!"};
        String[] stringArrB = new String[5];

        System.arraycopy(stringArrA, 0, stringArrB, 2, stringArrA.length);

        // CHECK-NEXT: [null, null, hello, world, !]
        System.out.println(Arrays.toString(stringArrB));
    }
}