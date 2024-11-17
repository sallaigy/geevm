package org.geevm.tests.init.ex2;

import org.geevm.tests.Printer;

public class Sub extends Super {
    static { Printer.println("Sub "); }
}
