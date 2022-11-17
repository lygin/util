#include "hashtable.h"

#define DEBUG
#ifdef DEBUG
#define LOG(frm, argc...) {\
    printf("[%s : %d] ", __func__, __LINE__);\
    printf(frm, ##argc);\
    printf("\n");\
}
#else
#define LOG(frm, argc...)
#endif

/**
 * @brief test
 * 
 */
int main() {
    // In C
    dict *hash = dictCreate(&dictTypeHeapStrings, NULL);
    char k[10] = "abc";
    char v[10] = "def";
    if (dictAdd(hash, k, v)) {
        LOG("add error");
    }
    void *res = dictFetchValue(hash, k);
    if (res) {
        LOG("%s", (char*)res);
    }
    char vv[10] = "xxx";
    dictReplace(hash, k, vv);

    res = dictFetchValue(hash, k);
    if (res) {
        LOG("%s", (char*)res);
    }
    // In CPP
    HashTable ht(&dictTypeHeapStringCopyKeyValue);
    string a = "kkk";
    string b = "111";
    ht.Add(const_cast<char*>(a.data()), const_cast<char*>(b.data()));
    char* s = (char*)ht.Get(a.data());
    b = "222";
    LOG("%s", s);
    ht.Update(const_cast<char*>(a.data()), const_cast<char*>(b.data()));
    s  = (char*)ht.Get(a.data());
    LOG("%s", s);
    for(int i=0; i<1000; ++i) {
        string a = "kkk" + to_string(i);
        string b = "vvv" + to_string(i);
        ht.Add(const_cast<char*>(a.data()),const_cast<char*>(b.data()));
    }
    a = "kkk100";
    s  = (char*)ht.Get(a.data());
    LOG("%s", s);
    return 0;
}