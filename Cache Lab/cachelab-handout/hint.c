void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (N == 32)
    {
        for (int i = 0; i < N; i += 8)
        {
            for (int j = 0; j < M; j += 8)
            {
                for (int k = i; k < i + 8; ++k)
                {
                    int temp_value0 = A[k][j];
                    int temp_value1 = A[k][j + 1];
                    int temp_value2 = A[k][j + 2];
                    int temp_value3 = A[k][j + 3];
                    int temp_value4 = A[k][j + 4];
                    int temp_value5 = A[k][j + 5];
                    int temp_value6 = A[k][j + 6];
                    int temp_value7 = A[k][j + 7];

                    B[j][k] = temp_value0;
                    B[j + 1][k] = temp_value1;
                    B[j + 2][k] = temp_value2;
                    B[j + 3][k] = temp_value3;
                    B[j + 4][k] = temp_value4;
                    B[j + 5][k] = temp_value5;
                    B[j + 6][k] = temp_value6;
                    B[j + 7][k] = temp_value7;
                }
            }
        }
    }
    else if (N == 64)
    {
        for (int i = 0; i < N; i += 8)
        {
            for (int j = 0; j < M; j += 8)
            {
                for (int k = i; k < i + 4; ++k)
                {
                    // 1 2
                    int temp_value0 = A[k][j];
                    int temp_value1 = A[k][j + 1];
                    int temp_value2 = A[k][j + 2];
                    int temp_value3 = A[k][j + 3];
                    int temp_value4 = A[k][j + 4];
                    int temp_value5 = A[k][j + 5];
                    int temp_value6 = A[k][j + 6];
                    int temp_value7 = A[k][j + 7];

                    B[j][k] = temp_value0;
                    B[j + 1][k] = temp_value1;
                    B[j + 2][k] = temp_value2;
                    B[j + 3][k] = temp_value3;
                    B[j][k + 4] = temp_value7;
                    B[j + 1][k + 4] = temp_value6;
                    B[j + 2][k + 4] = temp_value5;
                    B[j + 3][k + 4] = temp_value4;
                }

                for (int l = 0; l < 4; ++l)
                {
                    int temp_value0 = A[i + 4][j + 3 - l];
                    int temp_value1 = A[i + 5][j + 3 - l];
                    int temp_value2 = A[i + 6][j + 3 - l];
                    int temp_value3 = A[i + 7][j + 3 - l];
                    int temp_value4 = A[i + 4][j + 4 + l];
                    int temp_value5 = A[i + 5][j + 4 + l];
                    int temp_value6 = A[i + 6][j + 4 + l];
                    int temp_value7 = A[i + 7][j + 4 + l];

                    B[j + 4 + l][i] = B[j + 3 - l][i + 4];
                    B[j + 4 + l][i + 1] = B[j + 3 - l][i + 5];
                    B[j + 4 + l][i + 2] = B[j + 3 - l][i + 6];
                    B[j + 4 + l][i + 3] = B[j + 3 - l][i + 7];

                    B[j + 3 - l][i + 4] = temp_value0;
                    B[j + 3 - l][i + 5] = temp_value1;
                    B[j + 3 - l][i + 6] = temp_value2;
                    B[j + 3 - l][i + 7] = temp_value3;
                    B[j + 4 + l][i + 4] = temp_value4;
                    B[j + 4 + l][i + 5] = temp_value5;
                    B[j + 4 + l][i + 6] = temp_value6;
                    B[j + 4 + l][i + 7] = temp_value7;
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < N; i += 16)
        {
            for (int j = 0; j < M; j += 16)
            {
                for (int k = i; k < i + 16 && k < N; ++k)
                {
                    int temp_position = -1;
                    int temp_value = 0;
                    int l;
                    for (l = j; l < j + 16 && l < M; ++l)
                    {
                        if (l == k)
                        {
                            temp_position = k;
                            temp_value = A[k][k];
                        }
                        else
                        {
                            B[l][k] = A[k][l];
                        }
                    }
                    if (temp_position != -1)
                    {
                        B[temp_position][temp_position] = temp_value;
                    }
                }
            }
        }
    }
}
