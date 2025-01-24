// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.errors;

public class UnknownNativeMethod {
    public static void main(String[] args) {
        callee();
    }

    public static native void callee();

    // CHECK: Exception in thread "main" java.lang.UnsatisfiedLinkError: void org.geevm.tests.errors.UnknownNativeMethod.callee()
    // CHECK-NEXT: at org.geevm.tests.errors.UnknownNativeMethod.callee(Native Method)
    // CHECK-NEXT: at org.geevm.tests.errors.UnknownNativeMethod.main(UnknownNativeMethod.java:6)
}
