// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.controlflow;

import org.geevm.util.Printer;

public class TableSwitch {

    public static void main(String[] args) {
        // CHECK: Tuesday
        Printer.println(getDay(2));
        // CHECK-NEXT: Friday
        Printer.println(getDay(5));
        // CHECK-NEXT: Sunday
        Printer.println(getDay(7));
        // CHECK-NEXT: Invalid day
        Printer.println(getDay(10));
    }

    public static String getDay(int day) {
        switch (day) {
            case 1:
                return "Monday";
            case 2:
                return "Tuesday";
            case 3:
                return "Wednesday";
            case 4:
                return "Thursday";
            case 5:
                return "Friday";
            case 6:
                return "Saturday";
            case 7:
                return "Sunday";
            default:
                return "Invalid day";
        }
    }
}