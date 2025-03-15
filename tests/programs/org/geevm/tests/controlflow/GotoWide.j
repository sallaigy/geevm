; RUN: %compile -d %t "%s" 2>&1 | FileCheck "%s"
.bytecode 61.0
.class org/geevm/tests/controlflow/GotoWide
.super java/lang/Object

.method public <init>()V
   aload_0
   invokenonvirtual java/lang/Object/<init>()V
   return
.end method

.method public static main([Ljava/lang/String;)V
    .limit stack 2
    goto_w Label
    ; CHECK-NOT: fail
    ldc "fail"
    invokestatic org/geevm/util/Printer/println(Ljava/lang/String;)V
Label:
    .stack
    .end stack
    ; CHECK: success
    ldc "success"
    invokestatic org/geevm/util/Printer/println(Ljava/lang/String;)V

    return

.end method
