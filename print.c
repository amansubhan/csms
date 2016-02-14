#include <stdio.h>
 
int main()
{
   int value = 1;
   int number;

   printf("Enter number of itrerations you want to process: ");
   scanf("%d",&number);
   while(value <= number)
   {
      printf("Value is %d\n", value);
      value++;
   }
 
   return 0;
}
