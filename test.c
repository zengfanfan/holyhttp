#include <errno.h>
#include <utils/time.h>
#include <utils/print.h>
#include <server/template.h>

int main(void)
{

    DEBUG("%s",
        frender("test.tmpl",
        "name=%s,age=%d,a.b.c=1,a.b.d=2,a.b.c.d=dd,a.b.c.e=ee",
        "my friend",
        get_now_us() % 100));

    return 0;


    char indent[10], left[100], op[10], right[100];
    int ret = sscanf(
        //"#if @a >= 1:\n",
        "\n\t#if @age > 60:\r\n\t\tyou are an old man, aren't you?\r\n\t#end\r\n",
        //"%9[ \t]#if @%100[0-9a-zA-Z.] %3[><=!] %100[^:]:%10[\r\n]",
        "\n#if %100[@a-zA-Z0-9.] %9[><=!] %100[@a-zA-Z0-9.]:",
        indent, left, op, right);

    return 0;
}

