// RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
package org.geevm.tests.errors;

public class UnknownNativeMethod {
    public static void main(String[] args) {
        callee();
    }

    public static native void callee();

    // CHECK: Exception java.lang.UnsatisfiedLinkError: 'void org.geevm.tests.errors.UnknownNativeMethod.callee()'
    // CHECK-NEXT: at org.geevm.tests.errors.UnknownNativeMethod.callee(Unknown Source)
    // CHECK-NEXT: at org.geevm.tests.errors.UnknownNativeMethod.main(Unknown Source)
}
