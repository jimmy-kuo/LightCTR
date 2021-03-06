//
//  matrix.h
//  LightCTR
//
//  Created by SongKuangshi on 2017/10/24.
//  Copyright © 2017年 SongKuangshi. All rights reserved.
//

#ifndef matrix_h
#define matrix_h

#include <cmath>
#include <vector>
#include "random.h"
#include "../common/avx.h"
#include "../common/memory_pool.h"
#include "assert.h"
using namespace std;

struct Matrix_Description {
    size_t x_len, y_len;
};

// 2D Matrix
class Matrix {
public:
    Matrix() {
        x_len = y_len = 0;
    }
    Matrix(size_t _x_len, size_t _y_len): x_len(_x_len), y_len(_y_len) {
        matrix = new vector<float, ArrayAllocator<float> >();
        matrix->resize(x_len * y_len);
    }
    ~Matrix() {
        matrix->clear();
        delete matrix;
        matrix = NULL;
    }
    
    inline void reset(size_t _x_len, size_t _y_len) {
        x_len = _x_len;
        y_len = _y_len;
        if (!matrix) {
            matrix = new vector<float, ArrayAllocator<float> >();
            matrix->resize(x_len * y_len);
        }
        assert(size() == matrix->size());
    }
    
    inline Matrix* copy(Matrix* newM = NULL) const {
        if (newM == NULL) {
            newM = new Matrix(x_len, y_len);
        }
        assert(x_len == newM->x_len);
        assert(y_len == newM->y_len);
        newM->pointer()->assign(matrix->begin(), matrix->end());
        return newM;
    }
    inline Matrix* reshape(size_t new_x, size_t new_y) {
        if (x_len == new_x && y_len == new_y) {
            return this;
        }
        x_len = new_x;
        y_len = new_y;
        matrix->resize(new_x * new_y);
        return this;
    }
    
    inline size_t size() const {
        return x_len * y_len;
    }
    
    inline float* getEle(size_t x, size_t y) const {
        assert(x * y_len + y < size() && x < x_len);
        return matrix->data() + x * y_len + y;
    }
    
    inline void debugPrint() const {
        for (size_t i = 0; i < x_len; i++) {
            for (size_t j = 0; j < y_len; j++) {
                printf("%f ", *getEle(i, j));
            }
            puts("");
        }
        puts("");
    }
    
    inline void zeroInit() {
        assert(matrix);
        std::fill(matrix->begin(), matrix->end(), 0.0f);
    }
    inline void randomInit() {
        for (auto it = matrix->begin(); it != matrix->end(); it++) {
            *it = GaussRand();
        }
    }
    
    inline bool checkConvergence(const Matrix* another) {
        assert(size() == another->size());
        for (auto it = matrix->begin(), it2 = another->pointer()->begin();
             it != matrix->end(); it++, it2++) {
            if (fabs(*it - *it2) > 1e-4) {
                return false;
            }
        }
        return true;
    }
    
    template<typename Func>
    inline void operate(Func f) {
        f(this->matrix);
    }
    
    inline Matrix* rot180() {
        assert(x_len == y_len);
        if (x_len == 1) {
            return this;
        }
        for (size_t i = 0; i < x_len / 2; i++) {
            for (size_t j = 0; j < y_len; j++) {
                size_t new_i = x_len - 1 - i;
                size_t new_j = y_len - 1 - j;
                swap(*getEle(i, j), *getEle(new_i, new_j));
            }
        }
        return this;
    }
    
    inline Matrix* transpose() {
        if (x_len == 1 || y_len == 1) {
            swap(x_len, y_len);
        } else {
            Matrix* newM = new Matrix(y_len, x_len);
            for (size_t i = 0; i < y_len; i++) {
                for (size_t j = 0; j < x_len; j++) {
                    *newM->getEle(i, j) = *getEle(i, j);
                }
            }
            matrix->clear();
            matrix = newM->pointer();
            swap(x_len, y_len);
        }
        return this;
    }
    
    inline Matrix* inverse() {
        float* ptr = matrix->data();
        avx_vecRcp(ptr, ptr, size());
        return this;
    }
    
