; RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
.class org/geevm/tests/basic/Wide
.super java/lang/Object

.method public <init>()V
    aload_0
    invokenonvirtual java/lang/Object/<init>()V
    return
.end method

.method public static main([Ljava/lang/String;)V
    .limit stack 4

    ; CHECK: 501
    invokestatic org/geevm/tests/basic/Wide/wideIInc()V
    ; CHECK-NEXT: 1
    invokestatic org/geevm/tests/basic/Wide/wideILoadIStore()V
    ; CHECK-NEXT: 1
    invokestatic org/geevm/tests/basic/Wide/wideLLoadLStore()V
    ; CHECK-NEXT: 1
    invokestatic org/geevm/tests/basic/Wide/wideFLoadFStore()V
    ; CHECK-NEXT: 1
    invokestatic org/geevm/tests/basic/Wide/wideDLoadDStore()V
    ; CHECK-NEXT: hello
    invokestatic org/geevm/tests/basic/Wide/wideALoadAStore()V

    return

.end method

.method public static wideIInc()V
    .limit stack 4
    .limit locals 300
    iconst_1
    istore_0
    iinc_w 0 500
    iload_0
    invokestatic org/geevm/util/Printer/println(I)V

    return
.end method

.method public static wideILoadIStore()V
    .limit stack 4
    .limit locals 400
    iconst_1
    istore_w 300
    iload_w 300
    invokestatic org/geevm/util/Printer/println(I)V

    return
.end method

.method public static wideLLoadLStore()V
    .limit stack 4
    .limit locals 400
    lconst_1
    lstore_w 300
    lload_w 300
    invokestatic org/geevm/util/Printer/println(J)V

    return
.end method

.method public static wideFLoadFStore()V
    .limit stack 4
    .limit locals 400
    fconst_1
    fstore_w 300
    fload_w 300
    invokestatic org/geevm/util/Printer/println(F)V

    return
.end method

.method public static wideDLoadDStore()V
    .limit stack 4
    .limit locals 400
    dconst_1
    dstore_w 300
    dload_w 300
    invokestatic org/geevm/util/Printer/println(D)V

    return
.end method

.method public static wideALoadAStore()V
    .limit stack 4
    .limit locals 400
    ldc "hello"
    astore_w 300
    aload_w 300
    invokestatic org/geevm/util/Printer/println(Ljava/lang/String;)V

    return
.end method