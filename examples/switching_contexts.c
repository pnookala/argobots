#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

static ucontext_t ctx[3];

static void
f1 (void)
{
    puts("start f1");
    swapcontext(&ctx[1], &ctx[0]);
    puts("finish f1");
    swapcontext(&ctx[1], &ctx[0]);
} 

static void
f2 (void)
{
    puts("start f2");
    swapcontext(&ctx[2], &ctx[1]);
    puts("finish f2");
    swapcontext(&ctx[2], &ctx[0]);
    puts("finished!\n");
}

int main(int argc, const char *argv[]){
        ucontext_t context;
 
    char st1[8192];
    char st2[8192];

    getcontext(&ctx[1]);
    ctx[1].uc_stack.ss_sp = st1;
    ctx[1].uc_stack.ss_size = sizeof st1;
    //ctx[1].uc_link = &ctx[0];
    makecontext(&ctx[1], f1, 0);
    
    printf("size of ctx %u\n", sizeof(ctx[1]));    
    getcontext(&ctx[2]);
    ctx[2].uc_stack.ss_sp = st2;
    ctx[2].uc_stack.ss_size = sizeof st2;
    ctx[2].uc_link = &ctx[0];
    makecontext(&ctx[2], f2, 0);

    //swapcontext(&ctx[0], &ctx[2]);
   
    int count = 2;
    while(count > 0) { 
        swapcontext(&ctx[0], &ctx[2]);
        printf("back to main 1\n");
        swapcontext(&ctx[0], &ctx[1]);
        printf("back to main 2\n");
        swapcontext(&ctx[0], &ctx[2]);
        printf("done %d!\n", count);
        count--;
    }
    printf("over!\n");
    return 0;
}
