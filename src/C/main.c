#include <unistd.h>

const char msg[] = "Ready? Set! GO!\x0A";

int main(int argc, const char** argv)
{
    write(1, msg, sizeof(msg) - 1);
    return 0;
}