    inline Matrix* clipping(float clip_threshold) {
        assert(clip_threshold > 0);
        for (auto it = matrix->begin(); it != matrix->end(); it++) {
            if (*it < -clip_threshold) {
                *it = -clip_threshold;
            } else if (*it > clip_threshold) {
                *it = clip_threshold;
            }
        }
        return this;
    }
    
    inline Matrix* add(const Matrix* another, float scale = 1.0, float self_scale = 1.0) {
        assert(x_len == another->x_len && y_len == another->y_len);
        float* ptr = matrix->data();
        avx_vecScale(ptr, ptr, size(), self_scale);
        avx_vecScalerAdd(ptr, another->pointer()->data(), ptr, scale, size());
        return this;
    }
    inline Matrix* add(float delta) {
        avx_vecAdd(matrix->data(), delta, matrix->data(), size());
        return this;
    }
    
    inline Matrix* subtract(const Matrix* another, float scale = 1.0) {
        assert(x_len == another->x_len && y_len == another->y_len);
        float* ptr = matrix->data();
        avx_vecScalerAdd(ptr, another->pointer()->data(), ptr, -scale, size());
        return this;
    }
    inline Matrix* subtract(float delta) {
        avx_vecAdd(matrix->data(), -delta, matrix->data(), size());
        return this;
    }
    
    inline Matrix* scale(float scale_fac) {
        float* ptr = matrix->data();
        avx_vecScale(ptr, ptr, size(), scale_fac);
        return this;
    }
    
    inline Matrix* pow(float fac) {
        float* ptr = matrix->data();
        if (fac == 0.5) {
            avx_vecSqrt(ptr, ptr, size());
        } else if (fac == -0.5) {
            avx_vecRsqrt(ptr, ptr, size());
        } else if (fac == 2) {
            avx_vecScale(ptr, ptr, size(), ptr);
        } else {
            for (auto it = matrix->begin(); it != matrix->end(); it++) {
                *it = ::pow(*it, fac);
            }
        }
        return this;
    }
    
    inline Matrix* dotProduct(const Matrix* another) {
        assert(x_len == another->x_len && y_len == another->y_len);
        float* ptr = matrix->data();
        avx_vecScale(ptr, ptr, size(), another->pointer()->data());
        return this;
    }
    
    inline Matrix* Multiply(Matrix* ansM, const Matrix* another) {
        assert(another && y_len == another->x_len);
        if (ansM == NULL) {
            ansM = new Matrix(x_len, another->y_len);
        }
        ansM->zeroInit();
        assert(ansM->x_len == x_len);
        assert(ansM->y_len == another->y_len);
        float tmp;
        for (size_t i = 0; i < x_len; i++) {
            for (size_t k = 0; k < y_len; k++) {
                tmp = *getEle(i, k);
                if (tmp == 0)
                    continue;
                avx_vecScalerAdd(ansM->getEle(i, 0), another->getEle(k, 0),
                                 ansM->getEle(i, 0), tmp, another->y_len);
            }
        }
        return ansM;
    }
    
