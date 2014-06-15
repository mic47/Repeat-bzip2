#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
using namespace std;

int main() {
    char a[1000];
    int r;
    while ((r = read(0, a, 1000)) > 0) {
        for(int i=0;i<r;i++) {
            for(int j=7;j>=0;j--) {
                if (a[i] & (1<<j)) {
                    printf("1\n");
                } else {
                    printf("0\n");
                }
            }
        }
    }
    return 0;
}

