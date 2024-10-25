package org.geevm.tests.basic;

public class StaticFields {

    private static final int START = 1;
    private static final int END = 100;
    private static int inc = 1;

    public static void main(String[] args) {
        int sum = 0;

        for (int i = START; i < END; i++) {
            sum += inc;
            if (i % 10 == 0) {
                inc += 1;
            }
        }

        // Should print 540
        Printer.println(sum);
    }

}
