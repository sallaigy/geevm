; RUN: %compile -v -d %t "%s" | FileCheck "%s"
.class org/geevm/tests/basic/Dup
.super java/lang/Object

.method public <init>()V
   aload_0
   invokenonvirtual java/lang/Object/<init>()V
   return
.end method

.method public static main([Ljava/lang/String;)V
   .limit stack 3

   iconst_0
   iconst_1
   dup
   ; CHECK: 1
   invokestatic org/geevm/util/Printer/println(I)V
   ; CHECK-NEXT: 1
   invokestatic org/geevm/util/Printer/println(I)V

   return

.end method