#include <Arduino.h>


String Uid_Maker::uuidToString() {
    char v[65] = "0123456789zxcvbnmasdfghjklqwertyuiopZXCVBNMASDFGHJKLQWERTYUIOP";

    static char buf[33] = {0};

    //gen random for all spaces because lazy
    for(int i = 0; i < 32; ++i) {
        buf[i] = v[rand()%62];
    }

    //put dashes in place
    buf[8] = '-';
    buf[16] = '-';
    buf[24] = '-';

    //needs end byte
    buf[33] = '\0';

    return buf;
}
