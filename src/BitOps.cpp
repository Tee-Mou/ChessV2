#include "../inc/BitOps.h"
#include <random>

int BitOps::countTrailingZeroes(unsigned long long num) {
    int i = 0;
    if (num == 0) {
        return 64;
    };
    while ((num & 1) == 0) {
        num >>= 1;
        i++;
    }
    return i;
}

int BitOps::countSetBits(unsigned long long num) {
    int i = 0;
    if (num == 0) {
        return 0;
    };
    while (num > 0) {
        i += num & 1;
        num >>= 1;
    }
    return i;
}

unsigned long long BitOps::generateMagicNumber() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<unsigned long long int> dist;

    unsigned long long magicNumber = dist(gen) & dist(gen) & dist(gen);
    return magicNumber;
}