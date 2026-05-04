#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "svd.h"

#define EPS 1e-10
#define MAX_ITER 1000

typedef struct Matrix {
    double **data;
    int rows;
    int cols;
} Matrix;

typedef struct SVD {
    Matrix *U;      // левые сингулярные векторы (m × m)
    double *S;      // сингулярные числа (min(m,n))
    Matrix *V;      // правые сингулярные векторы (n × n)
} SVD;

//Берем пару столбцов p, q из матрицы, применяем вращение, которое делает столбцы ортогональными.
// Идея: вместо того, чтобы напрямую искать U, Σ, V, последовательно применяем вращения к исходной матрице, чтобы сделать ее ортог.
//То бишь: J₁ᵀ · A · J₂ = B, где J₁, J₂ - матрицы вращений, которые мы выбираем так, чтобы занулить внедиаг. элементы исходн. матр.
//То есть мы строим ортогон. U и V как произведения простых вращений, которые зануляют внедиаг.элементы исходгной матр. и 
//делают каждую пару столбцов ортогнолальной друг другу: U = J₁ · J₂ · J₃ · .. V = K₁ · K₂ · K₃ · ..., где каждое J и K — вращение в плоскости двух координат.
static void processing_pair(Matrix *usmatrix, Matrix *V, int p, int q, int m, int n, double eps) 
{
    double pnorm = 0.0, qnorm = 0.0, dot = 0.0;
    for (int i = 0; i < m; i++) 
    {
        double bp = usmatrix->data[i][p];
        double bq = usmatrix->data[i][q];
        pnorm += bp * bp;
        qnorm += bq * bq;
        dot += bp * bq;
    }
    pnorm = sqrt(pnorm);
    qnorm = sqrt(qnorm);
    if (pnorm < 1e-12 || qnorm < 1e-12) return;
    //вычисляем косинус угла между столбцами с помощью скалярного произвед., определяем нужно ли вращение (желаемой точностью)
    double off_diag = fabs(dot) / (pnorm * qnorm);
    if (off_diag < eps) return;
    //Вычисляем угол вращения.
    //Как ранее и говорилось, каждое вращение J и K (будем применять только K) задаются матрицей вращения в плоскости,
    // которая представляет собой единичную матрицу с измененными эл-ами в позициях (p,p), (p,q), (q,p), (q,q) на c, s, -s, c.
    //K изменяет только столбцы с номерами p и q, K — ортогональная: Kᵀ·K = I
    //Как нам найти угол вращения? Пусть ap и aq — два столбца текущей матрицы. Нам нужно найти угол θ такой, что после вращения:
    //bp = c·ap + s·aq, bq = -s·ap + c·aq: (bp, bq) = 0.
    //bp·bq = (c·ap + s·aq)·(-s·ap + c·aq) = -cs·(ap·ap) + c²·(ap·aq) - s²·(ap·aq) + cs·(aq·aq)= cs·(||aq||² - ||ap||²) + (c² - s²)·(ap·aq)
    //α = ||ap||², β = ||aq||², γ = ap·aq, тогда: bp·bq = cs·(β - α) + (c² - s²)·γ. так как c = cosθ, s = sinθ:
    //bp·bq = ½·sin(2θ)·(β - α) + cos(2θ)·γ => ½·tan(2θ)·(β - α) + γ = 0 => tan(2θ) = -2γ / (β - α) = 2γ / (α - β)
    //Чтобы избежать проблем с делением на ноль и потери точности: tan(2θ) = 1/τ,  τ = (β - α) / (2γ)
    //Используем формулу тангенса половинного угла: tanθ = sign(τ) / (|τ| + √(1+τ²))
    double tau = (qnorm * qnorm - pnorm * pnorm) / (2.0 * dot);
    double t;
    if (tau >= 0) t = 1.0 / (tau + sqrt(1.0 + tau * tau));
    else t = -1.0 / (-tau + sqrt(1.0 + tau * tau));
    double c = 1.0 / sqrt(1.0 + t * t);
    double s = t * c;
    //Применяем вращение к используемой матрице (оператору)
    for (int i = 0; i < m; i++) 
    {
        double bip = usmatrix->data[i][p];
        double biq = usmatrix->data[i][q];
        //новые столбцы - это линейная комбинация старых векторов, то есть A * K,  причем ортог. столбцы ост. ортог. после вращения. 
        usmatrix->data[i][p] = c * bip - s * biq;
        usmatrix->data[i][q] = s * bip + c * biq;
    }
    // Накапливаем вращение в правых сингулярных векторах V : V = V * K = K1 * K2 * ...
    for (int i = 0; i < n; i++) 
    {
        double vip = V->data[i][p];
        double viq = V->data[i][q];
        V->data[i][p] = c * vip - s * viq;
        V->data[i][q] = s * vip + c * viq;
    }
}

