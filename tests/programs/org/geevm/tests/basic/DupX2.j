; RUN: %compile -d %t "%s" | FileCheck "%s"
.class org/geevm/tests/basic/DupX2
.super java/lang/Object

.method public <init>()V
    aload_0
    invokenonvirtual java/lang/Object/<init>()V
    return
.end method

.method public static main([Ljava/lang/String;)V
    .limit stack 5

    iconst_0
    iconst_3
    iconst_2
    iconst_1
    dup_x2

    ; CHECK: 1
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 2
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 3
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 1
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 0
    invokestatic org/geevm/util/Printer/println(I)V

    ldc2_w 2
    iconst_1
    dup_x2

    ; CHECK-NEXT: 1
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 2
    invokestatic org/geevm/util/Printer/println(J)V
    ; CHECK-NEXT: 1
    invokestatic org/geevm/util/Printer/println(I)V

    return

.end method
