// ============================================================
// Standalone Noise Determinism Validator
// Compile: g++ -std=c++17 -O2 -o noise_validator StandaloneNoiseValidator.cpp
// Run:     ./noise_validator
// ============================================================
// Validates the EXACT same algorithm logic used in the UE plugin
// using standard C++ types — no Unreal Engine required.
// ============================================================

#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>

static float Fade(float t) { return t*t*t*(t*(t*6.0f-15.0f)+10.0f); }
static float Lerp(float a, float b, float t) { return a+t*(b-a); }
static float Grad(int32_t h, float x, float y)
{
    h &= 0x7;
    float u=h<4?x:y, v=h<4?y:x;
    return ((h&1)?-1.0f:1.0f)*u + ((h&2)?-1.0f:1.0f)*v;
}

class StandaloneNoiseGenerator
{
public:
    int32_t P[512];
    void Initialize(int32_t Seed)
    {
        int32_t B[256]; for (int i=0;i<256;++i) B[i]=i;
        uint32_t S=static_cast<uint32_t>(Seed);
        for (int i=255;i>0;--i) { S=S*1103515245u+12345u; int j=S%(i+1); int t=B[i];B[i]=B[j];B[j]=t; }
        for (int i=0;i<256;++i) P[i]=P[i+256]=B[i];
    }
    float Noise(float x, float y) const
    {
        int X=static_cast<int>(std::floor(x))&255, Y=static_cast<int>(std::floor(y))&255;
        float xf=x-std::floor(x), yf=y-std::floor(y);
        float u=Fade(xf), v=Fade(yf);
        int A=P[X]+Y, B=P[X+1]+Y;
        return Lerp(Lerp(Grad(P[A],xf,yf),Grad(P[B],xf-1,yf),u),Lerp(Grad(P[A+1],xf,yf-1),Grad(P[B+1],xf-1,yf-1),u),v);
    }
    float FBM(float x, float y, int oct, float pers, float lac) const
    {
        float t=0,a=1,f=1,mv=0;
        for (int i=0;i<oct;++i){t+=a*Noise(x*f,y*f);mv+=a;a*=pers;f*=lac;}
        return mv>1e-5f?t/mv:t;
    }
};

static int passed=0, failed=0;
void CHECK(bool ok, const std::string& name)
{
    if (ok){++passed;printf("  [PASS] %s\n",name.c_str());}
    else   {++failed;printf("  [FAIL] %s\n",name.c_str());}
}

int main()
{
    printf("=== RTSMapForge Standalone Noise Validator ===\n\n");

    printf("TEST 1: Same seed -> same noise\n");
    {
        StandaloneNoiseGenerator A, B;
        A.Initialize(123456); B.Initialize(123456);
        bool ok=true;
        for (float x=0;x<=10;x+=1.25f) for (float y=0;y<=10;y+=1.25f)
            if (std::fabs(A.Noise(x,y)-B.Noise(x,y))>1e-5f){ok=false;break;}
        CHECK(ok,"Perlin determinism");
    }

    printf("TEST 2: Different seed -> different noise\n");
    {
        StandaloneNoiseGenerator A, B;
        A.Initialize(1); B.Initialize(2);
        float da=A.FBM(5,5,6,0.5f,2)+A.FBM(1,3,6,0.5f,2);
        float db=B.FBM(5,5,6,0.5f,2)+B.FBM(1,3,6,0.5f,2);
        CHECK(std::fabs(da-db)>1e-4f,"Different seeds differ");
    }

    printf("TEST 3: FBM determinism\n");
    {
        StandaloneNoiseGenerator A, B;
        A.Initialize(7777); B.Initialize(7777);
        bool ok=true;
        for (float x=0;x<=5;x+=0.5f) for (float y=0;y<=5;y+=0.5f)
            if (std::fabs(A.FBM(x,y,6,0.5f,2)-B.FBM(x,y,6,0.5f,2))>1e-5f){ok=false;break;}
        CHECK(ok,"FBM determinism");
    }

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed>0?1:0;
}
