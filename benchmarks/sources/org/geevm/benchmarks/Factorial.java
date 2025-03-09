package org.geevm.benchmarks;

import java.math.BigInteger;

public class Factorial {

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("USAGE: factorial <number>");
            return;
        }
        int n = Integer.parseInt(args[0]);


        System.out.println(factorial(n));
    }

    public static BigInteger factorial(int n) {
        BigInteger result = BigInteger.ONE;
        for (int i = 2; i <= n; i++) {
            result = result.multiply(BigInteger.valueOf(i));
        }

        return result;
    }
}
