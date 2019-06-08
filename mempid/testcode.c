#include <stdio.h>

void add_one(int *f){
    for (int i = 0; i < 5; i++)
        f[i] = 10;
}

int main(){
    int f[] = { 1, 2, 3, 4, 5 };

    add_one(f);
    for (int i = 0; i < 5; i++)
        printf("%d ", f[i]);

}
