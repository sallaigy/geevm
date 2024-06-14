package org.geevm.tests.classfile;

import java.util.ArrayList;
import java.util.Iterator;

public class Inheritance extends Fields implements Comparable<String>, Iterable<String> {
  private final String x;

  public Inheritance(String x) {
    this.x = x;
  }

  @Override
  public int compareTo(String s) {
    return 0;
  }

  @Override
  public Iterator<String> iterator() {
    return new ArrayList<String>().iterator();
  }
}
