#include <cassert>
#include "geometry.h"

Matrix::Matrix(int r, int c)
    : rows(r),
      cols(c),
      m(std::vector<std::vector<float>>(r, std::vector<float>(c, 0.0f))) {}

inline int Matrix::nrows() const { return rows; }

inline int Matrix::ncols() const { return cols; }

Matrix Matrix::identity(int dimensions) {
    Matrix I(dimensions, dimensions);
    for (int i = 0; i < dimensions; i++) {
        I[i][i] = 1.0f;
    }
    return I;
}

std::vector<float> &Matrix::operator[](const int i) {
    assert(i >= 0 && i < rows);
    return m[i];
}

Matrix Matrix::operator*(const Matrix& a) const {
    assert(cols == a.rows);
    Matrix res(rows, a.cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < a.cols; j++) {
            for (int k = 0; k < cols; k++) {
                res[i][j] += m[i][k] * a.m[k][j];
            }
        }
    }
    return res;
}

std::ostream &operator<<(std::ostream &s, Matrix &m) {
    for (int i = 0; i < m.nrows(); i++) {
        for (int j = 0; j < m.ncols(); j++) {
            s << m[i][j];
            if (j == m.ncols() - 1) s << '\n'; else s << '\t';
        }
    }
    return s;
}