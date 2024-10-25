package org.geevm.tests.basic;

public class Count {

    public static void main(String[] args) {
        int start = 0;
        int end = 10;
        int sum = 0;

        for (int i = start; i < end; i++) {
            sum += 1;
        }

        Printer.println(sum);
    }

}
