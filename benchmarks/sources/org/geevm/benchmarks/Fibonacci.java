package org.geevm.benchmarks;

import java.math.BigInteger;

public class Fibonacci {

    public static void main(String[] args) {
        System.out.println(fibonacci(50000));
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
