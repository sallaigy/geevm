; RUN: %compile -d %t "%s" | FileCheck "%s"
; RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/LocalVariables#main([Ljava/lang/String;)V" "%s" 2>&1 | FileCheck "%s"
.bytecode 61.0
.class org/geevm/tests/basic/LocalVariables
.super java/lang/Object

.method public <init>()V
    aload_0
    invokenonvirtual java/lang/Object/<init>()V
    return
.end method

.method public static main([Ljava/lang/String;)V
    .limit stack 100
    .limit locals 100

    ; CHECK: 0
    iconst_0
    istore_0
    iload_0
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 1
    iconst_1
    istore_1
    iload_1
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 2
    iconst_2
    istore_2
    iload_2
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 3
    iconst_3
    istore_3
    iload_3
    invokestatic org/geevm/util/Printer/println(I)V
    ; CHECK-NEXT: 4
    iconst_4
    istore 4
    iload 4
    invokestatic org/geevm/util/Printer/println(I)V


    ; CHECK-NEXT: 0
    lconst_0
    lstore_0
    lload_0
    invokestatic org/geevm/util/Printer/println(J)V
    ; CHECK-NEXT: 1
    lconst_1
    lstore_1
    lload_1
    invokestatic org/geevm/util/Printer/println(J)V
    ; CHECK-NEXT: 2
    ldc2_w 2
    lstore_2
    lload_2
    invokestatic org/geevm/util/Printer/println(J)V
    ; CHECK-NEXT: 3
    ldc2_w 3
    lstore_3
    lload_3
    invokestatic org/geevm/util/Printer/println(J)V
    ; CHECK-NEXT: 4
    ldc2_w 4
    lstore 4
    lload 4
    invokestatic org/geevm/util/Printer/println(J)V

    ; CHECK-NEXT: 0
    fconst_0
    fstore_0
    fload_0
    invokestatic org/geevm/util/Printer/println(F)V
    ; CHECK-NEXT: 1
    fconst_1
    fstore_1
    fload_1
    invokestatic org/geevm/util/Printer/println(F)V
    ; CHECK-NEXT: 2
    fconst_2
    fstore_2
    fload_2
    invokestatic org/geevm/util/Printer/println(F)V
    ; CHECK-NEXT: 3
    ldc 3.0f
    fstore_3
    fload_3
    invokestatic org/geevm/util/Printer/println(F)V
    ; CHECK-NEXT: 4
    ldc 4.0f
    fstore 4
    fload 4
    invokestatic org/geevm/util/Printer/println(F)V

    ; CHECK-NEXT: 0
    dconst_0
    dstore_0
    dload_0
    invokestatic org/geevm/util/Printer/println(D)V
    ; CHECK-NEXT: 1
    dconst_1
    dstore_1
    dload_1
    invokestatic org/geevm/util/Printer/println(D)V
    ; CHECK-NEXT: 2
    ldc2_w 2.0
    dstore_2
    dload_2
    invokestatic org/geevm/util/Printer/println(D)V
    ; CHECK-NEXT: 3
    ldc2_w 3.0
    dstore_3
    dload_3
    invokestatic org/geevm/util/Printer/println(D)V
    ; CHECK-NEXT: 4
    ldc2_w 4.0f
    dstore 4
    dload 4
    invokestatic org/geevm/util/Printer/println(D)V

    ; CHECK-NEXT: one
    ldc "one"
    astore_0
    aload_0
    invokestatic org/geevm/util/Printer/println(Ljava/lang/String;)V
    ; CHECK-NEXT: two
    ldc "two"
    astore_1
    aload_1
    invokestatic org/geevm/util/Printer/println(Ljava/lang/String;)V
    ; CHECK-NEXT: three
    ldc "three"
    astore_2
    aload_2
    invokestatic org/geevm/util/Printer/println(Ljava/lang/String;)V
    ; CHECK-NEXT: four
    ldc "four"
    astore_3
    aload_3
    invokestatic org/geevm/util/Printer/println(Ljava/lang/String;)V
    ; CHECK-NEXT: five
    ldc "five"
    astore 4
    aload 4
    invokestatic org/geevm/util/Printer/println(Ljava/lang/String;)V

    return

.end method