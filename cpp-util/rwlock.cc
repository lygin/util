#include "rwlock.h"

int Rwlock::kWlock_ = INT_MAX - 1;
int Rwlock::kNone_ = 0;