    inline void deconvolution_Delta(Matrix*& ansM, const Matrix* filter, size_t padding = 0, size_t stride = 1) {
        size_t recover_x = (x_len - 1) * stride + filter->x_len - 2 * padding;
        size_t recover_y = (x_len - 1) * stride + filter->x_len - 2 * padding;
        
        if (ansM == NULL) {
            ansM = new Matrix(recover_x, recover_y);
        }
        ansM->zeroInit();
        
        vector<float> tmp_vec;
        tmp_vec.resize(filter->size());
        for (size_t i = 0; i < recover_x + 2*padding - filter->x_len + 1; i+=stride) {
            for (size_t j = 0; j < recover_y + 2*padding - filter->y_len + 1; j+=stride) {
                // loop filter size
                const float tmp = *getEle(i / stride, j / stride);
                avx_vecScale(filter->pointer()->data(), tmp_vec.data(), tmp_vec.size(), tmp);
                for (size_t xc = 0; xc < filter->x_len; xc++) {
                    for (size_t yc = 0; yc < filter->y_len; yc++) {
                        if (i + xc < padding || j + yc < padding || i + xc >= padding + recover_x || j + yc >= padding + recover_x) {
                            continue;
                        }
                        *ansM->getEle(i + xc, j + yc) += tmp_vec[xc * filter->y_len + yc];
                    }
                }
            }
        }
    }
    inline void deconvolution_Filter(const Matrix* filterDelta, const Matrix* input, size_t padding = 0, size_t stride = 1) {
        size_t recover_x = input->x_len;
        size_t recover_y = input->y_len;
        
        vector<float> tmp_vec;
        tmp_vec.resize(filterDelta->size());
        for (size_t i = 0; i < recover_x + 2*padding - filterDelta->x_len + 1; i+=stride) {
            for (size_t j = 0; j < recover_y + 2*padding - filterDelta->y_len + 1; j+=stride) {
                // loop filterDelta size
                const float tmp = *getEle(i / stride, j / stride);
                std::fill(tmp_vec.begin(), tmp_vec.end(), 0);
                for (size_t xc = 0; xc < filterDelta->x_len; xc++) {
                    for (size_t yc = 0; yc < filterDelta->y_len; yc++) {
                        if (i + xc < padding || j + yc < padding || i + xc >= padding + recover_x || j + yc >= padding + recover_x) {
                            continue;
                        }
                        // Weight Gradient descent
                        tmp_vec[xc * filterDelta->y_len + yc] = *input->getEle(i + xc, j + yc);
                    }
                }
                avx_vecScalerAdd(filterDelta->pointer()->data(), tmp_vec.data(),
                                 filterDelta->pointer()->data(), tmp, tmp_vec.size());
            }
        }
    }
    // conv conform to commutative principle
    inline void convolution(Matrix*& ansM, const Matrix* filter, size_t padding = 0, size_t stride = 1) {
        assert(filter && (filter->x_len <= x_len) && (filter->y_len <= y_len));
        size_t new_x_len = (x_len - filter->x_len + 2 * padding) / stride + 1;
        size_t new_y_len = (y_len - filter->y_len + 2 * padding) / stride + 1;
        
        if (ansM == NULL) {
            ansM = new Matrix(new_x_len, new_y_len);
        }
        ansM->zeroInit();
        
        vector<float> tmp_vec;
        tmp_vec.resize(filter->size());
        // left corner (-padding, -padding) to (xlen-1+padding, ylen-1+padding)
        for (size_t i = 0; i < x_len + 2 * padding - filter->x_len + 1; i+=stride) {
            for (size_t j = 0; j < y_len + 2 * padding - filter->y_len + 1; j+=stride) {
                // loop filter size
                std::fill(tmp_vec.begin(), tmp_vec.end(), 0);
                for (size_t xc = i; xc < i + filter->x_len; xc++) {
                    for (size_t yc = j; yc < j + filter->y_len; yc++) {
                        if (xc < padding || yc < padding || xc >= padding + x_len || yc >= padding + y_len) {
                            continue;
                        }
                        tmp_vec[(xc - i) * filter->y_len + yc - j] = *getEle(xc - padding, yc - padding);
                    }
                }
                float sum = avx_dotProduct(tmp_vec.data(), filter->pointer()->data(), tmp_vec.size());
                *ansM->getEle(i / stride, j / stride) = sum;
            }
        }
    }
    
    inline vector<float, ArrayAllocator<float> >* pointer() const {
        assert(matrix);
        return this->matrix;
    }
    inline vector<float>& reference() const { // adapte to non-Allocator
        assert(matrix);
        return *(vector<float>*)this->matrix;
    }
    
    size_t x_len, y_len;
    
private:
    vector<float, ArrayAllocator<float> >* matrix = NULL;
};


struct MatrixArr {
    vector<Matrix*> arr;
    ~MatrixArr() {
        for (size_t i = 0; i < arr.size(); i++) {
            delete arr[i];
            arr[i] = NULL;
        }
        arr.clear();
    }
};

#endif /* matrix_h */
