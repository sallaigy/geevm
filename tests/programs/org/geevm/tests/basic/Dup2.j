; RUN: %compile -d %t "%s" | FileCheck "%s"
.class org/geevm/tests/basic/Dup2
.super java/lang/Object

.method public <init>()V
   aload_0
   invokenonvirtual java/lang/Object/<init>()V
   return
.end method

.method public static main([Ljava/lang/String;)V
   .limit stack 6

   iconst_0
   iconst_2
   iconst_1
   dup2
   ; CHECK: 1
   invokestatic org/geevm/util/Printer/println(I)V
   ; CHECK-NEXT: 2
   invokestatic org/geevm/util/Printer/println(I)V
   ; CHECK-NEXT: 1
   invokestatic org/geevm/util/Printer/println(I)V
   ; CHECK-NEXT: 2
   invokestatic org/geevm/util/Printer/println(I)V
   ; CHECK-NEXT: 0
   invokestatic org/geevm/util/Printer/println(I)V

   lconst_0
   lconst_1
   dup2
   ; CHECK-NEXT: 1
   invokestatic org/geevm/util/Printer/println(J)V
   ; CHECK-NEXT: 1
   invokestatic org/geevm/util/Printer/println(J)V
   ; CHECK-NEXT: 0
   invokestatic org/geevm/util/Printer/println(J)V

   return

.end method