//находим самую неортогональную пару столбцов p, q (по косинусу угла между ними) (для лучшей числ. устойчивости и быстрой сходимости)
static double compute_max_off_diag(Matrix *usmatrix, int *p, int *q) 
{
    int n = usmatrix->cols;
    int m = usmatrix->rows;
    double maxval = 0.0;
    int pmax = 0, qmax = 1;
    for (int p = 0; p < n; p++) 
    {
        double pnorm = 0.0;
        for (int i = 0; i < m; i++) pnorm += usmatrix->data[i][p] * usmatrix->data[i][p];
        pnorm = sqrt(pnorm);
        if (pnorm < 1e-12) continue;
        for (int q = p + 1; q < n; q++) 
        {
            double qnorm = 0.0;
            for (int i = 0; i < m; i++) qnorm += usmatrix->data[i][q] * usmatrix->data[i][q];
            qnorm = sqrt(qnorm);
            if (qnorm < 1e-12) continue;
            double dot = 0.0;
            for (int i = 0; i < m; i++) dot += usmatrix->data[i][p] * usmatrix->data[i][q];
            double val = fabs(dot) / (pnorm * qnorm);
            if (val > maxval) 
            {
                maxval = val;
                pmax = p;
                qmax = q;
            }
        }
    }
    *p = pmax;
    *q = qmax;
    return maxval;
}
//реализуем односторонний вариант двустороннего метода: построение V сподсчетом U = A·V·Σ⁻¹
SVD* svd_double_sided_jacobi(Matrix *matrix, double eps, int maxiter)
{
    int m = matrix->rows;
    int n = matrix->cols;
    int rank = (m < n) ? m : n;
    Matrix *usmatrix = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(usmatrix, m, n);
    copyMatrix(matrix, usmatrix);
    Matrix *U = (Matrix*)malloc(sizeof(Matrix));
    Matrix *V = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(U, m, m);
    createMatrix(V, n, n);
    identityMatrix(U, m);
    identityMatrix(V, n);
    double *S = (double*)malloc(rank * sizeof(double));
    for (int iter = 0; iter < maxiter; iter++) 
    {
        //выбираем 2 столбца, которые наименее ортогональные
        int p, q;
        double max_off = compute_max_off_diag(usmatrix, &p, &q);
        if (max_off < eps) 
        {
            printf("Сходимость достигнута на iter %d (max_off = %e)\n", iter + 1, max_off);
            break;
        }
        for (p = 0; p < n; p++) 
        {
            for (q = p + 1; q < n; q++) 
            {
                //применяя поворот к каждым двум столбцам постепенно сходимся и диагонализируем матрицу
                processing_pair(usmatrix, V, p, q, m, n, eps);
            }
        }
    }
    //После сходимости столбцы usematrix = B = Uᵀ · A · V (U·B = A·V) ортогональны но не нормированны, длины столбцов равны сингулярным числам
    //Сингулярные числа матрицы A — это квадратные корни из собственных чисел матрицы Aᵀ·A. Длины образов ортонормированных векторов при отображении A.
    //Если у нас есть ортонормированные векторы v₁, v₂, ..., vr, то: σi = ||A·vi||. 
    // Так как U·(B[:,j]) = A·(V[:,j]), то B[:,j] — это образ вектора V[:,j] под действием A, но выраженный в базисе, заданном столбцами U.
    //Поскольку U — ортогональная матрица, она сохраняет длины векторов: ||B[:,j]|| = ||U·(B[:,j])|| = σj => ||B[:,j]|| = σj
    //Извлекаем сингулярные числа и считаем U: для каждого столбца j: σj = ||B[:,j]||, U[:,j] = B[:,j] / σj
    for (int j = 0; j < rank; j++) 
    {
        double norm = 0.0;
        for (int i = 0; i < m; i++) norm += usmatrix->data[i][j] * usmatrix->data[i][j];
        S[j] = sqrt(norm);
        for (int i = 0; i < m; i++) U->data[i][j] = usmatrix->data[i][j] / S[j];
    }
    //Сортируем выбором по убыванию σj, переставляем соответственно U, V (т.к. первые σ содержат основную "энергию" изображения)
    for (int i = 0; i < rank - 1; i++) 
    {
        int mi = i;
        for (int j = i + 1; j < rank; j++) if (S[j] > S[mi]) mi = j;
        if (mi != i) 
        {
            double ts = S[i];
            S[i] = S[mi];
            S[mi] = ts;
            for (int k = 0; k < m; k++) 
            {
                double tu = U->data[k][i];
                U->data[k][i] = U->data[k][mi];
                U->data[k][mi] = tu;
            }
            for (int k = 0; k < n; k++) 
            {
                double tv = V->data[k][i];
                V->data[k][i] = V->data[k][mi];
                V->data[k][mi] = tv;
            }
        }
    }
    SVD *result = (SVD*)malloc(sizeof(SVD));
    result->U = U;
    result->S = S;
    result->V = V;
    freeMatrix(usmatrix);
    return result;
}
//восстанавливаем приближеннную матрицу по svd-разложению выбранных k-сингул.чисел: Ak[i][j] = Σ{t=1}^{k} U[i][t] · σt · V[j][t]
Matrix* svd_reconstruct(SVD *svd, int k) 
{
    int m = svd->U->rows;
    int n = svd->V->rows;
    int rank = (m < n) ? m : n;
    if (k > rank) k = rank;
    Matrix *rec = (Matrix*)malloc(m * n * sizeof(Matrix));
    createMatrix(rec, m, n);
    for (int i = 0; i < m; i++) 
    {
        for (int j = 0; j < n; j++) 
        {
            double sum = 0.0;
            for (int t = 0; t < k; t++) sum += svd->U->data[i][t] * svd->S[t] * svd->V->data[j][t];
            rec->data[i][j] = sum;
        }
    }
    return rec;
}

void free_svdstate(SVD *result) 
{
    if (!result) return;
    if (result->U) freeMatrix(result->U);
    if (result->S) free(result->S);
    if (result->V) freeMatrix(result->V);
    free(result);
}