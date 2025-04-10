; RUN: %compile -d %t "%s" | FileCheck "%s"
.class org/geevm/tests/basic/DupX1
.super java/lang/Object

.method public <init>()V
    aload_0
    invokenonvirtual java/lang/Object/<init>()V
    return
.end method

.method public static main([Ljava/lang/String;)V
    .limit stack 4

    iconst_0
    iconst_2
    iconst_1
    dup_x1
    ; CHECK: 1
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 2
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 1
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 0
    invokestatic org/geevm/util/Printer/println(I)V

    return

.end method
