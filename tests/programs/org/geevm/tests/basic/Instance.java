package org.geevm.tests.basic;

public class Instance {

    public Instance() {
    }

    public static void main(String[] args) {
        Instance instance = new Instance();
        int sum = instance.getNumber();

        // Should print 42
        Printer.println(sum);
    }

    private final int getNumber() {
        return 42;
    }

}
