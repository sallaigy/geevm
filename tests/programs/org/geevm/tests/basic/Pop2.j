; RUN: %compile -d %t "%s" | FileCheck "%s"
; RUN: %compile -d %t -f "-Xjit org/geevm/tests/basic/Pop2#main([Ljava/lang/String;)V" "%s" 2>&1 | FileCheck "%s"
.bytecode 61.0
.class org/geevm/tests/basic/Pop2
.super java/lang/Object

.method public <init>()V
   aload_0
   invokenonvirtual java/lang/Object/<init>()V
   return
.end method

.method public static main([Ljava/lang/String;)V
   .limit stack 4

   iconst_0
   iconst_1
   iconst_2
   iconst_2
   pop2
   ; CHECK: 1
   invokestatic org/geevm/util/Printer/println(I)V
   ; CHECK-NEXT: 0
   invokestatic org/geevm/util/Printer/println(I)V

   return

.end method