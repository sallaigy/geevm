package org.geevm.benchmarks;

import java.math.BigInteger;

public class Fibonacci {

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("USAGE: fibonacci <number>");
            return;
        }
        long n = Long.valueOf(args[0]);
        System.out.println(fibonacci(n));
    }

    public static BigInteger fibonacci(long n) {
        BigInteger first = BigInteger.ZERO, second = BigInteger.ONE;
        for (long i = 2; i <= n; i++) {
            BigInteger next = first.add(second);
            first = second;
            second = next;
        }

        return second;
    }

}
