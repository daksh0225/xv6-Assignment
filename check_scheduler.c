#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int main(int argc, char *argv[])
{
  for(int i = 0; i < 100; i++)
  {
    int pid = fork();

    if(pid == 0)
    {
      for(int j = 0; j < 1000; j++)
      {
        int x = (456 + 56/7)%10;
        x*=2;
        x>>=1;
      }
      exit();
    }

    // else
    // {
    //   wait();
    // }
  }

  for(int i = 0; i < 100; i++)
  {
    wait();
  }
  exit();
}
