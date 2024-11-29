// RUN: %compile -d %t "%s" | FileCheck "%s"
package org.geevm.tests.controlflow;

import org.geevm.util.Printer;

public class LookupSwitch {

    public static void main(String[] args) {
        // CHECK: Invalid day
        Printer.println(getDay(2));
        // CHECK-NEXT: Monday
        Printer.println(getDay(10));
        // CHECK-NEXT: Friday
        Printer.println(getDay(310));
        // CHECK-NEXT: Thursday
        Printer.println(getDay(200));
    }

    public static String getDay(int day) {
        switch (day) {
            case 10:
                return "Monday";
            case 55:
                return "Tuesday";
            case 111:
                return "Wednesday";
            case 200:
                return "Thursday";
            case 310:
                return "Friday";
            case 980:
                return "Saturday";
            case 1000:
                return "Sunday";
            default:
                return "Invalid day";
        }
    }
}