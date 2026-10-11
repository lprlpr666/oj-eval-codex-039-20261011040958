#ifndef CSR_MATRIX_HPP
#define CSR_MATRIX_HPP

#include <vector>
#include <exception>
#include <algorithm>

namespace sjtu {

class size_mismatch : public std::exception {
public:
    const char *what() const noexcept override {
        return "Size mismatch";
    }
};

class invalid_index : public std::exception {
public:
    const char *what() const noexcept override {
        return "Index out of range";
    }
};

template <typename T>
class CSRMatrix {
private:
    size_t n_rows{};
    size_t n_cols{};
    std::vector<size_t> indptr;   // size n_rows+1
    std::vector<size_t> indices;  // column indices of non‑zero values
    std::vector<T> data;          // non‑zero values

    // helper to locate position of column j in row i
    size_t locate(size_t i, size_t j) const {
        size_t start = indptr[i];
        size_t end = indptr[i + 1];
        auto it = std::lower_bound(indices.begin() + start, indices.begin() + end, j);
        return static_cast<size_t>(it - indices.begin());
    }

public:
    CSRMatrix &operator=(const CSRMatrix &other) = delete;
    CSRMatrix &operator=(CSRMatrix &&other) = delete;

    // empty matrix constructor
    CSRMatrix(size_t n, size_t m) : n_rows(n), n_cols(m), indptr(n + 1, 0) {}

    // constructor from CSR components
    CSRMatrix(size_t n, size_t m, size_t count,
              const std::vector<size_t> &indptr_,
              const std::vector<size_t> &indices_,
              const std::vector<T> &data_)
        : n_rows(n), n_cols(m), indptr(indptr_), indices(indices_), data(data_) {
        if (indptr.size() != n + 1) throw size_mismatch();
        if (indices.size() != count || data.size() != count) throw size_mismatch();
        if (indptr.back() != count) throw size_mismatch();
        // basic validation of column indices
        for (size_t col : indices) {
            if (col >= n_cols) throw invalid_index();
        }
    }

    // constructor from dense matrix
    CSRMatrix(size_t n, size_t m, const std::vector<std::vector<T>> &dense)
        : n_rows(n), n_cols(m) {
        if (dense.size() != n) throw size_mismatch();
        indptr.reserve(n + 1);
        indptr.push_back(0);
        for (size_t i = 0; i < n; ++i) {
            if (dense[i].size() != m) throw size_mismatch();
            for (size_t j = 0; j < m; ++j) {
                if (dense[i][j] != T{}) {
                    indices.push_back(j);
                    data.push_back(dense[i][j]);
                }
            }
            indptr.push_back(indices.size());
        }
    }

    ~CSRMatrix() = default;

    size_t getRowSize() const { return n_rows; }
    size_t getColSize() const { return n_cols; }
    size_t getNonZeroCount() const { return data.size(); }

    T get(size_t i, size_t j) const {
        if (i >= n_rows || j >= n_cols) throw invalid_index();
        size_t start = indptr[i];
        size_t end = indptr[i + 1];
        auto it = std::lower_bound(indices.begin() + start, indices.begin() + end, j);
        if (it != indices.begin() + end && *it == j) {
            return data[static_cast<size_t>(it - indices.begin())];
        }
        return T{}; // default constructed when not stored
    }

    void set(size_t i, size_t j, const T &value) {
        if (i >= n_rows || j >= n_cols) throw invalid_index();
        size_t pos = locate(i, j);
        size_t row_start = indptr[i];
        size_t row_end = indptr[i + 1];
        if (pos < row_end && indices[pos] == j) {
            // existing entry, replace value
            data[pos] = value;
            return;
        }
        // insert new entry at pos
        indices.insert(indices.begin() + pos, j);
        data.insert(data.begin() + pos, value);
        // update indptr for subsequent rows
        for (size_t r = i + 1; r <= n_rows; ++r) {
            ++indptr[r];
        }
    }

    const std::vector<size_t> &getIndptr() const { return indptr; }
    const std::vector<size_t> &getIndices() const { return indices; }
    const std::vector<T> &getData() const { return data; }

    std::vector<std::vector<T>> getMatrix() const {
        std::vector<std::vector<T>> dense(n_rows, std::vector<T>(n_cols, T{}));
        for (size_t i = 0; i < n_rows; ++i) {
            for (size_t k = indptr[i]; k < indptr[i + 1]; ++k) {
                dense[i][indices[k]] = data[k];
            }
        }
        return dense;
    }

    std::vector<T> operator*(const std::vector<T> &vec) const {
        if (vec.size() != n_cols) throw size_mismatch();
        std::vector<T> result(n_rows, T{});
        for (size_t i = 0; i < n_rows; ++i) {
            for (size_t k = indptr[i]; k < indptr[i + 1]; ++k) {
                result[i] += data[k] * vec[indices[k]];
            }
        }
        return result;
    }

    CSRMatrix getRowSlice(size_t l, size_t r) const {
        if (l > r || r > n_rows) throw invalid_index();
        size_t new_n = r - l;
        std::vector<size_t> new_indptr(new_n + 1, 0);
        std::vector<size_t> new_indices;
        std::vector<T> new_data;
        for (size_t i = l; i < r; ++i) {
            size_t start = indptr[i];
            size_t end = indptr[i + 1];
            new_indices.insert(new_indices.end(), indices.begin() + start, indices.begin() + end);
            new_data.insert(new_data.end(), data.begin() + start, data.begin() + end);
            new_indptr[i - l + 1] = new_indices.size();
        }
        return CSRMatrix(new_n, n_cols, new_data.size(), new_indptr, new_indices, new_data);
    }
};

} // namespace sjtu

#endif // CSR_MATRIX_HPP
