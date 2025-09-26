; RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
; RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/Dup2X2#main([Ljava/lang/String;)V" "%s" 2>&1 | FileCheck "%s"
.class org/geevm/tests/basic/Dup2X2
.super java/lang/Object

.method public <init>()V
    aload_0
    invokenonvirtual java/lang/Object/<init>()V
    return
.end method

.method public static main([Ljava/lang/String;)V
    .limit stack 7

    ; Form 1: all values are category 1
    iconst_0
    iconst_4
    iconst_3
    iconst_2
    iconst_1
    dup2_x2
    ; CHECK: 1
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 2
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 3
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 4
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 1
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 2
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 0
    invokestatic org/geevm/util/Printer/println(I)V

    ; Form 2: value1 is category 2, value2 and value3 are category one
    iconst_0
    iconst_3
    iconst_2
    lconst_1
    dup2_x2
    ; CHECK-NEXT: 1
    invokestatic org/geevm/util/Printer/println(J)V
    ; CHECK-NEXT: 2
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 3
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 1
    invokestatic org/geevm/util/Printer/println(J)V
    ; CHECK-NEXT: 0
    invokestatic org/geevm/util/Printer/println(I)V

    ; Form 3: value1 and value2 are category 1, value 3 is category 2
    iconst_0
    ldc2_w 3
    iconst_2
    iconst_1
    dup2_x2
    ; CHECK-NEXT: 1
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 2
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 3
    invokestatic org/geevm/util/Printer/println(J)V
    ; CHECK-NEXT: 1
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 2
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 0
    invokestatic org/geevm/util/Printer/println(I)V

    ; Form 4: both value1 and value2 are category 2
    ldc2_w 2
    lconst_1
    dup2_x2
    ; CHECK-NEXT: 1
    invokestatic org/geevm/util/Printer/println(J)V
    ; CHECK-NEXT: 2
    invokestatic org/geevm/util/Printer/println(J)V
    ; CHECK-NEXT: 1
    invokestatic org/geevm/util/Printer/println(J)V

    return

.end method
