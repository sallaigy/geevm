package org.geevm.tests.basic;

class MathHelper {

    public static int ten() {
        return 10;
    }

    public static int inc(int v, int i) {
        if (i % 2 == 0) {
            return v + 1;
        }
        return v + 2;
    }

}