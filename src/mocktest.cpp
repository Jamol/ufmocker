#include <stdio.h>
#include "ufmock.h"

typedef struct tagT1
{
    int i;
    char c;
    long l;
    long long ll;
    float f;
    double d;
    void* p;
}T1;

class C1
{
public:
    C1() {};
    virtual ~C1() {};
    virtual int test() { printf("C1::test()\n"); return 0; }
    int add(int i1, int i2) { return i1 + i2; }
public:
    int i;
    char c;
    long l;
    long long ll;
    float f;
    double d;
    void* p;
};

int f1(int i, char* p)
{
    return 8;
}

T1 f2(char c, int i)
{
    T1 t;
    t.i = 8;
    t.c = 'o';
    t.l = 0x12345678;
    t.ll = 0x3333333355555555LL;
    t.f = 1.38;
    t.d = 2.53;
    t.p = (void*)990;
    return t;
}

long long f4 (long l)
{
    long long ll = 0x2222222211111111LL;
    return ll;
}

C1 f5(char c, int i)
{
    C1 t;
    t.i = 8;
    t.c = 'o';
    t.l = 0x12345678;
    t.ll = 0x3333333355555555LL;
    t.f = 1.38;
    t.d = 2.53;
    t.p = (void*)990;
    return t;
}

C1* f6(char c, int i)
{
    static C1 t;
    t.i = 8;
    t.c = 'o';
    t.l = 0x12345678;
    t.ll = 0x3333333355555555LL;
    t.f = 1.38;
    t.d = 2.53;
    t.p = (void*)990;
    return &t;
}

float ff(float f)
{
    return .8;
}

double fd()
{
    double d = 2.99;
    return d;
}

long double fld()
{
    return 7.888;
}

bool* fp_bool()
{
    bool b[10] = {0};
    return b;
}

int main(int argc, char** argv)
{
    int a, b = 5;
    a = b++;
    b = 5;
    a = ++b;
    printf("%d %d %d %d %d\n", sizeof(long), sizeof(long long), sizeof(void*), sizeof(double), sizeof(long double));
    UFMocker<int> fmi;
    fmi.mock((void*)f1);
    fmi.setReturnValue(2);
    int ret = f1(2, NULL);
    printf("i: %d\n", ret);

    T1 t;
    t.i = 3;
    t.c = 'a';
    t.l = 0x22334455;
    t.ll = 0x1111111122222222LL;
    t.f = 6.21;
    t.d = 334.995;
    t.p = (void*)100;
    UFMocker<T1> fmt;
    fmt.mock((void*)f2);
    fmt.setReturnValue(t);
    T1 t2 = f2('v', 5);
    printf("t2: %d %c %lx %llx %f %lf %p\n", t2.i, t2.c, t2.l, t.ll, t2.f, t2.d, t2.p);

    UFMocker<long long> fm4;
    fm4.mock((void*)f4);
    fm4.setReturnValue(99);
    long long ll = f4(77);
    printf("ll: %lld\n", ll);

    C1 c;
    c.i = 3;
    c.c = 'a';
    c.l = 0x55667788;
    c.ll = 0x7777777788888888LL;
    c.f = 33.94;
    c.d = 156.889;
    c.p = (void*)100;
    int sz = sizeof(c);
    UFMocker<C1> fmc;
    fmc.mock((void*)f5);
    fmc.setReturnValue(c);
    C1 c2 = f5('v', 5);
    printf("c2: %d %c %lx %llx %f %lf %p\n", c2.i, c2.c, c2.l, c.ll, c2.f, c2.d, c2.p);
    c2.test();

    UFMocker<C1*> fmcp;
    fmcp.mock((void*)f6);
    fmcp.setReturnValue(&c);
    C1* cp = f6('c', 99);
  
	UFMocker<long double> fmld;
    fmld.mock((void*)fld);
    long double ld = 9988.098934323;
    fmld.setReturnValue(ld);
    long double rld = fld();

    ff(2.0);

    UFMocker<bool*> fmfpb;
    fmfpb.mock((void*)fp_bool);
    bool bb[10] = {1};
    fmfpb.setReturnValue(bb);
    bool* bb1 = fp_bool();

    UFMocker<double> fmfd;
    fmfd.mock((void*)fd);
    double dd = 9933.888;
    fmfd.setReturnValue(dd);
    double dd1 = fd();

    // mock class method, non-virtual
    printf("pp: %p\n", &C1::add);
    UFMocker<int> fmcm;
    typedef int (C1::*C1ADD)(int, int);
    C1ADD c1add = &C1::add;
    fmcm.mock((void*)(*(void**)&c1add));
    fmcm.setReturnValue(int(1234));
    C1 cc;
    ret = cc.add(10, 90);

    getchar();
    return 0;
}
