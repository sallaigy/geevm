package org.geevm.benchmarks;

public class NQueens {

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("USAGE: nqueens <number>");
            return;
        }
        int n = Integer.valueOf(args[0]);
        nQueens(n);
    }

    public static void nQueens(int n) {
        byte[][] board = new byte[n][n];
        solveUntil(board, 0, n);

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                System.out.print(board[i][j] == 1 ? "X" : "o");
            }
            System.out.println();
        }
    }

    public static boolean solveUntil(byte[][] board, int row, int n) {
        if (row >= n) {
            return true;
        }

        for (int col = 0; col < n; col++) {
            if (isValidPlacement(board, n, row, col)) {
                // Place the queen to (row, col)
                board[row][col] = 1;

                if (solveUntil(board, row + 1, n)) {
                    return true;
                }

                board[row][col] = 0;
            }
        }

        return false;
    }

    public static boolean isValidPlacement(byte[][] board, int n, int row, int col) {
        for (int i = 0; i < row; i++) {
            if (board[i][col] == 1) {
                return false;
            }
        }

        for (int i = 0; i < col; i++) {
            if (board[row][i] == 1) {
                return false;
            }
        }

        for (int i = row, j = col; i >= 0 && j >= 0; i--, j--) {
            if (board[i][j] == 1) {
                return false;
            }
        }

        for (int i = row, j = col; j >= 0 && i < n; i++, j--) {
            if (board[i][j] == 1) {
                return false;
            }
        }

        return true;
    }

}
