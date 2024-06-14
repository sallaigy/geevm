package org.geevm.tests.classfile;

import java.util.List;
import java.util.ArrayList;
import java.io.IOException;

public abstract class Methods {

  public void publicMethod() {}
  protected void protectedMethod() {}
  private void privateMethod() {}
  void packageInternalMethod() {}

  public static void staticMethod() {}
  public final void finalMethod() {}
  public synchronized void synchronizedMethod() {}

  public int simpleMethod(int x) {
    return x;
  }

  public String varArgsMethod(String... args) {
    return String.join(",", args);
  }

  public abstract void abstractMethod();
  public strictfp float strictFpMethod() {
    return 1.5f;
  }

  public native void nativeMethod();

  public static class MyException extends Exception {}

  public abstract void methodWithExceptions() throws IOException, MyException;

  public <X, Y extends X> X genericMethod(Y y) {
    return y;
  }

}