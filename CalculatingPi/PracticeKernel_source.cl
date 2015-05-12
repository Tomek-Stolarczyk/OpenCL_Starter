__kernel void helloworld(__global int* len, __global int* x, __global int* z)
{
	int local_len = *len;
    x[1] = z[1] = 2;
    int a = 1;
	int b = 3;
	int d, j, c, run;

   while(1)
	{
		d = 0;
        for(j=local_len-1; j > 0; j--)
		{
            c = z[j] * a + d;
            z[j] = c % 10;
            d = c / 10;
		}
        d = 0;
        for (j=0; j < local_len; j++)
		{
            c = z[j] + d * 10;
            z[j] = c / b;
            d = c % b;
		}
        run = 0;
        for(j=local_len-1; j > 0; j--)
		{
            c = x[j] + z[j];
            x[j] = c % 10;
            x[j - 1] += c / 10;
            run |= z[j];
		}
        if (!run)
            break;
        a += 1;
        b += 2;
	}

	return;
}