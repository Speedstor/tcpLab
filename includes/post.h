#ifndef POST_H
#define POST_H
    void* db_put(void* jsonPayload);
    char *stringToBinary(char *s);
    void* async_db_put(void* jsonPayloadP, int option);
#endif