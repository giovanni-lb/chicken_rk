#include <linux/kernel.h>
#include <linux/module.h>

bool is_prime(int number)
{
    if (number < 2)
        return false;

    for (int i = 2; i * i <= number; i++)
    {
        if (number % i == 0)
            return false;
    }

    return true;
}

int complex_check(int num)
{
    if (num == 0)
        return false;

    int xor_result = 0;
    while (num)
    {
        xor_result ^= num & 0xF;
        num >>= 4;
    }

    xor_result = xor_result * 17 % 256;

    int num1 = xor_result / 16;
    int num2 = xor_result % 16;

    num1 = (num1 + 5) % 16;
    num2 = (num2 + 11) % 16;

    xor_result = (num1 << 4) + num2;

    return xor_result;
}

void generate_reference_arr(const char *input, int *reference_arr,
                            int max_length)
{
    int arr_length = 0;

    for (int i = 0; i < 43 - 1; i++)
    {
        if (input[i] == '\0')
            break;

        if (i % 2 == 0)
        {
            if (is_prime(i))
                reference_arr[arr_length] = (int)(input[i]) << 4;
            else
                reference_arr[arr_length] = complex_check((int)(input[i]));
        }
        else
            reference_arr[arr_length] = complex_check((int)(input[i]) - 30);

        arr_length++;
    }
}

int check_password(const char *input)
{
    int reference_arr[43];
    int len = 0;

    while (input[len] != 0)
        len++;

    if (len != 44)
    {
#ifndef DEBUG
        printk(KERN_ERR "len = %d, input= %s \n", len, input);
#endif
        return 1;
    }
    generate_reference_arr(input, reference_arr, 43);

    int expected_arr[] = { 57,  142, 1856, 194, 159, 40,  40,  91,  91,
                           23,  91,  108,  57,  159, 125, 142, 125, 91,
                           142, 108, 160,  159, 6,   91,  91,  194, 142,
                           108, 57,  142,  108, 57,  6,   23,  40,  194,
                           160, 57,  194,  160, 6,   142, 142 };

    for (int i = 0; i < 43 - 1; i++)
    {
        if (reference_arr[i] != expected_arr[i])
            return 1;
    }

    return 0;
}
